//===-- sotoc/src/Visitor.cpp ---------------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file implements the classes DiscoverTypesInDeclVisitor and
/// FindTargetCodeVisitor.
///
//===----------------------------------------------------------------------===//

#include <sstream>
#include <string>

#include "clang/AST/ASTContext.h"
#include "clang/AST/Attr.h"
#include "clang/AST/Decl.h"
#include "clang/AST/Stmt.h"
#include "clang/AST/StmtOpenMP.h"
#include "clang/Basic/OpenMPKinds.h"
#include "clang/Basic/SourceLocation.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Rewrite/Core/Rewriter.h"

#include "Debug.h"
#include "DeclResolver.h"
#include "TargetCode.h"
#include "TargetCodeFragment.h"
#include "Visitors.h"

static bool stmtNeedsSemicolon(const clang::Stmt *S) {
  while (1) {
    if (auto *CS = llvm::dyn_cast<clang::CapturedStmt>(S)) {
      S = CS->getCapturedStmt();
    } else if (auto *OS = llvm::dyn_cast<clang::OMPExecutableDirective>(S)) {
      S = OS->getInnermostCapturedStmt();
    } else {
      break;
    }
  }
  if (llvm::isa<clang::CompoundStmt>(S) || llvm::isa<clang::ForStmt>(S) ||
      llvm::isa<clang::IfStmt>(S)) {
    return false;
  }
  return true;
}

llvm::Optional<std::string> getSystemHeaderForDecl(clang::Decl *D) {
  clang::SourceManager &SM = D->getASTContext().getSourceManager();

  DEBUGPDECL(D, "Get system header for Decl: ");

  if (!SM.isInSystemHeader(D->getBeginLoc())) {
    return llvm::Optional<std::string>();
  }

  // we dont want to include the original system header in which D was
  // declared, but the system header which exposes D to the user's file
  // (the last system header in the include stack)
  auto IncludedFile = SM.getFileID(D->getBeginLoc());

  // Fix for problems with math.h
  // If our declaration is really a macro expansion, we need to find the actual
  // spelling location first.
  bool SLocInvalid = false;
  auto SLocEntry = SM.getSLocEntry(IncludedFile, &SLocInvalid);
  if (SLocEntry.isExpansion()) {
    IncludedFile = SM.getFileID(SLocEntry.getExpansion().getSpellingLoc());
  }

  auto IncludingFile = SM.getDecomposedIncludedLoc(IncludedFile);

  while (SM.isInSystemHeader(SM.getLocForStartOfFile(IncludingFile.first))) {
    IncludedFile = IncludingFile.first;
    IncludingFile = SM.getDecomposedIncludedLoc(IncludedFile);
  }

  return llvm::Optional<std::string>(
      std::string(SM.getFilename(SM.getLocForStartOfFile(IncludedFile))));
}

bool FindTargetCodeVisitor::VisitStmt(clang::Stmt *S) {
  if (auto *TD = llvm::dyn_cast<clang::OMPTargetDirective>(S)) {
    processTargetRegion(TD);
  } else if (auto *TD = llvm::dyn_cast<clang::OMPTargetTeamsDirective>(S)) {
    processTargetRegion(TD);
  } else if (auto *TD = llvm::dyn_cast<clang::OMPTargetParallelDirective>(S)) {
    processTargetRegion(TD);
  } else if (auto *LD = llvm::dyn_cast<clang::OMPLoopDirective>(S)) {
    if (auto *TD = llvm::dyn_cast<clang::OMPTargetParallelForDirective>(LD)) {
      processTargetRegion(TD);
    } else if (auto *TD =
                   llvm::dyn_cast<clang::OMPTargetParallelForSimdDirective>(
                       LD)) {
      processTargetRegion(TD);
    } else if (auto *TD = llvm::dyn_cast<clang::OMPTargetSimdDirective>(LD)) {
      processTargetRegion(TD);
    } else if (auto *TD =
                   llvm::dyn_cast<clang::OMPTargetTeamsDistributeDirective>(
                       LD)) {
      processTargetRegion(TD);
    } else if (auto *TD = llvm::dyn_cast<
                   clang::OMPTargetTeamsDistributeParallelForDirective>(LD)) {
      processTargetRegion(TD);
    } else if (auto *TD = llvm::dyn_cast<
                   clang::OMPTargetTeamsDistributeParallelForSimdDirective>(
                   LD)) {
      processTargetRegion(TD);
    } else if (auto *TD =
                   llvm::dyn_cast<clang::OMPTargetTeamsDistributeSimdDirective>(
                       LD)) {
      processTargetRegion(TD);
    }
  }
  return true;
}

