#include <string>
#include <iostream>

#include "../common/common.h"

#include "clang/AST/AST.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendActions.h"
#include "clang/Rewrite/Core/Rewriter.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"
#include "llvm/Support/raw_ostream.h"

using namespace std;
using namespace clang;
using namespace clang::ast_matchers;
using namespace clang::driver;
using namespace clang::tooling;

vector<string> lines;

string getCode(int l, int c, int size) {
  string line = lines[l-1];
  return line.substr(c, size);
}

void getPosition(const Stmt* stmt, Pos& s, Pos& e, ASTContext* context) {
  SourceManager& srcMgr = context->getSourceManager();
  PresumedLoc locStart, locEnd;
  locStart = srcMgr.getPresumedLoc(stmt->getBeginLoc());
  locEnd = srcMgr.getPresumedLoc(stmt->getEndLoc());
  s.line = locStart.getLine();
  s.column = locStart.getColumn();
  e.line = locEnd.getLine();
  e.column = locEnd.getColumn();
  
  
  return;
}

static llvm::cl::OptionCategory MatcherSampleCategory("Matcher Sample");


class PointerOpHandler : public MatchFinder::MatchCallback {
public:
  PointerOpHandler(Rewriter &Rewrite) : Rewrite(Rewrite) {}
  virtual void run(const MatchFinder::MatchResult &Result) {
    ASTContext *Context = Result.Context;
    SourceManager& srcMgr = Context->getSourceManager();
    const BinaryOperator *bop = Result.Nodes.getNodeAs<BinaryOperator>("bop");
    const UnaryOperator *uop = Result.Nodes.getNodeAs<UnaryOperator>("uop");
    const DeclRefExpr *subscriptDef = Result.Nodes.getNodeAs<DeclRefExpr>("subscriptDef");
    const ArraySubscriptExpr *subscript = Result.Nodes.getNodeAs<ArraySubscriptExpr>("subscript");
    
    
    if(subscriptDef && subscriptDef->getType().getTypePtr()->isPointerType()) {
      
      
      Rewrite.ReplaceText(SourceRange(subscript->getRHS()->getBeginLoc(), subscript->getRHS()->getEndLoc()), "0");
    }
    
    if(bop && bop->getLHS()->getType().getTypePtr()->isPointerType()) {
      Rewrite.ReplaceText(SourceRange(bop->getRHS()->getBeginLoc(), bop->getRHS()->getEndLoc()), "0");
    }
    if(bop && bop->getRHS()->getType().getTypePtr()->isPointerType()) {
      Rewrite.ReplaceText(SourceRange(bop->getLHS()->getBeginLoc(), bop->getLHS()->getEndLoc()), "0");
    }

    
    if(uop && uop->getSubExpr()->getType().getTypePtr()->isPointerType()) {
      UnaryOperatorKind op = uop->getOpcode();
      SourceLocation op_loc = uop->getOperatorLoc();
        Rewrite.RemoveText(op_loc, 2);
    }

    return;
  }
private:
  Rewriter &Rewrite;
};




class MyASTConsumer : public ASTConsumer {
public:
  MyASTConsumer(Rewriter &R) : HandlerForPop(R) {
    Matcher.addMatcher(binaryOperator(hasAnyOperatorName("+", "-","+=", "-=")).bind("bop") , &HandlerForPop);
    Matcher.addMatcher(unaryOperator(hasAnyOperatorName("++", "--")).bind("uop") , &HandlerForPop);
    Matcher.addMatcher(arraySubscriptExpr(hasBase(
      implicitCastExpr(hasSourceExpression(declRefExpr().bind("subscriptDef"))))).bind("subscript"), &HandlerForPop);
  }
  void HandleTranslationUnit(ASTContext &Context) override {
    
    Matcher.matchAST(Context);
  }

private:
  PointerOpHandler HandlerForPop;
  MatchFinder Matcher;
};



class MyFrontendAction : public ASTFrontendAction {
public:
  MyFrontendAction() {}
  void EndSourceFileAction() override {
    TheRewriter.getEditBuffer(TheRewriter.getSourceMgr().getMainFileID())
        .write(llvm::outs());
  }

  std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &CI,
                                                 StringRef file) override {
    TheRewriter.setSourceMgr(CI.getSourceManager(), CI.getLangOpts());
    return std::make_unique<MyASTConsumer>(TheRewriter);
  }

private:
  Rewriter TheRewriter;
};

int main(int argc, const char **argv) {

  auto ExpectedParser = CommonOptionsParser::create(argc, argv, MatcherSampleCategory);
  if (!ExpectedParser) {
    
    llvm::errs() << ExpectedParser.takeError();
    return 1;
  }
  CommonOptionsParser& OptionsParser = ExpectedParser.get();
  ClangTool Tool(OptionsParser.getCompilations(),
                 OptionsParser.getSourcePathList());

  return Tool.run(newFrontendActionFactory<MyFrontendAction>().get());
}