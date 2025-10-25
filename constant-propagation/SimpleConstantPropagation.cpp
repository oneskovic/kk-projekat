#include <llvm/ADT/STLExtras.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/InstrTypes.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/PassManager.h>
#include <llvm/Passes/PassBuilder.h>
#include <llvm/Passes/PassPlugin.h>

using namespace llvm;

namespace {

class ConstantBinaryExpression {
private:
  Instruction::BinaryOps m_op;
  ConstantInt *m_c0;
  ConstantInt *m_c1;

public:
  ConstantBinaryExpression(Instruction::BinaryOps op, ConstantInt *c0,
                           ConstantInt *c1)
      : m_op(op), m_c0(c0), m_c1(c1) {}

  Constant *calculate() {
    Constant *Result = nullptr;
    Type *T = m_c0->getType();
    auto V0 = m_c0->getValue();
    auto V1 = m_c1->getValue();
    switch (m_op) {
    case Instruction::Add:
      Result = ConstantInt::get(T, V0 + V1);
      break;
    case Instruction::Sub:
      Result = ConstantInt::get(T, V0 - V1);
      break;
    case Instruction::Mul:
      Result = ConstantInt::get(T, V0 * V1);
      break;
    case Instruction::UDiv:
      if (!m_c1->isZero())
        Result = ConstantInt::get(T, V0.udiv(V1));
      break;
    case Instruction::SDiv:
      if (!m_c1->isZero())
        Result = ConstantInt::get(T, V0.sdiv(V1));
      break;
    default:
      break;
    }
    return Result;
  }
};

std::optional<ConstantBinaryExpression>
tryParseConstantBinaryExpression(Instruction &I) {
  if (auto *BinOp = dyn_cast<BinaryOperator>(&I)) {
    Value *Op0 = BinOp->getOperand(0);
    Value *Op1 = BinOp->getOperand(1);
    if (auto *C0 = dyn_cast<ConstantInt>(Op0))
      if (auto *C1 = dyn_cast<ConstantInt>(Op1))
        return ConstantBinaryExpression(BinOp->getOpcode(), C0, C1);
  }
  return std::nullopt;
}

bool propagateConstants(BasicBlock &BB) {
  bool Changed = false;
  for (auto &I : make_early_inc_range(BB)) {
    if (auto Expr = tryParseConstantBinaryExpression(I)) {
      if (Constant *Result = Expr->calculate()) {
        I.replaceAllUsesWith(Result);
        I.removeFromParent();
        Changed = true;
      }
    }
  }
  return Changed;
}

struct SimpleConstantPropagation
    : public PassInfoMixin<SimpleConstantPropagation> {
  PreservedAnalyses run(Function &F, FunctionAnalysisManager &AM) {
    bool Changed = false;
    for (auto &BB : F) {
      Changed |= propagateConstants(BB);
    }
    return (Changed ? PreservedAnalyses::none() : PreservedAnalyses::all());
  }
};

} // namespace

// Pass registration
extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
  return {LLVM_PLUGIN_API_VERSION, "SimpleConstantPropagation", "v0.1",
          [](PassBuilder &PB) {
            PB.registerPipelineParsingCallback(
                [](StringRef Name, FunctionPassManager &FPM,
                   ArrayRef<PassBuilder::PipelineElement>) {
                  if (Name == "simple-const-prop") {
                    FPM.addPass(SimpleConstantPropagation());
                    return true;
                  }
                  return false;
                });
          }};
}
