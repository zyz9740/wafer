#include "../common/common.h"


using namespace std;

#define LOOP_EXIT_PRO   5       
#define BREAK_PRO       1     
#define MAX_ITERATION   10   
#define MAX_LOOP_DEPTH  10    
#define CHOOSE_VAR_PRO  5     
#define DEFAULT_TURN    20     
#define DEFAULT_FUZZ    1      
#define GLOBAL_VAR      2        
#define ESCAPE_FUNC     "/home/oem/zyz/CrazyDiamond/lib/escape.func"  
#define ESCAPE_STRUCT   "/home/oem/zyz/CrazyDiamond/lib/escape.structs"  
#define ESCAPE_TD       "/home/oem/zyz/CrazyDiamond/lib/escape.typedef"  
#define DANGER_FUNC     "/home/oem/zyz/CrazyDiamond/lib/danger.func"  




vector<Brick> bricks;                                     


set<string> used_dependency_node;

map<string, int> escape_funcs;
map<string, int> danger_funcs;
set<string> init_used_dependency_node;  
set<string> skip_function = {"abort", "exit", "free", "scanf", "mempcpy", "transparent_crc", "platform_main_begin", "crc32_gentab", "platform_main_end"};
set<string> skip_word = {"off_t", "double_t", "_Atomic", "vector_size", "EINVAL","EBADF","EFAULT","ENOMEM"}; 
vector<Variable> simpleTypes = {};



bool filter(string code) {
  
  for(string s : skip_word) {
    if(code.find(s) != string::npos) {
      return true;
    }
  }
  return false;
}

string getLoopSkipStmt(int loop_depth) {
  int count = getRand(MAX_ITERATION);
  return "if(g_count[" + to_string(loop_depth-1) + "]++ > " + to_string(count) + ") {g_count[" +to_string(loop_depth-1)+ "] = 0;break;}";
}


void init() {
  for(string name : SimpleImpl::typeName) {
    TypeBase* tp = new SimpleImpl(0, name);
    string v_name = "g_" + to_string(simpleTypes.size());
    Variable v(tp->name, v_name);
    v.type_ptr = tp;
    simpleTypes.push_back(v);
  }
  return;
}


void get(string dir) {
  vector<string> files = getFiles(dir);
  int s_count = 0, b_count = 0, td_count = 0;
  int sum = 0;
  
  map<string, TypeBase*> name_type_map;
  for(string filename : files) {
    sum++;
    if(sum % 100 == 0) cout << "Loaded " << sum << " bricks..." << endl;
    string full_path = path_join(2, dir.c_str(), filename.c_str());
    ifstream fin(full_path);
    std::stringstream buffer;
    buffer << fin.rdbuf();
    string contents(buffer.str());
    
    vector<string> inter_ext = split(filename, '.');
    if(inter_ext.size() < 2 || inter_ext[inter_ext.size()-1] != "json") continue;
    json j;
    j = json::parse(contents);
    
    map<unsigned int, TypeBase*> sid_type_map;
    for (json::iterator it = j["types"].begin(); it != j["types"].end(); ++it) {
      
      TypeBase* tp = restoreType(*it, name_type_map);
      unsigned int sid;
      (*it)["sid"].get_to(sid);
      sid_type_map[sid] = tp;
    }
    for(map<unsigned int, TypeBase*>::iterator it = sid_type_map.begin(); it != sid_type_map.end(); it++) {
      TypeBase* tp = it->second;
      tp->restorePtr(sid_type_map);
    }
    for (json::iterator it = j["bricks"].begin(); it != j["bricks"].end(); ++it) {
      Brick b = Brick::restore(*it, sid_type_map);
      b.file = filename;
      
      if(b.code.find("register ") != string::npos)
        
        continue;
      bricks.push_back(b);
      b_count++;
    }
    
    fin.close();
  }
  cout << "b_count" << b_count  << " s_count " << s_count << endl;
  return ;
}

void insertHeader(ofstream& out) {
  
  
  out << "#include \"cd.h\"" << endl;
  out << endl;
  return;
}



string hash_variable(Variable v, set<string>& hashed) {
  return v.type_ptr->getHashStmt(v.name, hashed);
}


