PKG_CHECK_MODULES(GOBJECT REQUIRED "gobject-2.0")
PKG_CHECK_MODULES(CURL REQUIRED "libcurl")
PKG_CHECK_MODULES(LIBXML REQUIRED "libxml-2.0")

set(LIB_SOURCES
	xmrservice.c
	songinfo.c
	radioinfo.c
)

set(LIB_HEADERS
	xmrservice.h
	songinfo.h
	radioinfo.h
)

set(LIB_VERSION 0.2)

include_directories(${GOBJECT_INCLUDE_DIRS})
include_directories(${CURL_INCLUDE_DIRS})
include_directories(${LIBXML_INCLUDE_DIRS})
include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_library(xmrservice SHARED ${LIB_SOURCES})
set_target_properties(xmrservice
	PROPERTIES VERSION ${LIB_VERSION}
	SOVERSION 0
)

target_link_libraries(xmrservice ${GOBJECT_LIBRARIES}
	${CURL_LIBRARIES}
	${LIBXML_LIBRARIES}
)

configure_file(libxmrservice.pc.in ${CMAKE_CURRENT_BINARY_DIR}/libxmrservice.pc)

install(TARGETS xmrservice LIBRARY DESTINATION ${libdir})
install(FILES ${LIB_HEADERS} DESTINATION "${includedir}/xmrservice")
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/libxmrservice.pc DESTINATION "${libdir}/pkgconfig")
