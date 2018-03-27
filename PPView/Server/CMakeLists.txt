#include路径
include_directories(/usr/local/mioji/include/)
include_directories(${PROJECT_SOURCE_DIR})
include_directories(/usr/include/openssl/)
#链接查找路径
link_directories(/usr/local/mioji/lib)
#添加可执行程序
set(PPVIEWCHAT_SRC 
	main.cpp MQQueryProcessor.cpp MQProducer.cpp MQConsumer.cpp
	worker.cpp MyConfig.cpp MYHotUpdate.cpp
   )
add_executable(PPViewChat ${PPVIEWCHAT_SRC})
target_link_libraries(PPViewChat 
					  pthread
	         		  toolFunc route
					  mjcommon json myrabbitmq rabbitmq myredis hiredis mj_http_shell 
					 )
install(TARGETS PPViewChat DESTINATION bin)
