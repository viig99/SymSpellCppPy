cmake_minimum_required(VERSION 3.17)
project(SymSpellCppPy)

set(CMAKE_CXX_STANDARD 14)

add_library(SymSpellCppPy SHARED library.cpp library.h)
target_include_directories(SymSpellCppPy PRIVATE include)