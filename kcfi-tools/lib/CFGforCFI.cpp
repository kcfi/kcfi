// kcfi-tools Copyright (C) 2015 Universidade Estadual de Campinas
//
// This software was developed by Joao Moreira <jmoreira@suse.de>
// at Universidade Estadual de Campinas, SP, Brazil, in 2015.
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.

#include <stdio.h>
#include <string.h>
#include <cstdlib>
#include <fstream>
#include <list>
#include <sstream>
#include <regex>
#include <sys/file.h>
#include "CFGforCFI.h"
#include <iostream>
#include <dirent.h>

unsigned int last_id = 1;
unsigned int last_tag = 1;

void CFICFG::storeTags(){
  std::ofstream f;
  std::string filen = "tags.cfi";
  f.open(filen, std::ios::binary | std::ios::out);
  f.write((char *) &last_tag, sizeof(last_tag));
  f.close();
}

void CFICFG::loadTags(){
  std::ifstream f;
  f.open("tags.cfi", std::ios::binary | std::ios::in);
  f.read((char *) &last_tag, sizeof(last_tag));
  f.close();
}

void CFICFG::lockTags(FILE** fd){
  *fd = fopen("tags.lock", "r");
  if(*fd == NULL){
    *fd = fopen("tags.lock", "w");
  }
  while(flock(fileno(*fd), LOCK_EX)){};
}

void CFICFG::unlockTags(FILE** fd){
  while(flock(fileno(*fd), LOCK_UN)){};
  fclose(*fd);
}

unsigned int CFICFG::getNextTag(){
  FILE *lock;
  lockTags(&lock);
  loadTags();
  last_tag++;
  if(last_tag > 0x00dead00 && last_tag < 0x00deadff) last_tag = 0x00deae00;
  if(last_tag > 0x8fffffff) fprintf(stderr, "TAG OVERFLOW %x.\n", last_tag);
  storeTags();
  unlockTags(&lock);
  return last_tag;
}

void CFICFG::storeIds(){
  std::ofstream f;
  std::string filen = "ids.cfi";
  f.open(filen, std::ios::binary | std::ios::out);
  f.write((char *) &last_id, sizeof(last_id));
  f.close();
}

void CFICFG::loadIds(){
  std::ifstream f;
  f.open("ids.cfi", std::ios::binary | std::ios::in);
  f.read((char *) &last_id, sizeof(last_id));
  f.close();
}

void CFICFG::lockIds(FILE** fd){
  *fd = fopen("ids.lock", "r");
  if(*fd == NULL){
    *fd = fopen("ids.lock", "w");
  }
  while(flock(fileno(*fd), LOCK_EX)){};
}

void CFICFG::unlockIds(FILE** fd){
  while(flock(fileno(*fd), LOCK_UN)){};
  fclose(*fd);
}

unsigned int CFICFG::getNextId(){
  FILE *lock;
  lockIds(&lock);
  loadIds();
  last_id++;
  if(last_id > 0x8fffffff) fprintf(stderr, "ID OVERFLOW %x.\n", last_id);
  storeIds();
  unlockIds(&lock);
  return last_id;
}

int CFICFG::countNodes(std::string proto){
  std::list<CFINode>::iterator i, ie;
  i = CFINodes.begin();
  ie = CFINodes.end();
  int n = 0;
  for(; i != ie; i++){
    if(strcmp(i->proto, proto.c_str())==0) n++;
  }
  return n;
}

bool CFICFG::mergeItens(unsigned int id1, unsigned int id2){
  CFICluster c1, *c2;
  CFINode n1, *n2;
  unsigned int new_hid, new_tid;

  n1 = getNode(id1);
  if(n1.id){
    new_hid = n1.head_id;
    new_tid = n1.tail_id;
  } else {
    if(cluster_id_map.count(id1)==0){
      printf("ID1 not found\n");
      return false;
    }
    c1 = *cluster_id_map[id1];
    new_hid = c1.head_id;
    new_tid = c1.tail_id;
  }

  if(node_id_map.count(id2)>0){
    n2 = node_id_map[id2];
    n2->head_id = new_hid;
    n2->tail_id = new_tid;
    return true;
  }

  if(cluster_id_map.count(id2)>0){
    c2 = cluster_id_map[id2];
    c2->head_id = new_hid;
    c2->tail_id = new_tid;
    return true;
  }

  printf("ID2 not found\n");
  return false;
}

void CFICFG::dumpFunctionMap(){
  std::map<std::string, char *>::iterator i, ie;
  i = function_map.begin();
  ie = function_map.end();
  for(; i != ie; i++){
    fprintf(stderr, "%s %s\n", i->first.c_str(), i->second);
  }
}

bool CFICFG::fixDecl(std::string name, std::string oldp, std::string newp){
  std::list<CFIDecl>::iterator i = CFIDecls.begin();
  std::list<CFIDecl>::iterator ie = CFIDecls.end();

  for(; i != ie; i++){
    if((strcmp(i->name, name.c_str())==0)&&(strcmp(i->proto, oldp.c_str())==0)){
      strcpy(i->name, newp.c_str());
      return true;
    }
  }
  return false;
}

std::string CFICFG::getDeclProto(std::string name){
  if(decls_n_map.count(name) > 0) return decls_n_map[name];
  return "";
}

void CFICFG::loadFunctionIds(){
  unsigned int id;
  std::map<std::string, char *>::iterator i, ie;
  i = function_map.begin();
  ie = function_map.end();
  for(; i != ie; i++){
    id = getIdFastest(i->first);
    if(id != 0) function_id_map[i->first] = id;
  }
}

CFINode CFICFG::createAsmNode(std::string name, std::string addr){
  CFIUtils u;
  unsigned int g_id;
  CFINode h;

  h.head_id = 0x1337beef;
  h.tail_id = getNextTag();
  g_id = getNextId();

  std::string type = "ASM";

  CFINode n(name.c_str(), type.c_str(), addr.c_str(), g_id, h.head_id,
            h.tail_id, 0);

  addToGraph(n);
  return n;
}

CFINode CFICFG::createDataNode(std::string name, std::string addr){
  CFIUtils u;
  unsigned int g_id;
  CFINode h;

  h.head_id = 0x1337b00b;
  h.tail_id = 0x1337b00b;
  g_id = getNextId();

  std::string type = "DATA";

  CFINode n(name.c_str(), type.c_str(), addr.c_str(), g_id, h.head_id,
            h.tail_id, 0);

  addToGraph(n);
  return n;
}

void CFICFG::addParsedFunction(std::string address, char * p){
  if(function_map.find(address) != function_map.end())
    fprintf(stderr, "ERROR: overwriting function that already has an entry\n");
  p = p + 1 + strlen(p); // point to first line instead of header
  function_map[address] = p;
}

