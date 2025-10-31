#include "llvm/Analysis/AliasAnalysis.h"
#include "llvm/Analysis/LoopAnalysisManager.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/MemoryLocation.h"
#include "llvm/Analysis/ValueTracking.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/IPO/FunctionAttrs.h"
#include <llvm-14/llvm/ADT/SmallVector.h>
#include <llvm-14/llvm/IR/InstrTypes.h>
#include <llvm-14/llvm/Support/Casting.h>

using namespace llvm;

struct LicmPass : PassInfoMixin<LicmPass> {
  bool isLoopInvariant(const Instruction &I, Loop *L) {
    bool invariant = true;
    for (auto &op : I.operands()) {
      if (!L->isLoopInvariant(op)) {
        invariant = false;
      }
    }
    return invariant;
  }

  bool isLoadInstructionInvariant(LoadInst *LI, Loop *L,
                                  ArrayRef<StoreInst *> loopStores,
                                  ArrayRef<CallBase *> loopCalls,
                                  AAResults &AA) {
    if (LI->isVolatile() || LI->isAtomic())
      return false;

    // The pointer must be loop invariant
    if (!L->isLoopInvariant(LI->getPointerOperand()))
      return false;

    MemoryLocation locLoad = MemoryLocation::get(LI);

    for (auto *SI : loopStores) {
      MemoryLocation locStore = MemoryLocation::get(SI);
      if (AA.alias(locLoad, locStore) != AliasResult::NoAlias) {
        dbgs() << "Load not invariant the following instruction writes to "
                  "same addr:"
               << *SI << "\n";
        return false;
      }
    }
    for (auto *CB : loopCalls) {
      // This is a call instruction if it doesn't write to memory just skip
      if (!CB->mayWriteToMemory()) {
        continue;
      }
      // If the call instruction modifies the memory location of the load
      // instruction we can't hoist
      ModRefInfo modRefInfo = AA.getModRefInfo(CB, locLoad);
      if (isModSet(modRefInfo)) {
        return false;
      }
    }

    return true;
  }

  static bool pointerArgsLoopInvariant(CallBase &CB, Loop &L) {
    for (unsigned i = 0; i < CB.arg_size(); ++i) {
      Value *A = CB.getArgOperand(i);
      if (A->getType()->isPointerTy() && !L.isLoopInvariant(A))
        return false;
    }
    return true;
  }

  static bool canHoistCall(CallBase &CB, Loop &L, AAResults &AA,
                           ArrayRef<StoreInst *> loopStores,
                           ArrayRef<CallBase *> loopCalls) {
    if (!CB.onlyReadsMemory())
      return false;
    if (!CB.doesNotThrow())
      return false;
    if (!pointerArgsLoopInvariant(CB, L))
      return false;

    // Check if any of the store instructions write to the location accessed by
    // this call
    for (auto *LI : loopStores) {
      auto writeLoc = MemoryLocation::get(LI);
      if (isRefSet(AA.getModRefInfo(&CB, writeLoc))) {
        return false;
      }
    }
    // Check if any of the other calls write to the location accessed by this
    // call
    for (CallBase *K : loopCalls) {
      if (K == &CB)
        continue;

      // True if K writes to memory that CB reads or writes to
      // Since we already know that CB only reads that means this is equivalent
      // to K writes to memory that CB reads
      bool kWritesCBReads = isModSet(AA.getModRefInfo(K, &CB));
      if (kWritesCBReads)
        return false;
    }
    return true;
  }

  static void getLoopStoresAndCalls(Loop &L,
                                    SmallVector<StoreInst *> &LoopStores,
                                    SmallVector<CallBase *> &LoopCalls) {
    for (BasicBlock *BB : L.blocks()) {
      for (Instruction &J : *BB) {
        if (auto *SI = dyn_cast<StoreInst>(&J)) {
          LoopStores.push_back(SI);
        } else if (auto *CB = dyn_cast<CallBase>(&J)) {
          LoopCalls.push_back(CB);
        }
      }
    }
  }

  void tempDebugCb(CallBase *cb) {
    dbgs() << "Call base inst: " << "onlyReadsMemory=" << cb->onlyReadsMemory()
           << " onlyAccessesArgMemory=" << cb->onlyAccessesArgMemory() << "\n";
  }

