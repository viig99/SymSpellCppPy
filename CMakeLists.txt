cmake_minimum_required(VERSION 3.18)
project(SymSpellCppPy)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)
set(CMAKE_BUILD_TYPE "Release")

# Apple Silicon specific optimizations
if(APPLE AND CMAKE_HOST_SYSTEM_PROCESSOR STREQUAL "arm64")
    set(CMAKE_OSX_ARCHITECTURES "arm64")
    message(STATUS "Detected Apple Silicon, enabling arm64 optimizations")
endif()

option(BUILD_FOR_PYTHON "Build for Python" OFF)
option(BUILD_FOR_TEST "Build Tests" ON)

add_library(SymSpellCpp STATIC library.cpp library.h)
target_include_directories(SymSpellCpp PUBLIC ${CMAKE_SOURCE_DIR}/include)

if (BUILD_FOR_PYTHON)
    set(CMAKE_BUILD_TYPE "Release")
    Include(FetchContent)

    # Use newer pybind11 version with better CMake support
    FetchContent_Declare(
            pybind11
            GIT_REPOSITORY https://github.com/pybind/pybind11.git
            GIT_TAG v2.11.1)
    FetchContent_MakeAvailable(pybind11)
    pybind11_add_module(SymSpellCppPy MODULE SymSpellCppPy.cpp)
    target_sources(SymSpellCppPy PRIVATE library.cpp library.h)
    target_link_libraries(SymSpellCppPy PRIVATE SymSpellCpp)
    
    # Platform-specific compile options
    if(APPLE AND CMAKE_HOST_SYSTEM_PROCESSOR STREQUAL "arm64")
        # Apple Silicon optimizations
        target_compile_options(SymSpellCppPy PRIVATE
                $<$<CONFIG:Release>:-O3 -DNDEBUG -march=armv8.4-a -fvisibility=hidden
                -flto -ffat-lto-objects>)
    else()
        # Standard x86_64 optimizations
        target_compile_options(SymSpellCppPy PRIVATE
                $<$<CONFIG:Release>:-O3 -DNDEBUG -march=native -mtune=native -fvisibility=hidden
                -flto -ffat-lto-objects>)
    endif()
    
    message(STATUS "Build for Python = " ${BUILD_FOR_PYTHON})
else ()
    Include(FetchContent)

    FetchContent_Declare(
            Catch2
            GIT_REPOSITORY https://github.com/catchorg/Catch2.git
            GIT_TAG v2.13.10)
    FetchContent_MakeAvailable(Catch2)

    add_executable(Catch2Test tests/CatchMain.cpp library.cpp library.h)
    target_include_directories(Catch2Test PUBLIC tests ${CMAKE_SOURCE_DIR}/include)
    target_link_libraries(Catch2Test Catch2::Catch2)
endif ()