unsigned int CFICFG::getOrder(ctype t, unsigned int org, unsigned int tgt){
  unsigned int o = 0;
  std::list<CFIEdge>::iterator i;
  std::list<CFIEdge>::iterator ie;

  i = CFIEdges.begin();
  ie = CFIEdges.end();

  for(; i != ie; ++i){
    if(i->type == t && i->origin == org && i->target == tgt) o++;
  }
  return o;
}

unsigned int CFICFG::getTailJmps(unsigned int to, std::list<CFIEdge>& tgts){
  std::list<CFIEdge>::iterator i;
  std::list<CFIEdge>::iterator ie;
  int n = 0;

  i = CFIEdges.begin();
  ie = CFIEdges.end();

  for(; i != ie; ++i){
    if(i->type == jmp && i->target == to){
      tgts.push_back(*i);
      n++;
    }
  }
  return n;
}

unsigned int CFICFG::getDCalls(unsigned int to, std::list<CFIEdge>& tgts){
  std::list<CFIEdge>::iterator i;
  std::list<CFIEdge>::iterator ie;
  int n = 0;

  i = CFIEdges.begin();
  ie = CFIEdges.end();

  for(; i != ie; ++i){
    if(i->type == dcall && i->target == to){
      tgts.push_back(*i);
      n++;
    }
  }
  return n;
}

CFIEdge CFICFG::createJmpEdge(unsigned int from, unsigned int to){
  unsigned int g_id = getNextId();

  unsigned int o = getOrder(jmp, from, to);
  CFIEdge e(jmp, g_id, from, to, o);

  addToGraph(e);
  return e;
}

CFIEdge CFICFG::createDirectEdge(unsigned int from, unsigned int to){
  unsigned int g_id = getNextId();

  // Skip repeated edges to reduce data structure size and improve speed.
  // If duplicated edges are needed, remove or comment the line below.
  long key = (long) from << 32 | to;
  if(edge_ot_map.count(key) > 0) return *edge_ot_map[key];

  CFIEdge e(dcall, g_id, from, to, 0);
  addToGraph(e);
  return e;
}

unsigned int CFICFG::getIdByAddressThin(std::string dump, std::string address){
  unsigned int id, v1, v2;
  std::stringstream pstream;
  std::ifstream file(dump);
  std::string line;
  pstream << "^" << address << ":.*movl[^%]+$";
  std::string pattern = pstream.str();
  std::regex rgx(pattern);
  std::string validator = "^[1234567890abcdef]+$";
  std::regex validate(validator);
  std::stringstream hex_to_int;
  std::smatch match;
  while(std::getline(file, line)){
    if(std::regex_search(line, match, rgx)){
      std::string sid = line.substr(28,8);
      if(std::regex_search(sid, match, validate)){
        file.close();
        hex_to_int << std::hex << sid;
        hex_to_int >> id;
        return id;
      }
      else break;
    }
  }
  file.close();
  return 0;
}

CFIEdge CFICFG::createJmpEdge(std::string dump,
                              std::string from, std::string to){
  unsigned int g_id = getNextId();

  std::string f_addr = getAddress(dump, from);
  unsigned int f_id = getIdByAddressThin(dump, f_addr);

  std::string t_addr = getAddress(dump, to);
  unsigned int t_id = getIdByAddressThin(dump, t_addr);

  CFIEdge e = createJmpEdge(f_id, t_id);
  if(!e.id){
    std::cerr << "COULD NOT JMP EDGE: " << from << "->" << to <<"\n";
  }
  return e;
}

CFIEdge CFICFG::createDirectEdge(std::string dump,
                                 std::string from, std::string to){
  unsigned int g_id = getNextId();

  std::string f_addr = getAddress(dump, from);
  unsigned int f_id = getIdByAddressThin(dump, f_addr);

  std::string t_addr = getAddress(dump, to);
  unsigned int t_id = getIdByAddressThin(dump, t_addr);

  CFIEdge e = createDirectEdge(f_id, t_id);
  if(!e.id){
    std::cerr << "COULD NOT CREATE EDGE: " << from << "->" << to <<"\n";
  }
  return e;
}

std::string CFICFG::getAddress(std::string dump, std::string f){
  std::stringstream pstream;
  pstream << "^[1234567890abcdef]+ <" << f << ">:$";
  std::smatch match;
  std::regex pattern(pstream.str());
  std::ifstream file(dump);
  std::string line;
  while(std::getline(file, line)){
    if(std::regex_search(line, match, pattern)){
      std::string sid = line.substr(0,16);
      std::cerr << sid << "\n";
      return sid;
    }
  }
  return "";
}

CFICluster CFICFG::getCluster(std::string proto){
  CFICluster *c = cluster_p_map[proto];
  CFIUtils u;
  if(c) return *c;
  else return u.returnFCluster();
}

bool CFICFG::fixClusterProto(unsigned int id, std::string proto){
  CFICluster *c = cluster_id_map[id];
  if(c){
    strcpy(c->proto, proto.c_str());
    return true;
  }
  return false;
}



bool CFICFG::fixProto(unsigned int id, std::string proto){
  CFINode *n = node_id_map[id];
  if(n){
    strcpy(n->proto, proto.c_str());
    return true;
  }
  return false;
}

bool CFICFG::declsLoaded(){
  if(decls_map.size() > 0) return true;
  return false;
}

bool CFICFG::fixNodeIds(unsigned int id, unsigned int hid, unsigned int tid){
  CFINode *n = node_id_map[id];
  if(n){
    if(hid) n->head_id = hid;
    if(tid) n->tail_id = tid;
    return true;
  }
  return false;
}

bool CFICFG::fixIds(unsigned int id, unsigned int hid, unsigned int tid){
  CFINode *n = node_id_map[id];
  if(n){
    if(hid) n->head_id = hid;
    if(tid) n->tail_id = tid;
    if(!declsLoaded()){
      std::cerr << "Decls not Loaded!\n";
      return true;
    }
    std::cerr << "name: " << n->name << "\n";
    std::string p = getDeclProto(n->name);
    std::cerr << "proto: " << p << "\n";
    CFICluster *c = cluster_id_map[getCluster(p).id];
    if(c){
      std::cerr << "FOUND CLUSTER\n";
      if(hid) c->head_id = hid;
      if(tid) c->tail_id = tid;
    }
    if(!c) std::cerr << "DID NOT FIND CLUSTER\n";
    return true;
  }
  return false;
}

CFINode CFICFG::getNode(unsigned int id){
  CFINode *n = node_id_map[id];
  CFIUtils u;
  if(n) return *n;
  else return u.returnFNode();
}

CFINode CFICFG::getAsmNodeOffset(std::string callee){
  std::stringstream ss(callee);
  std::string tr_callee;
  std::getline(ss, tr_callee, '+');
  CFINode *n = node_asm_namemap[tr_callee];
  CFIUtils u;
  if(n) return *n;
  else return u.returnFNode();
}

