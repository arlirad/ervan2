add_library(smtp session.cpp main.cpp)
target_link_libraries(smtp common eaio eipc)