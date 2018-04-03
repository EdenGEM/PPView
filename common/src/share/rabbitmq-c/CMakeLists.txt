set(HEADERS
    amqp_framing.h
    amqp.h
    amqp_ssl_socket.h
    amqp_tcp_socket.h
    )

install(FILES ${HEADERS} DESTINATION include)
install(FILES librabbitmq.so.4.2.0 DESTINATION lib)
install(FILES librabbitmq.so.4 DESTINATION lib)
install(FILES librabbitmq.so DESTINATION lib)

