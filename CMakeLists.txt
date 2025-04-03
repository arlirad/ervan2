cmake_minimum_required(VERSION 3.31.6)
project(ervan2)

enable_testing()
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_compile_options(-fno-exceptions)

include_directories(include)
include_directories(common/include)
include_directories(eaio/include)
include_directories(eipc/include)
include_directories(main/include)
include_directories(smtp/include)
add_subdirectory(common)
add_subdirectory(eaio)
add_subdirectory(eipc)
add_subdirectory(main)
add_subdirectory(smtp)

add_subdirectory(tests)

set_target_properties(ervan PROPERTIES LINKER_LANGUAGE CXX)