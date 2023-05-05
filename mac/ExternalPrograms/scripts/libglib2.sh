#!/usr/bin/env bash
# ------------------
#     libglib2
# ------------------
# $Id: libglib2.sh 1902 2008-01-02 22:27:47Z Harry $
# Copyright (c) 2007, Ippei Ukai
# script skeleton Copyright (c) 2007, Ippei Ukai
# libglib2 specifics 2012, Harry van der Wolf

# -------------------------------
# 20120411.0 HvdW Script tested
# 20121010.0 hvdw remove openmp stuff
# -------------------------------

export PATH="$PATH:$REPOSITORYDIR/bin/"
export MSGFMT="$REPOSITORYDIR/bin/msgfmt"

meson _build

env \
 CC="$CC" CXX="$CXX" \
 CFLAGS="-isysroot $MACSDKDIR $ARGS   -O3 -Wno-implicit-function-declaration -Wno-compound-token-split-by-macro -Wno-incompatible-pointer-types -Wno-unused-command-line-argument -Wno-int-conversion -Wno-declaration-after-statement" \
 CXXFLAGS="-isysroot $MACSDKDIR $ARGS -O3 -Wno-implicit-function-declaration -Wno-compound-token-split-by-macro -Wno-incompatible-pointer-types -Wno-unused-command-line-argument -Wno-int-conversion -Wno-declaration-after-statement" \
 CPPFLAGS="-I$REPOSITORYDIR/include" \
 LDFLAGS="-L$REPOSITORYDIR/lib $LDARGS" \
 ZLIB_CFLAGS="-I$MACSDKDIR/usr/include" ZLIB_LIBS="-L$MACSDKDIR/usr/lib" \
 GETTEXT_CFLAGS="-I$REPOSITORYDIR/include" GETTEXT_LIBS="-L$REPOSITORYDIR/lib" \
 LIBFFI_CFLAGS="-I$REPOSITORYDIR/include" LIBFFI_LIBS="-L$REPOSITORYDIR/lib -lffi" \
 NEXT_ROOT="$MACSDKDIR" \
 ./configure --prefix="$REPOSITORYDIR" --disable-dependency-tracking \
 --disable-selinux --disable-fam --disable-xattr \
 --disable-man --disable-dtrace --disable-systemtap \
 --enable-static=no --enable-shared --with-pcre=internal || fail "configure step";

make clean  || fail "make clean step"
make $MAKEARGS || fail "make step of $ARCH"
make install || fail "make install step of $ARCH"
