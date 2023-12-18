#!/usr/bin/env bash
# ------------------
#     gsl
# ------------------
# $Id: $
# Copyright (c) 2008, Ippei Ukai
# 2012, Harry van der Wolf

# -------------------------------
# 20120111.0 initial version of gnu science library
# Dependency for new enblend after GSOC 2011
# 20121010.0 hvdw remove ppc and ppc64 stuff
# -------------------------------


env \
 CC="$CC" CXX="$CXX" \
 CFLAGS="-isysroot $MACSDKDIR $ARGS -O3" \
 CXXFLAGS="-isysroot $MACSDKDIR $ARGS -O3" \
 CPPFLAGS="-I$REPOSITORYDIR/include" \
 LDFLAGS="-L$REPOSITORYDIR/lib $LDARGS -prebind" \
 NEXT_ROOT="$MACSDKDIR" \
 ./configure --prefix="$REPOSITORYDIR" --disable-dependency-tracking \
 --enable-shared --enable-static=no  || fail "configure step"

make clean || fail "make clean step"
make $MAKEARGS || fail "make step"
make install || fail "make install step"
