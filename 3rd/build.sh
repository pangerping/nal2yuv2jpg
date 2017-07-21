#!/bin/bash
basepath=$(cd `dirname $0`; pwd)

cd ${basepath}/x264
chmod +x configure
chmod +x *.sh
make clean
./configure --disable-opencl --disable-lavf --enable-static --disable-asm --prefix="./../install"
make -j4 && make install

cd ${basepath}/ffmpeg
chmod +x configure
chmod +x *.sh
make clean
make distclean
#--extra-cflags="-fvisibility=hidden" 
CFLAGS="-I./../install/include" LDFLAGS="-L./../install/lib" ./configure  --prefix="./../install" --enable-gpl  --disable-shared --enable-static --enable-libx264 --enable-nonfree --enable-pic  --enable-decoder=h264 --enable-swscale  --enable-swresample --disable-optimizations --disable-stripping  --enable-debug
make -j4 install

cd ${basepath}/gflags/src
chmod +x configure
./configure --disable-shared --prefix=/home/pang/git/gitlib/test/demo/transtools/3rd/install
make install
