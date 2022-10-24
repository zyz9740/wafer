#include "../common/common.h"

#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"

using namespace clang;
using namespace clang::tooling;
using namespace llvm;
using namespace clang::ast_matchers;
using namespace std;

int file_id = 1;
int global_id = 1;

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

void getDeclPosition(const Decl* decl, Pos& s, Pos& e, ASTContext* context) {
  SourceManager& srcMgr = context->getSourceManager();
  PresumedLoc locStart, locEnd;
  locStart = srcMgr.getPresumedLoc(decl->getBeginLoc());
  locEnd = srcMgr.getPresumedLoc(decl->getEndLoc());
  s.line = locStart.getLine();
  s.column = locStart.getColumn();
  e.line = locEnd.getLine();
  e.column = locEnd.getColumn();
  
  
  return;
}



QualType getRootElement(QualType QType) {
  if(!QType.getTypePtr()->isArrayType()) return QType;
  QualType ele = QType.getTypePtr()->getAsArrayTypeUnsafe()->getElementType();
  while(ele.getTypePtr()->isArrayType()) ele = ele.getTypePtr()->getAsArrayTypeUnsafe()->getElementType();
  return ele;
}


static llvm::cl::OptionCategory MatcherSampleCategory("Matcher Sample");

class Printer: public MatchFinder::MatchCallback {

public :
  vector<Brick> data;
  vector<Signature> functions;
  map<string, TypeBase*> type_colloction;
  
  unsigned int sid = 1;
  Printer() {};

  PointerImpl* CreatePointer(QualType QType) {
    string name = QualType::getAsString(QType.split(), LangOptions());
    PointerImpl* pointerType = new PointerImpl(sid++, name);
    
    type_colloction[name] = pointerType;
    pointerType->ptr_type_sid = getVarType(QType.getTypePtr()->getPointeeType())->sid;
    return pointerType;
  }

  ArrayImpl* CreateArray(QualType QType) {
    string name = QualType::getAsString(QType.split(), LangOptions());
    vector<string> dims;
    
    vector<int> left_idx, right_idx;
    int pos = 0;
    int idx = -1;
    while((idx = name.find("[", pos)) != string::npos) {
        left_idx.push_back(idx);
        idx = name.find("]", idx);
        if(idx == string::npos) {
            cout << "Error array format!";
            exit(1);
        }
        right_idx.push_back(idx);
        pos = idx+1;
    }
    for(int i=0;i<left_idx.size();i++) {
        int range = right_idx[i]-left_idx[i]-1;
        dims.push_back(name.substr(left_idx[i]+1, range));
    }
    ArrayImpl* arrayType = new ArrayImpl(sid++, name, dims);
    
    type_colloction[name] = arrayType;
    
    QualType ele = getRootElement(QType);
    arrayType->ptr_type_sid = getVarType(ele)->sid;
    return arrayType;
  }
  
  
  TypedefImpl* CreateTypedef(QualType QType) {
    string name = QualType::getAsString(QType.split(), LangOptions());
    TypedefImpl* typedefType = new TypedefImpl(sid++, name);
    const TypedefType* RealType = QType.getTypePtr()->getAs<TypedefType>();
    
    type_colloction[name] = typedefType;
    TypeBase* realTypePtr = getVarType(RealType->desugar());
    
    typedefType->ptr_type_sid = realTypePtr->sid;
    return typedefType;
  }

  CompoundImpl* CreateCompoundType(QualType QType) {
    bool isUnion = QType.getTypePtr()->isUnionType();
    string name = QualType::getAsString(QType.split(), LangOptions());
    
    const Type* type = QType.getTypePtr();
    const RecordType* rt = isUnion ? type->getAsUnionType() : type->getAsStructureType();
    CompoundImpl* compoundType =  new CompoundImpl(sid++, name, isUnion);
    
    if(name.find("struct ") == name.npos && name.find("union ") == name.npos) compoundType->isTypedef = 1;
    
    if(QType.getTypePtr()->hasUnnamedOrLocalType()) { compoundType->isUnname = 1;}
    
    type_colloction[name] = compoundType;
    for (FieldDecl *FD : rt->getDecl()->fields()) {
      
      compoundType->fields_type.push_back(getVarType(FD->getType())->sid);
      compoundType->fields_name.push_back(FD->getNameAsString());
    }
    return compoundType;
  }
  
