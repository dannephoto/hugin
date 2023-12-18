#!/usr/bin/env bash
# Configuration for 64-bit build, only possible from Leopard and up
# Copyright (c) 2008, Ippei Ukai
# Somewhere end of 2010 removed the 64bit ppc stuf as Apple doesn;t support it anymore
# April 2012: Remove support for PPC and Tiger.
# November 2012: tune for only 64-bit. This is the simple version without openmp

########### Adjust these as neccessary: #############################


# The compiler(s) to use
CC="/usr/local/Cellar/llvm/16.0.3/bin/clang"
CXX="/usr/local/Cellar/llvm/16.0.3/bin/clang++"


# The minimum macOS version required to run the compiled files
# USE AT LEAST VERSION 10.9
DEPLOY_TARGET="10.10"


# The version of your Xcode macOS-SDK.
SDKVERSION=$(xcrun --show-sdk-version)

#####################################################################
#####################################################################
#####################################################################

# number of jobs that make can use, probably same as the number of CPUs.
PROCESSNUM=$(sysctl -n hw.ncpu)

DIR="$(cd "$(dirname "$BASH_SOURCE")" && pwd)"
REPOSITORYDIR="$(realpath "$DIR/..")/repository" # "..../mac/ExternalPrograms/repository"
mkdir -p "$REPOSITORYDIR"

export \
 REPOSITORYDIR SDKVERSION DEPLOY_TARGET CC CXX \
 MACSDKDIR=$(xcrun --show-sdk-path) \
 MAKEARGS="--jobs=$PROCESSNUM" \
 OPTIMIZE="-march=core2" \
 ARGS="$OPTIMIZE -mmacosx-version-min=$DEPLOY_TARGET" \
 LDARGS="-mmacosx-version-min=$DEPLOY_TARGET" \
 PKG_CONFIG_PATH="$REPOSITORYDIR/lib/pkgconfig" \
 PATH="$REPOSITORYDIR/bin:$PATH"

fail()
{
	echo
    echo "** Failed at $1 **"
    exit 1
}
cmake() {
	command cmake "$@" \
		-DCMAKE_INSTALL_PREFIX="$REPOSITORYDIR" \
		-DCMAKE_OSX_DEPLOYMENT_TARGET="$DEPLOY_TARGET" \
		-DCMAKE_OSX_SYSROOT="$MACSDKDIR" \
		-DCMAKE_BUILD_TYPE=Release \
		-DCMAKE_INCLUDE_PATH="$REPOSITORYDIR/include" \
		-DCMAKE_LIBRARY_PATH="$REPOSITORYDIR/lib" \
		-DCMAKE_INSTALL_NAME_DIR="$REPOSITORYDIR/lib" \
		-DCMAKE_C_FLAGS="$OPTIMIZE" \
		-DCMAKE_CXX_FLAGS="$OPTIMIZE"
}
export -f fail cmake



