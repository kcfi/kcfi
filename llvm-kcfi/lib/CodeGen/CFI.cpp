//===---- CFI.cpp - kCFI LLVM IR analyzes and transformation --------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// LLVM IR analyzes / transformations to enable kCFI backend instrumentation
//
//===----------------------------------------------------------------------===//

#include <iostream>
#include <sstream>
#include <fstream>
#include <list>

#include "llvm/ADT/IndexedMap.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SetVector.h"
#include "llvm/ADT/SmallSet.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/CodeGen/CFGforCFI.h"
#include "llvm/CodeGen/MachineDominators.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/CodeGen/MachineLoopInfo.h"
#include "llvm/CodeGen/MachineModuleInfo.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/CodeGen/Passes.h"
#include "llvm/CodeGen/RegisterScavenging.h"
#include "llvm/CodeGen/StackProtector.h"
#include "llvm/CodeGen/WinEHFuncInfo.h"
#include "llvm/IR/DiagnosticInfo.h"
#include "llvm/IR/InlineAsm.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/ValueMap.h"
#include "llvm/Transforms/IPO.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/IR/CFG.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/Pass.h"
#include "llvm/Transforms/Utils/Cloning.h"
#include "llvm/Transforms/Utils/CodeExtractor.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Compiler.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Target/TargetFrameLowering.h"
#include "llvm/Target/TargetInstrInfo.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetRegisterInfo.h"
#include "llvm/Target/TargetSubtargetInfo.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include <climits>

using namespace llvm;

#define DEBUG_TYPE "cfi"

extern cl::opt<bool> CFICoarse;
extern CFICFG cfi;
extern cl::opt<bool> CFIBuildCFG;
extern cl::opt<bool> CFIv;
extern cl::opt<bool> CFIvv;
extern cl::opt<bool> CFICGD;
extern cl::opt<bool> CFIMapDecls;
static CFIUtils u;

namespace {
  class CFI : public ModulePass {
  public:
    static char ID;
    CFI() : ModulePass(ID) {
      initializeCFIPass(*PassRegistry::getPassRegistry());
    }

    bool runOnModule(Module &M) override;

    bool shouldCGD(Function *F);

    void dumpAliases(Module &M);

    void computeCalls(Module &M);

    void computeFunctions(Module &M);

    int tagCalls(Module &M);

    void CGD(Module &M);

    int tagFunctions(Module &M);

    void computeAndTag(Module &M);
  };
}

char CFI::ID = 0;
char &llvm::CFIID = CFI::ID;

INITIALIZE_PASS_BEGIN(CFI, "CFITagger", "CFI Function Tag Placer", false, false)
INITIALIZE_PASS_END(CFI, "CFITagger", "CFI Function Tag Placer", false, false)

bool CFI::runOnModule(Module &M) {
  // Templates generated using LLVM's C++ backend.
  // TODO: Improve variables/types/function declarations below
  if(CFIBuildCFG){
    GlobalVariable* CFIGlobalVar = M.getGlobalVariable("global_signature");
    if(!CFIGlobalVar){
      CFIGlobalVar = new GlobalVariable(M, Type::getInt32Ty(M.getContext()),
                                        false, GlobalValue::LinkOnceODRLinkage,
                                        0, "global_signature");
      CFIGlobalVar->setAlignment(1);
      CFIGlobalVar->setThreadLocal(false);
    }

    ConstantInt* type = ConstantInt::get(M.getContext(),
                                         APInt(32, StringRef("0"), 10));

    CFIGlobalVar->setInitializer(type);
  }


  std::vector<Type*>args;
  args.push_back(IntegerType::get(M.getContext(), 32));
  args.push_back(IntegerType::get(M.getContext(), 32));
  FunctionType* func_type = FunctionType::get(Type::getVoidTy(M.getContext()),
                                              args, false);

  Function* cvh = M.getFunction("call_violation_handler");
  if (!cvh) {
    cvh = Function::Create(func_type, GlobalValue::ExternalLinkage,
                           "call_violation_handler", &M);
  }
  cvh->setCallingConv(CallingConv::PreserveMost);

  Function* rvh = M.getFunction("ret_violation_handler");
  if (!rvh) {
    rvh = Function::Create(func_type, GlobalValue::ExternalLinkage,
                           "ret_violation_handler", &M);
  }
  rvh->setCallingConv(CallingConv::PreserveMost);

  AttributeSet cvh_PAL;
  {
    SmallVector<AttributeSet, 4> Attrs;
    AttributeSet PAS;
    {
      AttrBuilder B;
      B.addAttribute(Attribute::NoUnwind);
      B.addAttribute(Attribute::UWTable);
      PAS = AttributeSet::get(M.getContext(), ~0U, B);
    }
    Attrs.push_back(PAS);
    cvh_PAL = AttributeSet::get(M.getContext(), Attrs);
  }
  cvh->setAttributes(cvh_PAL);

  AttributeSet rvh_PAL;
  {
    SmallVector<AttributeSet, 4> Attrs;
    AttributeSet PAS;
    {
      AttrBuilder B;
      B.addAttribute(Attribute::NoUnwind);
      B.addAttribute(Attribute::UWTable);
      PAS = AttributeSet::get(M.getContext(), ~0U, B);
    }
    Attrs.push_back(PAS);
    rvh_PAL = AttributeSet::get(M.getContext(), Attrs);
  }
  rvh->setAttributes(cvh_PAL);

  // CFI required symbols created above, now start the analysis
  computeAndTag(M);

  return true;
}

