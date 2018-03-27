#include路径
include_directories(/usr/local/mioji/include/)
include_directories(${PROJECT_SOURCE_DIR})
#链接查找路径
link_directories(/usr/local/mioji/lib)

#打包basis
set(LIB_BASIS_SRC
	NodeJ.cpp
	PathGenerator.cpp
	DaysPlan.cpp
    ../PostProcessor.cpp
	)
add_library(basis SHARED ${LIB_BASIS_SRC})
target_link_libraries(basis
					  toolFunc base
		              json mjcommon
					 )
add_executable(Ptest PathGeneratorTest.cpp)
target_link_libraries(Ptest 
    basis json)
install(TARGETS basis DESTINATION lib)
install(TARGETS Ptest DESTINATION bin)
