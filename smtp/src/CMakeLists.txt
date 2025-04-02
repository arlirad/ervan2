add_library(smtp session.cpp main.cpp)

target_sources(smtp PRIVATE
    session/commands.cpp
    session/file.cpp
)

target_link_libraries(smtp common eaio eipc)