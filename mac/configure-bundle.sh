TYPE="Release"
PREFIX="/"

REPOSITORYDIR=$(cd .. && pwd)"/hugin/mac/ExternalPrograms/repository"

MACOSX_DEPLOYMENT_TARGET=11.0 \
PKG_CONFIG_PATH=$REPOSITORYDIR/lib/pkgconfig \
CC="/opt/homebrew/Cellar/llvm/17.0.5/bin/clang" \
CXX="/opt/homebrew/Cellar/llvm/17.0.5/bin/clang++" \
cmake ../hugin -B. \
-DCMAKE_OSX_SYSROOT="$(xcrun --show-sdk-path)" \
-DCMAKE_INSTALL_PREFIX="$PREFIX" -DCMAKE_FIND_ROOT_PATH="$REPOSITORYDIR" \
-DBUILD_HSI=OFF -DENABLE_LAPACK=ON -DMAC_SELF_CONTAINED_BUNDLE=ON \
-DHUGIN_BUILDER="Author name" \
-DCMAKE_BUILD_TYPE="$TYPE" -G "Unix Makefiles" \
-DCMAKE_C_FLAGS="-march=native -Wno-implicit-function-declaration -Wno-deprecated-declarations" \
-DCMAKE_CXX_FLAGS="-march=native -Wno-implicit-function-declaration -Wno-deprecated-declarations"
