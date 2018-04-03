cmake_minimum_required(VERSION 2.8)
project(mioji C CXX)
SET(CMAKE_VERBOSE_MAKEFILE ON)

SET(CMAKE_CXX_COMPILER "g++")

SET(CMAKE_CXX_FLAGS "-D_FILE_OFFSET_BITS=64 -Wall -lpthread -std=c++14")

set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-g -O2 -DNDEBUG")

if (NOT CMAKE_BUILD_TYPE)
    message(STATUS "No build type selected, default to Release")
    set(CMAKE_BUILD_TYPE "Release")
endif()

MESSAGE(STATUS "CMAKE_BUILD_TYPE = " ${CMAKE_BUILD_TYPE})

set(MY_PROJECT_VERSION 2.0.0)



MESSAGE(STATUS "CMAKE_CXX_FLAGS = " ${CMAKE_CXX_FLAGS})


#ADD_SUBDIRECTORY(src/share/glog)
ADD_SUBDIRECTORY(src/share/json)
#ADD_SUBDIRECTORY(src/share/ssdb)
ADD_SUBDIRECTORY(src/share/rabbitmq-c)
ADD_SUBDIRECTORY(src/share/hiredis)
ADD_SUBDIRECTORY(src/share/thrift)
ADD_SUBDIRECTORY(src/share/libevent)
ADD_SUBDIRECTORY(src/share/protobuf)
#ADD_SUBDIRECTORY(src/share/tcmalloc)
ADD_SUBDIRECTORY(src/share/boost)
ADD_SUBDIRECTORY(src/share/mysql)
ADD_SUBDIRECTORY(src/share/loader)
#ADD_SUBDIRECTORY(src/share/rapidjson)

ADD_SUBDIRECTORY(src)
ADD_SUBDIRECTORY(src/http)
ADD_SUBDIRECTORY(src/threads)
