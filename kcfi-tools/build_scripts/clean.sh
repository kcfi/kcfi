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

cd ..
touch clusters.cfi
touch nodes.cfi
touch edges.cfi
touch vmlinux.cfgdump
touch vmlinux.final
rm *.cfi
rm vmlinux*
cd ./cfi_files/
touch any.cfi
rm *.cfi
cd ../
make clean
sed -i '/EXTRAVERSION =/ s/.*/EXTRAVERSION = -rc4/' Makefile
pwd
