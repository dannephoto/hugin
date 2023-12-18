#!/usr/bin/env bash
# ------------------
#     libjpeg
# ------------------
# $Id: libjpeg-8.sh 1902 2007-02-04 22:27:47Z ippei $
# Copyright (c) 2007, Ippei Ukai

# -------------------------------
# 20091206.0 sg Script tested and used to build 2009.4.0-RC3
# 20100121.0 sg Script updated for version 8
# 20100624.0 hvdw More robust error checking on compilation
# 20120427.0 hvdw compile x86_64 with gcc 4.6 for Lion and up openmp compatibility
# -------------------------------

env \
 CC="$CC" CXX="$CXX" \
 CFLAGS="-isysroot   $MACSDKDIR $ARGS -O3" \
 CXXFLAGS="-isysroot $MACSDKDIR $ARGS -O3" \
 CPPFLAGS="-I$REPOSITORYDIR/include" \
 LDFLAGS="-L$REPOSITORYDIR/lib $LDARGS" \
 NEXT_ROOT="$MACSDKDIR" \
 ./configure --prefix="$REPOSITORYDIR" --disable-dependency-tracking \
   --enable-shared --enable-static=no || fail "configure step for $ARCH";

make clean || fail "make clean step"
make $MAKEARGS || fail "make step of $ARCH";
make install || fail "make install step of $ARCH";
