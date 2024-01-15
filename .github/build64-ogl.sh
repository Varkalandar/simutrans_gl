#!/bin/sh
# build open gl simutrans nightly

# libbrotli static is broken in MinGW for freetype2
#for f in libbrotlidec libbrotlienc libbrotlicommon; do
#	mv "/mingw64/lib/$f.dll.a" "/mingw64/lib/$f.dll._"
#	cp "/mingw64/lib/$f-static.a" "/mingw64/lib/$f.a"
#done

# debug support
pkg-config --libs openal
find /mingw64 -name \*.a

# normal build
echo "BACKEND = ogl" >config.default
echo "OSTYPE = mingw" >>config.default
echo "DEBUG = 0" >>config.default
echo "MSG_LEVEL = 3" >>config.default
echo "OPTIMISE = 1" >>config.default
echo "MULTI_THREAD = 1" >>config.default
echo "USE_FREETYPE = 1" >>config.default
echo "USE_ZSTD = 1" >>config.default
echo "USE_FREETYPE = 1" >>config.default
echo "WITH_REVISION = 0" >>config.default
echo "FLAGS = -DREVISION=$(svn info --show-item revision svn://servers.simutrans.org/simutrans) " >>config.default
echo "STATIC = 1" >>config.default
echo "VERBOSE = 1" >>config.default

make

cp /mingw64/bin/libopenal-1.dll .
sh tools/distribute.sh
mv simu*.zip simuwin64-ogl-nightly.zip
