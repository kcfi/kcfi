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
#include <stdlib.h>
#include <math.h>
#include "../lib/CFGforCFI.h"
#include <iostream>
#include <fstream>

CFICFG cfi;

int main(int argc, char *argv[]){
  char a[300];
  if(argc != 2){
    fprintf(stderr, "usage: %s <callees>\n", argv[0]);
    exit(-1);
  }

  cfi.loadCFG();
  cfi.loadDecls();
  if(pthread_mutex_init(&cfi.cfi_lock, NULL) != 0){
    fprintf(stderr, "MUTEX FAILED\n");
    exit(-2);
  }

  std::ifstream input;
  input.open(argv[1]);
  for(std::string line; getline(input, line);){
    CFINode n = cfi.getNodeName(line.c_str());
    if(!n.id) n.tail_id = 0x1337beef;
    if(n.tail_id == 0x1337beef){
      strcpy(a, line.c_str());
      if(a[0] == 's' && a[1] == 'y' && a[2] == 's'){
        a[0] = 'S';
        a[2] = 'S';
        fprintf(stderr, "trying %s\n", a);
        CFINode n2 = cfi.getNodeName(a);
        if(n2.id){
          n.tail_id = n2.tail_id;
          strcpy(n.proto, n2.proto);
        }
      } else {
        if(strncmp(a, "compat_sys", 10)==0){
          a[7] = 'S';
          a[9] = 'S';
          fprintf(stderr, "trying %s\n", a);
          CFINode n2 = cfi.getNodeName(a);
          if(n2.id){
            n.tail_id = n2.tail_id;
            strcpy(n.proto, n2.proto);
          }
        }
      }
    }
    CFICluster c;
    c.id = 0;
    std::string proto;
    if(strcmp(n.proto, "ASM")==0){
      proto = cfi.getDeclProto(line);
    } else {
      proto = n.proto;
    }

    c = cfi.getCluster(proto);
    if(c.id) printf("%s;%x\n", line.c_str(), c.tail_id);
    else printf("%s;%x\n", line.c_str(), n.tail_id);
  }
}
