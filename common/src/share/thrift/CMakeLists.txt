#当前版本是0.10.0  b2a4d4a

install(DIRECTORY thrift DESTINATION include)
install(FILES data/globals.thrift DESTINATION data)
install(FILES libthrift.so.0.10.0 DESTINATION lib)
install(FILES libthrift.so DESTINATION lib)
install(FILES libthriftz.so.0.10.0 DESTINATION lib)
install(FILES libthriftz.so DESTINATION lib)
install(FILES libthriftnb.so.0.10.0 DESTINATION lib)
install(FILES libthriftnb.so DESTINATION lib)

ADD_SUBDIRECTORY(compiler)
