cmake_minimum_required(VERSION 3.31.6)
project(ervan2)

enable_testing()
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(include)
include_directories(eipc/include)
add_subdirectory(common)
add_subdirectory(eipc)
add_subdirectory(main)

set_target_properties(ervan PROPERTIES LINKER_LANGUAGE CXX)