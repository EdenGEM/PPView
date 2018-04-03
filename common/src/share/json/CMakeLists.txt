#####json cpp######
#v1.8.1版本
include_directories(.)
set(HEADERS
    json/json.hpp
    json/autolink.h
    json/assertions.h
    json/config.h
    json/features.h
    json/forwards.h
    json/json.h
    json/reader.h
    json/value.h
    json/writer.h
    json/version.h
    )

AUX_SOURCE_DIRECTORY(. ALL_FILES)
ADD_LIBRARY(json SHARED ${ALL_FILES})

install(FILES ${HEADERS} DESTINATION include/json)
SET_TARGET_PROPERTIES(json PROPERTIES VERSION 0.10.6)
install(TARGETS json DESTINATION lib)

###### nlohmann json ########
install(FILES ${HEADERS} DESTINATION include/json)

