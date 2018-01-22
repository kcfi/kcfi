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

#include <map>
#include <list>
#include <stdio.h>
#include <string.h>
#include <sstream>

#define ASM_DISTANCE 100000
#define UNSET_DISTANCE 110000
#define MAX_IMMUTABLE 2

enum ctype{
  dcall = 1,
  icall = 2,
  jmp = 3
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

  void dump(){
    fprintf(stderr, "CFIDecl: %s!%s\n", name, proto);
  }
};

class CFIAlias{
public:
  char alias[1000];
  char aliasee[1000];

  CFIAlias(){};
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
    fprintf(stderr, "CFINode: %s:%s:%s !id:%x head_id:%x tail_id:%x ",
            name, proto, module, id, head_id, tail_id);
    fprintf(stderr, "leaf_distance:%d\n", leaf_distance);
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
    leaf_distance = UNSET_DISTANCE;
  }
};

class CFICluster{
public:
  char proto[500];
  unsigned int id;
  unsigned int head_id;
  unsigned int tail_id;
  unsigned int count;

  void dump(){
    std::list<CFINode>::iterator i;
    std::list<CFINode>::iterator ie;
    CFINode n;
    fprintf(stderr, "CFICluster: %s !id:%x head_id:%x tail_id:%x count:%d\n",
            proto, id, head_id, tail_id, count);
  }

  CFICluster(){};

  CFICluster(const char p[], unsigned int xid, unsigned int hid,
             unsigned int tid){
    fprintf(stderr, "CFICluster\n");
    strncpy(proto, p, 500);
    id = xid;
    head_id = hid;
    tail_id = tid;
    count = 0;
  }
};

class CFIEdge{
public:
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
  std::list<CFINode> CFINodes;
  std::list<CFICluster> CFIClusters;
  std::list<CFIEdge> CFIEdges;
  std::list<CFIDecl> CFIDecls;
  std::map<std::string, std::string> aliases_map;
  std::map<std::string, bool> decls_map;
  std::map<std::string, std::string> decls_n_map;
  std::map<unsigned int, CFINode*> node_id_map;
  std::map<std::string, CFINode*> node_asm_map;
  std::map<std::string, CFINode*> node_asm_namemap;
  std::map<std::string, CFINode*> node_npm_map;
  std::map<std::string, CFINode*> node_np_map;
  std::map<unsigned int, CFIEdge*> edge_id_map;
  std::map<unsigned long, CFIEdge*> edge_ot_map;
  std::map<unsigned int, std::list<CFIEdge> > edge_childs_map;
  std::map<unsigned int, std::list<CFIEdge> > edge_parents_map;
  std::map<unsigned int, CFICluster*> cluster_id_map;
  std::map<std::string, CFICluster*> cluster_p_map;
  std::map<std::string, CFINode*> node_data_namemap;
  std::map<std::string, char*> function_map;
  std::map<std::string, unsigned int> function_id_map;

public:
  pthread_mutex_t cfi_lock;

public:
  void storeTags();

  void loadTags();

  void lockTags(FILE** fd);

  void unlockTags(FILE** fd);

  unsigned int getNextTag();

  void storeIds();

  void loadIds();

  void lockIds(FILE** fd);

  void unlockIds(FILE** fd);

  unsigned int getNextId();

  bool fixDecl(std::string name, std::string oldp, std::string newp);

  bool fixProto(unsigned int id, std::string proto);

  bool fixClusterProto(unsigned int id, std::string proto);

  bool fixNodeIds(unsigned int id, unsigned int hid, unsigned int tid);

  bool fixIds(unsigned int id, unsigned int hid, unsigned int tid);

  bool duplicatedInBinary(std::string dump, std::string callee);

  std::string getDeclProto(std::string name);

  CFINode getNodeBinary(std::string dump, std::string callee);

  CFINode getNodeName(std::string name);

  bool checkNameDuplicates(CFINode n);

  int countNodes(std::string proto);

  bool mergeItens(unsigned int id1, unsigned int id2);

  void loadFunctionIds();

  void dumpFunctionMap();

  void mergeCFIFiles(std::string path);

  void getAllCFIFiles(std::string path, std::list<std::string>& nodes,
                      std::list<std::string>& edges,
                      std::list<std::string>& clusters,
                      std::list<std::string>& aliases,
                      std::list<std::string>& tails,
                      std::list<std::string>& decls
                     );

  unsigned int getTailJmps(unsigned int to, std::list<CFIEdge>& tgts);

  unsigned int getDCalls(unsigned int to, std::list<CFIEdge>& tgts);

  std::string getAliasee(std::string alias);

  void setAlias(std::string alias, std::string aliasee);

  void addParsedFunction(std::string address, char * p);

  CFINode createAsmNode(std::string name, std::string addr);

  CFINode createDataNode(std::string name, std::string addr);

  bool isInDataSection(std::string name, std::string dump);

  unsigned int getIdFastest(std::string address);

  std::string getIdFast(char * funcs[], int nfuncs, std::string address);

  int createAsmNodes(char * funcs[], int nf);

  CFIEdge createJmpEdge(unsigned int from, unsigned int to);

  CFIEdge createJmpEdge(std::string dump, std::string from, std::string to);

  std::string getIdByAddress(char *funcs[], int nfuncs, std::string address);

  int createDirectEdgesThread(char * funcs[], int begin, int end, int nr);

  int mapLeavesThread(char * funcs[], int begin, int end, int nr);

  int createDirectEdges(char * funcs[], int nfuncs);

  int createJmpEdges(char * funcs[], int nfuncs);

  unsigned int getIdInMap(char * file, char * lines[], unsigned int nlines,
                          std::string address, unsigned int skip);

  bool getChilds(std::list<CFINode>& tgts, unsigned int from);

  void getParents(std::list<CFINode>& parents, unsigned int tgt_id);

  CFIEdge createDirectEdge(unsigned int from, unsigned int to);

  CFINode getNode(unsigned int id);

  CFINode getAsmNode(std::string addr);

  CFINode getAsmNodeOffset(std::string callee);

  std::string getAddress(std::string dump, std::string f);

  CFIEdge createDirectEdge(std::string dump, std::string from, std::string to);

  CFINode getNodeBinary(std::string dump, std::string caller,
                        std::string callee, bool tailjmp);

  CFICluster getCluster(std::string proto);

  void fixMergedEdges(CFICluster c);

  CFIEdge getEdge(unsigned int id);

  unsigned int getOrder(ctype t, unsigned int org, unsigned int tgt);

  bool declsLoaded();

  void storeMergedDecls();

  void storeMergedAliases();

  void storeMergedTails();

  void loadAliases();

  void storeCFG();

  void loadDecls();

  void loadCFG();

  void dumpCFG();

  void dumpDecls();

  void addToGraph(CFIDecl d);

  void addToGraph(CFINode n);

  void addToGraph(CFIEdge e);

  void addToGraph(CFICluster c);

  unsigned int getIdByAddressThin(std::string dump, std::string address);

private:
  unsigned int getIdByAddress(std::string dump, std::string address,
                              unsigned int skip = 0);

  std::string getCalleeAddress(std::string dump, std::string caller,
                               std::string callee, bool tailjmp = false);

  bool checkDuplicates(CFINode n);
};

class CFIUtils{
public:
  CFINode returnFNode();

  CFIEdge returnFEdge();

  CFICluster returnFCluster();
};
