#!/bin/sh
cd ../lib/
g++ -c CFGforCFI.cpp -std=c++11
cd ../asm_id_resolver/
g++ asm_head_resolver.cpp ../lib/CFGforCFI.o -o asm_head_id -lpthread
g++ asm_id_resolver.cpp ../lib/CFGforCFI.o -o asm_id -lpthread
