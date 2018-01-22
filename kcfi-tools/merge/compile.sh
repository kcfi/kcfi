#!/bin/sh
cd ../lib/
g++ -c CFGforCFI.cpp -std=c++11
cd ../merge/
g++ merge_decls.cpp ../lib/CFGforCFI.o -lpthread -o merge