CFINode CFICFG::getAsmNode(std::string addr){
  CFINode *n = node_asm_map[addr];
  CFIUtils u;
  if(n) return *n;
  else return u.returnFNode();
}

int CFICFG::createAsmNodes(char * funcs[], int nf){
  std::stringstream pstream;
  std::string line;
  std::string address;

  char * file;
  file = funcs[0];
  int nid;
  int counter = 0;

  std::string f, id;
  std::string pattern = "[1234567890abcdef\\s]*<.*>:$";
  std::regex function(pattern);

  pattern = "^[1234567890abcdef]+:.*movl[^%]+$";
  std::regex hc_id(pattern);
  pattern = "^[1234567890abcdef]+:.*nopl\\s*0x[01234567890abcdef]*$";
  std::regex nop_id(pattern);

  std::smatch match;
  for(int i = 0; i < nf; i++){
    f = funcs[i];
    if(!std::regex_search(f, match, function)){
      continue;
    }

    file = funcs[i] + f.length() + 1;
    id = file;
    if(!std::regex_search(id, match, hc_id) && !std::regex_search(id, match,
                                                                  nop_id)){
      std::string asm_name = f.substr(18, f.length() - 17);
      asm_name = asm_name.substr(0, asm_name.length() - 2);
      address = f.substr(0,16);
      std::cerr << "Creating ASM Node: " << asm_name << " " << address << "\n";
      createAsmNode(asm_name, address);
      counter++;
    }
  }
  return counter;
}

std::string CFICFG::getIdByAddress(char *funcs[], int nfuncs,
                                   std::string address){
  char * p;
  unsigned int u1, u2;
  std::string pattern, p_str;
  std::stringstream pstream;
  std::regex hex_number("^[1234567890abcdef]+$");
  std::smatch match;
  pstream << "^" << address << ":.*movl[^%]+$";
  pattern = pstream.str();
  std::regex idp(pattern);
  for(int i = 0; i < nfuncs; i++){
    p = funcs[i] + strlen(funcs[i]) + 1;
    p_str = p;
    if(std::regex_search(p_str, match, idp)){
      std::string id = p_str.substr(28,p_str.length()-8);
      std::stringstream a(id); // remove , if id is shorter than 8b
      std::getline(a, id, ',');
      return id;
    } else {
      std::string addr = p_str.substr(0,16);
      if(regex_search(addr, match, hex_number)){
        u1 = stoul(p_str.substr(0,16), nullptr, 16);
        u2 = stoul(address, nullptr, 16);
        if(u1 > u2){
          break;
        }
      }
    }
  }
  return "0";
}

int CFICFG::createJmpEdges(char * funcs[], int nfuncs){
  CFINode n1, n2;
  unsigned int calls_to_asm = 0;
  bool skip = false;

  std::string fname;
  std::string line;
  std::string instr;
  std::string pattern;
  std::string function;
  std::string address;
  std::string callee_name;
  std::string callee_addr;
  std::string id;
  char * id_p, * p, * instr_p;

  pattern = "[1234567890abcdef\\s]*<.*>:$";
  std::regex fn_pattern(pattern);

  pattern = "^[1234567890abcdef]+:.*movl[^%]+$";
  std::regex hc_id(pattern);

  pattern = ".*jmp.*<[^+]+>$";
  std::regex call(pattern);

  std::smatch match;

  for(int i = 0; i < nfuncs; i++){
    p = funcs[i];
    function = p;
    if(!std::regex_search(function, match, fn_pattern)) continue;
    id_p = funcs[i] + strlen(funcs[i]) + 1;
    std::string id_str = id_p;
    address = function.substr(0,16);
    if(std::regex_search(id_str, match, hc_id)){
      id = getIdFast(funcs, nfuncs, address);
    } else {
      id = "0";
    }
    if(strcmp(id.c_str(),"0")==0){
      n1 = getAsmNode(address);
      if(n1.id){
        fprintf(stderr, "ignoring edges that start from ASM f: %s\n", n1.name);
        continue;
      }
    } else {
      n1 = getNode(stoul(id,nullptr,16));
    }
    if(n1.id==0){
      fprintf(stderr, "ERROR: %s\n", function.c_str());
      fprintf(stderr, "ERROR ID: %s\n", id.c_str());
      exit(-1);
    }
    // now check following instrs and create 1 edge for every call
    std::string aux;
    instr_p = id_p + id_str.length() + 1;
    instr = instr_p;
    while(1){
      if(strcmp(instr.c_str(),"")==0){
        break;
      }
      aux = instr;
      instr_p = instr_p + instr.length() + 1;
      instr = instr_p;
      if(!std::regex_search(aux, match, call)) continue;

      callee_name = aux.substr(43, aux.length()-43);
      callee_name = callee_name.substr(0, callee_name.length()-1);

      callee_addr = aux.substr(25, aux.length()-25);
      callee_addr = callee_addr.substr(0, 16);

      fprintf(stderr, "callee name:%s addr:%s\n", callee_name.c_str(),
              callee_addr.c_str());

      std::string id2 = getIdFast(funcs, nfuncs, callee_addr);
      if(strcmp(id2.c_str(),"0")==0){
        calls_to_asm++;
        n2 = getAsmNode(callee_addr);
        if(n2.id==0){
          n2 = getAsmNodeOffset(callee_name);
        }
        std::cerr << "[A JMP] 0x" << std::hex << n1.id << " -> 0x";
        std::cerr << std::hex << n2.id << "\n";
        std::cerr << "[A JMP] " << n1.name << " -> " << n2.name << "\n\n";
      } else {
        n2 = getNode(stoul(id2, nullptr, 16));
        std::cerr << "[C JMP] 0x" << std::hex << n1.id << " -> 0x";
        std::cerr << std::hex << n2.id << "\n";
        std::cerr << "[C JMP] " << n1.name << " -> " << n2.name << "\n\n";
      }
      if(n2.id==0){
        std::cerr << "WARNING, COULD NOT FIND N2 ID --skipping\n";
        std::cerr << "callee_addr: " << callee_addr << "\n";
        std::cerr << "callee_name: " << callee_name << "\n";
        std::cerr << "instr: " << instr << "\n";
        skip = true;
      }
      if(!skip) createJmpEdge(n1.id, n2.id);
      else skip = false;
    }
  }
  std::cerr << "Finished creating tail jmp edges\n";
  return calls_to_asm;
}