bool CFI::shouldCGD(Function *F){
  bool v = true;
  StringRef handle = "_kcfi";
  if(F->getName().endswith(handle)) v = false;
  handle = "call_violation_handler";
  if(F->getName().equals_lower(handle)) v = false;
  handle = "ret_violation_handler";
  if(F->getName().equals_lower(handle)) v = false;
  handle = "__brk_reservation_fn_";
  if(F->getName().startswith(handle)) v = false;
  if(!v) return false;
  if(cfi.differentEdgesToStrongerNode(F)){
    u.warn("Found weak linkage replacement that should be cloned\n");
    return true;
  }

  if(cfi.differentEdgesToNode(F)) return true;
  return false;
}

void CFI::CGD(Module &M){
  std::stringstream fname;
  for(Module::iterator m = M.begin(), me = M.end(); m != me; ++m){
    Function *Original = m, *Clone, *HackF;
    if(!shouldCGD(Original)){
      // if do not clone this f; check if it is an alias if it is an alias,
      // materialize the aliasee and then check if it should be cloned.
      std::string aliasee = cfi.checkWeakAlias(Original);
      if(aliasee.length() > 0){
        fname.str(std::string());
        fname.clear();
        fname << aliasee;
        Function *a;
        a = M.getFunction(fname.str());
        if(!a){ a = Function::Create(Original->getFunctionType(),
                                    GlobalValue::ExternalLinkage,
                                    fname.str(), &M);
        }
        a->setCallingConv(CallingConv::C);
        if(!shouldCGD(a)){
          a->removeFromParent();
        }
      }
      continue;
    }
    fname.str(std::string());
    fname.clear();
    fname << Original->getName().str() << "_kcfi";
    if(Original->isDeclaration()){
      Function* c = Function::Create(Original->getFunctionType(),
                                     GlobalValue::ExternalLinkage,
                                     fname.str(), &M);
      c->setCallingConv(CallingConv::C);
    } else {
      ValueToValueMapTy VMap;
      Clone = CloneFunction(Original, VMap, false);
      M.getFunctionList().push_back(Clone);
      CFINode n = cfi.getNode(Original, true);
      Clone->setCFINode(n);
      HackF = M.getFunction(fname.str());
      if(HackF){
        fprintf(stderr, "** into clone %s\n", fname.str().c_str());
        HackF->replaceAllUsesWith(Clone);
        HackF->eraseFromParent();
      }
      Clone->setName(fname.str());
    }
  }
}

void CFI::dumpAliases(Module &M){
  for(GlobalAlias &GA : M.aliases()){
    if(cfi.getAlias(GA.getName()).length() > 0){
      if(strcmp(cfi.getAlias(GA.getName()).c_str(),
                GA.getAliasee()->stripPointerCasts()->getName().str()
                .c_str())!=0){
        fprintf(stderr, "Clashing aliases:\n Alias/value: ");
        GA.dump();
        fprintf(stderr, "%s\n Aliasee/value:", GA.getName().str().c_str());
        GA.getAliasee()->dump();
        fprintf(stderr, "%s", GA.getAliasee()->stripPointerCasts()->getName()
                .str().c_str());
      } else {
        continue;
      }
    }
    cfi.setAlias(GA.getName(), GA.getAliasee()->stripPointerCasts()->getName()
                 .str().c_str());
  }
  cfi.storeAliases();
}