void insertBody(ofstream& out, DependencyNode& root, vector<Signature>& funcs, string body, int depth, 
  vector<Variable>& global_vars, vector<vector<Variable>> loop_context, vector<bool> loop_state, int loop_depth) {
    
  
  set<string> printed_dependency_node;
  out << root.print(printed_dependency_node);

  
  string gv_init = "";
  vector<TypeBase*> gv_tp;
  
  string g_count_stmt = "unsigned int g_count[" + to_string(MAX_LOOP_DEPTH) + "] = {";
  for(int i=0;i<MAX_LOOP_DEPTH-1;i++) g_count_stmt += "0,";
  g_count_stmt += "0};\n";
  out << g_count_stmt;
  for(int i=0;i<global_vars.size();i++) {
    Variable v = global_vars[i];
    
    
    
    
    
    
    string res;
    res += v.type_ptr->name;
    res += " ";
    res += v.name;
    
    out << res  << ";" << endl;
    
    res = v.name;
    res += " = ";
    if(i < simpleTypes.size()) {
      
      string type = v.type_ptr->name;
      if(type == "char" || type == "unsigned char" || type == "signed char") res += "\'a\'";
      else if(type == "float" || type == "double") res += "1.0";
      else res += "1";
    } else {
      res += v.type_ptr->getInitialization(gv_tp);
    }
    res += ";\n";
    gv_init += res;
    gv_tp.push_back(v.type_ptr);
  }

  
  string funcs_out;
  for(Signature s : funcs) funcs_out += s.print(gv_tp);
  
  out << funcs_out;
  out << "int main() { \n" << "crc32_gentab();\n";
  out << gv_init;
  out << "
  out << body;
  
  cout << "depth: " << depth << endl;
  string tail_hash = "";
  for(int i=0;i<depth;i++) {
    set<string> hashed;
    for(Variable v : loop_context[loop_context.size()-1]) {
      tail_hash += hash_variable(v, hashed);
    }
    if(loop_state.back() && getRand(BREAK_PRO) == 0) {
      tail_hash += getLoopSkipStmt(loop_depth--);
      
    }
    loop_state.pop_back();
    tail_hash += "}";
    loop_context.pop_back();
  }
  out << tail_hash;
  return;
}


void insertTail(ofstream& out, vector<Variable>& cur_context) {
  
  out << endl << "
  set<string> hashed;
  for(Variable v : cur_context) {
    out << hash_variable(v, hashed);
  }
  out << "printf (\"checksum = %X\\n\", crc32_context ^ 0xFFFFFFFFUL);\n";
  out << "return 0;\n";
  out << "}\n";
  return;
}


void del(vector<Brick>& bricks, int i) {
  bricks[i] = bricks[bricks.size()-1];
  bricks.pop_back();
}

vector<string> splitFile(string file) {
  ifstream fin(file);
  std::stringstream buffer;
  buffer << fin.rdbuf();
  string contents(buffer.str());
  vector<string> res = split(contents, '\n');
  return res;
}


void getEscape() {
  vector<string> funcs_list = splitFile(ESCAPE_FUNC);
  for(string func : funcs_list) {
    escape_funcs[func] = 1;
  }
  vector<string> funcs_list_danger = splitFile(DANGER_FUNC);
  for(string func : funcs_list_danger) {
    danger_funcs[func] = 1;
  }
  
  vector<string> struct_list = splitFile(ESCAPE_STRUCT);
  for(string s : struct_list) {
    init_used_dependency_node.insert("struct "+s);
  }
  vector<string> td_list = splitFile(ESCAPE_TD);
  for(string td : td_list) {
    init_used_dependency_node.insert(td);
    
    init_used_dependency_node.insert("const " + td);
  }
  return;
}