int CFICFG::mapLeavesThread(char * funcs[], int begin, int end, int nr){
  CFINode *n1;

  std::string instr;
  std::string pattern;
  std::string function;
  std::string address;
  unsigned int id;
  char * id_p, * p, * instr_p;

  pattern = "[1234567890abcdef\\s]*<.*>:$";
  std::regex fn_pattern(pattern);

  pattern = ".*call.*(<.*>$|\\*)";
  std::regex call(pattern);

  std::smatch match;

  fprintf(stderr, "THREAD %d: mapLeaves - begin:%d end:%d\n", nr, begin, end);
  for(int i = begin; i < end; i++){
    p = funcs[i];
    function = p;
    if(!std::regex_search(function, match, fn_pattern)) continue;
    id_p = funcs[i] + strlen(funcs[i]) + 1;
    std::string id_str = id_p;
    address = function.substr(0,16);
    if(function_id_map.find(address)!=function_id_map.end()){
      id = function_id_map[address];
      n1 = node_id_map[id];
    } else {
      CFINode _n1 = getAsmNode(address);
      n1 = node_id_map[_n1.id];
    }
    if(n1->id==0){
      fprintf(stderr, "%s\n", function.c_str());
      fprintf(stderr, "ERROR: %s\n", function.c_str());
      fprintf(stderr, "ERROR ID: %x\n", id);
      dumpCFG();
      exit(-1);
    }

    std::string aux;
    instr_p = id_p + id_str.length() + 1;
    instr = instr_p;
    while(1){
      if(strcmp(instr.c_str(),"")==0){
        break;
      }
      aux = instr;
      instr_p = instr_p + instr.length() + 1;
      instr = instr_p;
      if(std::regex_search(aux, match, call)) {
        n1->leaf_distance = 1;
      }
    }
  }
  fprintf(stderr, "THREAD %d: Finished mapping leaf functions\n", nr);
  return 1;
}

int CFICFG::createDirectEdgesThread(char * funcs[], int begin, int end, int nr){
  CFINode n1, n2;
  unsigned int calls_to_asm = 0;
  bool skip = false;

  std::string instr;
  std::string pattern;
  std::string function;
  std::string address;
  std::string callee_name;
  std::string callee_addr;
  unsigned int id, id2;
  char * id_p, * p, * instr_p;

  pattern = "[1234567890abcdef\\s]*<.*>:$";
  std::regex fn_pattern(pattern);

  pattern = ".*call.*<.*>$";
  std::regex call(pattern);

  std::smatch match;

  fprintf(stderr, "THREAD %d: Starting - begin:%d end:%d\n", nr, begin, end);
  for(int i = begin; i < end; i++){
    p = funcs[i];
    function = p;
    if(!std::regex_search(function, match, fn_pattern)) continue;
    id_p = funcs[i] + strlen(funcs[i]) + 1;
    std::string id_str = id_p;
    address = function.substr(0,16);
    if(function_id_map.find(address)!=function_id_map.end()){
      id = function_id_map[address];
      n1 = getNode(id);
    } else {
      n1 = getAsmNode(address);
      if(n1.id){
        fprintf(stderr, "ignoring edges that start from ASM f: %s\n", n1.name);
        continue;
      }
    }
    if(n1.id==0){
      fprintf(stderr, "%s\n", function.c_str());
      fprintf(stderr, "ERROR: %s\n", function.c_str());
      fprintf(stderr, "ERROR ID: %x\n", id);
      dumpCFG();
      exit(-1);
    }
    // now check following instrs and create 1 edge for every call
    std::string aux;
    instr_p = id_p + id_str.length() + 1;
    instr = instr_p;
    while(1){
      if(strcmp(instr.c_str(),"")==0){
        break;
      }
      aux = instr;
      instr_p = instr_p + instr.length() + 1;
      instr = instr_p;
      if(!std::regex_search(aux, match, call)) continue;

      callee_name = aux.substr(43, aux.length()-43);
      callee_name = callee_name.substr(0, callee_name.length()-1);

      callee_addr = aux.substr(25, aux.length()-25);
      callee_addr = callee_addr.substr(0, 16);

      if(function_id_map.find(callee_addr)!=function_id_map.end()){
        id2 = function_id_map[callee_addr];
        n2 = getNode(id2);
        fprintf(stderr, "THREAD %d:  [C]  0x%x -> 0x%x : %s -> %s\n\n", nr,
                n1.id, n2.id, n1.name, n2.name);
      } else {
        calls_to_asm++;
        n2 = getAsmNode(callee_addr);
        if(n2.id==0){
          n2 = getAsmNodeOffset(callee_name);
        }
        if(n2.id!=0){
          fprintf(stderr, "THREAD %d: [ASM] 0x%x -> 0x%x : %s -> %s\n\n", nr,
                  n1.id, n2.id, n1.name, n2.name);
        } else {
          if(isInDataSection(callee_name, "data.thin")){
            n2 = createDataNode(callee_name, callee_addr);
            fprintf(stderr, "THREAD %d: [DATA] 0x%x -> 0x%x : %s -> %s\n\n", nr,
                    n1.id, n2.id, n1.name, n2.name);
          }
        }
      }
      if(n2.id==0){
        std::cerr << "WARNING, COULD NOT FIND N2 ID -- skipping\n";
        std::cerr << "callee_addr: " << callee_addr << "\n";
        std::cerr << "callee_name: " << callee_name << "\n";
        std::cerr << "instr: " << instr << "\n";
        skip = true;
      }
      if(!skip) createDirectEdge(n1.id, n2.id);
      else skip = false;
    }
  }
  fprintf(stderr, "THREAD %d: Finished creating direct edges\n", nr);
  return calls_to_asm;
}

