AUX_SOURCE_DIRECTORY(. ALL_FILES)
AUX_SOURCE_DIRECTORY(./string ALL_FILES)
ADD_LIBRARY(mjextra SHARED ${ALL_FILES})
ADD_LIBRARY(mjextra_static STATIC ${ALL_FILES})

SET_TARGET_PROPERTIES(mjextra PROPERTIES VERSION 1.0 SOVERSION 1)
SET_TARGET_PROPERTIES(mjextra_static PROPERTIES OUTPUT_NAME mjextra)

install(TARGETS mjextra DESTINATION lib)
install(TARGETS mjextra_static DESTINATION lib)

set(HEADERS
    FuncSet.h
    Segmentor.h
    TimeIR.h
)

install(FILES ${HEADERS} DESTINATION include/extra)

set(HEADERS
   string/algorithm.h
   string/ascii.h
   string/byte_set.h
   string/compare.h
   string/string.h
   string/string_number.h
   string/string_piece.h
   string/StringTools.h
)

install(FILES ${HEADERS} DESTINATION include/extra/string)

set(HEADERS
    string/_inner/array_size.h
    string/_inner/scoped_ptr.h
    string/_inner/static_assert.h
    string/_inner/strict_bool.h
    string/_inner/type_traits.h
)

install(FILES ${HEADERS} DESTINATION include/extra/string/_inner)
