cmake_minimum_required(VERSION 3.9)
project(netmd_pp VERSION 1.0.1 DESCRIPTION "NetMD C++ library")
set(CMAKE_BUILD_TYPE Release)
set (CMAKE_CXX_STANDARD 17)
set (CMAKE_VERBOSE_BUILD 1)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC OR MSYS OR MINGW)
    set(WINDOWS TRUE)
endif()

set(LIB_SRC
    netmd_utils.cpp
    CMDiscHeader.cpp
    CNetMdApi.cpp
    CNetMdSecure.cpp
    CNetMdDev.cpp
    CNetMdPatch.cpp
)

if (NOT BUILD_SHARED_LIBS)
    set(BUILD_SHARED_LIBS OFF)
endif()

if (UNIX AND NOT APPLE)
	set(LINUX TRUE)
endif()

# STATIC or SHARED is decided by option BUILD_SHARED_LIBS = ON
add_library(netmd_pp ${LIB_SRC})
target_link_libraries(netmd_pp usb-1.0 gcrypt gpg-error)

if (APPLE)
    target_include_directories(netmd_pp PRIVATE
        /usr/local/Cellar/libusb/1.0.24/include
        /usr/local/Cellar/libgcrypt/1.9.4_1/include
        /usr/local/Cellar/libgpg-error/1.43/include
        /usr/local/include
    )
    target_link_directories(netmd_pp PRIVATE
        /usr/local/Cellar/libusb/1.0.24/lib
        /usr/local/Cellar/libgcrypt/1.9.4_1/lib
        /usr/local/Cellar/libgpg-error/1.43/lib
        /usr/local/lib
    )
endif()

set(MYCFLAGS
    -W -Wall
)

target_compile_options(netmd_pp PRIVATE ${MYCFLAGS})

if (BUILD_SHARED_LIBS)
    target_compile_options(netmd_pp PRIVATE -fPIC)
endif()

target_link_options(netmd_pp PRIVATE -s)

if (WINDOWS)
    target_link_libraries(netmd_pp ws2_32)
endif()

set_target_properties(netmd_pp PROPERTIES 
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
    PUBLIC_HEADER ../include/CNetMdApi.h
    POSITION_INDEPENDENT_CODE ${BUILD_SHARED_LIBS})

target_include_directories(netmd_pp PRIVATE .)

include(GNUInstallDirs)

install(TARGETS netmd_pp
    RUNTIME DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

configure_file(libnetmd_pp.pc.in libnetmd_pp.pc @ONLY)
install(FILES ${CMAKE_BINARY_DIR}/src/libnetmd_pp.pc DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)
