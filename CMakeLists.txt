cmake_minimum_required(VERSION 3.3)

project(http2go)

find_package(OPENGL REQUIRED)

set(FILES
    filesystemrequesthandler.cpp
    filesystemrequesthandler.h
    httpserver.cpp
    httpserver.h
    httprequest.cpp
    httprequest.h
    httpresponse.cpp
    httpresponse.h
    httputility.h
    main.cpp
    mimetypes.cpp
    mimetypes.h
    stringrequesthandler.cpp
    stringrequesthandler.h
    libs/imgui/imgui.cpp
    libs/imgui/imgui.h
    libs/imgui/imgui_draw.cpp
    libs/imgui/imgui_impl_win32gl2.cpp
    libs/imgui/imgui_impl_win32gl2.h
    libs/imgui/imgui_applog.h
    libs/system.io/include/system.io.directoryinfo.h
    libs/system.io/include/system.io.fileinfo.h
    libs/system.io/include/system.io.filesysteminfo.h
    libs/system.io/include/system.io.path.h
    libs/system.io/src/system.io.cpp
    )

add_executable(http2go WIN32 ${FILES})

target_include_directories(http2go
    PRIVATE libs/system.io/include
    PRIVATE libs/imgui
    )

target_compile_features(http2go
    PRIVATE cxx_auto_type
    PRIVATE cxx_nullptr
    PRIVATE cxx_range_for
    PRIVATE cxx_thread_local
    )

target_link_libraries(http2go
    ws2_32 ${OPENGL_LIBRARIES}
    )

target_compile_definitions(http2go
    PUBLIC -DWIN32
    )
