add_executable(ervan main.cpp monitor.cpp)
target_link_libraries(ervan common eipc smtp)