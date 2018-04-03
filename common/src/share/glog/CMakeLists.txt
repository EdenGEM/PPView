set(HEADERS
    glog/logging.h
    glog/raw_logging.h 
    glog/vlog_is_on.h
    glog/log_severity.h 
    glog/stl_logging.h
    )

install(FILES ${HEADERS} DESTINATION include/glog)
install(FILES libglog.so.0.3.4 DESTINATION lib)
install(FILES libglog.so.0 DESTINATION lib)
install(FILES libglog.so DESTINATION lib)

