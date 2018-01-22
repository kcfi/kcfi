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

#fix clang path below
CLANG="../llvm-kcfi-bin/bin/clang"
NUMCORES="32"
CFLAGS="-j${NUMCORES} CFI_DISABLE=1 CFINOTAIL=1 V=1"
./clean.sh
cd ..
sed -i '/EXTRAVERSION =/ s/.*/EXTRAVERSION = -vanilla/' Makefile
make ${CFLAGS} HOSTCC=${CLANG} CC=${CLANG} &> ./build_tools/logs/vanilla.txt
objdump -d vmlinux > vmlinux.vanilla
