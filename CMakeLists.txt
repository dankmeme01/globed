cmake_minimum_required(VERSION 3.5.0)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(GEODE_TARGET_PLATFORM Win32)
set(CMAKE_HOST_SYSTEM WIN32)

project(globed VERSION 0.1.0)

file(GLOB_RECURSE SOURCES
	src/*.cpp
)

if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
    if(MSVC)
        add_definitions(/FI"winsock2.h")
    else()
        # GCC or Clang
        add_definitions(-include winsock2.h)
    endif()
endif()

# add_compile_definitions(_HAS_ITERATOR_DEBUGGING=0)


# Set up the mod binary
add_library(${PROJECT_NAME} SHARED ${SOURCES})

if (NOT DEFINED ENV{GEODE_SDK})
    message(FATAL_ERROR "Unable to find Geode SDK! Please define GEODE_SDK environment variable to point to Geode")
else()
    message(STATUS "Found Geode: $ENV{GEODE_SDK}")
endif()

add_subdirectory($ENV{GEODE_SDK} ${CMAKE_CURRENT_BINARY_DIR}/geode)

target_include_directories(${PROJECT_NAME} PRIVATE src/)

# Set up dependencies, resources, link Geode
setup_geode_mod(${PROJECT_NAME})

# find_package(Boost REQUIRED)

# if(Boost_FOUND)
#     include_directories(${Boost_INCLUDE_DIRS})
# endif()

# link boost
# target_link_libraries(${PROJECT_NAME} ${Boost_LIBRARIES})