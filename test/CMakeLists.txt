set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_executable( testnetmd test.cpp )

target_include_directories(testnetmd PRIVATE ${netmd++_SOURCE_DIR}/include/)
target_link_libraries(testnetmd usb-1.0 gcrypt gpg-error netmd++)

if (APPLE)
    target_include_directories(testnetmd PRIVATE /usr/local/include)
    target_link_directories(testnetmd PRIVATE /usr/local/lib)
endif()
