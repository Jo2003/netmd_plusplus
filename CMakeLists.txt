cmake_minimum_required(VERSION 3.9)
project("netmd++" VERSION 1.0.2 DESCRIPTION "NetMD C++ library")

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_FLAGS "-W -Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-DNDEBUG")

add_subdirectory(src)
add_subdirectory(test)