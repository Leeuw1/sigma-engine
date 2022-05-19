cmake_minimum_required(VERSION 3.22)
set(CMAKE_CXX_STANDARD 20)

project(demo)

add_executable(demo ${CMAKE_SOURCE_DIR}/demo/src/main.cpp)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
add_subdirectory(engine)

target_include_directories(demo PRIVATE ${CMAKE_SOURCE_DIR}/engine/src)

target_link_libraries(demo sigma-engine)