#!/usr/bin/env bash

pre="<<<<<<<<<<<<<<<<<<<<"
pst=">>>>>>>>>>>>>>>>>>>>"

DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$DIR" || exit 1
source SetEnv.sh

download(){
    name=$1
    url=$2
    out=$3
    echo "$pre Downloading $name $pst"
    if [[ -n $out ]]; then
        curl -L -o "$out" "$url"
    else
        curl -L -O "$url"
    fi
    if [[ $? != 0 ]]; then
        echo "++++++ Download of $name failed"
        exit 1
    fi
}


cd "$REPOSITORYDIR"

rm -rf _build
mkdir -p _src && cd _src

download "boost"     "https://boostorg.jfrog.io/artifactory/main/release/1.75.0/source/boost_1_75_0.tar.bz2"
download "exiv2"     "https://github.com/Exiv2/exiv2/releases/download/v0.27.6/exiv2-0.27.6-Source.tar.gz"
download "fftw"      "http://www.fftw.org/fftw-3.3.8.tar.gz"
download "gettext"   "https://ftp.gnu.org/pub/gnu/gettext/gettext-0.21.1.tar.gz"
download "glew"      "https://sourceforge.net/projects/glew/files/glew/2.1.0/glew-2.1.0.zip/download"                       "glew-2.1.0.tgz"
download "glib"      "https://download.gnome.org/sources/glib/2.76/glib-2.76.1.tar.xz"
download "gsl"       "http://ftpmirror.gnu.org/gsl/gsl-2.6.tar.gz"
download "openexr"   "https://github.com/AcademySoftwareFoundation/openexr/archive/refs/tags/v2.4.1.tar.gz"                           "openexr-v2.5.8.tar.gz"
download "exiftool"  "https://exiftool.org/Image-ExifTool-12.50.tar.gz"
download "jpeg"      "http://www.ijg.org/files/jpegsrc.v9e.tar.gz"
download "lcms2"     "https://sourceforge.net/projects/lcms/files/lcms/2.9/lcms2-2.9.tar.gz/download"                       "lcms2-2.9.tar.gz"
download "libffi"    "ftp://sourceware.org/pub/libffi/libffi-3.4.3.tar.gz"
download "libomp"    "https://github.com/llvm/llvm-project/releases/download/llvmorg-13.0.1/openmp-13.0.1.src.tar.xz"
download "libpano13" "https://sourceforge.net/projects/panotools/files/libpano13/libpano13-2.9.19/libpano13-2.9.19.tar.gz/download" "libpano13-2.9.19.tar.gz"
download "libpng"    "https://download.sourceforge.net/libpng/libpng-1.6.39.tar.gz"                                          "libpng-1.6.39.tar.gz"
download "tiff"      "http://download.osgeo.org/libtiff/tiff-4.1.0.tar.gz"
download "vigra"     "https://github.com/ukoethe/vigra/releases/download/Version-1-11-1/vigra-1.11.1-src.tar.gz"
download "wxWidgets" "https://github.com/wxWidgets/wxWidgets/releases/download/v3.2.2.1/wxWidgets-3.2.2.1.tar.bz2"

cd ..

echo "$pre Downloading enblend-enfuse $pst"
hg clone http://hg.code.sf.net/p/enblend/code enblend-enfuse -r stable-4_2

for f in _src/*; do
    echo "Extracting $f"
    tar xf "$f";
done