#include路径
include_directories(/usr/local/mioji/include/)
include_directories(${PROJECT_SOURCE_DIR})
#链接查找路径
link_directories(/usr/local/mioji/lib)
#打包vlist
set(LIB_VLIST_SRC
	FiltCons.h HotelRankProcessor.cpp PoiRanker.cpp RestaurantRankProcessor.cpp ShopRankProcessor.cpp ViewRankProcessor.cpp ViewShopRankProcessor.cpp
	)
add_library(vlist SHARED ${LIB_VLIST_SRC})
target_link_libraries(vlist
					  json mjcommon 
					 )
install(TARGETS vlist DESTINATION lib)
