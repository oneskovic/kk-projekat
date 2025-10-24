#include "llvm/Analysis/LoopAnalysisManager.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Scalar/LoopPassManager.h"

using namespace llvm;

struct LicmPass : PassInfoMixin<LicmPass> {
  PreservedAnalyses run(Loop &L, LoopAnalysisManager &,
                        LoopStandardAnalysisResults &, LPMUpdater &) {
    bool changed = false;
    for (BasicBlock *BB : L.blocks()) {
      for (Instruction &I : *BB) {
        dbgs() << I << "\n";
        if (I.isTerminator()) {
          dbgs() << "Terminator\n";
          continue;
        }
        bool invariant = true;
        for (auto &op : I.operands()) {
          if (!L.isLoopInvariant(op)) {
            invariant = false;
          }
        }
        if (!invariant) {
          dbgs() << "Not invariant\n";
          continue;
        }

        dbgs() << "Can be hoisted: " << I << "\n";
      }
    }
    return PreservedAnalyses::all();
  }
};

extern "C" LLVM_ATTRIBUTE_WEAK PassPluginLibraryInfo llvmGetPassPluginInfo() {
  errs() << "[licm-pass] plugin loaded\n";
  return {LLVM_PLUGIN_API_VERSION, "licm-pass", LLVM_VERSION_STRING,
          [](PassBuilder &PB) {
            PB.registerPipelineParsingCallback(
                [](StringRef Name, LoopPassManager &LPM,
                   ArrayRef<PassBuilder::PipelineElement>) {
                  if (Name == "licm-pass") {
                    LPM.addPass(LicmPass());
                    return true;
                  }
                  return false;
                });
          }};
}
