# Building a self contained macOS bundle for distribution

Install cmake and llvm from homebrew (OpenMP support is missing from Xcode's clang)

```sh
$ brew install llvm cmake
```

Build the external programs (see in folder ExternalProgramms) by running
download-all.sh and build-all.sh

Create and `cd` into a folder next to the "hugin" source folder (i.e. "build"):

```
someFolder
 ├── hugin (the source folder)
 └── build (cd into this folder)
```

Then run:

```sh
$ ../hugin/mac/configure-bundle.sh
$ make
```

For packaging (to create a dmg image):

```sh
$ make package
```

Styling the dmg can sometimes be a bit fiddly, retry if it doesn't work correctly.

You can add -DHUGIN_BUILDER="YOUR NAME" to the cmake call

http://wiki.panotools.org/Hugin_Compiling_OSX might also be of interest.

## Problems

You might need to adjust the macOS SDK versions in these files:

```
mac/ExternalPrograms/scripts/SetEnv.sh
mac/configure-bundle.sh
```

Make sure that CMake doesn't pick your system libraries (there might be more here, maybe check CMakeCache.txt after running CMake and ensure that no library in `/usr/local/lib/...` was selected).

## Changing the minimum macOS version

Change the version in these files

```
mac/ExternalPrograms/scripts/SetEnv.sh
mac/configure-bundle.sh
```

You might also want to change the `-march` flag to target architecture used by the oldest Mac supported by the chosen minimum version.