bool getCompoundTypeContext(TypeBase* type, string name, vector<Variable>& cur_context, int depth) {
  if(depth > 1) return true;
  
  TypeBase* tp = type->getDesugaredTypePtr();
  bool isStruct = false, isStructPointer = false;
  if(tp->eType == eStruct || tp->eType == eUnion) isStruct = true;
  if(tp->eType == ePointer && (tp->ptr_type->getDesugaredTypePtr()->eType == eUnion || tp->ptr_type->getDesugaredTypePtr()->eType == eStruct)) isStructPointer = true;
  if(isStruct || isStructPointer) {
    
    CompoundImpl* compoundTp;
    if(isStruct) compoundTp = (CompoundImpl*) tp;
    else compoundTp = (CompoundImpl*) tp->ptr_type->getDesugaredTypePtr(); 
    for(int i=0;i<compoundTp->fields_type_ptr.size();i++) {
      TypeBase* fieldTp = compoundTp->fields_type_ptr[i];
      string op = isStruct ? "." : "->" ;
      if(compoundTp->isUnname && compoundTp->fields_name[i] == "") op = "";
      string field_name = name + op + compoundTp->fields_name[i];
      if(!getCompoundTypeContext(fieldTp, field_name, cur_context, depth+1)) {
        Variable new_var(fieldTp->name, field_name);
        new_var.type_ptr = fieldTp;
        cur_context.push_back(new_var);
      }
    }
    return true;
  } else {
      return false;
  }
}


void generatePostContext(vector<Variable> post_context, vector<Variable>& cur_context) {
  for(Variable v : post_context) {
    
    if(v.name == "") continue;
    getCompoundTypeContext(v.type_ptr, v.name, cur_context, 0);
    
    cur_context.push_back(v);
  }
  return;
}


int compile(string target) {
  
  char compile_cmd[500];
  pid_t status;
  
  
  
  
  
  
  
  
  

  
    
    
    
    
    
    
    
    

    memset(compile_cmd, '\0', 500);
    sprintf(compile_cmd, "/opt/cheerp/bin/clang -w -fsyntax-only -target cheerp-wasm -I /home/oem/zyz/CrazyDiamond/include %s -o %s.cheerp.js", target.c_str(), target.c_str());
    cout << compile_cmd << endl << flush;
    status = system(compile_cmd);
    if (-1 == status || !WIFEXITED(status) || WEXITSTATUS(status)) {
        cout << "system error!" << endl;
        return 1;
    }
  
  return 0;
}



