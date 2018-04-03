include_directories(..)
include_directories(../share/json)
include_directories(../share/hiredis)
LINK_DIRECTORIES(${PROJECT_SOURCE_DIR}/src/share/libevent/)
LINK_DIRECTORIES(${PROJECT_SOURCE_DIR}/src/share/hiredis/)
LINK_DIRECTORIES(/usr/lib64/mysql/)

SET(SERVER_LIB_FILES
    HttpServer.cpp
    QueryProcessor.cpp
    TaskBase.cpp
    HttpWorker.cpp
    HttpDefine.cpp
    MJHotUpdate.cpp
    SocketClientEpoll.cpp
    )
ADD_LIBRARY(mj_http_shell SHARED ${SERVER_LIB_FILES})
ADD_LIBRARY(mj_http_shell_static STATIC ${SERVER_LIB_FILES})
SET_TARGET_PROPERTIES(mj_http_shell PROPERTIES VERSION 1.0 SOVERSION 1)
SET_TARGET_PROPERTIES(mj_http_shell_static PROPERTIES OUTPUT_NAME mj_http_shell)
target_link_libraries (mj_http_shell mjcommon mysqlclient)
install(TARGETS mj_http_shell DESTINATION lib)
install(TARGETS mj_http_shell_static DESTINATION lib)

set(HTTP_SHELL_HEADERS
    HttpServer.h
    QueryProcessor.h
    TaskBase.h
    HttpWorker.h
    HttpDefine.h
    MJHotUpdate.h
    MJHttpShell.h
    SocketClientEpoll.h
)
install(FILES ${HTTP_SHELL_HEADERS} DESTINATION include/http)




add_executable (mj_loader 
        mj_loader.cpp)
target_link_libraries (mj_loader mj_http_shell myredis)
install(TARGETS mj_loader DESTINATION bin)




