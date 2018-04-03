mkdir build
rm -fr release
mkdir release
cd build
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=../release ..
make -j
make
make install