int CFICFG::createDirectEdges(char * funcs[], int nfuncs){
  CFINode n1, n2;
  unsigned int calls_to_asm = 0;
  bool skip = false;

  std::string instr;
  std::string pattern;
  std::string function;
  std::string address;
  std::string callee_name;
  std::string callee_addr;
  std::string id;
  char *id_p, *p, *instr_p;

  pattern = "[1234567890abcdef\\s]*<.*>:$";
  std::regex fn_pattern(pattern);

  pattern = "^[1234567890abcdef]+:.*movl[^%]+$";
  std::regex hc_id(pattern);

  pattern = ".*call.*<.*>$";
  std::regex call(pattern);

  std::smatch match;

  for(int i = 0; i < nfuncs; i++){
    p = funcs[i];
    function = p;
    if(!std::regex_search(function, match, fn_pattern)) continue;
    id_p = funcs[i] + strlen(funcs[i]) + 1;
    std::string id_str = id_p;
    address = function.substr(0,16);
    if(std::regex_search(id_str, match, hc_id)){
      id = getIdFast(funcs, nfuncs, address);
    } else {
      id = "0";
    }
    if(strcmp(id.c_str(),"0")==0){
      n1 = getAsmNode(address);
      if(n1.id){
        fprintf(stderr, "ignoring edges that start from ASM f: %s\n", n1.name);
        continue;
      }
    } else {
      n1 = getNode(stoul(id,nullptr,16));
    }
    if(n1.id==0){
      fprintf(stderr, "ERROR: %s\n", function.c_str());
      fprintf(stderr, "ERROR ID: %s\n", id.c_str());
      exit(-1);
    }
    // now check following instrs and create 1 edge for every call
    std::string aux;
    instr_p = id_p + id_str.length() + 1;
    instr = instr_p;
    while(1){
      if(strcmp(instr.c_str(),"")==0){
        break;
      }
      aux = instr;
      instr_p = instr_p + instr.length() + 1;
      instr = instr_p;
      if(!std::regex_search(aux, match, call)) continue;

      callee_name = aux.substr(43, aux.length()-43);
      callee_name = callee_name.substr(0, callee_name.length()-1);

      callee_addr = aux.substr(25, aux.length()-25);
      callee_addr = callee_addr.substr(0, 16);

      std::cerr << "callee name:" << callee_name << " addr:" << callee_addr << "\n";

      std::string id2 = getIdFast(funcs, nfuncs, callee_addr);
      if(strcmp(id2.c_str(),"0")==0){
        calls_to_asm++;
        n2 = getAsmNode(callee_addr);
        if(n2.id==0){
          n2 = getAsmNodeOffset(callee_name);
        }
        std::cerr << "[A xCALL] 0x" << std::hex << n1.id << " -> 0x" << std::hex << n2.id << "\n";
        std::cerr << "[A xCALL] " << n1.name << " -> " << n2.name << "\n\n";
      } else {
        n2 = getNode(stoul(id2, nullptr, 16));
        std::cerr << "[C xCALL] 0x" << std::hex << n1.id << " -> 0x" << std::hex << n2.id << "\n";
        std::cerr << "[C xCALL] " << n1.name << " -> " << n2.name << "\n\n";
      }
      if(n2.id==0){
        std::cerr << "WARNING, COULD NOT FIND N2 ID --skipping\n";
        std::cerr << "callee_addr: " << callee_addr << "\n";
        std::cerr << "callee_name: " << callee_name << "\n";
        std::cerr << "instr: " << instr << "\n";
        skip = true;
      }
      if(!skip) createDirectEdge(n1.id, n2.id);
      else skip = false;
    }
  }
  std::cerr << "Finished creating direct edges\n";
  return calls_to_asm;
}

bool CFICFG::getChilds(std::list<CFINode>& tgts, unsigned int from){
  std::list<CFIEdge>::iterator i;
  std::list<CFIEdge>::iterator ie;
  CFINode n;

  bool flag = false;
  bool has_dcalls = false;

  i = edge_childs_map[from].begin();
  ie = edge_childs_map[from].end();

  for(; i != ie; i++){
    flag = true;
    if(i->type == dcall){
      has_dcalls = true;
      n = getNode(i->target);
      tgts.push_back(getNode(i->target));
    }
  }

  if(flag == true && has_dcalls == false){
    n = getNode(from);
    n.dump();
    std::cerr << "DOES NOT HAVE DCALLS, BUT IS NO LEAF\n";
  }
  return flag;
}

void CFICFG::getParents(std::list<CFINode>& parents, unsigned int tgt_id){
  std::list<CFIEdge>::iterator i;
  std::list<CFIEdge>::iterator ie;

  i = edge_parents_map[tgt_id].begin();
  ie = edge_parents_map[tgt_id].end();

  for(; i != ie; i++) parents.push_back(getNode(i->origin));
}

// REQUIRES FURTHER TESTING
bool CFICFG::duplicatedInBinary(std::string dump, std::string callee){
  unsigned int id, v1, v2;
  bool first = false;
  std::stringstream pstream;
  std::ifstream file(dump);
  std::string line;
  pstream << "<" << callee << ">:";
  std::string pattern = pstream.str();
  std::regex rgx(pattern);
  std::smatch match;
  while(std::getline(file, line)){
    if(std::regex_search(line, match, rgx)){
      std::cout << line;
      if(!first) first = true;
      else return true;
    }
  }
  return false;
}

// REQUIRES FURTHER TESTING
CFINode CFICFG::getNodeBinary(std::string dump, std::string callee){
  unsigned int id, v1, v2;
  std::stringstream pstream;
  std::ifstream file(dump);
  std::string line;
  pstream << "[1234567890abcdef\\s]*<" << callee << ">:$";
  std::string pattern = pstream.str();
  std::regex rgx(pattern);
  std::string validator = "^[1234567890abcdef]+$";
  std::regex validate(validator);
  std::stringstream hex_to_int;
  std::smatch match;
  while(std::getline(file, line)){
    if(std::regex_search(line, match, rgx)){
      std::string addr = line.substr(0,16);
      if(std::regex_search(addr, match, validate)){
        std::getline(file, line);
        std::string sid = line.substr(28,8);
        hex_to_int << std::hex << sid;
        hex_to_int >> id;
        file.close();
        return getNode(id);
      }
      else break;
    }
  }
  file.close();
  CFIUtils u;
  return u.returnFNode();
}

CFINode CFICFG::getNodeBinary(std::string dump, std::string caller,
                              std::string callee, bool tailjmp){
  CFIUtils u;
  std::string address;
  CFINode dst, org;

  address = getCalleeAddress(dump, caller, callee, tailjmp);
  if(strcmp(address.c_str(),"0")==0){
    fprintf(stderr, "CFIError: Unable to find duplicated address in dump\n");
    fprintf(stderr, "CFIError: %s calling %s\n", caller.c_str(),
            callee.c_str());
    return u.returnFNode();
  }

  unsigned int id = getIdByAddress(dump, address);
  if(id == 0){
    fprintf(stderr, "CFIError: Unable to find duplicated funct id in dump\n");
    fprintf(stderr, "CFIError: %s calling %s addr %s\n", caller.c_str(),
            callee.c_str(), address.c_str());
    return u.returnFNode();
  }
  dst = getNode(id);
  if(dst.id == 0){
    fprintf(stderr, "CFIError: invalid id for duplicated function:\n");
    fprintf(stderr, "CFIError: %s calling %s\n", caller.c_str(),
            callee.c_str());
    fprintf(stderr, "Address: %s Id: %x\n", address.c_str(), id);
    return u.returnFNode();
  }
  return dst;
}

CFIEdge CFICFG::getEdge(unsigned int id){
  CFIEdge *e = edge_id_map[id];
  CFIUtils u;
  if(e) return *e;
  else return u.returnFEdge();
}

