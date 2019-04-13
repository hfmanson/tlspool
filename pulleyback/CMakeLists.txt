project(tlspool-pulleyback C)

# TODO:
#  - CFLAGS from Makefile are -ggdb3 -O0 -fPIC -std=gnu11
#  - CFLAGS on Windows    are -D_WIN32_WINNT=0x0600 -I ../include/windows
#  - On Windows, extra source and extra libraries to link to

add_definitions(-fPIC)  # Compile flag

# TODO: this should get some kind of support in SteamWorks so we can get
#       consistent install paths, and the PREFIX property is set right.

include_directories(${BDB_INCLUDE_DIR} ${Quick-DER_INCLUDE_DIRS})
set(pulleyback_tlspool_SRC
    api.c connect.c parse.c update.c
)
if("${CMAKE_HOST_SYSTEM}" MATCHES ".*Windows.*")
	set(pulleyback_tlspool_SRC
		${pulleyback_tlspool_SRC}
		windows/syslog.c
	)
endif()

add_library(pulleyback_tlspool SHARED
	${pulleyback_tlspool_SRC}
)

set_target_properties(pulleyback_tlspool PROPERTIES PREFIX "")  # no "lib"
target_link_libraries(pulleyback_tlspool ${BDB_LIBRARY} ${Quick-DER_LIBRARIES})
if("${CMAKE_HOST_SYSTEM}" MATCHES ".*Windows.*")
	target_link_libraries(pulleyback_tlspool wsock32 ws2_32)
endif()

install(
    TARGETS pulleyback_tlspool
    LIBRARY DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/steamworks/pulleyback/
    ARCHIVE DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/steamworks/pulleyback/
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
