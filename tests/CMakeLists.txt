find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})

add_executable(ervantests test.cpp)
target_link_libraries(ervantests eipc common ${GTEST_BOTH_LIBRARIES} pthread)
gtest_discover_tests(ervantests test)