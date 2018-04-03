
install(DIRECTORY event2 DESTINATION include)
install(FILES event.h DESTINATION include)
install(FILES evdns.h DESTINATION include)
install(FILES evhttp.h DESTINATION include)
install(FILES evrpc.h DESTINATION include)
install(FILES evutil.h DESTINATION include)

install(FILES libevent-2.0.so.5.1.9 DESTINATION lib)
install(FILES libevent-2.0.so.5 DESTINATION lib)
install(FILES libevent.so DESTINATION lib)

