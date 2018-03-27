#include路径
include_directories(/usr/local/mioji/include/)
include_directories(${PROJECT_SOURCE_DIR})
#链接查找路径
link_directories(/usr/local/mioji/lib)
#打包toolFunc
set(LIB_TOOLFUNC_SRC ToolFunc.cpp)
add_library(toolFunc SHARED ${LIB_TOOLFUNC_SRC})
target_link_libraries(toolFunc
					  /usr/lib64/mysql/libmysqlclient.so
					  mjcommon mj_http_shell myredis
					)
install(TARGETS toolFunc DESTINATION lib)
#打包base
set(LIB_BASE_SRC
	BasePlan.cpp Utils.cpp define.h LogDump.cpp ConstDataCheck.cpp PrivateConstData.cpp
	ReqChecker.cpp ReqParser.cpp TrafficPair.cpp KeyNodeBuilder.cpp DataList.cpp DataChecker.cpp PlaceInfoAlloc.cpp
	LYConstData.cpp RouteConfig.cpp BagParam.cpp PathView.cpp PlaceGroup.cpp
	PathTraffic.cpp TrafficData.cpp TimeIR.cpp
	PathCross.cpp PathEval.cpp PathPerfect.cpp TrafRoute.cpp PathUtil.cpp PathStat.cpp RealTimeTraffic.cpp
	)
add_library(base SHARED ${LIB_BASE_SRC})
target_link_libraries(base 
					  boost_thread
					  toolFunc vlist
		              json mjcommon 
					 )
install(TARGETS base DESTINATION lib)
