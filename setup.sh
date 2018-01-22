#!/bin/sh
set -e
BUILDDIR="llvm-kcfi-build" #change this as desired
BINDIR="llvm-kcfi-bin"
KCFIDIR=$(pwd)

# Force everyting on /tmp to prevent accidents
# change below as desired (also update kernel build_tools/*.sh)
BUILDDIR="./${BUILDDIR}"
BINDIR="./${BINDIR}"

# verify required tools
# Lua, cmake, ninja(-build)

str=""
command -v lua >/dev/null ||{ str="lua5.2 "; }
command -v cmake >/dev/null ||{ str="$str""cmake "; }
command -v ninja >/dev/null || command -v ninja-build >/dev/null ||
	{ str="$str""ninja(-build)"; }

if [ ! "$str" = "" ]
then
	echo "ERROR: Please install $str"
	exit 1
fi

if [ -d "${BUILDDIR}" ]
then
  rm -rf ${BUILDDIR}
fi

if [ -d "/${BINDIR}" ]
then
  rm -rf ${BINDIR}
fi

mkdir ${BUILDDIR}
mkdir ${BINDIR}
REALBINDIR=$(realpath ${BINDIR})

#compile LLVM
ninja --version >/dev/null
if [ $? -eq 0 ]
then
  cd ${BUILDDIR}
  cmake ${KCFIDIR}/llvm-kcfi/ -G Ninja -DCMAKE_INSTALL_PREFIX="${REALBINDIR}" -DCMAKE_BUILD_TYPE="Release" -DLLVM_TARGETS_TO_BUILD="X86"
  ninja install
fi

#compile and install kCFI Tools
cd ${KCFIDIR}/kcfi-tools
./setup.sh
