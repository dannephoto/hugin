#!/usr/bin/env bash

cmake -DDEPENDENCY_SEARCH_PREFIX="$REPOSITORYDIR" \
  -DWITH_OPENEXR=1 -DWITH_HDF5=0 -DWITH_LEMON=0 -DWITH_BOOST_GRAPH=0 -DWITH_VIGRANUMPY=0 \
  -DFFTW3F_LIBRARY="$REPOSITORYDIR/lib" -DFFTW3F_INCLUDE_DIR="$REPOSITORYDIR/include" . || fail "configure step"

make clean || fail "make clean step"
make $MAKEARGS || fail "make step"
make install || fail "make install step"
