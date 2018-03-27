#include路径
include_directories(/usr/local/mioji/include/)
include_directories(${PROJECT_SOURCE_DIR})
#链接查找路径
link_directories(/usr/local/mioji/lib)
#添加子目录
add_subdirectory(vlist)
add_subdirectory(base)
add_subdirectory(bag)
add_subdirectory(basis)
#打包route
set(LIB_ROUTE_SRC
	Route.cpp
	Planner.cpp
	Processor.cpp
	PostProcessor.cpp
	LightPlan.cpp
	PlanThroughtPois.cpp
	TourSet.cpp
	)
add_library(route SHARED ${LIB_ROUTE_SRC})
target_link_libraries(route
					  pthread 
					  /usr/lib64/mysql/libmysqlclient.so
					  mjcommon json 
					  toolFunc vlist base bag basis
					 )
install(TARGETS route DESTINATION lib)
#添加可执行程序test
add_executable(test testView.cpp)
target_link_libraries(test route)
install(TARGETS test DESTINATION bin)

