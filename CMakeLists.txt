cmake_minimum_required(VERSION 3.5.0)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(globed VERSION 0.4.2)

file(GLOB_RECURSE SOURCES
	src/*.cpp
)

# Include winsock
if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
    if(MSVC)
        add_definitions(/FI"winsock2.h")
    else()
        # GCC or Clang
        add_definitions(-include winsock2.h)
    endif()
endif()

# Set up the mod binary
add_library(${PROJECT_NAME} SHARED ${SOURCES})

if (NOT DEFINED ENV{GEODE_SDK})
    message(FATAL_ERROR "Unable to find Geode SDK! Please define GEODE_SDK environment variable to point to Geode")
else()
    message(STATUS "Found Geode: $ENV{GEODE_SDK}")
endif()

add_subdirectory($ENV{GEODE_SDK} ${CMAKE_CURRENT_BINARY_DIR}/geode)

target_include_directories(${PROJECT_NAME} PRIVATE src/)

# link to fmod on android
if (ANDROID)
    target_link_libraries(${PROJECT_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/libfmod.so)
endif()

# Set up dependencies, resources, link Geode
setup_geode_mod(${PROJECT_NAME})
