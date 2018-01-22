//===---- CFGforCFI.cpp - kCFI Control-Flow Graph -------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Control-Flow Graph representation and respective operations used in kCFI
//
//===----------------------------------------------------------------------===//

#include <stdlib.h>
#include <iostream>
#include <sstream>
#include <fstream>
#include <list>
#include <regex>
#include <unistd.h>
#include <sys/file.h>
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
#include "llvm/IR/Function.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Compiler.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/Regex.h"
#include "llvm/Target/TargetFrameLowering.h"
#include "llvm/Target/TargetInstrInfo.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetRegisterInfo.h"
#include "llvm/Target/TargetSubtargetInfo.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include <climits>


using namespace llvm;

extern cl::opt<bool> CFIv;
extern cl::opt<bool> CFIBuildCFG;

std::stringstream cfi_er;
std::stringstream cfi_msg;
std::stringstream cfi_warn;

unsigned int last_id = 1;
unsigned int last_tag = 1;

CFIDecl CFICFG::createDecl(const Function *F){
  CFIUtils u;
  CFIDecl nd(F->getName().str().c_str(),
             u.typeToStr(F->getFunctionType()).c_str());
  addToGraph(nd);
  return nd;
}

CFINode CFICFG::createNode(const Function *F){
  CFIUtils u;
  unsigned int g_id;

  CFINode h = getHCNode(F->getName().str().c_str(),
                        u.typeToStr(F->getFunctionType()).c_str());
  if(!h.head_id){
    h.head_id = getNextTag();
    h.tail_id = getNextTag();
  }

  g_id = getNextId();

  std::string name = F->getName().str();
  std::string type = u.typeToStr(F->getFunctionType());
  std::string module = F->getParent()->getName().str();

  CFINode n(name.c_str(), type.c_str(), module.c_str(), g_id, h.head_id,
            h.tail_id, 0);

  n.has_duplicate = markDuplicates(n);
  addToGraph(n);
  return n;
}

CFIEdge CFICFG::createEdge(const Function *From, const Function *To){
  unsigned int f1_id, f2_id;

  unsigned int g_id = getNextId();

  f1_id = getNode(From).id;
  f2_id = getNode(To).id;

  unsigned int o = getOrder(CFIEdge::ctype::dcall, f1_id, f2_id);
  CFIEdge e(CFIEdge::ctype::dcall, g_id, f1_id, f2_id, o);

  addToGraph(e);
  return e;
}

CFIEdge CFICFG::createEdgeBinary(const Function *From, const Function *To){
  CFIUtils u;
  unsigned int f1_id, f2_id;

  unsigned int g_id = getNextId();

  f1_id = From->getCFINode();
  f2_id = getNodeBinary(From, To).id;
  if(!f1_id || !f2_id) return u.returnFEdge();

  unsigned int o = getOrder(CFIEdge::ctype::dcall, f1_id, f2_id);
  CFIEdge e(CFIEdge::ctype::dcall, g_id, f1_id, f2_id, o);

  addToGraph(e);
  return e;
}

