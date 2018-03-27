#!/bin/bash
mkdir -p build
rm -fr release
mkdir release
cd build
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=../release ..
make -j12
make install
cd ..
###
ln -sf `pwd`/release/bin/PPViewChat ./Server/