void CFI::computeAndTag(Module &M){
  if(CFICoarse) return;
  if(CFIBuildCFG){
    computeFunctions(M);
    computeCalls(M);
    cfi.storeCFG();
    if(CFIMapDecls) cfi.storeDecls();
    dumpAliases(M);
  }
  else {
    tagFunctions(M);
    if(CFICGD) CGD(M);
    tagCalls(M);
  }
}

int CFI::tagCalls(Module &M){
  int n = 0;
  CFICluster c;
  for(Module::iterator m = M.begin(), me = M.end(); m != me; ++m){
    Function &Fn = *m;
    for(Function::iterator bb = Fn.begin(), bbe = Fn.end(); bb != bbe; ++bb){
      BasicBlock &b = *bb;
      for(BasicBlock::iterator i = b.begin(), ie = b.end(); i != ie; ++i){
        CallInst *CI = dyn_cast_or_null<CallInst>(i);
        if(CI && u.isICall(CI)){
          c = cfi.getCluster(CI->getFunctionType());
          if(!c.id){
            cfi_er << "Unable to get cluster " << Fn.getName().str();
            u.error(true);
          }
          CI->setCFITag(c.id);
          n++;
          if(CFIvv)
            fprintf(stderr, "CFIMsg: tagged icall: Tag ID %x %s to %s\n",
                    c.id, Fn.getName().str().c_str(), c.proto);
        }
      }
    }
  }
  return n;
}

void CFI::computeCalls(Module &M){
  CFIEdge e;
  std::string t;
  for(Module::iterator m = M.begin(), me = M.end(); m != me; ++m){
    Function &Fn = *m;
    for(Function::iterator bb = Fn.begin(), bbe = Fn.end(); bb != bbe; ++bb){
      BasicBlock &b = *bb;
      for(BasicBlock::iterator i = b.begin(), ie = b.end(); i != ie; ++i){
        CallInst *CI = dyn_cast_or_null<CallInst>(i);
        if(CI){
          if(u.isICall(CI)){
            CallInst *CI = dyn_cast<CallInst>(i);
            Type *proto = CI->getFunctionType();
            e = cfi.createEdge(m, proto);
            if(!e.id) u.error("Unable to create edge", true);
          }
          else if(u.isDCall(CI)){
            CallInst *CI = dyn_cast<CallInst>(i);
            Function *F = CI->getCalledFunction();
            if(F){
              std::stringstream s;
              if(F->getName().startswith("llvm.")) continue;
            }
          }
        }
      }
    }
  }
}

void CFI::computeFunctions(Module &M){
  CFINode n;
  std::string t;
  for(Module::iterator m = M.begin(), me = M.end(); m != me; ++m){
    Function &Fn = *m;
    if(Fn.isDeclaration()){
      if(Fn.getName().startswith("llvm.")) continue;
      if(CFIMapDecls) cfi.createDecl(&Fn);
      continue;
    }
    n = cfi.createNode(&Fn);
    if(!n.id) u.error("Unable to create node", true);
    if(CFIv){
      cfi_msg << "Tags created for " << Fn.getName().str() << "\n";
      u.msg();
    }
    Fn.setCFINode(n);
  }
}

int CFI::tagFunctions(Module &M){
  CFINode n;
  CFICluster c;
  std::string t;
  int i = 0;
  for(Module::iterator m = M.begin(), me = M.end(); m != me; ++m){
    Function &Fn = *m;
    if(Fn.isDeclaration()) continue;
    n = cfi.getNode(m, true);
    c = cfi.getCluster(Fn.getFunctionType());
    if(!n.id){
      cfi.dumpCFG();
      Fn.dump();
      cfi_er << "Unable to get node" << Fn.getName().str() << "\n";
      u.error(true);
    }
    Fn.setCFINode(n);
    Fn.setCFICluster(c);
    i++;
  }
  return i;
}
