set (CMAKE_CXX_STANDARD 17)
set (CMAKE_VERBOSE_BUILD 1)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC OR MSYS OR MINGW)
    set(WINDOWS TRUE)
endif()

set(PATCH CNetMdPatch.cpp)
set(SRC 
    netmd_utils.cpp
    CMDiscHeader.cpp
    CNetMdApi.cpp
    CNetMdSecure.cpp
    CNetMdDev.cpp
    CNetMdTOC.cpp
)

set(LIB_SRC ${PATCH} ${SRC})

if (NOT BUILD_SHARED_LIBS)
    set(BUILD_SHARED_LIBS OFF)
endif()

if (UNIX AND NOT APPLE)
	set(LINUX TRUE)
endif()

# STATIC or SHARED is decided by option BUILD_SHARED_LIBS = ON
add_library("netmd++" ${LIB_SRC})
target_link_libraries("netmd++" usb-1.0 gcrypt gpg-error)

if (APPLE)
    target_include_directories("netmd++" PRIVATE /usr/local/include)
    target_link_directories("netmd++" PRIVATE /usr/local/lib)
endif()

if (CMAKE_BUILD_TYPE MATCHES Release)
    if (APPLE)
        set_source_files_properties(${PATCH} PROPERTIES COMPILE_FLAGS -O1)
        set_source_files_properties(${SRC} PROPERTIES COMPILE_FLAGS -O2)
	endif()
endif()

if (BUILD_SHARED_LIBS)
    target_compile_options("netmd++" PRIVATE -fPIC)
endif()

target_link_options("netmd++" PRIVATE 
    $<$<CONFIG:Release>:-s>
)

if (WINDOWS)
    target_link_libraries("netmd++" ws2_32)
endif()

set_target_properties("netmd++" PROPERTIES 
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
    PUBLIC_HEADER ../include/netmd++.h
    POSITION_INDEPENDENT_CODE ${BUILD_SHARED_LIBS})

target_include_directories("netmd++" PRIVATE .)

include(GNUInstallDirs)

install(TARGETS "netmd++"
    RUNTIME DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

configure_file(libnetmd++.pc.in libnetmd++.pc @ONLY)
install(FILES ${CMAKE_BINARY_DIR}/src/libnetmd++.pc DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)
