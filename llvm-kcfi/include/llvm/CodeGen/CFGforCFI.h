//===---- CFGforCFI.h - kCFI Control-Flow Graph ---------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Control-Flow Graph implementation used in kCFI (header for CFGforCFI.cpp)
//
//===----------------------------------------------------------------------===//
// TODO:
// * getCalleeByAddressFast should be getCalleeByAddress
//   - "Fast" prefix inherited as this version optimizes deleted code

#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/Function.h"
#include <list>
#include <sstream>

#ifndef CFGforCFI_classes
#define CFGforCFI_classes

extern std::stringstream cfi_er;
extern std::stringstream cfi_msg;
extern std::stringstream cfi_warn;

namespace llvm {
  class CFIAlias{
  public:
    char alias[1000];
    char aliasee[1000];
  };

  class CFIDecl{
  public:
    char name[300];
    char proto[500];

    CFIDecl(){};

    CFIDecl(const char n[], const char p[]){
      strncpy(name, n, 300);
      strncpy(proto, p, 500);
    }
  };

  class CFINode{
  public:
    char name[300];
    char proto[500];
    char module[300];
    unsigned int id;
    unsigned int parent_id;
    unsigned int head_id;
    unsigned int tail_id;
    unsigned int leaf_distance;
    bool has_duplicate;
    bool has_alias;

    void dump(){
      fprintf(stderr, "CFINode: %s:%s:%s id:%x head_id:%x tail_id:%x\n",
                name, proto, module, id, head_id, tail_id);
    }

    CFINode(){};

    CFINode(const char n[], const char p[], const char m[], unsigned int xid,
            unsigned int hid, unsigned int tid, unsigned int pid = 0){
      strncpy(name, n, 300);
      strncpy(proto, p, 500);
      strncpy(module, m, 300);
      id = xid;
      parent_id = pid;
      head_id = hid;
      tail_id = tid;
      leaf_distance = 0;
      has_duplicate = false;
    }
  };

  class CFICluster{
  public:
    char proto[500];
    unsigned int id;
    unsigned int head_id;
    unsigned int tail_id;
    unsigned int count = 0;

    void dump(){
      std::list<CFINode>::iterator i;
      std::list<CFINode>::iterator ie;
      CFINode n;
      fprintf(stderr, "CFICluster: %s id:%x head_id:%x tail_id:%x count:%d\n",
              proto, id, head_id, tail_id, count);
    }

    CFICluster(){};

    CFICluster(const char p[], unsigned int xid, unsigned int hid,
                           unsigned int tid){
      strncpy(proto, p, 500);
      id = xid;
      head_id = hid;
      tail_id = tid;
    }
  };

  class CFIEdge{
  public:
    enum ctype{
      dcall = 1,
      icall = 2,
      jmp = 3
    };
    unsigned int id;
    unsigned int origin;
    unsigned int target;
    unsigned int order;
    ctype type;

    CFIEdge(){};

    CFIEdge(ctype t, unsigned int i, unsigned int org, unsigned int tgt,
            unsigned int o){
      type = t;
      id = i;
      origin = org;
      target = tgt;
      order = o;
    }

    void dump(){
      switch(type){
      case 1:
        fprintf(stderr, "CFIEdge: dcall ");
        break;
      case 2:
        fprintf(stderr, "CFIEdge: icall ");
        break;
      case 3:
        fprintf(stderr, "CFIEdge: jmp   ");
        break;
      }
      fprintf(stderr, "%x %x %x %d\n", id, origin, target, order);
    }
  };

  class CFICFG {
  public:
    void loadTags();

    void lockTags(FILE **lock);

    void unlockTags(FILE **lock);

    void storeTags();

    void loadIds();

    void lockIds(FILE **lock);

    void unlockIds(FILE **lock);

    void storeIds();

    int getSecondaryIds(std::string f);

    unsigned int getSecondaryId(std::string f, unsigned int pos);

    bool differentEdgesToStrongerNode(Function *F);

    std::string checkWeakAlias(StringRef f);

    std::string checkWeakAlias(const Function *F);

    void setAlias(std::string alias, std::string aliasee);

    std::string getAlias(std::string alias);

    CFINode getNodeIgnore(const Function *F, std::list<unsigned int> id);

    CFINode getDataNode(std::string name);

    CFINode getDataNode(const Function *data_f);

    bool hasAsmParents(CFINode n);

    CFINode getAsmNode(std::string name);

    CFINode getAsmNode(const Function *asm_f);

    CFIDecl createDecl(const Function *F);

