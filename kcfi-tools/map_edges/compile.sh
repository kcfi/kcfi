#!/bin/sh
cd ../lib/
g++ -c CFGforCFI.cpp -std=c++11
cd ../map_edges/
g++ -g map_edges.cpp ../lib/CFGforCFI.o -lpthread -o mapedges
