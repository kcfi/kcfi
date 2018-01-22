#!/bin/sh
cd ../lib/
g++ -c CFGforCFI.cpp -std=c++11
cd ../dump/
g++ dump.cpp ../lib/CFGforCFI.o -lpthread -o dump
g++ dumpcsv.cpp ../lib/CFGforCFI.o -lpthread -o dumpcsv
g++ dumpd.cpp ../lib/CFGforCFI.o -lpthread -o dumpd
