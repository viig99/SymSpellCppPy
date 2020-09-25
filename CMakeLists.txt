cmake_minimum_required(VERSION 3.17)
project(SymSpellCppPy)

set(CMAKE_CXX_STANDARD 14)

add_library(SymSpellCppPy SHARED library.cpp library.h include/BaseDistance.h include/BaseSimilarity.h include/Helpers.h include/DamerauOSA.h include/Levenshtein.h include/EditDistance.h)
target_include_directories(SymSpellCppPy PRIVATE include)