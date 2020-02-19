// kcfi-tools Copyright (C) 2015 Universidade Estadual de Campinas
//
// This software was developed by Joao Moreira <joao@overdrivepizza.com>
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

#define NUnits 40

char * buffer;
char * rbuffer;
char ** lines;
char * funcs[1000000];
long f;

typedef struct unit {
  int begin;
  int end;
  int number;
} cunit;

pthread_t tid[NUnits];
CFICFG cfi;

void* start_new_computation(void * params);

int main(int argc, char *argv[]){
  if(argc != 2){
    fprintf(stderr, "usage: %s <thin dump>\n", argv[0]);
    exit(-1);
  }

  cfi.loadCFG();
  if(pthread_mutex_init(&cfi.cfi_lock, NULL) != 0){
    fprintf(stderr, "MUTEX FAILED\n");
    exit(-2);
  }

  cunit units[NUnits];

  long size = 0, j = 0;
  f = 0;

  FILE * dump = fopen(argv[1], "r");
  buffer = (char *) malloc(1000*pow(10,6));
  lines = (char **) malloc(sizeof(char *) * 10000020);

  fseek(dump, 0, SEEK_END);
  size = ftell(dump);
  rewind(dump);

  funcs[0] = buffer;
  f++;
  lines[0] = buffer;
  for(int i = 0; i < size; i++){
    long result = fread(&buffer[i],1,1,dump);
    if(buffer[i]=='\n'){
      buffer[i] = '\0';
      lines[j+1] = &buffer[i+1];
      if(strcmp(lines[j],"")==0){
        funcs[f] = lines[j+1];
        f++;
      }
      j++;
    }
  }
  lines[j] = 0x0;

  // remove empty lines
  for(int i = 0; i < f; i++){
    if(strcmp(funcs[i], "")==0){
      f = f - 1;
      for(int j = i; j < f; j++){
        funcs[j] = funcs[j+1];
      }
      i = -1;
    }
  }

  for(int i = 0; i < f; i++){
    std::string function = funcs[i];
    std::string addr = function.substr(0,16);
    cfi.addParsedFunction(addr, funcs[i]);
  }

  printf("processing\n");
  cfi.createAsmNodes(funcs, f);

  cfi.loadFunctionIds();

  int csize = f / NUnits;
  int last = f % NUnits;
  int begin = 0;
  int err = 0;
  int i;

  for(i = 0; i < NUnits; i++){
    units[i].begin = begin;
    units[i].end = begin + csize;
    units[i].number = i;
    begin = begin + csize;
    if(i==NUnits-1) units[i].end = units[i].end + last;
    err = pthread_create(&tid[i], NULL, &start_new_computation, &units[i]);
    if(err != 0){
      fprintf(stderr, "ERROR while creating thread %d.\n", i);
      exit(-3);
    }
  }

  for(i = 0; i < NUnits; i++){
    void* status;
    pthread_join(tid[i], &status);
  }

  pthread_mutex_destroy(&cfi.cfi_lock);

  cfi.storeCFG();
  cfi.dumpCFG();
  fclose(dump);
  free(buffer);
}

void* start_new_computation(void * params){
  cunit * u = (cunit *) params;
  fprintf(stderr, "Starting Thread %d: id %d, bg %d, end %d\n", u->number,
          tid[u->number], u->begin, u->end);
  cfi.createDirectEdgesThread(funcs, u->begin, u->end, u->number);
  cfi.mapLeavesThread(funcs, u->begin, u->end, u->number);
  fprintf(stderr, "Thread %d returned\n", u->number);
  return NULL;
};