  ConstImpl* CreateConst(QualType QType) {
    string name = QualType::getAsString(QType.split(), LangOptions());
    ConstImpl* originType = new ConstImpl(sid++, name, QType.getCVRQualifiers());
    
    type_colloction[name] = originType;
    originType->ptr_type_sid = getVarType(QType.getUnqualifiedType())->sid;
    return originType;
  }

  FunctionPointerImpl* CreateFuncPointer(QualType QType) {
    string name = QualType::getAsString(QType.split(), LangOptions());
    FunctionPointerImpl* fp = new FunctionPointerImpl(sid++, name);
    type_colloction[name] = fp;
    if(QType.getTypePtr()->getPointeeType()->isFunctionNoProtoType()) {
      const FunctionNoProtoType* ft = QType.getTypePtr()->getPointeeType().getTypePtr()->getAs<FunctionNoProtoType>();
      TypeBase* return_type = getVarType(ft->getReturnType());
      fp->params_sid.push_back(return_type->sid);
    } else {
      const FunctionProtoType* ft = QType.getTypePtr()->getPointeeType().getTypePtr()->getAs<FunctionProtoType>();
      TypeBase* return_type = getVarType(ft->getReturnType());
      fp->params_sid.push_back(return_type->sid);
      for(int i=0;i<ft->getNumParams();i++) {
          TypeBase* param_type = getVarType(ft->getParamType(i));
          fp->params_sid.push_back(param_type->sid);
          string type = param_type->name;
      }
    }
    return fp;
  }

  EnumImpl* CreateEnum(QualType QType) {
    string name = QualType::getAsString(QType.split(), LangOptions());
    const EnumType* t = QType.getTypePtr()->getAs<EnumType>();
    vector<string> tmp = {t->getDecl()->getNameAsString()+"_1"};
    EnumImpl* EnumTp = new EnumImpl(sid++, name, tmp);
    cout << "Enum!" << endl;
    return EnumTp;
  }


  TypeBase* getVarType(QualType QType) {
    if(QType.isNull()) return nullptr;
    TypeBase* t = nullptr;
    string name = QualType::getAsString(QType.split(), LangOptions());
    cout << "Processing type " << name << endl << flush;
    
    map<string, TypeBase*>::iterator it = type_colloction.find(name);
    if(it != type_colloction.end()) return it->second;
    if(QType.getCVRQualifiers() && !QType.getTypePtr()->isArrayType()) {
      t = CreateConst(QType);
    } else if (QType.getTypePtr()->isTypedefNameType()) {
      t = CreateTypedef(QType);
    } else if(QType.getTypePtr()->isFunctionPointerType()) {
        t = CreateFuncPointer(QType);
    } else if (QType.getTypePtr()->isPointerType()) {
      t = CreatePointer(QType);
    } else if (QType.getTypePtr()->isArrayType()) {
      t = CreateArray(QType);
    } else if(QType.getTypePtr()->isStructureType() || QType.getTypePtr()->isUnionType()) {
      t = CreateCompoundType(QType);
    } else if(QType.getTypePtr()->isEnumeralType()) {
      t = CreateEnum(QType);
    } else {
      
      t = new SimpleImpl(sid++, name);
    }
    type_colloction[name] = t;
    cout << "Save type " << name << " as " << t->sid << endl;
    cout << t->dump().dump() << endl;
    return t;
  }


  
  string getStmtType(const MatchFinder::MatchResult &Result, string id) {
    if(Result.Nodes.getNodeAs<IfStmt>(id)) {
      return "if";
    }
    if (Result.Nodes.getNodeAs<WhileStmt>(id)) {
      return "while";
    }
    if (Result.Nodes.getNodeAs<ReturnStmt>(id)) {
      return "return";
    }
    if (Result.Nodes.getNodeAs<ForStmt>(id)) {
      return "for";
    }
    if (Result.Nodes.getNodeAs<DefaultStmt>(id)) {
      return "default";
    }
    if (Result.Nodes.getNodeAs<CaseStmt>(id)) {
      return "case";
    }
    if (Result.Nodes.getNodeAs<SwitchStmt>(id)) {
      return "switch";
    }
    if (Result.Nodes.getNodeAs<GotoStmt>(id)) {
      return "goto";
    }
    if (Result.Nodes.getNodeAs<LabelStmt>(id)) {
      return "label";
    }
    if (Result.Nodes.getNodeAs<DoStmt>(id)) {
      return "do";
    }
    return "unknown";
  } 

  
  Brick* findBrick(const Stmt* stmt, ASTContext* Context, const MatchFinder::MatchResult &Result) {
    Pos start(0, 0), end(0, 0);
    getPosition(stmt, start, end, Context);
    
    
    for(int i=0;i<data.size();i++) {
      if(data[i].start == start && data[i].end == end) {
        return &data[i];
      }
    }
    Brick b(start, end);
    b.type = getStmtType(Result, "stmt");
    b.id =  to_string(file_id) + "-" + to_string(global_id++);
    
    if(b.type == "case" || b.type == "switch" || b.type == "default" || b.type == "label" || b.type == "do") return nullptr;
    
    if(updateLoopPos(stmt, Context, &b) == -1) return nullptr;
    data.push_back(b);
    return &data[data.size()-1];
  }
  