    CFINode createNode(const Function *F);

    CFIEdge createEdge(const Function *from, const Function *to);

    CFIEdge createEdge(const Function *from, Type *to);

    CFIEdge createEdgeBinary(const Function *From, const Function *to);

    CFICluster createCluster(Type *t);

    void getParents(std::list<CFINode>& parents, unsigned int tgt_id);

    CFINode getNode(unsigned int id);

    CFINode getNodeBinary(std::string callee);

    CFINode getNodeBinary(const Function *Caller, std::string Callee,
                          bool tailjmp = false);

    CFINode getNodeBinary(const Function *Caller, const Function *Callee,
                          bool tailjmp = false);

    CFINode getNode(const Function *F, bool checkModule = true);

    CFICluster getCluster(char * p);

    CFICluster getCluster(unsigned int id);

    CFICluster getCluster(std::string proto);

    CFICluster getCluster(Type *to);

    CFINode getHCNode(const char n[], const char p[]);

    CFIEdge getEdge(unsigned int id);

    CFIEdge getEdge(const Function *origin, const Function *target);

    CFIEdge getEdge(const Function *From, Type *t, unsigned int order = 0);

    CFIEdge getAnyDirectEdgeToTgt(unsigned int to_id);

    bool differentEdgesToNode(const Function *F);

    unsigned int getOrder(CFIEdge::ctype t, unsigned int org, unsigned int tgt);

    void createHCNodes();

    std::string getDeclProto(std::string name);

    void loadDecls();

    void storeDecls();

    void storeAliases();

    void loadAliases();

    void storeCFG();

    void loadCFG();

    bool loadedDecls() { return loaded_decls; };

    void loadNodeMap();

    void loadEdgeMap();

    void loadClusterMap();

    void loadBinary(std::string filename);

    void dumpBinary();

    void dumpMaps();

    void dumpCFG();

    void addToHCGraph(CFINode r);

    void addToGraph(CFIDecl d);

    void addToGraph(CFINode n);

    void addToGraph(CFIEdge e);

    void addToGraph(CFICluster c);

    bool checkDuplicates(CFINode n);

    bool checkNameDuplicates(CFINode n);
  private:
    unsigned int getNextTag();

    unsigned int getNextId();

    unsigned int getIdByAddress(std::string address, std::string callee);

    std::string getCalleeAddress(std::string caller, std::string callee,
                                     bool tailjmp = false);

    bool markDuplicates(CFINode n);

    std::list<CFINode> CFINodes;
    std::list<CFICluster> CFIClusters;
    std::list<CFIEdge> CFIEdges;
    std::list<CFINode> HCNodes;
    std::list<CFIDecl> CFIDecls;

    std::map<std::string, std::list<std::string> > ids_secs;
    std::map<std::string, std::string> aliases_map;
    std::map<unsigned int, CFINode*> node_id_map;
    std::map<std::string, CFINode*> node_data_map;
    std::map<std::string, CFINode*> node_asm_map;
    std::map<std::string, CFINode*> node_npm_map;
    std::map<std::string, CFINode*> node_np_map;
    std::map<std::string, bool> decls_map;
    std::map<std::string, std::string> decls_n_map;
    std::map<std::string, std::list<CFINode*> > node_list_n_map;
    std::map<std::string, std::list<CFINode*> > node_list_np_map;
    std::map<unsigned int, CFIEdge*> edge_id_map;
    std::map<unsigned long, CFIEdge*> edge_ot_map;
    std::map<unsigned int, std::list<CFIEdge> > edge_childs_map;
    std::map<unsigned int, std::list<CFIEdge> > edge_parents_map;
    std::map<unsigned int, CFICluster*> cluster_id_map;
    std::map<std::string, CFICluster*> cluster_p_map;
    std::map<std::string, std::list<std::string>::iterator> fs_name_map;
    std::map<std::string, std::list<std::string>::iterator> fs_addr_map;

    std::list<std::string> binary_reference;
    bool loaded_binary;
    bool loaded_aliases;
    bool loaded_decls;
  };

  class CFIUtils{
  public:
    std::string typeToStr(Type *x);

    bool isDCall(const CallInst *CI);

    bool isICall(const CallInst *CI);

    CFINode returnFNode();

    CFICluster returnFCluster();

    CFIEdge returnFEdge();

    void error(std::string er, bool fatal);

    void error(bool fatal);

    void warn(std::string warn);

    void warn();

    void msg();

    void msg(std:: string msg);

  };
}

#endif
