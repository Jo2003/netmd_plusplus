add_executable( testnetmd test.cpp )

target_include_directories(testnetmd PRIVATE ${netmd++_SOURCE_DIR}/include/)

target_link_libraries(testnetmd usb-1.0 gcrypt gpg-error netmd++)
if (APPLE)
    target_include_directories(testnetmd PRIVATE
        /usr/local/Cellar/libusb/1.0.24/include
        /usr/local/Cellar/libgcrypt/1.9.4_1/include
        /usr/local/Cellar/libgpg-error/1.43/include
        /usr/local/include
    )
    target_link_directories(testnetmd PRIVATE
        /usr/local/Cellar/libusb/1.0.24/lib
        /usr/local/Cellar/libgcrypt/1.9.4_1/lib
        /usr/local/Cellar/libgpg-error/1.43/lib
        /usr/local/lib
    )
endif()
