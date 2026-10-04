# Kameo - ReXGlue Recompiled Project
#
# SDK boilerplate lives in generated/rexglue.cmake.

cmake_minimum_required(VERSION 3.25)
project(Kameo LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(generated/rexglue.cmake)

# Sources
set(KAMEO_SOURCES
    src/main.cpp
    src/hooks.cpp
)

if(WIN32)
    add_executable(Kameo WIN32 ${KAMEO_SOURCES})
else()
    add_executable(Kameo ${KAMEO_SOURCES})
endif()

rexglue_setup_target(Kameo)