  Signature* getSignature(const DeclRefExpr *declRefExpr) {
    Signature* s = new Signature();
    s->name = declRefExpr->getNameInfo().getAsString();
    stringstream sig;
    
    if(FunctionNoProtoType::classof(declRefExpr->getType().getTypePtr())) {
      const FunctionNoProtoType* ft = declRefExpr->getType().getTypePtr()->getAs<FunctionNoProtoType>();
      TypeBase* return_type = getVarType(ft->getReturnType());
      Variable* v = new Variable(return_type->name, "r0"); 
      v->type_sid = return_type->sid;
      s->returnValue = *v;
      sig << return_type->name << " " << s->name << "()";
      s->sig = sig.str();
      return s;
    }
    
    const FunctionProtoType* ft = declRefExpr->getType().getTypePtr()->getAs<FunctionProtoType>();
    TypeBase* return_type = getVarType(ft->getReturnType());
    Variable* v = new Variable(return_type->name, "r0"); 
    v->type_sid = return_type->sid;
    s->returnValue = *v;
    sig << return_type->name << " " << s->name << "(";
    for(int i=0;i<ft->getNumParams();i++) {
        TypeBase* param_type = getVarType(ft->getParamType(i));
        
        string name = "p"+itostr(i);
        Variable* v = new Variable(param_type->name, name);
        v->type_sid = param_type->sid;
        s->params.push_back(*v);
        string type = param_type->name;
        if(type.find("(*)") != type.npos) {
          sig << getFuncPointerStr(type, name);
        } else {
          sig << type << " " << name;
        }
        sig << ((i == ft->getNumParams()-1) ? "" : ", ");
    }
    if(ft->isVariadic()) sig << ", ...";
    sig << ")";
    s->sig = sig.str();
    return s;
  }

  
  Variable* getDeclRefExpr(const DeclRefExpr *declRefExpr, SourceManager& srcMgr ) {
    const QualType QType = declRefExpr->getType();
    
    string type = QualType::getAsString(QType.split(), LangOptions());
    string name = declRefExpr->getNameInfo().getAsString();
    PresumedLoc locStart = srcMgr.getPresumedLoc(declRefExpr->getBeginLoc());
    
    TypeBase* tp = getVarType(QType);
    Variable* v = new Variable(type, name);
    v->declRef = Pos(locStart.getLine(), locStart.getColumn());
    v->type_sid = tp->sid;
    return v;
  }

  
  
  int updateLoopPos(const Stmt* stmt, ASTContext* Context, Brick* b) {
    Pos start(0, 0), end(0,0);
    string bodyType;
    if(b->type == "if") {
      const IfStmt* ifStmt = static_cast<const IfStmt*>(stmt);
      const Stmt* body = ifStmt->getThen();
      getPosition(body, start, end, Context);
      bodyType = body->getStmtClassName();
    } else if(b->type == "for") {
      const ForStmt* forStmt = static_cast<const ForStmt*>(stmt);
      const Stmt* body = forStmt->getBody();
      getPosition(body, start, end, Context);
      bodyType = body->getStmtClassName();
      
      
      
      

      
      

      
      

    } else if(b->type == "while") {
      const WhileStmt* whileStmt = static_cast<const WhileStmt*>(stmt);
      const Stmt* body = whileStmt->getBody();
      getPosition(body, start, end, Context);
      bodyType = body->getStmtClassName();

      
      
      

    } else {
        return 0;
    }
    
    
    
    
    if(bodyType != "CompoundStmt") return -1;
    b->bodyStart = start;
    return 0;
  }