int main(int argc, char *argv[]) {
  
  int opt = 0;
  string source_path = "", target_dir = "";
  int turns = DEFAULT_TURN;
  int files = DEFAULT_FUZZ;
  int step_compile = 1;
  while((opt = getopt(argc, argv, "s:o:l:r:f:c:")) != -1) {
    switch(opt) {
      case 's':
        cout << "Path: " <<  optarg << endl;
        source_path = optarg;
        break;
      case 'o':
        cout << "Target: " << optarg << endl;
        target_dir = optarg;
        break;
      case 'l':
        turns = stoi(optarg);
        break;
      case 'c':
        step_compile = 1;
        break;
      case 'r': 
      {
        randAction = 0;
        ifstream fin(optarg);
        std::stringstream buffer;
        buffer << fin.rdbuf();
        string contents(buffer.str());
        vector<string> tmp = split(contents, ' ');
        for(string n : tmp) randRecord.push_back(stoi(n));
        break;
      }      
      case 'f':
        files = stoi(optarg);
        break;
    }
  }
  if(source_path.size() == 0) {
    cout << "Must specify source code path by -s !" << endl;
    return 1;
  }
  if(target_dir.size() == 0) {
    cout << "Must specify target path by -o !" << endl;
    return 1;
  }
  srand((unsigned)time(NULL));
  
  getEscape();
  used_dependency_node = init_used_dependency_node;
  
  cout << "Loading database..." << endl;
  init();
  get(source_path);
  cout << "Loaded!" << endl;
  
  vector<Brick> pureDeclBricks; 
  vector<Brick> returnBricks;   
  vector<Brick> funcBricks;   
  int i = 0;
  while(i < bricks.size()) {
    if(bricks[i].preFunc.size() != 0) {
      funcBricks.push_back(bricks[i]);
      del(bricks, i);
    }
    if(bricks[i].type == "return") {
      returnBricks.push_back(bricks[i]);
      del(bricks, i);
    } else if(bricks[i].preContext.size() == 0) {
      
      if(bricks[i].postContext.size() > 0) {
        pureDeclBricks.push_back(bricks[i]);
        
      }
      
      del(bricks, i);
      
    } else {
      i++;
    }
  }
  cout << "纯声明语句数量：" << pureDeclBricks.size() << endl;
  cout << "return语句数量：" << returnBricks.size() << endl;
  cout << "func语句数量：" << funcBricks.size() << endl;
  
  cout << "语句数量：" << bricks.size() << endl;

  
  int fuzz_count = 0;
  while(fuzz_count < files){
    string filename = "fuzz" + to_string(fuzz_count++) + ".c";
    string target_path = path_join(2, target_dir.c_str(), filename.c_str());
    cout << "Start generating file " << target_path << " ..." << endl;
    
    DependencyNode root;                          
    vector<Variable> global_vars = simpleTypes;   
    vector<Variable> cur_context;                 
    string body = "";                             
    vector<Signature> funcs;                      
    int depth = 0;                                
    int loop_depth = 0;                           
    vector<vector<Variable>> loop_context;        
    vector<bool> loop_state;                       
    bool dont_exit_loop = 0;                      
    cout << "cur_context: " << cur_context.size() << endl;

    
    for(int turn = 0;turn < turns; turn++) {
      cout << "============================="  << endl;
      
      
      
      
      
      
      
      
      
      
      

      
      if(depth > 0 && !dont_exit_loop && getRand(LOOP_EXIT_PRO) == 0) {
        set<string> hashed;
        for(Variable v : loop_context[loop_context.size()-1]) {
          body += hash_variable(v, hashed);
        }
        
        
        
        if(loop_state.back() && getRand(BREAK_PRO) == 0) {
          body += getLoopSkipStmt(loop_depth);
          
        }
        body += "}";
        if(loop_state.back()) loop_depth--;
        loop_state.pop_back();
        depth--;
        loop_context.pop_back();
        cout << "exit loop, loop_context size:" << loop_context.size() << endl;
      } else dont_exit_loop = 0;

      
      
      vector<Variable> real_context = cur_context;
      for(Variable v : global_vars) real_context.push_back(v);
      for(vector<Variable> c:loop_context) {
        for(Variable v : c) {
          real_context.push_back(v);
        }
      }
      
      Brick chosen_brick;
      Brick* chosen = &chosen_brick;              
      Brick* stat;                              
      string new_code = "";
      
      DependencyNode* tmp_root = new DependencyNode();
      vector<Signature> tmp_funcs;
      vector<Variable> tmp_global_vars = global_vars;
      
      bool choose_varDecl = (turn <= 3 || getRand(CHOOSE_VAR_PRO) == 0);
      if(!choose_varDecl) {
        int try_count = 1;
        
        while(try_count <= bricks.size()) {
          int rand = getRand(bricks.size());
          stat = &bricks[rand];
          map<int, string> renameMap = isSubset(real_context, stat->preContext);
          if(renameMap.size() != 0 && stat->fail_count == 0 && !filter(stat->code)) {
            *chosen = *stat;
            
            
            new_code = chosen->rename(renameMap);
            
            if(new_code.size() == 0) continue;
            if(loop_depth > MAX_LOOP_DEPTH && (chosen->type == "for" || chosen->type == "while"))
            cout << new_code << endl;
            break;
          }
          try_count++;
        }
        if(try_count > bricks.size()) {
          cout << "超过尝试次数！" << endl;
          
          choose_varDecl = true;
        }
        cout << "尝试次数： " << try_count << endl;
      }
      if(choose_varDecl) {
        
        do {        
          int rand = getRand(pureDeclBricks.size());
          stat = &pureDeclBricks[rand];
          *chosen = pureDeclBricks[rand];
        } while(stat->fail_count > 0);
        
        cout << chosen->code << endl;
        cout << "choose varDecl" << endl;
      }
      
      int nameDup = 0;
      for(Variable new_v : chosen->postContext) {
        for(Variable now_context : real_context) {
          if(new_v.name == now_context.name) {
            nameDup = 1;
            break;
          }
        }
      }
      if(nameDup) {
        cout << "选取brick中含有重名变量，跳过" << endl;
        continue;
      }
      
      
      for(vector<Variable>::iterator it = chosen->postContext.begin(); it < chosen->postContext.end(); it++) {
        
        vector<TypeBase*> new_gv_tp;
        DependencyNode* d = it->type_ptr->saveDependency(used_dependency_node, new_gv_tp);
        
        for(TypeBase* tp : new_gv_tp) {
          string v_name = "g_" + to_string(tmp_global_vars.size());
          Variable v(tp->name, v_name);
          v.type_ptr = tp;
          tmp_global_vars.push_back(v);
        }
        if(d) tmp_root->dependency.push_back(d);
      }
      for(vector<TypeBase*>::iterator it = chosen->dependency_types.begin(); it < chosen->dependency_types.end(); it++) {
        
        vector<TypeBase*> dummy;
        DependencyNode* d = (*it)->saveDependency(used_dependency_node, dummy);
        if(d) tmp_root->dependency.push_back(d);
      }
      
      if(choose_varDecl) {
          chosen->addInitialization(tmp_global_vars);
          new_code = chosen->code;
      }
      
      int fail = 0;
      for(Signature s : chosen->preFunc) {
        
        if(skip_function.find(s.name) != skip_function.end()) {fail = 1; break;}
        if(escape_funcs.find(s.name) != escape_funcs.end()) continue;
        if(danger_funcs.find(s.name) != danger_funcs.end()) {fail = 1; break;};
        
        int found = 0;
        for(Signature s_in_funcs: funcs) if(s.name == s_in_funcs.name) found = 1;
        for(Signature s_in_funcs: tmp_funcs) if(s.name == s_in_funcs.name) found = 1;
        
        if(!found) {
          tmp_funcs.push_back(s);
          s.params.push_back(s.returnValue);
          for(Variable p : s.params) {
              vector<TypeBase*> dummy;
              DependencyNode* d = p.type_ptr->saveDependency(used_dependency_node, dummy);
              if(d) tmp_root->dependency.push_back(d);
          }
          s.params.pop_back();
        }
      } 
      if(fail) continue;
      
      vector<Variable> tmp_context;
      generatePostContext(chosen->postContext, tmp_context);
      if(chosen->type == "for" || chosen->type == "while" || chosen->type == "if") {
        
        depth++;
        
        if(chosen->type == "for" || chosen->type == "while") {
          loop_state.push_back(true);
          loop_depth++;
        } else
          loop_state.push_back(false);
        loop_context.push_back(tmp_context);
        cout << "choose condition" << endl;
        dont_exit_loop = 1;
        
        if(chosen->code[chosen->code.size()-1] != '{') chosen->code += "{";
      } else {
        if(depth == 0) 
          for(Variable v : tmp_context) cur_context.push_back(v);
        else 
          for(Variable v : tmp_context) loop_context[depth-1].push_back(v);
      }

      
      if(step_compile) {
        
        string body_try = body + new_code + "\n";
        root.dependency.push_back(tmp_root);
        for(Signature s : tmp_funcs) funcs.push_back(s);
        
        ofstream tmp_out;
        string path_try = target_dir + "/tmp.c";
        tmp_out.open(path_try, ios::out);
        insertHeader(tmp_out);
        insertBody(tmp_out, root, funcs, body_try, depth, tmp_global_vars, loop_context, loop_state, loop_depth);
        insertTail(tmp_out, cur_context);
        
        tmp_out.close();
        if(compile(path_try)) {
          
          cout << chosen->code << ":插入失败，放弃。" << endl;
          stat->fail_count++;
          
          root.dependency.pop_back();
          for(Signature s : tmp_funcs) funcs.pop_back();
          if(chosen->type == "for" || chosen->type == "while" || chosen->type == "if") {
            depth--;
            loop_context.pop_back();
            if(loop_state.back()) loop_depth--;
            loop_state.pop_back();
            dont_exit_loop = 0;
          } else {
            if(depth == 0) 
              for(Variable v : tmp_context) cur_context.pop_back();
            else 
              for(Variable v : tmp_context) loop_context[depth-1].pop_back();
          }
          continue;
        } else {
          cout << chosen->code << ":插入成功！" << endl;
          global_vars = tmp_global_vars;
          
          body += new_code;
          body += '\n';

        }
      }

      
      
      
      
      
      
      
      
      
      
      
      
      
      

      
    }
    
    ofstream out;
    out.open(target_path, ios::out);
    insertHeader(out);
    insertBody(out, root, funcs, body, depth, global_vars, loop_context, loop_state, loop_depth);
    insertTail(out, cur_context);
    out.close();
    
    for(int i : randRecord) cout << i << " ";
    cout << endl;
    if(randAction == -1) {
      randRecord.clear();
    }
    
    char cmd[100];
    sprintf(cmd, "clang-format-9 %s -i", target_path.c_str());
    system(cmd);
    
    
    
    
    used_dependency_node = init_used_dependency_node;
    
  }
}