  bool licmSingleLoopPass(Loop *L, AAResults &AA) {
    bool changed = false;
    BasicBlock *preheader = L->getLoopPreheader();
    if (!preheader)
      return false;

    // Collect all loop stores and calls so we don't iterate over all
    // instructions every time
    SmallVector<StoreInst *> loopStores;
    SmallVector<CallBase *> loopCalls;
    getLoopStoresAndCalls(*L, loopStores, loopCalls);

    auto preheaderTerminator = preheader->getTerminator();
    dbgs() << "Preheader terminator:" << *preheaderTerminator;
    for (BasicBlock *BB : L->blocks()) {
      for (auto It = BB->begin(), End = BB->end(); It != End;) {
        Instruction &I = *It++;
        dbgs() << I << "\n";

        // todo: remove
        if (auto *cb = dyn_cast<CallBase>(&I)) {
          bool b = isSafeToSpeculativelyExecute(&I);
          tempDebugCb(cb);
        }

        // Can't hoist terminators
        if (I.isTerminator()) {
          dbgs() << "Terminator\n";
          continue;
        }

        // If any operand of this instruction is not invariant we can't hoist
        if (!isLoopInvariant(I, L)) {
          dbgs() << "Not invariant\n";
          continue;
        }

        // Can't hoist a load instruction if it is not invariant
        // (the isLoopInvariant is not a sufficient check for loads)
        if (auto *LI = llvm::dyn_cast<llvm::LoadInst>(&I)) {
          if (!isLoadInstructionInvariant(LI, L, loopStores, loopCalls, AA)) {
            dbgs() << "Load instruction not invariant\n";
            continue;
          }
        } else if (auto *CB = llvm::dyn_cast<llvm::CallBase>(&I)) {
          if (!canHoistCall(*CB, *L, AA, loopStores, loopCalls)) {
            dbgs() << "Can't hoist call instruction\n";
            continue;
          }
        } else {
          // We will hoist only instructions that are safe to speculate
          // (the behaviour is equivalent if this instruction is executed even
          // if the original loop wouldn't run even once)
          if (!isSafeToSpeculativelyExecute(&I)) {
            dbgs() << "Not safe to speculate\n";
            continue;
          }
        }

        // Can be hoisted - move before the preheader terminator
        dbgs() << "Can be hoisted\n";
        I.moveBefore(preheader->getTerminator());
        changed = true;
      }
    }
    return changed;
  }

  PreservedAnalyses run(Function &F, FunctionAnalysisManager &AM) {
    auto &LI = AM.getResult<LoopAnalysis>(F);
    auto &AA = AM.getResult<AAManager>(F);
    bool anyLoopChanged = false;
    for (Loop *L : LI) {
      bool loopChanged = false;
      do {
        loopChanged = licmSingleLoopPass(L, AA);
        anyLoopChanged |= loopChanged;
      } while (loopChanged);
    }

    return anyLoopChanged ? PreservedAnalyses::none()
                          : PreservedAnalyses::all();
  }
};

extern "C" LLVM_ATTRIBUTE_WEAK PassPluginLibraryInfo llvmGetPassPluginInfo() {
  return {LLVM_PLUGIN_API_VERSION, "licm-pipeline", LLVM_VERSION_STRING,
          [](PassBuilder &PB) {
            PB.registerPipelineParsingCallback(
                [](StringRef Name, FunctionPassManager &FPM,
                   ArrayRef<PassBuilder::PipelineElement>) {
                  if (Name == "licm-pass") {
                    FPM.addPass(RequireAnalysisPass<AAManager, Function>());
                    FPM.addPass(LicmPass());
                    return true;
                  }
                  return false;
                });
            PB.registerPipelineParsingCallback(
                [](StringRef Name, ModulePassManager &MPM,
                   ArrayRef<PassBuilder::PipelineElement>) {
                  if (Name == "licm-pipeline") {
                    // (a) Interprocedural attribute inference
                    MPM.addPass(createModuleToPostOrderCGSCCPassAdaptor(
                        PostOrderFunctionAttrsPass()));

                    // (c) Now run our function pass with AA required
                    FunctionPassManager FPM;
                    FPM.addPass(RequireAnalysisPass<AAManager, Function>());
                    FPM.addPass(LicmPass());
                    MPM.addPass(
                        createModuleToFunctionPassAdaptor(std::move(FPM)));
                    return true;
                  }
                  return false;
                });
          }};
}
