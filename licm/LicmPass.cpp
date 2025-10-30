#include "llvm/Analysis/LoopAnalysisManager.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/ValueTracking.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"

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

  bool isLoadInstructionInvariant(LoadInst *LI, Loop *L) {
    Value *Ptr = LI->getPointerOperand();

    // Iterate over all blocks in this loop and check if anything writes to the
    // address used in this load instruction
    for (BasicBlock *BB : L->blocks()) {
      for (Instruction &J : *BB) {
        if (auto *SI = dyn_cast<StoreInst>(&J)) {
          // This is a store instruction check if it writes to the same address
          if (SI->getPointerOperand() == Ptr) {
            dbgs() << "Load not invariant the following instruction writes to "
                      "same addr:"
                   << *SI << "\n";
            return false;
          }
        } else if (auto *CB = dyn_cast<CallBase>(&J)) {
          // This is a call instruction if it writes to memory we avoid hoisting
          // This is too strict as the called function maybe doesn't modify the
          // address from the load
          if (CB->mayWriteToMemory()) {
            dbgs() << "Load not invariant the following instruction writes to "
                      "memory:"
                   << *CB << "\n";
            return false;
          }
        }
      }
    }
    return true;
  }

  bool licmSingleLoopPass(Loop *L) {
    bool changed = false;
    BasicBlock *preheader = L->getLoopPreheader();
    if (!preheader)
      return false;

    auto preheaderTerminator = preheader->getTerminator();
    dbgs() << "Preheader terminator:" << *preheaderTerminator;
    for (BasicBlock *BB : L->blocks()) {
      for (auto It = BB->begin(), End = BB->end(); It != End;) {
        Instruction &I = *It++;
        dbgs() << I << "\n";
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

        // We will hoist only instructions that are safe to speculate
        // (the behaviour is equivalent if this instruction is executed even
        // if the original loop wouldn't run even once)
        if (!isSafeToSpeculativelyExecute(&I)) {
          dbgs() << "Not safe to speculate\n";
          continue;
        }

        // Don't hoist if an instruction may have side-effects
        if (I.mayHaveSideEffects()) {
          dbgs() << "May have side effects\n";
          continue;
        }

        // Can't hoist a load instruction if it is not invariant
        // (the isLoopInvariant is not a sufficient check for loads)
        if (auto *LI = llvm::dyn_cast<llvm::LoadInst>(&I)) {
          if (!isLoadInstructionInvariant(LI, L)) {
            dbgs() << "Load instruction not invariant\n";
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
    bool anyLoopChanged = false;
    for (Loop *L : LI) {
      bool loopChanged = false;
      do {
        loopChanged = licmSingleLoopPass(L);
        anyLoopChanged |= loopChanged;
      } while (loopChanged);
    }

    return anyLoopChanged ? PreservedAnalyses::none()
                          : PreservedAnalyses::all();
  }
};

extern "C" LLVM_ATTRIBUTE_WEAK PassPluginLibraryInfo llvmGetPassPluginInfo() {
  return {LLVM_PLUGIN_API_VERSION, "licm-pass", LLVM_VERSION_STRING,
          [](PassBuilder &PB) {
            PB.registerPipelineParsingCallback(
                [](StringRef Name, FunctionPassManager &FPM,
                   ArrayRef<PassBuilder::PipelineElement>) {
                  if (Name == "licm-pass") {
                    FPM.addPass(LicmPass());
                    return true;
                  }
                  return false;
                });
          }};
}
