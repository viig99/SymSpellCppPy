cmake_minimum_required(VERSION 3.17)
project(SymSpellCppPy)

set(CMAKE_CXX_STANDARD 14)

Include(FetchContent)

FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG        v2.13.1)

FetchContent_MakeAvailable(Catch2)


add_library(SymSpellCppPy SHARED library.cpp library.h)
target_include_directories(SymSpellCppPy PRIVATE include)

add_executable(Catch2Test tests/CatchMain.cpp)
target_sources(Catch2Test PRIVATE library.cpp library.h)
target_include_directories(Catch2Test PRIVATE tests)
target_link_libraries(Catch2Test Catch2::Catch2)