class CollectOMPClausesVisitor : public clang::RecursiveASTVisitor<CollectOMPClausesVisitor> {
  std::shared_ptr<TargetCodeRegion> TCR;
public:
  CollectOMPClausesVisitor(std::shared_ptr<TargetCodeRegion> &TCR)
      :  TCR(TCR) {};
  bool VisitStmt(clang::Stmt *S) {
    if (auto *OED = llvm::dyn_cast<clang::OMPExecutableDirective>(S)) {
      for (auto *Clause : OED->clauses()) {
        TCR->addOpenMPClause(Clause);
      }
    }
    return true;
  };
};

bool FindTargetCodeVisitor::processTargetRegion(
    clang::OMPExecutableDirective *TargetDirective) {
  // TODO: Not sure why to iterate the children, because I think there
  // is only one child. For me this looks wrong.
  for (auto i = TargetDirective->child_begin(),
            e = TargetDirective->child_end();
       i != e; ++i) {

    if (auto *CS = llvm::dyn_cast<clang::CapturedStmt>(*i)) {
      while (auto *NCS =
                 llvm::dyn_cast<clang::CapturedStmt>(CS->getCapturedStmt())) {
        CS = NCS;
      }

      auto TCR = std::make_shared<TargetCodeRegion>(
          CS, TargetDirective, LastVisitedFuncDecl, Context);
      // if the target region cannot be added we dont want to parse its args
      if (TargetCodeInfo.addCodeFragment(TCR)) {

        // look for nested clause
        CollectOMPClausesVisitor(TCR).TraverseStmt(CS);
        for (auto C : TargetDirective->clauses()) {
          TCR->addOpenMPClause(C);
        }

        // For more complex data types (like structs) we need to traverse the
        // tree
        DiscoverTypeVisitor.TraverseStmt(CS);
        DiscoverFunctionVisitor.TraverseStmt(CS);
        addTargetRegionArgs(CS, TCR);
        TCR->NeedsSemicolon = stmtNeedsSemicolon(CS);
        TCR->TargetCodeKind = TargetDirective->getDirectiveKind();
      }
    }
  }
  return true;
}

void FindTargetCodeVisitor::addTargetRegionArgs(
    clang::CapturedStmt *S, std::shared_ptr<TargetCodeRegion> TCR) {

  DEBUGP("Add target region args");
  for (const auto &i : S->captures()) {
    if (!(i.capturesVariableArrayType())) {
      clang::VarDecl *var = i.getCapturedVar();
      DEBUGP("captured Var: " + var->getNameAsString());
      TCR->addCapturedVar(var);
    } else {
      // Not sure what exactly is caputred here. It looks like we have an
      // additional capture in cases of VATs.
      DEBUGP("Current capture is a variable-length array type (skipped)");
    }
  }

  FindLoopStmtVisitor FindLoopVisitor;
  FindLoopVisitor.TraverseStmt(S);

  std::unordered_set<clang::VarDecl *> tmpSet;

  // printf("%lu \n", FindLoopVisitor.getVarSet()->size());
  for (const auto i : *FindLoopVisitor.getVarSet()) {
    DEBUGP("Iterating var set");
    // i->print(llvm::outs());
    for (auto j : *TCR->getOMPClauses()) {
      for (auto CC : j->children()) {
        if (auto CC_DeclRefExpr =
                llvm::dyn_cast_or_null<clang::DeclRefExpr>(CC)) {
          // CC_DeclRefExpr->dumpColor();
          if (i->getCanonicalDecl() == CC_DeclRefExpr->getDecl())
            tmpSet.insert(i);
        }
      }
    }

    for (auto j = TCR->getCapturedVarsBegin(), e = TCR->getCapturedVarsEnd();
         j != e; ++j) {
      if (i->getCanonicalDecl() == *j) {
        // i->print(llvm::outs());
        DEBUGPDECL(i, "Add captured var: ");
        // FindLoopVisitor.getVarSet()->erase(i);
        tmpSet.insert(i);
      }
    }
  }

  for (const auto i : tmpSet) {
    FindLoopVisitor.getVarSet()->erase(FindLoopVisitor.getVarSet()->find(i));
  }

  tmpSet.clear();

  // printf("%lu \n", FindLoopVisitor.getVarSet()->size());
  for (const auto i : *FindLoopVisitor.getVarSet()) {
    // i->print(llvm::outs());
    TCR->addCapturedVar(i);
  }
}