  virtual void run(const MatchFinder::MatchResult &Result) {
    const Stmt *stmt = Result.Nodes.getNodeAs<Stmt>("stmt");
    
    const VarDecl *varDecl = Result.Nodes.getNodeAs<VarDecl>("varDecl");
    const DeclRefExpr *declRefExpr = Result.Nodes.getNodeAs<DeclRefExpr>("declRefExpr");
    const Stmt *sizeOfStmt = Result.Nodes.getNodeAs<Stmt>("sizeof");
    const Expr *left = Result.Nodes.getNodeAs<Expr>("left");

    if (stmt && (varDecl || declRefExpr || sizeOfStmt || left)) {
      cout << "=================================" << endl;
      ASTContext *Context = Result.Context;
      SourceManager& srcMgr = Context->getSourceManager();
      
      if (!Context->getSourceManager().isWrittenInMainFile(stmt->getBeginLoc())) return;
      Brick* b = findBrick(stmt, Context, Result);
      if(b == nullptr) return;
      if(left) {
        const BinaryOperator *op = Result.Nodes.getNodeAs<BinaryOperator>("stmt");
        const Expr *right = Result.Nodes.getNodeAs<Expr>("right");
        Pos lstart(0, 0), lend(0, 0), rstart(0, 0), rend(0, 0);
        getPosition(left, lstart, lend, Context);
        getPosition(right, rstart, rend, Context);
        cout << op->getOpcodeStr().str() << lstart.str() << " " << lend.str() << " " << rstart.str() << " " << rend.str() << " " << endl;
        Shift* sh = new Shift(lstart, lend, rstart, rend, op->getOpcodeStr().str());
        b->sh = sh;
        return;
      }
      
      if(sizeOfStmt) {
        UnaryExprOrTypeTraitExpr* sizeofExpr = (UnaryExprOrTypeTraitExpr*)sizeOfStmt;
        QualType QType = sizeofExpr->getTypeOfArgument();
        TypeBase* t = getVarType(QType);
        if(t != nullptr) b->dependency_types_sid.push_back(t->sid);
        return;
      }
      
      if(varDecl) {
        cout << "varDecl " << varDecl->getName().str() << " : " << endl << flush;
        TypeBase* t = getVarType(varDecl->getType());
        if(t == nullptr) return;
        
        Variable v(t->name, varDecl->getName().str());
        
        if(v.name == "") return;
        
        
        PresumedLoc locStart = srcMgr.getPresumedLoc(varDecl->getLocation());
        v.declRef = Pos(locStart.getLine(), locStart.getColumn());
        v.type_sid = t->sid;
        if(varDecl->hasInit()) v.hasInit = true;
        
        b->postContext.push_back(v);
        
        
        
        
        
        
        

      }
      if(declRefExpr) {
        const QualType QualType = declRefExpr->getType();
        if(QualType.getTypePtr()->isFunctionType()) {
          
          Signature* s = getSignature(declRefExpr);
          for(Signature s_in_func : b->preFunc) {
            
            if(s_in_func.sig == s->sig) return;
          }
          b->preFunc.push_back(*s);
        } else {
            
            Variable* v = getDeclRefExpr(declRefExpr, srcMgr);
            if(v == nullptr) return;
            
            
            if(b->type == "for") {
              int localDef = false;
              for(Variable local : b->postContext) {
                if(local == *v) localDef = true;
              }
              if(localDef) return;
            }
            
            
            
            
            for(Variable old : b->preContext) {
              if(old.name == v->name && old.declRef == v->declRef)
                return;
            }
            b->preContext.push_back(*v);
        }
      }
    }
  }

  
  
  
  void insertCode(string src) {
    json j;
    ifstream is(src);
    if (!is.is_open())
    {
      cerr << "文件打开失败" << endl;
      return;
    }
    vector<string> lines;
    
    lines.push_back("");
    string line = "";
    while(getline(is, line, '\n')) {
      lines.push_back(line);
    }
    for(int i=0;i<data.size();i++) {
      
      Pos endPos = data[i].end;
      if(data[i].type == "for" || data[i].type == "if" || data[i].type == "while") {
        endPos = data[i].bodyStart;
        
        endPos.column--;
      }
      for(int j=data[i].start.line;j<=endPos.line;j++) {
        if(j == data[i].start.line) {
          
          data[i].code += lines[j].substr(data[i].start.column-1, lines[j].size()-data[i].start.column+1);
        } else if(j == endPos.line) {
          data[i].code += "\n";
          data[i].code += lines[j].substr(0, endPos.column+1);
        } else {
          data[i].code += "\n";
          data[i].code += lines[j];
        }
      }
      
    }
  }

