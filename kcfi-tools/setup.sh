#!/bin/sh
set -e
KERNEL_PATH="../kernel-3.19-rc4-kcfi/build_tools"
CPP="g++" # make sure your gcc is >= gcc4.9"

# compiling all tools
rm -f build/* &> /dev/null
echo "Building tools. This may take a (short) while..."
${CPP} -c lib/CFGforCFI.cpp -o build/CFGforCFI.o -std=c++11
${CPP} -g cfg_fixes/fix.cpp build/CFGforCFI.o -lpthread -o build/fixalternatives
${CPP} -g map_edges/map_edges.cpp build/CFGforCFI.o -lpthread -o build/mapedges
${CPP} -g dump/dump.cpp build/CFGforCFI.o -lpthread -o build/dump
${CPP} -g merge/merge_decls.cpp build/CFGforCFI.o -lpthread -o build/merge
${CPP} -g asm_id_resolver/asm_head_resolver.cpp build/CFGforCFI.o -o build/asm_head_id -lpthread
${CPP} -g asm_id_resolver/asm_id_resolver.cpp build/CFGforCFI.o -o build/asm_id -lpthread

# copying...
echo "Copying tools into kernel tree."
cp build/fixalternatives ${KERNEL_PATH}
cp build/mapedges ${KERNEL_PATH}
cp build/dump ${KERNEL_PATH}
cp build/merge ${KERNEL_PATH}
cp build_scripts/* ${KERNEL_PATH}
cp build/asm*id ${KERNEL_PATH}
cp asm_patchers/* ${KERNEL_PATH}/asm_patchers

echo "kcfi-tools setup done :-)"
