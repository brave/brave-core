cmake_minimum_required(VERSION 3.15)

project(botan)

# Set the minimum macOS deployment target if building on macOS
if(APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0")
endif()

# Set the compiler to Clang if building on macOS
if(APPLE)
    set(CMAKE_C_COMPILER "clang")
    set(CMAKE_CXX_COMPILER "clang++")
endif()

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/botan-3)

message(STATUS "CMAKE_SOURCE_DIR: ${CMAKE_SOURCE_DIR}")

# Determine the library directory based on the operating system
if(APPLE)
    set(LIB_DIR "${CMAKE_SOURCE_DIR}/libs/mac")
elseif(WIN32)
    set(LIB_DIR "${CMAKE_SOURCE_DIR}/libs/win")
elseif(UNIX)
    set(LIB_DIR "${CMAKE_SOURCE_DIR}/libs/linux")
else()
    message(FATAL_ERROR "Unsupported operating system")
endif()

message(STATUS "LIB_DIR: ${LIB_DIR}")

# Find the Botan library in the specified directory
find_library(BOTAN_LIBRARY NAMES botan-3 PATHS ${LIB_DIR} NO_DEFAULT_PATH)

if(NOT BOTAN_LIBRARY)
    message(FATAL_ERROR "Could not find the botan-3 library in ${LIB_DIR}")
endif()

# Add the shared library you are building from source
add_library(botan SHARED src/pkcs.cc)

# Set C++20 standard for the botan target
set_target_properties(botan PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED YES
)

# Explicitly add the library directory to the linker search path
target_link_directories(botan PRIVATE ${LIB_DIR})

# Link the built shared library with the external dynamic library
target_link_libraries(botan ${BOTAN_LIBRARY})

# Print verbose output for debugging
set(CMAKE_VERBOSE_MAKEFILE ON)