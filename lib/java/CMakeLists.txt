add_jar(tlspoolsocket
	src/main/java/nl/mansoft/tlspoolsocket/TlspoolSocket.java
	src/main/java/nl/mansoft/tlspoolsocket/TlspoolSSLSocketFactory.java
	src/main/java/nl/mansoft/tlspoolsocket/TestSSL.java
	GENERATE_NATIVE_HEADERS tlspoolsocket-headers)
add_library(tlspooljni SHARED
	src/main/cpp/nl_mansoft_tlspoolsocket_TlspoolSocket.cpp
	src/main/cpp/nl_mansoft_tlspoolsocket_TlspoolSocket_PlainInputStream.cpp
	src/main/cpp/nl_mansoft_tlspoolsocket_TlspoolSocket_PlainOutputStream.cpp)
target_link_libraries(tlspooljni tlspoolsocket-headers tlspool_shared )
if("${CMAKE_HOST_SYSTEM}" MATCHES ".*Windows.*")
	target_link_libraries(tlspooljni wsock32 ws2_32)
endif()
install(
    TARGETS tlspooljni
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