void CFICFG::storeCFG(){
  CFINode n;
  CFICluster c;
  CFIEdge e;
  std::list<CFINode>::iterator ni;
  std::list<CFINode>::iterator nie;
  std::list<CFICluster>::iterator ci;
  std::list<CFICluster>::iterator cie;
  std::list<CFIEdge>::iterator ei;
  std::list<CFIEdge>::iterator eie;
  std::list<CFIDecl>::iterator di, die;


  std::ofstream f;
  f.open("nodes.cfi", std::ios::binary | std::ios::out);
  ni = CFINodes.begin();
  nie = CFINodes.end();
  for(; ni != nie ; ++ni){
    n = *ni;
    f.write((char *) &n, sizeof(CFINode));
  }
  f.close();

  f.open("edges.cfi", std::ios::binary | std::ios::out);
  ei = CFIEdges.begin();
  eie = CFIEdges.end();
  for(; ei != eie ; ++ei){
    e = *ei;
    f.write((char *) &e, sizeof(CFIEdge));
  }
  f.close();

  f.open("clusters.cfi", std::ios::binary | std::ios::out);
  ci = CFIClusters.begin();
  cie = CFIClusters.end();
  for(; ci != cie ; ++ci){
    c = *ci;
    f.write((char *) &c, sizeof(CFICluster));
  }
  f.close();
}

void CFICFG::storeMergedAliases(){
  CFIAlias a;

  std::ofstream f;
  std::map<std::string, std::string>::iterator ai, aie;

  f.open("aliases.cfi", std::ios::binary | std::ios::out);
  ai = aliases_map.begin();
  aie = aliases_map.end();
  for(; ai != aie ; ++ai){
    strcpy(a.alias, ai->first.c_str());
    strcpy(a.aliasee, ai->second.c_str());
    f.write((char *) &a, sizeof(CFIAlias));
  }
  f.close();
}

void CFICFG::storeMergedDecls(){
  std::ofstream f;
  std::list<CFIDecl>::iterator di, die;
  CFIDecl d;

  f.open("decls.cfi", std::ios::binary | std::ios::out);
  di = CFIDecls.begin();
  die = CFIDecls.end();
  for(; di != die ; ++di){
    d = *di;
    f.write((char *) &d, sizeof(CFIDecl));
  }
  f.close();
}

std::string CFICFG::getAliasee(std::string alias){
  if(aliases_map.count(alias) <= 0) return "";
  return aliases_map[alias];
}

void CFICFG::loadAliases(){
  std::ifstream f;
  CFIAlias a;
  f.open("aliases.cfi", std::ios::binary | std::ios::in);
  f.read((char *) &a, sizeof(CFIAlias));
  while(!f.fail()){
    aliases_map[a.alias] = a.aliasee;
    f.read((char *) &a, sizeof(CFIAlias));
  }
}

void CFICFG::loadCFG(){
  CFIUtils u;
  CFINode n;

  std::ifstream f;
  f.open("nodes.cfi", std::ios::binary | std::ios::in);
  f.read((char *) &n, sizeof(CFINode));
  while(!f.fail()){
    addToGraph(n);
    f.read((char *) &n, sizeof(CFINode));
  }
  f.close();

  CFIEdge e;
  f.open("edges.cfi", std::ios::binary | std::ios::in);
  f.read((char *) &e, sizeof(CFIEdge));
  while(!f.fail()){
    addToGraph(e);
    f.read((char *) &e, sizeof(CFIEdge));
  }
  f.close();

  CFICluster c;
  f.open("clusters.cfi", std::ios::binary | std::ios::in);
  f.read((char *) &c, sizeof(CFICluster));
  while(!f.fail()){
    addToGraph(c);
    f.read((char *) &c, sizeof(CFICluster));
  }
  f.close();
}

void CFICFG::loadDecls(){
  std::ifstream f;

  CFIDecl d;
  f.open("decls.cfi", std::ios::binary | std::ios::in);
  f.read((char *) &d, sizeof(CFIDecl));
  while(!f.fail()){
    addToGraph(d);
    f.read((char *) &d, sizeof(CFIDecl));
  }
  f.close();
}

void CFICFG::dumpCFG(){
  std::list<CFINode>::iterator ni, nie;
  std::list<CFICluster>::iterator ci, cie;
  std::list<CFIEdge>::iterator ei, eie;
  std::map<std::string, std::string>::iterator ai, aie;

  fprintf(stderr, "CFI Nodes:\n");
  ni = CFINodes.begin();
  nie = CFINodes.end();
  for(; ni != nie ; ++ni) ni->dump();

  fprintf(stderr, "CFI Edges:\n");
  ei = CFIEdges.begin();
  eie = CFIEdges.end();
  for(; ei != eie ; ++ei) ei->dump();

  fprintf(stderr, "CFI Clusters:\n");
  ci = CFIClusters.begin();
  cie = CFIClusters.end();
  for(; ci != cie ; ++ci) ci->dump();

  fprintf(stderr, "CFI Aliases:\n");
  ai = aliases_map.begin();
  aie = aliases_map.end();
  for(; ai != aie ; ++ai)
    fprintf(stderr, "Alias: %s Aliasee: %s\n", ai->first.c_str(),
            ai->second.c_str());
}

void CFICFG::dumpDecls(){
  std::list<CFIDecl>::iterator di, die;

  fprintf(stderr, "CFI Decls:\n");
  di = CFIDecls.begin();
  die = CFIDecls.end();
  for(; di != die ; ++di) di->dump();
}

CFINode CFICFG::getNodeName(std::string name){
  CFIUtils u;

  if(node_asm_namemap.count(name) > 0){
    return *node_asm_namemap[name];
  }

  std::list<CFINode>::iterator ni = CFINodes.begin();
  std::list<CFINode>::iterator ne = CFINodes.end();

  for(; ni != ne; ni++){
    CFINode n = *ni;
    if(strcmp(n.name, name.c_str())==0){
      if(checkNameDuplicates(n)){
        fprintf(stderr, "%s duplicated: id taken from vmlinux.thin \
                - possibly unsafe\n", n.name);
        n = getNodeBinary("vmlinux.thin", name);
        if(n.id != 0) return n;
      }
      return n;
    }
  }
  return u.returnFNode();
}

void CFICFG::addToGraph(CFIDecl d){
  std::stringstream ss;
  ss << d.name << d.proto;
  std::string s = ss.str();
  if(decls_map.count(s) > 0) return;

  decls_map[s] = true;
  decls_n_map[d.name] = d.proto;
  CFIDecls.push_back(d);
}

void CFICFG::addToGraph(CFINode n){
  pthread_mutex_lock(&cfi_lock);
  CFINodes.push_back(n);
  std::list<CFINode>::iterator ni = CFINodes.end();
  ni--;

  std::string name(n.name);
  std::string proto(n.proto);
  std::string module(n.module);
  node_id_map[ni->id] = &(*ni);
  node_npm_map[name + proto + module] = &(*ni);
  node_np_map[name + proto] = &(*ni);
  if(proto.compare("ASM")==0){
    node_asm_map[module] = &(*ni);
    node_asm_namemap[name] = &(*ni);
  }
  if(proto.compare("DATA")==0){
    node_data_namemap[name] = &(*ni);
  }
  pthread_mutex_unlock(&cfi_lock);
}