  void print() {
    for(int i=0;i<data.size();i++) {
      Brick b = data[i];
      b.print();
    }
  }

  json dump() {
    json j;
    json bricks;
    for(int i=0;i<data.size();i++) {
      Brick b = data[i];
      bricks.push_back(b.dump());
    }
    j["bricks"] = bricks;
    json types_j;
    for(map<string, TypeBase*>::iterator it = type_colloction.begin(); it != type_colloction.end(); it++) {
      types_j.push_back(it->second->dump());
    }
    j["types"] = types_j;
    return j;
  }

  json dumpSig() {
    json j;
    for(int i=0;i<functions.size();i++) {
      Signature sig = functions[i];
      j.push_back(sig.dump());
    }
    return j;
  }
};












int main(int argc, char *argv[]) {
  
  const char** argv_fake = new const char*[2];
  argv_fake[0] = argv[0];
  argv_fake[1] = argv[1];
  int argc_fake = 2;
  
  int opt = 0;
  string source_path = "";
  while((opt = getopt(argc, argv, "s:i:")) != -1) {
    switch(opt) {
      case 's':
        cout << "Path: " <<  optarg << endl;
        source_path = optarg;
        break;
      case 'i':
        file_id = stoi(optarg);
        cout << "file id: " << file_id << endl;
        break;
    }
  }
  if(source_path.size() == 0) {
    cout << "Must specify source code path by -s !" << endl;
  }

  auto ExpectedParser = CommonOptionsParser::create(argc_fake, argv_fake, MatcherSampleCategory);
  if (!ExpectedParser) {
      
      errs() << ExpectedParser.takeError();
      return 1;
  }
  CommonOptionsParser& OptionsParser = ExpectedParser.get();
  ClangTool Tool(OptionsParser.getCompilations(),
                  OptionsParser.getSourcePathList());

  MatchFinder Finder;

  
  
  DeclarationMatcher VarDeclMatcher = 
    varDecl(hasAncestor(
      stmt(hasParent(
          compoundStmt(hasAncestor(
            functionDecl().bind("func")
          )))
      ).bind("stmt"))
    ).bind("varDecl");

  StatementMatcher DeclRefExprMatcher = 
    declRefExpr(hasAncestor(
      stmt(hasParent(
          compoundStmt(hasAncestor(
            functionDecl().bind("func")
          )))
      ).bind("stmt"))
    ).bind("declRefExpr");
  

	StatementMatcher sizeofMatcher = 
    unaryExprOrTypeTraitExpr(hasAncestor(
      stmt(hasParent(
          compoundStmt(hasAncestor(
            functionDecl().bind("func")
          )))
      ).bind("stmt"))
    ).bind("sizeof");

  StatementMatcher shiftMatcher =
    binaryOperator(
      hasAnyOperatorName(">>", "<<"),
      hasLHS(expr().bind("left")),
      hasRHS(expr().bind("right"))
    ).bind("stmt");

  Printer printer;
  Finder.addMatcher(VarDeclMatcher, &printer);
  Finder.addMatcher(DeclRefExprMatcher, &printer);
  Finder.addMatcher(sizeofMatcher, &printer);
  Finder.addMatcher(shiftMatcher, &printer);

  if(int r = Tool.run(newFrontendActionFactory(&Finder).get())) {
      return r;
  }
  cout << "Catch end. Start to insert code." << endl;
  
  string filename = OptionsParser.getSourcePathList()[0];
  printer.insertCode(filename);

  json j;
  j = printer.dump();
  writeFileJson(source_path, j);
  cout << "Split Succeed !" << endl;
  return 0;
}
