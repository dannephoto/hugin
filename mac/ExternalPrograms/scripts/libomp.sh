#!/usr/bin/env bash


mkdir -p "$REPOSITORYDIR/bin";
mkdir -p "$REPOSITORYDIR/lib";
mkdir -p "$REPOSITORYDIR/include";

cmake -DLIBOMP_ARCH="32e" . || fail "configure step for $ARCH";

make $MAKEARGS || fail "failed at make step of $ARCH";
make install || fail "make install step of $ARCH";

#install_name_tool -id "$REPOSITORYDIR/lib/libiomp5.dylib" "$REPOSITORYDIR/lib/libiomp5.dylib"
install_name_tool -id "$REPOSITORYDIR/lib/libomp.dylib" "$REPOSITORYDIR/lib/libomp.dylib"
#ln -sf "libiomp5.dylib" "$REPOSITORYDIR/lib/libomp.dylib"