// Order individualizes edges: Supported but not used in any feature currently.
unsigned int CFICFG::getOrder(CFIEdge::ctype t, unsigned int org,
                              unsigned int tgt){
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

CFIEdge CFICFG::createEdge(const Function *From, Type *To){
  CFIUtils u;
  unsigned int g_id = getNextId();

  unsigned int f1_id = getNode(From).id;

  if(!f1_id) u.error("creating an edge from node that does not exist", true);
  unsigned int f2_id = getCluster(To).id;
  if(!f2_id){
    f2_id  = createCluster(To).id;
  }

  if(!f1_id || !f2_id) return u.returnFEdge();

  unsigned int o = getOrder(CFIEdge::ctype::icall, f1_id, f2_id);
  CFIEdge e(CFIEdge::ctype::icall, g_id, f1_id, f2_id, o);

  addToGraph(e);
  return e;
}

CFICluster CFICFG::createCluster(Type *t){
  CFIUtils u;
  unsigned int h_id, t_id, g_id;

  g_id = getNextId();
  h_id = getNextTag();
  t_id = getNextTag();

  CFICluster c(u.typeToStr(t).c_str(), g_id, h_id, t_id);
  addToGraph(c);
  return c;
}

void CFICFG::setAlias(std::string alias, std::string aliasee){
  aliases_map[alias] = aliasee;
}

std::string CFICFG::getAlias(std::string alias){
  if(aliases_map.count(alias)){
    return aliases_map[alias];
  }
  return "";
}

CFINode CFICFG::getNode(unsigned int id){
  CFINode *n = node_id_map[id];
  CFIUtils u;
  if(n) return *n;
  else return u.returnFNode();
}

CFINode CFICFG::getNodeIgnore(const Function *F, std::list<unsigned int> ids){
  CFIUtils u;
  CFINode n;
  std::list<CFINode>::iterator i;
  std::list<CFINode>::iterator ie;
  std::list<unsigned int>::iterator ii, iie;
  std::string name, p, m;

  name = F->getName().str();
  p = u.typeToStr(F->getFunctionType());
  m = F->getParent()->getName().str();

  i = CFINodes.begin();
  ie = CFINodes.end();

  ii = ids.begin();
  iie = ids.end();

  for(; i != ie; ++i){
    n = *i;
    if(strcmp(n.name, name.c_str())==0 && strcmp(n.proto, p.c_str())==0){
      bool ignored = false;
      for(; ii != iie; ++ii)
        if(n.id == *ii) ignored = true;
      if(!ignored) return n;
    }
  }
  return u.returnFNode();
}

CFINode CFICFG::getNode(const Function *F, bool checkModule){
  CFINode *n;
  CFIUtils u;
  std::list<CFINode>::iterator i;
  std::list<CFINode>::iterator ie;
  std::string name, p, m;

  name = F->getName().str();
  StringRef nref = name;
  if(nref.endswith("_kcfi")){
    std::size_t pos = nref.find("_kcfi");
    name = nref.substr(0, pos);
  }
  p = u.typeToStr(F->getFunctionType());
  m = F->getParent()->getName().str();

  if(checkModule){
    std::string npm = name + p + m;
    n = node_npm_map[npm];
  } else {
    std::string np = name + p;
    n = node_np_map[np];
  }
  if(n) return *n;
  return u.returnFNode();
}

CFIEdge CFICFG::getEdge(unsigned int id){
  CFIEdge *e = edge_id_map[id];
  CFIUtils u;
  if(e) return *e;
  else return u.returnFEdge();
}

CFIEdge CFICFG::getEdge(const Function *origin, const Function *target){
  CFIUtils u;
  CFIEdge e;
  std::list<CFIEdge>::iterator i;
  std::list<CFIEdge>::iterator ie;

  i = CFIEdges.begin();
  ie = CFIEdges.end();

  unsigned int org_id = getNode(origin).id;
  unsigned int tgt_id;
  if(target->isDeclaration()){
    tgt_id = getNodeBinary(origin, target).id;
    if(!tgt_id){
      tgt_id = getAsmNode(target).id;
    }
    if(!tgt_id){
      tgt_id = getDataNode(target).id;
    }
  } else {
    tgt_id = getNode(target).id;
  }

  if(tgt_id){
    long key = (long) org_id << 32 | tgt_id;
    CFIEdge *ep = edge_ot_map[key];
    if(ep) return *ep;
  }

  std::string alias = checkWeakAlias(target);
  if(alias.length() > 0){
    tgt_id = getNodeBinary(origin, alias).id;
    if(!tgt_id){
      tgt_id = getAsmNode(alias).id;
    }
  }

  if(!tgt_id){
    std::stringstream p;
    u.warn(p.str());
    u.returnFEdge();
  }

  long key = (long) org_id << 32 | tgt_id;
  CFIEdge *ep = edge_ot_map[key];
  if(ep) return *ep;

  CFINode n1, n2;
  n1 = getNode(tgt_id);

  if(target->hasWeakLinkage()){
    i = edge_childs_map[org_id].begin();
    ie = edge_childs_map[org_id].end();
    for(; i != ie; ++i){
      e = *i;
      n2 = getNode(e.target);
      if((strcmp(n1.name, n2.name)==0)&&(strcmp(n1.proto, n2.proto)==0)){
        if(CFIv){
          u.warn("getEdge: weak linkage function replacement detected.\n");
          e.dump();
        }
        return e;
      }
    }
  }

  std::stringstream p;
  p << "getEdge: could not find edge: " << origin->getName().str() << " to ";
  p << target->getName().str() << "\n";
  u.warn(p.str());
  return u.returnFEdge();
}

bool CFICFG::differentEdgesToStrongerNode(Function *F){
  if(!F->hasWeakLinkage()) return false;
  CFICluster c = getCluster(F->getFunctionType());
  if(!c.id){
    return false;
  }

  CFIUtils u;

  CFINode n1, n2;
  n1 = getNode(F);

  std::string name = n1.name;
  std::string proto = n1.proto;

  std::list<CFINode*>::iterator i = node_list_np_map[name + proto].begin();
  std::list<CFINode*>::iterator ie = node_list_np_map[name + proto].end();

  for(; i != ie; i++){
    u.warn("getEdge: possible weak linkage function detected.");
    n2 = **i;
    std::list<CFINode> parents;
    getParents(parents, n2.id);
    if(parents.size() > 0) return true;
  }
  return false;
}

CFINode CFICFG::getAsmNode(const Function *asm_f){
  return getAsmNode(asm_f->getName());
}

CFINode CFICFG::getAsmNode(std::string name){
  CFINode *n = node_asm_map[name];
  CFIUtils u;
  if(n) return *n;
  else return u.returnFNode();
}

CFINode CFICFG::getDataNode(const Function *data_f){
  return getDataNode(data_f->getName());
}

CFINode CFICFG::getDataNode(std::string name){
  CFINode *n = node_data_map[name];
  CFIUtils u;
  if(n) return *n;
  else return u.returnFNode();
}

// used by CGD optimization
bool CFICFG::differentEdgesToNode(const Function *F){
  CFICluster c = getCluster(F->getFunctionType());
  CFINode n = getNode(F, !F->isDeclaration());
  if(n.id && !c.id){
    return false;
  }

  // check if any edge to a function with F's name exists if yes we clone
  // No harm if we clone declarations: this is easily fixed in the backend
  std::list<CFINode> parents;
  std::string fn = F->getName().str();

  // Heuristics to handle functions declared with wrong prototype.
  // 1 - Find a node with same name (duplications are warned by getNodeBinary)
  // 2 - Check its prototype for cluster (means it can be called from pointer)
  if(!n.id){
    // regularly type mismatch because function is declaration with wrong type
    n = getNodeBinary(fn);
    // Check if cluster to the newfound node exists, if not, return false;
    c = getCluster(n.proto);
    if(!c.id) return false;
  }

  // check if exists a node with the same name which is called directly
  std::list<CFINode*>::iterator i = node_list_n_map[fn].begin();
  std::list<CFINode*>::iterator ie = node_list_n_map[fn].end();

  for(; i != ie; i++){
    n = **i;
    getParents(parents, n.id);
    if(parents.size() > 0){
      return true;
    }
  }

  return false;
}

void CFICFG::getParents(std::list<CFINode>& parents, unsigned int tgt_id){
  std::list<CFIEdge>::iterator i;
  std::list<CFIEdge>::iterator ie;

  i = edge_parents_map[tgt_id].begin();
  ie = edge_parents_map[tgt_id].end();

  for(; i != ie; i++) parents.push_back(getNode(i->origin));
}

CFIEdge CFICFG::getAnyDirectEdgeToTgt(unsigned int to_id){
  CFIUtils u;
  CFIEdge e;
  std::list<CFIEdge>::iterator i = CFIEdges.begin();
  std::list<CFIEdge>::iterator ie = CFIEdges.end();

  for(; i != ie; ++i){
    e = *i;
    if(e.target == to_id && e.type == CFIEdge::ctype::dcall) return e;
  }
  return u.returnFEdge();
}

CFIEdge CFICFG::getEdge(const Function *From, Type *t, unsigned int order){
  CFIUtils u;
  CFIEdge *e;
  std::list<CFIEdge>::iterator i;
  std::list<CFIEdge>::iterator ie;

  unsigned int from_id = getNode(From).id;
  unsigned int to_id = getCluster(t).id;
  long key = (long) from_id << 32 | to_id;
  e = edge_ot_map[key];
  if(e) return *e;
  else return u.returnFEdge();
}

CFICluster CFICFG::getCluster(unsigned int id){
  CFIUtils u;
  CFICluster *c;
  c = cluster_id_map[id];
  if(c) return *c;
  else return u.returnFCluster();
}

CFICluster CFICFG::getCluster(std::string proto){
  CFIUtils u;
  CFICluster *c;

  c = cluster_p_map[proto];
  if(c) return *c;
  else return u.returnFCluster();
}

CFICluster CFICFG::getCluster(char * p){
  CFIUtils u;
  CFICluster *c;
  std::string proto = p;

  c = cluster_p_map[proto];
  if(c) return *c;
  else return u.returnFCluster();
}

CFICluster CFICFG::getCluster(Type *t){
  CFIUtils u;
  return getCluster(u.typeToStr(t));
}

void CFICFG::storeAliases(){
  CFIAlias a;
  std::ofstream f;
  std::map<std::string, std::string>::iterator i = aliases_map.begin();
  std::map<std::string, std::string>::iterator ie = aliases_map.end();

  for(; i != ie; i++){
    std::string head = "aliases";
    std::string mid = "";
    if(CFIBuildCFG) mid = "." + i->first;
    std::string ext = ".cfi";
    std::string fn = head + mid + ext;

    f.open(fn, std::ios::binary | std::ios::out);
    strcpy(a.alias, i->first.c_str());
    strcpy(a.aliasee, i->second.c_str());
    f.write((char *) &a, sizeof(CFIAlias));
    f.close();
  }
}

void CFICFG::storeDecls(){
  CFIDecl d;
  std::ofstream f;
  std::list<CFIDecl>::iterator di = CFIDecls.begin();
  std::list<CFIDecl>::iterator die = CFIDecls.end();

  char s[12];

  std::string filen;
  std::string head = "decls";
  std::string mid = "";
  std::string ext = "cfi";
  if(CFIBuildCFG){
    static const char alphanum[] = "0123456789abcdefghijklmnopqrstuvxz";
    for(int i = 0; i < 10; ++i){
      s[i] = alphanum[rand() % (sizeof(alphanum) -1)];
    }
    s[10] = '.';
    s[11] = '\0';
    mid = s;
    filen = head + mid + ext;
    while(access(filen.c_str(), F_OK)==0){
      for(int i = 0; i < 6; ++i){
        s[i] = alphanum[rand() % (sizeof(alphanum) -1)];
      }
      s[10] = '.';
      s[11] = '\0';
      std::string mid = s;
      filen = head + mid + ext;
    }
  }

  filen = head + mid + ext;
  f.open(filen, std::ios::binary | std::ios::out);
  di = CFIDecls.begin();
  die = CFIDecls.end();
  for(; di != die ; ++di){
    d = *di;
    f.write((char *) &d, sizeof(CFIDecl));
  }
  f.close();
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

  if(CFIClusters.size() + CFINodes.size() + CFIEdges.size() == 0) return;

  char s[12];

  std::string head = "nodes.";
  std::string ext = "cfi";
  std::string mid = "";
  std::string filen;
  if(CFIBuildCFG){
    static const char alphanum[] = "0123456789abcdefghijklmnopqrstuvxz";
    for(int i = 0; i < 10; ++i){
      s[i] = alphanum[rand() % (sizeof(alphanum) -1)];
    }
    s[10] = '.';
    s[11] = '\0';
    mid = s;
    filen = head + mid + ext;
    while(access(filen.c_str(), F_OK)==0){
      for(int i = 0; i < 6; ++i){
        s[i] = alphanum[rand() % (sizeof(alphanum) -1)];
      }
      s[10] = '.';
      s[11] = '\0';
      std::string mid = s;
      filen = head + mid + ext;
    }
  }

  std::ofstream f;

  filen = head + mid + ext;
  f.open(filen, std::ios::binary | std::ios::out);
  ni = CFINodes.begin();
  nie = CFINodes.end();
  for(; ni != nie ; ++ni){
    n = *ni;
    f.write((char *) &n, sizeof(CFINode));
  }
  f.close();

  head = "edges.";
  filen = head + mid + ext;
  f.open(filen, std::ios::binary | std::ios::out);
  ei = CFIEdges.begin();
  eie = CFIEdges.end();
  for(; ei != eie ; ++ei){
    e = *ei;
    f.write((char *) &e, sizeof(CFIEdge));
  }
  f.close();

  head = "clusters.";
  filen = head + mid + ext;
  f.open(filen, std::ios::binary | std::ios::out);
  ci = CFIClusters.begin();
  cie = CFIClusters.end();
  for(; ci != cie ; ++ci){
    c = *ci;
    f.write((char *) &c, sizeof(CFICluster));
  }
  f.close();
}

void CFICFG::loadNodeMap(){
  CFINode n;
  std::list<CFINode>::iterator ni;
  std::list<CFINode>::iterator nie;
  ni = CFINodes.begin();
  nie = CFINodes.end();

  for(; ni != nie; ++ni){
    std::string name(ni->name);
    std::string proto(ni->proto);
    std::string module(ni->module);
    node_id_map[ni->id] = &(*ni);
    node_npm_map[name + proto + module] = &(*ni);
    node_np_map[name + proto] = &(*ni);
  }
}

void CFICFG::loadEdgeMap(){
  CFIEdge e;
  std::list<CFIEdge>::iterator ei;
  std::list<CFIEdge>::iterator eie;
  ei = CFIEdges.begin();
  eie = CFIEdges.end();

  for(; ei != eie; ++ei){
    long key = (long) ei->origin << 32 | ei->target;
    edge_id_map[ei->id] = &(*ei);
    edge_ot_map[key] = &(*ei);
  }
}

void CFICFG::loadClusterMap(){
  CFICluster c;
  std::list<CFICluster>::iterator ci;
  std::list<CFICluster>::iterator cie;
  ci = CFIClusters.begin();
  cie = CFIClusters.end();

  for(; ci != cie; ++ci){
    std::string proto(ci->proto);
    cluster_id_map[ci->id] = &(*ci);
    cluster_p_map[proto] = &*(ci);
  }
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

// loads merged cfi files
void CFICFG::loadCFG(){
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
  createHCNodes();

  loaded_binary = false;
  loaded_aliases = false;
  loaded_decls = false;

  std::ifstream file("secondary.cfi");
  std::string line;
  while(std::getline(file, line)){
    std::stringstream lines(line);
    std::string f;
    std::getline(lines, f, ',');
    std::string ids_str;
    while(std::getline(lines, ids_str, ',')){
      ids_secs[f].push_back(ids_str);
    }
  }
}

std::string CFICFG::getDeclProto(std::string name){
  if(decls_n_map.count(name) > 0) return decls_n_map[name];
  return "";
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
  loaded_decls = true;
}

int CFICFG::getSecondaryIds(std::string f){
  if(ids_secs.count(f) > 0) return ids_secs[f].size();
  return 0;
}

// secondary ID is a hack/support to syscalls (see paper)
unsigned int CFICFG::getSecondaryId(std::string f, unsigned int pos){
  if(ids_secs.count(f) == 0) return 0;

  std::list<std::string>::iterator i = ids_secs[f].begin();
  std::list<std::string>::iterator ie = ids_secs[f].end();

  unsigned int step;

  for(step = 0; pos > step && i != ie; i++, step++);

  if(i != ie) return stoul(*i, nullptr, 16);
  return 0;
}

// debugging function to dump loaded maps
void CFICFG::dumpMaps(){
  std::map<unsigned int, CFINode*>::iterator i = node_id_map.begin();
  std::map<unsigned int, CFINode*>::iterator ie = node_id_map.end();

  fprintf(stderr, "NODE_ID_MAP\n");
  for(; i != ie; i++){
    CFINode *n = i->second;
    n->dump();
  }

  std::map<std::string, CFINode*>::iterator ii = node_npm_map.begin();
  std::map<std::string, CFINode*>::iterator iie = node_npm_map.end();

  fprintf(stderr, "NODE_NPM_MAP\n");
  for(; ii != iie; ii++){
    CFINode *n = ii->second;
    n->dump();
  }

  std::map<std::string, CFINode*>::iterator iii = node_np_map.begin();
  std::map<std::string, CFINode*>::iterator iiie = node_np_map.end();

  fprintf(stderr, "NODE_NP_MAP\n");
  for(; iii != iiie; iii++){
    CFINode *n = iii->second;
    n->dump();
  }
}

void CFICFG::dumpCFG(){
  std::list<CFINode>::iterator ni;
  std::list<CFINode>::iterator nie;
  std::list<CFICluster>::iterator ci;
  std::list<CFICluster>::iterator cie;
  std::list<CFIEdge>::iterator ei;
  std::list<CFIEdge>::iterator eie;

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

  fprintf(stderr, "CFI Hardcoded Nodes:\n");
  ni = HCNodes.begin();
  nie = HCNodes.end();
  for(; ni != nie ; ++ni) ni->dump();
}

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

// WARNING: May return unexpected nodes if fs with the same name exists in the
// binary. Should only be used to identify which homonimous fs survived linking
CFINode CFICFG::getNodeBinary(std::string callee){
  CFIUtils u;
  std::string addr;
  if(!loaded_binary) loadBinary("vmlinux.thin");
  if(fs_name_map.count(callee)) addr = *fs_name_map[callee];
  else{
    fprintf(stderr, "Could not find %s in binary\n", callee.c_str());
    addr = "0";
  }
  addr = addr.substr(0, 16);
  CFINode n = getNode(getIdByAddress(addr, callee));
  if(checkNameDuplicates(n)){
    u.warn("Using getNodeBinary on duplicated Node with no reference!\n");
    n.dump();
  }
  return n;
}

CFINode CFICFG::getNodeBinary(const Function *Caller, std::string Callee, bool
                              tailjmp){
  CFIUtils u;
  std::string address;
  CFINode dst, org;

  StringRef dst_name = Callee;
  StringRef org_name = Caller->getName();

  if(org_name.endswith("_kcfi")){
    std::size_t pos = org_name.find("_kcfi");
    org_name = org_name.substr(0, pos);
  }

  if(org_name.startswith("kcfi_")){
    std::string org_name = org_name.substr(5, org_name.size()-7);
  }

  if(!loaded_binary) loadBinary("vmlinux.thin");

  address = getCalleeAddress(org_name, dst_name, tailjmp);

  std::stringstream msg;

  if(strcmp(address.c_str(),"0")==0){
    msg << "getNodeBinary 1: Unable to find duplicated address in dump:\n";
    msg << org_name.str() << " calling " << dst_name.str();
    u.warn(msg.str());
    return u.returnFNode();
  }

  unsigned int id = getIdByAddress(address, dst_name.str());
  if(id == 0){
    msg << "getNodeBinary 2: Unable to find duplicated function id in dump\n";
    msg << org_name.str() << " calling " << dst_name.str();
    u.warn(msg.str());
    return u.returnFNode();
  }

  dst = getNode(id);
  if(dst.id == 0){
    msg << "getNodeBinary 3: Invalid id for duplicated function:\n";
    msg << org_name.str() << " calling " << dst_name.str();
    // FIX id below, do proper conversion
    msg << "\nAddress:" << address << "Id: " << id;
    u.warn(msg.str());
    return u.returnFNode();
  }
  return dst;
}

CFINode CFICFG::getNodeBinary(const Function *Caller, const Function *Callee,
                              bool tailjmp){
  CFIUtils u;
  std::string address;
  CFINode dst, org;
  dst = getNode(Callee, false);
  if(!checkDuplicates(dst)) return dst;

  StringRef dst_name = Callee->getName();
  StringRef org_name = Caller->getName();

  if(org_name.endswith("_kcfi")){
    std::size_t pos = org_name.find("_kcfi");
    org_name = org_name.substr(0, pos);
  }

  if(org_name.startswith("kcfi_")){
    org_name = org_name.substr(5, org_name.size()-7);
  }

  if(!loaded_binary) loadBinary("vmlinux.thin");

  address = getCalleeAddress(org_name, dst_name, tailjmp);

  std::stringstream msg;

  if(strcmp(address.c_str(),"0")==0){
    msg << "getNodeBinary 1: Unable to find duplicated address in dump:\n";
    msg << org_name.str() << " calling " << dst_name.str();
    u.warn(msg.str());
    return u.returnFNode();
  }

  unsigned int id = getIdByAddress(address, dst_name.str());
  if(id == 0){
    msg << "getNodeBinary 2: Unable to find duplicated function id in dump\n";
    msg << org_name.str() << " calling " << dst_name.str();
    u.warn(msg.str());
    return u.returnFNode();
  }
  dst = getNode(id);
  if(dst.id == 0){
    msg << "getNodeBinary 3: Invalid id for duplicated function:\n";
    msg << org_name.str() << " calling " << dst_name.str();
    // FIX id below, do proper conversion
    msg << "\nAddress:" << address << "Id: " << id;
    u.warn(msg.str());
    return u.returnFNode();
  }
  return dst;
}

void CFICFG::loadBinary(std::string filename){
  std::stringstream pstream;
  std::ifstream file(filename);
  std::string line;
  std::list<std::string>::iterator p;
  std::string name;
  std::string addr;

  Regex fn("[1234567890abcdef\\s]*<.*>:$");

  while(std::getline(file, line)){
    binary_reference.push_back(line);
    p = binary_reference.end();
    p--;
    if(fn.match(line)){
      name = line.substr(18,line.length()-20);
      addr = line.substr(0, 16);
      while(fs_name_map.count(name)){
        name = name + "i";
      }
      fs_name_map[name] = p;
      fs_addr_map[addr] = p;
    }
  }
  loaded_binary = true;
}

void CFICFG::dumpBinary(){
  std::list<std::string>::iterator p, pe;
  std::map<std::string, std::list<std::string>::iterator>::iterator m, me;

  fprintf(stderr, "** Binary:\n");

  p = binary_reference.begin();
  pe = binary_reference.end();

  for(; p != pe; p++){
    fprintf(stderr, "%s\n", p->c_str());
  }

  fprintf(stderr, "\n ** f_name_map:\n");

  m = fs_name_map.begin();
  me = fs_name_map.end();

  for(; m != me; m++){
    std::string pstr(*m->second);
    fprintf(stderr, "%s\n", pstr.c_str());
  }

  fprintf(stderr, "\n ** f_addr_map:\n");

  m = fs_addr_map.begin();
  me = fs_addr_map.end();

  for(; m != me; m++){
    std::string pstr(*m->second);
    fprintf(stderr, "%s\n", pstr.c_str());
  }
}

std::string CFICFG::getCalleeAddress(std::string caller, std::string callee,
                                     bool tailjmp){
  std::stringstream pstream;
  std::list<std::string>::iterator p, pe, f;
  Regex eofunct("^$");

  if(fs_name_map.find(caller)==fs_name_map.end()) return "0";

  pstream.str(std::string());
  if(!tailjmp) pstream << ".call.*<" << callee << ">$";
  else pstream << ".*jmp.*<" << callee << ">$";
  Regex call(pstream.str());

  f = fs_name_map[caller];

  p = f;

  for(int i = 0; i < 10; p++){
    std::string line = *p;
    if(eofunct.match(line)) break;
    if(call.match(line)){
      return line.substr(line.length()-(callee.length()+19),16);
    }
  }

  if(fs_name_map.count(caller)){
    return getCalleeAddress(caller + "i", callee, tailjmp);
  }
  else
    return "0";
}

unsigned int CFICFG::getIdByAddress(std::string address, std::string callee){
  unsigned int id;
  std::stringstream pstream;
  std::stringstream hex_to_int;
  std::list<std::string>::iterator p, pe;
  Regex vld("[1234567890abcdef]+");
  Regex vld_id("[1234567890abcdef]+.*movl.*[1234567890abcdef]+.*$");

  if(!vld.match(address) || address.length() < 16) return 0;

  p = fs_addr_map[address];
  p++;
  std::string sid = *p;

  if(!vld_id.match(sid)){
    if(node_asm_map.count(callee)) return node_asm_map[callee]->id;
    else return 0;
  }

  sid = sid.substr(28,8);
  std::stringstream a(sid);
  std::getline(a, sid, ',');
  if(vld.match(sid)){
    hex_to_int << std::hex << sid;
    hex_to_int >> id;
    return id;
  }
  return 0;
}

// some macro magic to easily feed all hard coded nodes
#define HCGEN(n, p, xid, hid, tid)      \
  r = getHCNode(#n, #p);                \
  if(r.id == 0){                        \
  r.id = xid;                           \
  r.head_id = hid;                      \
  r.tail_id = tid;                      \
  r.parent_id = 0;                      \
  strcpy(r.name, #n);                   \
  strcpy(r.proto, #p);                  \
  strcpy(r.module, "hardcoded");        \
  r.leaf_distance = 100000;             \
  addToHCGraph(r);                      \
}

CFINode CFICFG::getHCNode(const char name[], const char p[]){
  CFIUtils u;
  CFINode n;
  std::list<CFINode>::iterator i;
  std::list<CFINode>::iterator ie;

  i = HCNodes.begin();
  ie = HCNodes.end();

  for(; i != ie; ++i){
    n = *i;
    if(strcmp(n.name, name)==0 && strcmp(n.proto, p)==0 &&
       strcmp(n.module, "hardcoded")==0) return n;
  }
  return u.returnFNode();
}


// Create hardcoded tags for special functions (like glibc or macro generated)
void CFICFG::createHCNodes(){
  CFINode r;
  HCGEN("ia32_sys_call_table", "", 0x1337bf00, 0x1337be80, 0x1337be00);
  HCGEN("__audit_sycall_entry", "", 0x1337bf01, 0x1337be81, 0x1337be01);
  HCGEN("__audit_syscall_exit", "", 0x1337bf02, 0x1337be82, 0x1337be02);
  HCGEN("syscall_trace_enter", "", 0x1337bf03, 0x1337be83, 0x1337be03);
  HCGEN("ia32_ptregs_common", "", 0x1337bf04, 0x1337be84, 0x1337be04);
  HCGEN("__sw_hweight64", "", 0x1337bf05, 0x1337be85, 0x1337be05);
  HCGEN("lockdep_sys_exit_thunk", "", 0x1337bf06, 0x1337be86, 0x1337be06);
  HCGEN("trace_hardirqs_on_thunk", "", 0x1337bf07, 0x1337be87, 0x1337be07);
  HCGEN("trace_hardirqs_off_thunk", "", 0x1337bf08, 0x1337be88, 0x1337be08);
  HCGEN("fail_fn", "", 0x1337bf09, 0x1337be89, 0x1337be09);
  HCGEN("call_rwsem_down_read_failed", "", 0x1337bf0a, 0x1337be8a, 0x1337be0a);
  HCGEN("call_rwsem_down_write_failed", "", 0x1337bf0b, 0x1337be8b, 0x1337be0b);
  HCGEN("call_rwsem_wake", "", 0x1337bf0c, 0x1337be8c, 0x1337be0c);
  HCGEN("rwsem_downgrade_wake", "", 0x1337bf0d, 0x1337be8d, 0x1337be0d);
  HCGEN("__swith_to", "", 0x1337bf0e, 0x1337be8e, 0x1337be0e);
  HCGEN("early_make_pgtable", "", 0x1337bf0f, 0x1337be8f, 0x1337be0f);
  HCGEN("rwsem_down_read_failed", "", 0x1337bf10, 0x1337be90, 0x1337be10);
  HCGEN("rwsem_down_write_failed", "", 0x1337bf11, 0x1337be91, 0x1337be11);
  HCGEN("rwsem_wake", "", 0x1337bf12, 0x1337be92, 0x1337be12);
  HCGEN("call_rwsem_downgrade_wake", "", 0x1337bf13, 0x1337be93, 0x1337be13);
  HCGEN("__isoc99_scanf", "i32 (i8*, ...)", 0x1337bf14, 0x1337be94, 0x1337be14);
  HCGEN("printf", "i32 (i8*, ...)", 0x1337bf15, 0x1337be95, 0x1337be15);
  HCGEN("fprintf", "i32 (%struct._IO_FILE*, i8*, ...)", 0x1337bf16, 0x1337be96,
        0x1337be16);
}

bool CFICFG::hasAsmParents(CFINode n){
  std::list<CFIEdge>::iterator i, ie;
  i = edge_parents_map[n.id].begin();
  ie = edge_parents_map[n.id].end();

  for(; i != ie; i++){
    CFINode n2 = *node_id_map[i->origin];
    if(node_asm_map.count(n2.name)>0) return true;
  }
  return false;
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
  CFINodes.push_back(n);
  std::list<CFINode>::iterator ni = CFINodes.end();
  ni--;

  std::string name(n.name);
  std::string proto(n.proto);
  std::string module(n.module);
  node_id_map[ni->id] = &(*ni);
  node_npm_map[name + proto + module] = &(*ni);
  node_np_map[name + proto] = &(*ni);
  node_list_n_map[name].push_back(&(*ni));
  node_list_np_map[name + proto].push_back(&(*ni));
  if(proto.compare("ASM")==0){
    node_asm_map[name] = &(*ni);
    return;
  }
  if(proto.compare("DATA")==0){
    node_data_map[name] = &(*ni);
  }
}

void CFICFG::addToGraph(CFIEdge e){
  CFIEdges.push_back(e);
  std::list<CFIEdge>::iterator ei = CFIEdges.end();
  ei--;

  long key = (long) ei->origin << 32 | ei->target;
  edge_id_map[ei->id] = &(*ei);
  edge_ot_map[key] = &(*ei);
  edge_childs_map[ei->origin].push_back(e);
  edge_parents_map[ei->target].push_back(e);
}

void CFICFG::addToGraph(CFICluster c){
  CFIClusters.push_back(c);
  std::list<CFICluster>::iterator ci = CFIClusters.end();
  ci--;

  std::string proto(ci->proto);
  cluster_id_map[ci->id] = &(*ci);
  cluster_p_map[proto] = &*(ci);
}

void CFICFG::addToHCGraph(CFINode n){
  HCNodes.push_back(n);
}

extern CFICFG cfi;

std::string CFIUtils::typeToStr(Type *t){
  std::string type_str;
  raw_string_ostream rso(type_str);
  t->print(rso);
  return rso.str();
}

bool CFIUtils::isDCall(const CallInst *CI){
  if(!isICall(CI)) return true;
  return false;
}

void CFIUtils::error(std::string er, bool fatal){
  fprintf(stderr, "CFIError: %s\n", er.c_str());
  if(fatal){
    exit(-1);
  }
}

void CFIUtils::error(bool fatal){
  fprintf(stderr, "CFIError: %s\n", cfi_er.str().c_str());
  if(fatal){
    exit(-1);
  }
  cfi_er.str(std::string());
}

void CFIUtils::warn(std::string warn){
  fprintf(stderr, "CFIWarn: %s\n", warn.c_str());
}

void CFIUtils::warn(){
  fprintf(stderr, "CFIWarn: %s\n", cfi_warn.str().c_str());
  cfi_warn.str(std::string());
}

void CFIUtils::msg(std::string msg){
  if(!CFIv) return;
  fprintf(stderr, "CFIMsg: %s\n", msg.c_str());
}

void CFIUtils::msg(){
  if(!CFIv) return;
  fprintf(stderr, "CFIMsg: %s\n", cfi_msg.str().c_str());
  cfi_msg.str(std::string());
}

bool CFIUtils::isICall(const CallInst *CI){
  const Function *F = CI->getCalledFunction();
  if(!F){
    const Value *a = CI->getCalledValue();
    if(a != NULL){
      if(isa<InlineAsm>(a)) return false;
      F = dyn_cast_or_null<Function>(a->stripPointerCasts());
    }
    if(!F) return true;
  }
  return false;
}

bool CFICFG::checkNameDuplicates(CFINode n){
  std::string name = n.name;

  if(node_list_n_map[name].size() > 1) return true;
  return false;
}

bool CFICFG::checkDuplicates(CFINode n){
  std::string name = n.name;
  std::string proto = n.proto;

  if(node_list_np_map[name + proto].size() > 1) return true;
  return false;
}

bool CFICFG::markDuplicates(CFINode n){
  std::list<CFINode>::iterator i;
  std::list<CFINode>::iterator ie;

  bool flag = false;
  i = CFINodes.begin();
  ie = CFINodes.end();
  for(; i != ie; ++i){
    if((i->id != n.id)
       && (strcmp(n.name, i->name)==0)
       && (strcmp(n.proto, i->proto)==0)){
      i->has_duplicate = true;
      flag = true;
    }
  }
  return flag;
}

std::string CFICFG::checkWeakAlias(StringRef fname){
  if(!loaded_aliases) loadAliases();

  if(aliases_map.count(fname)){
    return aliases_map[fname];
  }

  if(fname.equals("memset")){
    std::string memset = "__memset";
    return memset;
  }
  return "";
}

std::string CFICFG::checkWeakAlias(const Function *F){
  if(!loaded_aliases) loadAliases();
  StringRef fname = F->getName();

  if(aliases_map.count(fname)){
    return aliases_map[fname];
  }

  if(fname.equals("memset")){
    std::string memset = "__memset";
    return memset;
  }
  return "";
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
  strncpy(c.proto, "", 1);
  c.id = 0;
  c.head_id = 0;
  c.tail_id = 0;
  return c;
}

CFIEdge CFIUtils::returnFEdge(){
  CFIEdge e;
  e.id = 0;
  e.origin = 0;
  e.target = 0;
  e.order = 0;
  e.type = CFIEdge::ctype::dcall;
  return e;
}
