cmake_minimum_required(VERSION 3.17)
project(SymSpellCppPy)

set(CMAKE_CXX_STANDARD 14)
find_package(Catch2 REQUIRED)

add_library(SymSpellCppPy SHARED library.cpp library.h)
add_executable(test_sym "library_test.cpp")
target_link_libraries(test_sym Catch2::Catch2)
target_include_directories(SymSpellCppPy PRIVATE include)