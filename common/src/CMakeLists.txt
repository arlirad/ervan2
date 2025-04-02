add_library(common)

target_sources(common PRIVATE
    io.cpp 
    ipc.cpp 
    log.cpp 
    message.cpp 
    parse.cpp 
    string.cpp
)