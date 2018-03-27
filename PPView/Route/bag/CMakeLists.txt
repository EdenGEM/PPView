#include路径
include_directories(/usr/local/mioji/include/)
include_directories(${PROJECT_SOURCE_DIR})
#链接查找路径
link_directories(/usr/local/mioji/lib)
#打包bag
set(LIB_BAG_SRC
	BagPlan.cpp BagSearch.cpp
	SPath.cpp SPathAlloc.h SPathHeap.cpp
	ThreadMemPool.cpp MyThreadPool.cpp
	DFSearch.cpp DFSWorker.cpp DFS.cpp
	RouteSearch.cpp RouteWorker.cpp
	MyCluster.cpp StaticRand.cpp
	)
add_library(bag SHARED ${LIB_BAG_SRC})
target_link_libraries(bag
					  pthread
					  toolFunc base
					  json mjcommon
					 )
install(TARGETS bag DESTINATION lib)