bool FindTargetCodeVisitor::VisitDecl(clang::Decl *D) {
  auto *FD = llvm::dyn_cast<clang::FunctionDecl>(D);
  if (FD) {
    LastVisitedFuncDecl = FD;

    auto search = FuncDeclWithoutBody.find(FD->getNameAsString());
    if (search != FuncDeclWithoutBody.end()) {
      Functions.addDecl(D);
      FuncDeclWithoutBody.erase(search);
    }
  }

  // search Decl attributes for 'omp declare target' attr
  for (auto &attr : D->attrs()) {
    if (attr->getKind() == clang::attr::OMPDeclareTargetDecl) {
      auto SystemHeader = getSystemHeaderForDecl(D);
      if (SystemHeader.hasValue()) {
        TargetCodeInfo.addHeader(SystemHeader.getValue());
        return true;
      }

      auto TCD = std::make_shared<TargetCodeDecl>(D);
      TargetCodeInfo.addCodeFragment(TCD);
      DiscoverTypeVisitor.TraverseDecl(D);
      if (FD) {
        if (FD->hasBody() && !FD->doesThisDeclarationHaveABody()) {
          FuncDeclWithoutBody.insert(FD->getNameAsString());
        }
      }
      if (!D->hasBody() || (FD && !FD->doesThisDeclarationHaveABody())) {
        TCD->NeedsSemicolon = true;
      }
      return true;
    }
  }
  return true;
}

bool FindLoopStmtVisitor::VisitStmt(clang::Stmt *S) {
  if (auto LS = llvm::dyn_cast<clang::ForStmt>(S)) {
    // LS->getInit()->dumpColor();
    FindDeclRefVisitor.TraverseStmt(LS->getInit());
  }
  // else if (auto LS = llvm::dyn_cast<clang::DoStmt>(S)) {
  //   FindDeclRefVisitor.TraverseStmt(LS);
  // } else if (auto LS = llvm::dyn_cast<clang::WhileStmt>(S)) {
  //   FindDeclRefVisitor.TraverseStmt(LS);
  // }
  return true;
}

// bool FindDeclRefExprVisitor::VisitDecl(clang::Decl *D) {
//   printf("VisitDecl\n");
//   D->dumpColor();
//   return true;
// }

bool FindDeclRefExprVisitor::VisitStmt(clang::Stmt *S) {
  // printf("VisitStmt\n");
  // S->dumpColor();
  if (auto DRE = llvm::dyn_cast<clang::DeclRefExpr>(S)) {
    if (auto DD = llvm::dyn_cast<clang::DeclaratorDecl>(DRE->getDecl())) {
      if (auto VD = llvm::dyn_cast<clang::VarDecl>(DD)) {
        if (VD->getNameAsString() != ".reduction.lhs") {
          // printf("VarDecl\n");
          // VD->print(llvm::outs());
          VarSet.insert(VD);
        }
      }
    }
  }
  return true;
}

bool DiscoverTypesInDeclVisitor::VisitDecl(clang::Decl *D) {
  if (auto *VD = llvm::dyn_cast<clang::ValueDecl>(D)) {
    if (const clang::Type *TP = VD->getType().getTypePtrOrNull()) {
      processType(TP);
    }
  }
  return true;
}

bool DiscoverTypesInDeclVisitor::VisitExpr(clang::Expr *E) {
  if (auto *DRE = llvm::dyn_cast<clang::DeclRefExpr>(E)) {
    if (auto *ECD = llvm::dyn_cast<clang::EnumConstantDecl>(DRE->getDecl())) {
      OnEachTypeRef(llvm::cast<clang::EnumDecl>(ECD->getDeclContext()));
      return true;
    }
  }
  if (const clang::Type *TP = E->getType().getTypePtrOrNull()) {
    processType(TP);
  }
  return true;
}

bool DiscoverTypesInDeclVisitor::VisitType(clang::Type *T) {
  processType(T);
  return true;
}

void DiscoverTypesInDeclVisitor::processType(const clang::Type *TP) {
  if (const clang::TypedefType *TDT = TP->getAs<clang::TypedefType>()) {
    OnEachTypeRef(TDT->getDecl());
  } else if (auto *TD = TP->getAsTagDecl()) {
    OnEachTypeRef(TD);
  }
}

DiscoverTypesInDeclVisitor::DiscoverTypesInDeclVisitor(
    TypeDeclResolver &Types) {
  OnEachTypeRef = [&Types](clang::Decl *D) { Types.addDecl(D); };
}

DiscoverFunctionsInDeclVisitor::DiscoverFunctionsInDeclVisitor(
    FunctionDeclResolver &Functions) {
  OnEachFuncRef = [&Functions](clang::FunctionDecl *FD) {
    Functions.addDecl(FD);
  };
}

bool DiscoverFunctionsInDeclVisitor::VisitExpr(clang::Expr *E) {
  clang::DeclRefExpr *DRE = llvm::dyn_cast<clang::DeclRefExpr>(E);
  if (DRE != nullptr) {
    if (auto *D = DRE->getDecl()) {
      if (auto *FD = llvm::dyn_cast<clang::FunctionDecl>(D)) {
        OnEachFuncRef(FD);
        auto *FDDefinition = FD->getDefinition();
        if (FDDefinition != FD && FDDefinition != NULL) {
          OnEachFuncRef(FDDefinition);
        }
      }
    }
  }
  return true;
}
