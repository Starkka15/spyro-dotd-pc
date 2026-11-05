# spyro_dotd - ReXGlue Recompiled Project
#
# SDK boilerplate lives in generated/rexglue.cmake.

cmake_minimum_required(VERSION 3.25)
project(spyro_dotd LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(generated/rexglue.cmake)

# Sources
set(SPYRODOTD_SOURCES
    src/main.cpp
)

if(WIN32)
    add_executable(spyro_dotd WIN32 ${SPYRODOTD_SOURCES})
else()
    add_executable(spyro_dotd ${SPYRODOTD_SOURCES})
endif()

rexglue_setup_target(spyro_dotd GPU_PLUGINS xenos)
