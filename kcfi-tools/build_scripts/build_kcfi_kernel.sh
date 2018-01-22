#!/bin/bash
#
# kcfi-tools Copyright (C) 2015 Universidade Estadual de Campinas
#
# This software was developed by Joao Moreira <jmoreira@suse.de>
# at Universidade Estadual de Campinas, SP, Brazil, in 2015.
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program. If not, see <http://www.gnu.org/licenses/>.

#fix below if needed
NUMCORES="32"
CLANG="../llvm-kcfi-bin/bin/clang"
CFG_FLAGS="-j${NUMCORES} CFI_MAP_DECL=1 CFINOTAIL=1 CFI_CFGBUILD=1"
KCFI_FLAGS="-j${NUMCORES} V=1 CFINOTAIL=1 CFI_FWD=1"

echo "Cleaning up..."
./clean.sh
cd ..

echo "Building kernel's CFG representation:"
sed -i '/EXTRAVERSION =/ s/.*/EXTRAVERSION = -cfg/' Makefile
make ${CFG_FLAGS} HOSTCC=${CLANG} CC=${CLANG} &> ./build_tools/logs/create_cfg.txt
objdump -d vmlinux > vmlinux.cfgdump

#preparation for binary analysis
SIGV=$(cat vmlinux.cfgdump | grep -A1 x86_64_start_kernel\>: | tail -n1 | cut -d, -f2)
cat vmlinux.cfgdump | grep -v "callq.*mcount" &> vmlinux.nomcount
cat vmlinux.nomcount | cut -f1,3 | grep -E "nopl|callq|movl.*${SIGV}.*$|>:|^$|jmpq.*<[^\+]*>" &> vmlinux.thin
perl -000pe 's/\n\n\n/\n\n/g' vmlinux.thin &> a
mv a vmlinux.thin
objdump -d -j .data vmlinux | cut -f1,3 | grep -E "call|movl.*ffffffff.*$|>:|^$|jmpq.*<[^\+]*>" &> data.dump
grep -v '^$' data.dump &> data.thin

mv *.cfi ./cfi_files/
./build_tools/merge &> ./build_tools/logs/merge.log
cp ./cfi_files/ids.cfi ./

# Start binary analysis
echo "- Mapping CFG edges"
./build_tools/mapedges vmlinux.thin &> ./build_tools/logs/direct.log

# Apply CFG fixes
echo "- Fixing CFG"
./build_tools/fixalternatives &> ./build_tools/logs/fixalternatives.log
./build_tools/dump &> cfi.dump

# Patch asm files
echo "- Patching ASM files"
./build_tools/apply_asm_patches.sh &> ./build_tools/logs/asm_patches.log

# Rebuild with CFG information
echo "Recompiling kernel with kCFI:"
make clean
sed -i '/EXTRAVERSION =/ s/.*/EXTRAVERSION = -kcfi/' Makefile
make ${KCFI_FLAGS} HOSTCC=${CLANG} CC=${CLANG} &> ./build_tools/logs/use_cfg.txt
objdump -d vmlinux > vmlinux.final
