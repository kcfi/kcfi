#!/bin/sh
cd ../lib/
g++ -c CFGforCFI.cpp -std=c++11
cd ../cfg_fixes/
g++ -g fix.cpp ../lib/CFGforCFI.o -lpthread -o fix