void CFICFG::addToGraph(CFIEdge e){
  pthread_mutex_lock(&cfi_lock);
  CFIEdges.push_back(e);
  std::list<CFIEdge>::iterator ei = CFIEdges.end();
  ei--;

  long key = (long) ei->origin << 32 | ei->target;
  edge_id_map[ei->id] = &(*ei);
  edge_ot_map[key] = &(*ei);
  edge_childs_map[ei->origin].push_back(e);
  edge_parents_map[ei->target].push_back(e);
  pthread_mutex_unlock(&cfi_lock);
}

void CFICFG::addToGraph(CFICluster c){
  CFIClusters.push_back(c);
  std::list<CFICluster>::iterator ci = CFIClusters.end();
  ci--;

  std::string proto(ci->proto);
  cluster_id_map[ci->id] = &(*ci);
  cluster_p_map[proto] = &*(ci);
}

void CFICFG::mergeCFIFiles(std::string path){
  std::list<std::string> nodes, edges, clusters, aliases, tails, decls;
  std::list<std::string>::iterator i, ie;
  CFINode n;
  CFIEdge e;
  CFICluster c;
  CFIAlias a;
  CFIDecl d;
  std::ifstream f;

  getAllCFIFiles(path, nodes, edges, clusters, aliases, tails, decls);

  i = nodes.begin();
  ie = nodes.end();

  for(; i != ie; i++){
    std::string file = path + *i;
    f.open(file, std::ios::binary | std::ios::in);
    f.read((char *) &n, sizeof(CFINode));
    while(!f.fail()){
      n.dump();
      addToGraph(n);
      f.read((char *) &n, sizeof(CFINode));
    }
    f.close();
  }

  i = edges.begin();
  ie = edges.end();

  for(; i != ie; i++){
    std::string file = path + *i;
    f.open(file, std::ios::binary | std::ios::in);
    f.read((char *) &e, sizeof(CFIEdge));
    while(!f.fail()){
      addToGraph(e);
      f.read((char *) &e, sizeof(CFIEdge));
    }
    f.close();
  }

  i = clusters.begin();
  ie = clusters.end();

  for(; i != ie; i++){
    std::string file = path + *i;
    f.open(file, std::ios::binary | std::ios::in);
    f.read((char *) &c, sizeof(CFICluster));
    while(!f.fail()){
      if(!getCluster(c.proto).id){
        addToGraph(c);
      } else {
        fixMergedEdges(c);
      }
      f.read((char *) &c, sizeof(CFICluster));
    }
    f.close();
  }

  i = aliases.begin();
  ie = aliases.end();

  for(; i != ie; i++){
    std::string file = path + *i;
    f.open(file, std::ios::binary | std::ios::in);
    f.read((char *) &a, sizeof(CFIAlias));
    while(!f.fail()){
      setAlias(a.alias, a.aliasee);
      f.read((char *) &a, sizeof(CFIAlias));
    }
    f.close();
  }

  i = decls.begin();
  ie = decls.end();

  for(; i != ie; i++){
    std::string file = path + *i;
    f.open(file, std::ios::binary | std::ios::in);
    f.read((char *) &d, sizeof(CFIDecl));
    while(!f.fail()){
      addToGraph(d);
      f.read((char *) &d, sizeof(CFIDecl));
    }
    f.close();
  }
}

void CFICFG::fixMergedEdges(CFICluster c){
  CFICluster k = getCluster(c.proto);
  fprintf(stderr, "FIXING EDGES: %x to %x\n", c.id, k.id);
  std::list<CFIEdge>::iterator i, ie;

  i = CFIEdges.begin();
  ie = CFIEdges.end();

  for(; i != ie; i++)
    if(i->target == c.id) i->target = k.id;
}

void CFICFG::setAlias(std::string alias, std::string aliasee){
  aliases_map[alias] = aliasee;
}

void CFICFG::getAllCFIFiles(std::string path, std::list<std::string>& nodes,
                    std::list<std::string>& edges,
                    std::list<std::string>& clusters,
                    std::list<std::string>& aliases,
                    std::list<std::string>& tails,
                    std::list<std::string>& decls
                    ){
  dirent* de;
  DIR* dp;
  errno = 0;
  dp = opendir(path.empty() ? "." : path.c_str());
  if(dp){
    while(true){
      errno = 0;
      de = readdir(dp);
      if(de == NULL) break;
      std::string name(de->d_name);
      if(name.compare(0,5,"nodes")==0){ nodes.push_back(name); continue; }
      if(name.compare(0,5,"edges")==0){ edges.push_back(name); continue; }
      if(name.compare(0,8,"clusters")==0){ clusters.push_back(name); continue; }
      if(name.compare(0,7,"aliases")==0){ aliases.push_back(name); continue;}
      if(name.compare(0,5,"tails")==0){ tails.push_back(name); continue;}
      if(name.compare(0,5,"decls")==0){ decls.push_back(name); continue;}

    }
    closedir(dp);
  }
}

unsigned int CFICFG::getIdInMap(char * file, char * lines[],
                                unsigned int nlines, std::string address,
                                unsigned int skip){
  unsigned int id, v1, v2;
  std::stringstream pstream;
  std::string line;
  pstream << "^" << address << ":.*movl[^%]+$";
  std::string pattern = pstream.str();
  std::regex rgx(pattern);
  std::string validator = "[1234567890abcdef]+";
  std::regex validate(validator);
  std::stringstream hex_to_int;
  std::smatch match;
  for(int i = skip-1; i < nlines; i++){
    line = lines[i];
    if(line.length() > 10){
      std::string speedup = line.substr(0,16);
      if(std::regex_search(speedup, match, validate) &&
         std::regex_search(address, match, validate)){
        v1 = std::stoul(address, nullptr, 16);
        v2 = std::stoul(speedup, nullptr, 16);
        if(v2 > v1) break;
      }
    }
    if(std::regex_search(line, match, rgx)){
      std::string sid = line.substr(50,8);
      if(std::regex_search(sid, match, validate)){
        hex_to_int << std::hex << sid;
        hex_to_int >> id;
        return id;
      }
      else break;
    }
  }
  return 0;
}

bool CFICFG::isInDataSection(std::string name, std::string dump){
  std::cerr << "isInDataSection: " << name << " " << dump << "\n";
  std::stringstream pstream;
  std::ifstream file(dump);
  std::string line;

  pstream << "^[1234567890abcdef\\s]+.*<" << name << ">:$";
  std::string pattern = pstream.str();
  std::regex rgx(pattern);

  std::smatch match;
  while(std::getline(file, line)){
    if(std::regex_search(line, match, rgx)) return true;
  }
  file.close();
  return false;
}

