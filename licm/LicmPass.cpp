#include "llvm/Analysis/LoopAnalysisManager.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/ValueTracking.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"
#include <llvm-14/llvm/Support/Debug.h>

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

    for (BasicBlock *BB : L->blocks()) {
      for (Instruction &J : *BB) {
        if (auto *SI = dyn_cast<StoreInst>(&J)) {
          if (SI->getPointerOperand() == Ptr) {
            dbgs() << "Load not invariant the following instruction writes to "
                      "same addr:"
                   << *SI << "\n";
            return false;
          }
        } else if (auto *CB = dyn_cast<CallBase>(&J)) {
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

  PreservedAnalyses run(Function &F, FunctionAnalysisManager &AM) {
    auto &LI = AM.getResult<LoopAnalysis>(F);
    auto &DT = AM.getResult<DominatorTreeAnalysis>(F);
    bool changed = false;
    for (Loop *L : LI) {
      BasicBlock *preheader = L->getLoopPreheader();
      if (!preheader)
        continue;
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

          if (!isSafeToSpeculativelyExecute(&I)) {
            dbgs() << "Not safe to speculate\n";
            continue;
          }

          if (I.mayHaveSideEffects()) {
            dbgs() << "May have side effects\n";
            continue;
          }

          if (auto *LI = llvm::dyn_cast<llvm::LoadInst>(&I)) {
            if (!isLoadInstructionInvariant(LI, L)) {
              dbgs() << "Load instruction not invariant\n";
              continue;
            }
          }

          dbgs() << "Can be hoisted\n";
          I.moveBefore(preheader->getTerminator());
          changed = true;
        }
      }
    }

    return changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
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
