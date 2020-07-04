cmake_minimum_required(VERSION 3.2)
project(ziper)

add_subdirectory(division)
add_subdirectory(zip)
set(SOURCE_FILES main.cpp)

add_executable(ziper ${SOURCE_FILES})
target_link_libraries(ziper division zip)
install(TARGETS ziper DESTINATION ${DIVISIBLE_INSTALL_BIN_DIR})
