#!/usr/bin/env bash
# ------------------
# enblend 4.0
# ------------------
# $Id: enblend3.sh 1908 2007-02-05 14:59:45Z ippei $
# Copyright (c) 2007, Ippei Ukai

# -------------------------------
# 20091206.0 sg Script tested and used to build 2009.4.0-RC3
# 20091209.0 sg Script enhanced to build Enblemd-Enfuse 4.0
# 20091210.0 hvdw Removed code that downgraded optimization from -O3 to -O2
# 20091223.0 sg Added argument to configure to locate missing TTF
#               Building enblend documentation requires tex. Check if possible.
# 20100624.0 hvdw More robust error checking on compilation
# 20120430.0 hvdw Patch too old vigra in enblend for libpng 14
# 20121010.0 hvdw simplify script to make this the default non openmp one
# -------------------------------

# prevent naming conflict with clang headers
mv VERSION ENBLEND_VERSION
patch -p1 -N < ../../patches/enblend.patch

LDFLAGS="-L$REPOSITORYDIR/lib $LDARGS" \
cmake . -DENABLE_OPENMP=OFF

make clean || fail "make clean step"
make $MAKEARGS || fail "make step"
make install || fail "make install step"