unsigned int CFICFG::getIdFastest(std::string address){
  char *p = function_map[address];
  std::string line = p;
  std::string id;

  std::stringstream pstream;
  pstream << "^" << address << ":.*movl[^%]+$";
  std::string pattern = pstream.str();
  std::regex rgx(pattern);
  std::string validator = "^[1234567890abcdef]+$";
  std::regex validate(validator);
  std::smatch match;

  if(std::regex_search(line, match, rgx)){
     std::string sid = line.substr(28,8);
     std::stringstream a(sid); // remove , if id is shorter than 8b
     std::getline(a, sid, ',');
     if(std::regex_search(sid, match, validate)){
       unsigned long nid = stoul(sid, nullptr, 16);
       return nid;
     }
  }

  std::stringstream nop;
  nop << "^" << address << ":.*nopl[^%]+$";
  pattern = nop.str();
  std::regex nop_rgx(pattern);

  if(std::regex_search(line, match, nop_rgx)){
    std::string sid = line.substr(27,8);
    if(std::regex_search(sid, match, validate)){
      unsigned long nid = stoul(sid, nullptr, 16);
      return nid;
    }
  }
  return 0;
}

std::string CFICFG::getIdFast(char * funcs[], int nf, std::string address){
  int first = 0;
  int last = nf - 1;
  int pivot;
  char * id, * p;

  std::string p_str;
  std::smatch match;

  std::stringstream pstream;
  pstream << "^" << address << ".*<.*>:$";
  std::regex rgx(pstream.str());

  std::string isid = "^[1234567890abcdef]+:.*movl.*ffffffff.*$";
  std::regex idvalid(isid);

  std::string validator = "^[1234567890abcdef]+$";
  std::regex validate(validator);

  std::string itsacall = "^[1234567890abcdef]+:.*callq.*<.*>.*$";
  std::regex callvalid(itsacall);

  while(first <= last){
    pivot = (first + last) / 2;
    std::string t = funcs[pivot];
    if(std::regex_search(t, match, rgx)){
      p = funcs[pivot] + strlen(funcs[pivot]) + 1;
      p_str = p;
      if(p_str.length()<28) return "0";
      if(std::regex_search(p_str, match, callvalid)) return "0";
      if(!std::regex_search(p_str, match, idvalid)) return "0";
      std::string sid = p_str.substr(28,8);
      std::stringstream a(sid); // remove , if id is shorter than 8b
      std::getline(a, sid, ',');
      if(std::regex_search(sid, match, validate)){
        return sid;
      } else {
        return "0";
      }
    } else {
      std::string m = funcs[pivot];
      unsigned long pivot_v = stoul(m.substr(0,16), nullptr, 16);
      if(stoul(address, nullptr, 16) < pivot_v){
        last = pivot - 1;
      } else {
        first = pivot + 1;
      }
    }
  }
  return "0";
}

unsigned int CFICFG::getIdByAddress(std::string dump, std::string address,
                                    unsigned int skip){

  unsigned int id, v1, v2;
  std::stringstream pstream;
  std::ifstream file(dump);
  std::string line;
  pstream << "^" << address << ":.*movl[^%]+$";
  std::string pattern = pstream.str();
  std::regex rgx(pattern);
  std::string validator = "^[1234567890abcdef]+$";
  std::regex validate(validator);
  std::stringstream hex_to_int;
  std::smatch match;
  for(int i = 0; i < skip; i++){
    std::getline(file, line);
  }
  while(std::getline(file, line)){
    if(std::regex_search(line, match, rgx)){
      std::string sid = line.substr(50,8);
      if(std::regex_search(sid, match, validate)){
        file.close();
        hex_to_int << std::hex << sid;
        hex_to_int >> id;
        return id;
      }
      else break;
    }
  }
  file.close();
  return 0;
}

std::string CFICFG::getCalleeAddress(std::string dump, std::string caller,
                                     std::string callee, bool tailjmp){
  std::stringstream pstream;
  std::ifstream file(dump);
  std::string line;

  pstream << "[1234567890abcdef\\s]*<" << caller << ">:$";
  std::string pattern = pstream.str();
  std::regex rgx(pattern);

  pattern = "[1234567890abcdef\\s]*<.*>:$";
  std::regex nf(pattern);

  pstream.str(std::string());
  if(!tailjmp) pstream << ".*call.*<" << callee << ">$";
  else pstream << ".*jmp.*<" << callee << ">$";
  pattern = pstream.str();
  std::regex callx(pattern);

  std::smatch match;
  while(std::getline(file, line)){
    if(std::regex_search(line, match, rgx)){
      while(std::getline(file, line)){
        if(std::regex_search(line, match, callx)){
          for(auto x:match) std::cerr << x;
          file.close();
          std::string addr = line.substr(line.length()-(callee.length()+19),16);
          return addr;
        }
        if(std::regex_search(line, match, nf)){
          file.close();
          return "0";
        }
      }
    }
  }
  file.close();
  return "0";
}

bool CFICFG::checkNameDuplicates(CFINode n){
  std::list<CFINode>::iterator i;
  std::list<CFINode>::iterator ie;

  i = CFINodes.begin();
  ie = CFINodes.end();
  for(; i != ie; ++i){
    if((i->id != n.id) && (strcmp(n.name, i->name)==0)){
      return true;
    }
  }
  return false;
}

bool CFICFG::checkDuplicates(CFINode n){
  std::list<CFINode>::iterator i;
  std::list<CFINode>::iterator ie;

  i = CFINodes.begin();
  ie = CFINodes.end();
  for(; i != ie; ++i){
    if((i->id != n.id)
       && (strcmp(n.name, i->name)==0)
       && (strcmp(n.proto, i->proto)==0)){
      return true;
    }
  }
  return false;
}

CFINode CFIUtils::returnFNode(){
  CFINode n;
  strncpy(n.name, "", 1);
  strncpy(n.proto, "", 1);
  strncpy(n.module, "", 1);
  n.id = 0;
  n.parent_id = 0;
  n.head_id = 0;
  n.tail_id = 0;
  n.leaf_distance = 0;
  return n;
}

CFICluster CFIUtils::returnFCluster(){
  CFICluster c;
  c.id = 0;
  c.head_id = 0;
  c.tail_id = 0;
  c.count = 0;
  strncpy(c.proto, "", 1);
  return c;
}

CFIEdge CFIUtils::returnFEdge(){
  CFIEdge e;
  e.id = 0;
  e.origin = 0;
  e.target = 0;
  e.order = 0;
  e.type = dcall;
  return e;
}

class CFGTool{
public:
  std::string dumpname;
  CFGTool(std::string dump){
    dumpname = dump;
  }

  void dumpDirectCalls(){
  }
};
