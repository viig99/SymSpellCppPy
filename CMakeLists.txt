cmake_minimum_required(VERSION 3.14)
project(SymSpellCppPy)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)
set(CMAKE_BUILD_TYPE "Release")

option(BUILD_FOR_PYTHON "Build for Python" OFF)

add_library(SymSpellCpp SHARED library.cpp library.h)

if (BUILD_FOR_PYTHON)
    Include(FetchContent)

    FetchContent_Declare(
            pybind11
            GIT_REPOSITORY https://github.com/pybind/pybind11.git
            GIT_TAG v2.5.0)
    FetchContent_MakeAvailable(pybind11)
    add_library(SymSpellCppPy MODULE SymSpellCppPy.cpp)
    target_sources(SymSpellCppPy INTERFACE library.cpp library.h)
    target_link_libraries(SymSpellCppPy PRIVATE SymSpellCpp pybind11::module)
    set_target_properties(SymSpellCppPy PROPERTIES PREFIX "${PYTHON_MODULE_PREFIX}"
            SUFFIX "${PYTHON_MODULE_EXTENSION}")
    target_compile_options(SymSpellCppPy PRIVATE
            $<$<CONFIG:Release>:-O3 -DNDEBUG -march=native -mtune=native -flto>)
    message(STATUS "Build for Python = " ${BUILD_FOR_PYTHON})
else ()
    Include(FetchContent)

    FetchContent_Declare(
            Catch2
            GIT_REPOSITORY https://github.com/catchorg/Catch2.git
            GIT_TAG v2.13.1)
    FetchContent_MakeAvailable(Catch2)

    add_executable(Catch2Test tests/CatchMain.cpp library.cpp library.h)
    target_include_directories(Catch2Test PRIVATE tests)
    target_link_libraries(Catch2Test Catch2::Catch2)
endif ()