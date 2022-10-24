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


class ShiftHandler : public MatchFinder::MatchCallback {
public:
  ShiftHandler(Rewriter &Rewrite) : Rewrite(Rewrite) {}
  virtual void run(const MatchFinder::MatchResult &Result) {
    ASTContext *Context = Result.Context;
    SourceManager& srcMgr = Context->getSourceManager();
    const BinaryOperator *op = Result.Nodes.getNodeAs<BinaryOperator>("stmt");
    
    if(!op) return;
    const Expr *left = Result.Nodes.getNodeAs<Expr>("left");
    const Expr *right = Result.Nodes.getNodeAs<Expr>("right");
    QualType t = left->getType();
    string type = QualType::getAsString(t.split(), LangOptions());  
    string type_width = "sizeof(" + type + ")";
    Rewrite.InsertText(right->getBeginLoc(), "((");
    Rewrite.InsertTextAfterToken(right->getEndLoc(), ") % " + type_width + ")");
    return;
  }
private:
  Rewriter &Rewrite;
};

static llvm::cl::OptionCategory MatcherSampleCategory("Matcher Sample");

class MyASTConsumer : public ASTConsumer {
public:
  MyASTConsumer(Rewriter &R) : HandlerForShift(R) {
    
    
    Matcher.addMatcher(binaryOperator(
      hasAnyOperatorName(">>", "<<",">>=", "<<="),
      hasLHS(expr().bind("left")),
      hasRHS(expr().bind("right"))
    ).bind("stmt"), &HandlerForShift);

  }

  void HandleTranslationUnit(ASTContext &Context) override {
    
    Matcher.matchAST(Context);
  }

private:
  ShiftHandler HandlerForShift;
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