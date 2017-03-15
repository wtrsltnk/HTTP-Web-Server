cmake_minimum_required(VERSION 3.3)

project(http2go)

set(FILES
    main.cpp
    server.cpp
    )

add_executable(http2go ${FILES})

#target_include_directories(http2go
#    PRIVATE libs/scintilla/include
#    )

target_compile_features(http2go
    PRIVATE cxx_auto_type
    PRIVATE cxx_nullptr
    PRIVATE cxx_range_for
    PRIVATE cxx_thread_local
    )

target_link_libraries(http2go
    ws2_32
    )

target_compile_definitions(http2go
    PUBLIC -DWIN32
    )
