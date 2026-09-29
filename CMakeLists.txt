cmake_minimum_required(VERSION 3.21.0)

set(CMAKE_GENERATOR_PLATFORM "Win32" CACHE STRING "" FORCE)

project(DivisionHud4SA VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CONFIGURATION_TYPES "Release" CACHE STRING "" FORCE)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded")

add_library(DivisionHud4SA SHARED
    src/DivisionHud4SA.cpp
    src/core/Config.cpp
    src/hud/Panel3D.cpp
    src/hud/HudWidgets.cpp
    src/render/Gfx.cpp
    src/render/BlurBackground.cpp
    src/render/TextRenderer.cpp
)

set_target_properties(DivisionHud4SA PROPERTIES
    SUFFIX .asi
    PREFIX ""
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "$ENV{GTA_SA_DIR}\\scripts"
)

target_include_directories(DivisionHud4SA PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    $ENV{PLUGIN_SDK_DIR}\\plugin_sa
    $ENV{PLUGIN_SDK_DIR}\\plugin_sa\\game_sa
    $ENV{PLUGIN_SDK_DIR}\\plugin_sa\\game_sa\\enums
    $ENV{PLUGIN_SDK_DIR}\\plugin_sa\\game_sa\\rw
    $ENV{PLUGIN_SDK_DIR}\\shared
    $ENV{PLUGIN_SDK_DIR}\\shared\\game
    $ENV{PLUGIN_SDK_DIR}\\hooking
    $ENV{PLUGIN_SDK_DIR}\\injector
    $ENV{PLUGIN_SDK_DIR}\\safetyhook
    $ENV{PLUGIN_SDK_DIR}\\stb
)

target_compile_options(DivisionHud4SA PRIVATE /MP /permissive-)
target_compile_definitions(DivisionHud4SA PRIVATE
    GTASA
    PLUGIN_SGV_10US
    RW
    NOMINMAX
    _CRT_SECURE_NO_WARNINGS
    _CRT_NON_CONFORMING_SWPRINTFS
    _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING
    _USE_MATH_DEFINES
)

target_link_directories(DivisionHud4SA PRIVATE $ENV{PLUGIN_SDK_DIR}\\output\\lib)
target_link_libraries(DivisionHud4SA PRIVATE plugin d3d9 d3dcompiler gdi32)
target_link_options(DivisionHud4SA PRIVATE /LTCG /INCREMENTAL:NO)
