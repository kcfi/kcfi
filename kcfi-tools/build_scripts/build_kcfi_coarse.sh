#!/bin/bash
#
# kcfi-tools Copyright (C) 2015 Universidade Estadual de Campinas
#
# This software was developed by Joao Moreira <joao@overdrivepizza.com>
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
KCFI_FLAGS="-j${NUMCORES} V=1 CFINOTAIL=1 CFI_FWD=1 CFI_COARSE=1"

echo "Cleaning up..."
./clean.sh
cd ..

# Patch asm files
echo "- Patching ASM files"
lua ./build_tools/asm_patchers/kcfi_tags_coarse.lua

# Rebuild with CFG information
echo "Recompiling kernel with kCFI:"
make clean
sed -i '/EXTRAVERSION =/ s/.*/EXTRAVERSION = -kcfi-coarse/' Makefile
make ${KCFI_FLAGS} HOSTCC=${CLANG} CC=${CLANG} &> ./build_tools/logs/use_cfg.txt
objdump -d vmlinux > vmlinux.final
