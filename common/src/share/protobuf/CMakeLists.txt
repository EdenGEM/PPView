#当前版本是Protocol Buffers v3.4.1   b04e5cb
#include_directories()
#cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_VERBOSE_MAKEFILE=ON -DBUILD_SHARED_LIBS=ON -DCMAKE_CXX_FLAGS="-std=c++14" -DCMAKE_INSTALL_PREFIX=../release ..

install(DIRECTORY google DESTINATION include)
install(FILES libprotobuf-lite.so DESTINATION lib)
install(FILES libprotobuf.so DESTINATION lib)
install(FILES libprotoc.so DESTINATION lib)

install(PROGRAMS protoc DESTINATION bin)


