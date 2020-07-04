cmake_minimum_required(VERSION 3.2)
project(zip C CXX)

set(SOURCE_FILES
    miniz.h
    zip.h
    zip.c
)

add_library(zip SHARED STATIC ${SOURCE_FILES})

install(TARGETS zip DESTINATION ${DIVISIBLE_INSTALL_LIB_DIR})
install(FILES zip.h DESTINATION ${DIVISIBLE_INSTALL_INCLUDE_DIR})
install(FILES miniz.h DESTINATION ${DIVISIBLE_INSTALL_INCLUDE_DIR})
