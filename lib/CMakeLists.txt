set(libtlspool_SRC
	libtlspool.c
	libtlspool_lidentry.c
	libtlspool_pinentry.c
	libtlspool_configvar.c
)
if("${CMAKE_HOST_SYSTEM}" MATCHES ".*Windows.*")
	set(libtlspool_SRC
		${libtlspool_SRC}
		windows/syslog.c
		windows/socketpair.c
	)
endif()
set(libpavlov_SRC
    libpavlov.c
)

# Include the error table from ../src
use_com_err_table (errortable)

# Compile the files only once
add_library(_libtlspool OBJECT ${libtlspool_SRC})
add_library(_libpavlov OBJECT ${libpavlov_SRC})

# Make libraries out of the resulting objects; these
# need to have unique names for CMake, and use OUTPUT_NAME
# so that they are called libtlspool.{so,a}.
add_library(tlspool_shared SHARED $<TARGET_OBJECTS:_libtlspool>)
set_target_properties(tlspool_shared PROPERTIES
    OUTPUT_NAME tlspool
)
add_library(tlspool_static STATIC $<TARGET_OBJECTS:_libtlspool>)
set_target_properties(tlspool_static PROPERTIES
    OUTPUT_NAME tlspool
)

if("${CMAKE_HOST_SYSTEM}" MATCHES ".*Windows.*")
	target_link_libraries(tlspool_shared -lwsock32 -lws2_32)
endif()

# The pavlov library
add_library(pavlov_shared SHARED $<TARGET_OBJECTS:_libpavlov>)
set_target_properties(pavlov_shared PROPERTIES
    OUTPUT_NAME pavlov
)
add_library(pavlov_static STATIC $<TARGET_OBJECTS:_libpavlov>)
set_target_properties(pavlov_static PROPERTIES
    OUTPUT_NAME pavlov
)
if("${CMAKE_HOST_SYSTEM}" MATCHES ".*Windows.*")
	target_link_libraries(pavlov_shared wsock32 ws2_32 gnurx)
endif()

install(
    TARGETS tlspool_shared pavlov_shared
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
install(
    TARGETS tlspool_static pavlov_static
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

option(LANGUAGE_SUPPORT_PYTHON
	"Add language support for Python"
	OFF)

if(LANGUAGE_SUPPORT_PYTHON)
    add_subdirectory(python)
endif()
