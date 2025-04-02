cmake_minimum_required(VERSION 3.10)

project(Framework VERSION 1.0)

add_subdirectory(src)
add_subdirectory(tests)

include_directories(${CMAKE_SOURCE_DIR}/include)
link_libraries(${CMAKE_SOURCE_DIR}/lib)
