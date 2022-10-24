#include <string>
#include <iostream>
#include <vector>
#include <utility>
#include <fstream>
#include <sstream>
#include <ios>
#include <algorithm>

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

int getRand(int n) {
  assert(n != 0);
  struct timespec tp;
  clock_gettime(CLOCK_THREAD_CPUTIME_ID,&tp);
  srand(tp.tv_nsec);
  int r = rand() % n;
  return r;
}

void getDeadCodeLineNums(vector<int>& deadCodeLineNums) {
  ifstream scalerF("/tmp/random_coverage.txt", ios::in);
  char buf[64] = {0};
  while(scalerF.getline(buf, 64)) {
    string s(buf);
    deadCodeLineNums.push_back(stof(s));
  }
  scalerF.close();
}

static llvm::cl::OptionCategory MatcherSampleCategory("Matcher Sample");

class PruneVisit : public MatchFinder::MatchCallback {
public:
  PruneVisit(Rewriter &Rewrite) : Rewrite(Rewrite) {
    if (deadCodeLineNums.empty()) {
      getDeadCodeLineNums(PruneVisit::deadCodeLineNums);
    }
  }

  bool checkDeadCode(int startLineNum, int endLineNum) {
    for (int i = startLineNum; i <= endLineNum; ++i) {
      if(find(deadCodeLineNums.begin(), deadCodeLineNums.end(), i) == deadCodeLineNums.end()) {
        return false;
      }
    }
    return true;
  }

  virtual void run(const MatchFinder::MatchResult &Result) override {
    ASTContext *Context = Result.Context;
    SourceManager& srcMgr = Context->getSourceManager();
    const Stmt *stmt = Result.Nodes.getNodeAs<Stmt>("stmt");
    if (!stmt || !Context->getSourceManager().isWrittenInMainFile(stmt->getBeginLoc()) || 
        DeclStmt::classof(stmt) || LabelStmt::classof(stmt)) {
      return;
    }
    // cout << stmt->getStmtClassName() << "\t" << stmt->getBeginLoc().printToString(srcMgr) << "\t" << stmt ->getEndLoc().printToString(srcMgr) << "\n\n\n";
    PresumedLoc locStart, locEnd;
    locStart = srcMgr.getPresumedLoc(stmt->getBeginLoc());
    locEnd = srcMgr.getPresumedLoc(stmt->getEndLoc());
    int startLineNum = locStart.getLine();
    int endLineNum = locEnd.getLine();

    if (checkDeadCode(startLineNum, endLineNum) && getRand(2) == 0) {
      for (auto deleteLine : deleteLines) {
        if (startLineNum >= deleteLine.first && endLineNum <= deleteLine.second) {
          return;
        }
      }
      // cout << stmt->getStmtClassName() << "\t" << stmt->getBeginLoc().printToString(srcMgr) << "\t" << stmt ->getEndLoc().printToString(srcMgr) << "\n\n\n";
      Rewrite.RemoveText(stmt->getSourceRange());
      
      deleteLines.push_back(make_pair(startLineNum, endLineNum));
    }
    return;
  }

private:
  static vector<int> deadCodeLineNums;
  static vector<pair<int, int> > deleteLines;
  Rewriter &Rewrite;
};

// Implementation of the ASTConsumer interface for reading an AST produced
// by the Clang parser. It registers a couple of matchers and runs them on
// the AST.
class MyASTConsumer : public ASTConsumer {
public:
  MyASTConsumer(Rewriter &R) : HandlerForPrune(R) {
    // https://clang.llvm.org/docs/LibASTMatchersReference.html
    Matcher.addMatcher(stmt(hasParent(
      compoundStmt())).bind("stmt"), &HandlerForPrune);
    
  }

  void HandleTranslationUnit(ASTContext &Context) override {
    // Run the matchers when we have the whole TU parsed.
    Matcher.matchAST(Context);
  }

private: 
  PruneVisit HandlerForPrune;
  MatchFinder Matcher;
};

// For each source file provided to the tool, a new FrontendAction is created.
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

vector<pair<int, int> > PruneVisit::deleteLines = {};
vector<int> PruneVisit::deadCodeLineNums = {};


int main(int argc, const char **argv) {

  auto ExpectedParser = CommonOptionsParser::create(argc, argv, MatcherSampleCategory);
  if (!ExpectedParser) {
    // Fail gracefully for unsupported options.
    llvm::errs() << ExpectedParser.takeError();
    return 1;
  }
  CommonOptionsParser& OptionsParser = ExpectedParser.get();
  ClangTool Tool(OptionsParser.getCompilations(),
                 OptionsParser.getSourcePathList());

  

  return Tool.run(newFrontendActionFactory<MyFrontendAction>().get());
}