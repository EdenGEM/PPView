include_directories(./share/rabbitmq-c)
#include_directories(./share/thrift)
#include_directories(./share/libevent)
include_directories(./share/boost)
#include_directories(./share/glog)
include_directories(./share/json)
include_directories(./share/hiredis)
include_directories(./)
#include_directories(./share/ssdb)
include_directories(/usr/include/openssl)
#LINK_DIRECTORIES(/usr/lib64/mysql/)
#LINK_DIRECTORIES(${PROJECT_SOURCE_DIR}/src/share/libevent/)
LINK_DIRECTORIES(${PROJECT_SOURCE_DIR}/src/share/mysql/)
LINK_DIRECTORIES(${PROJECT_SOURCE_DIR}/src/share/rabbitmq-c/)
#LINK_DIRECTORIES(${PROJECT_SOURCE_DIR}/src/share/ssdb/)
LINK_DIRECTORIES(${PROJECT_SOURCE_DIR}/src/share/hiredis/)


set(COMMON_FILES
    AuxTools.cpp
    ConfigParser.cpp
    Configuration.cpp
    SocketClient.cpp
    #SocketClientAsyn.cpp
    SocketUtil.cpp
    MJLog.cpp
    MJDataProc.cpp
	MJSquareGrid.cpp
    threads/MyThreadPool.cpp
    )
ADD_LIBRARY(mjcommon SHARED ${COMMON_FILES})
ADD_LIBRARY(mjcommon_static STATIC ${COMMON_FILES})
SET_TARGET_PROPERTIES(mjcommon PROPERTIES VERSION ${MY_PROJECT_VERSION})
SET_TARGET_PROPERTIES(mjcommon_static PROPERTIES OUTPUT_NAME mjcommon)
target_link_libraries (mjcommon ssl myredis json)
install(TARGETS mjcommon DESTINATION lib)
install(TARGETS mjcommon_static DESTINATION lib)

add_executable(mjSquareGrid MJSquareGrid.cpp)
install(TARGETS mjSquareGrid DESTINATION bin)

set(HEADERS
    AuxTools.h
    ConfigParser.h
    Configuration.h
    MyConnectPool.h
    MyRedis.h
    MyPika.h
    Mysql.h
    MJDataProc.h
    MyRabbitMQ.h
    MJLog.h
    MyTime.h
    MyTimer.h
    ServiceLog.h
    SocketClient.h
    SocketUtil.h
    MJSquareGrid.h
    MJCommon.h
)
install(FILES ${HEADERS} DESTINATION include)


set(MQ MyRabbitMQ.cpp)
ADD_LIBRARY(myrabbitmq SHARED ${MQ})
ADD_LIBRARY(myrabbitmq_static STATIC ${MQ})
SET_TARGET_PROPERTIES(myrabbitmq PROPERTIES VERSION ${MY_PROJECT_VERSION})
SET_TARGET_PROPERTIES(myrabbitmq_static PROPERTIES OUTPUT_NAME myrabbitmq)
target_link_libraries (myrabbitmq rabbitmq mjcommon)
install(TARGETS myrabbitmq DESTINATION lib)
install(TARGETS myrabbitmq_static DESTINATION lib)

set(MYREDIS MyRedis.cpp)
ADD_LIBRARY(myredis SHARED ${MYREDIS})
ADD_LIBRARY(myredis_static STATIC ${MYREDIS})
SET_TARGET_PROPERTIES(myredis PROPERTIES VERSION ${MY_PROJECT_VERSION})
SET_TARGET_PROPERTIES(myredis_static PROPERTIES OUTPUT_NAME myredis)
target_link_libraries (myredis hiredis)
install(TARGETS myredis DESTINATION lib)
install(TARGETS myredis_static DESTINATION lib)

set(MYPIKA MyPika.cpp)
ADD_LIBRARY(mypika SHARED ${MYPIKA})
ADD_LIBRARY(mypika_static STATIC ${MYPIKA})
SET_TARGET_PROPERTIES(mypika PROPERTIES VERSION ${MY_PROJECT_VERSION})
SET_TARGET_PROPERTIES(mypika_static PROPERTIES OUTPUT_NAME mypika)
target_link_libraries (mypika myredis hiredis)
install(TARGETS mypika DESTINATION lib)
install(TARGETS mypika_static DESTINATION lib)

set(MYSQL Mysql.cpp)
ADD_LIBRARY(mjmysql SHARED ${MYSQL})
ADD_LIBRARY(mjmysql_static STATIC ${MYSQL})
SET_TARGET_PROPERTIES(mjmysql PROPERTIES VERSION ${MY_PROJECT_VERSION})
SET_TARGET_PROPERTIES(mjmysql_static PROPERTIES OUTPUT_NAME mjmysql)
target_link_libraries (mjmysql mysqlclient)
install(TARGETS mjmysql DESTINATION lib)
install(TARGETS mjmysql_static DESTINATION lib)

#set(MYSSDB MySsdb.cpp)
#ADD_LIBRARY(myssdb SHARED ${MYSSDB})
#ADD_LIBRARY(myssdb_static STATIC ${MYSSDB})
#SET_TARGET_PROPERTIES(myssdb PROPERTIES VERSION ${MY_PROJECT_VERSION})
#SET_TARGET_PROPERTIES(myssdb_static PROPERTIES OUTPUT_NAME myssdb)
#install(TARGETS myssdb DESTINATION lib)
#install(TARGETS myssdb_static DESTINATION lib)




