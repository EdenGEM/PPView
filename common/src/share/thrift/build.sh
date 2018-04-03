rm -fr builder 

mkdir -p builder

cd builder

export CPLUS_INCLUDE_PATH=/usr/local/mioji/include:$CPLUS_INCLUDE_PATH
export C_INCLUDE_PATH=/usr/local/mioji/include:$C_INCLUDE_PATH

cmake -DBUILD_TESTING="OFF" -DWITH_LIBEVENT="ON" -DCMAKE_INCLUDE_PATH="/usr/local/mioji/include" -DCMAKE_LIBRARY_PATH="/usr/local/mioji/lib" -DCMAKE_BUILD_TYPE=Release -DCMAKE_VERBOSE_MAKEFILE=ON -DBUILD_SHARED_LIBS=ON -DCMAKE_CXX_FLAGS="-std=c++14" -DCMAKE_INSTALL_PREFIX=../release ..



make -j 4

make

make install
