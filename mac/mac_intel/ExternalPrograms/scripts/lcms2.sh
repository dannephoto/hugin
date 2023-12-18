#!/usr/bin/env bash
# ------------------
#     liblcms2
# ------------------
# $Id: liblcms.sh 1902 2008-01-02 22:27:47Z Harry $
# Copyright (c) 2007, Ippei Ukai
# script skeleton Copyright (c) 2007, Ippei Ukai
# lcms2 specifics 2012, Harry van der Wolf

# -------------------------------
# 20120111.0 hvdw initial lcms2 (lcms version 2) script
# 20121010.0 hvdw update to 2.4
# -------------------------------


env \
 CC="$CC" CXX="$CXX" \
 CFLAGS="-isysroot $MACSDKDIR $ARGS -O3" \
 CXXFLAGS="-isysroot $MACSDKDIR $ARGS -O3" \
 CPPFLAGS="-I$REPOSITORYDIR/include" \
 LDFLAGS="-L$REPOSITORYDIR/lib $LDARGS" \
 NEXT_ROOT="$MACSDKDIR" \
 ./configure --prefix="$REPOSITORYDIR" --disable-dependency-tracking \
 --enable-static=no --enable-shared --with-zlib="$MACSDKDIR/usr/lib" || fail "configure step";

make clean || fail "make clean step"
make $MAKEARGS || fail "failed at make step"
make install || fail "make install step"
