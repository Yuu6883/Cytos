cmake_minimum_required(VERSION 3.10.0 FATAL_ERROR)

project (cytos-addon CXX)

set(CMAKE_CXX_STANDARD 20)
message(VERBOSE CXX)

set(ADDON_FILES
    "source-cpp/misc/pool.cpp"
    "source-cpp/game/control.cpp"
    "source-cpp/game/handle.cpp"
    "source-cpp/game/bot.cpp"
    "source-cpp/addon/server.cc"
    "source-cpp/addon/player.cc"
)

set(GFX_FILES
    "source-cpp/addon/graphics.cpp"
)

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

message(INFO "cmake-js include dir: ${CMAKE_JS_INC}")

include_directories(${CMAKE_JS_INC})

add_library(${PROJECT_NAME} SHARED ${ADDON_FILES} ${CMAKE_JS_SRC})
set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "" SUFFIX ".node")
target_link_libraries(${PROJECT_NAME} ${CMAKE_JS_LIB})

add_library(gfx-addon SHARED ${GFX_FILES} ${CMAKE_JS_SRC})
set_target_properties(gfx-addon PROPERTIES PREFIX "" SUFFIX ".node")
target_link_libraries(gfx-addon ${CMAKE_JS_LIB})