cmake_minimum_required(VERSION 3.26)
project(SymSpellCppPy LANGUAGES C CXX)

option(BUILD_FOR_PYTHON "Build the Python extension" OFF)
option(BUILD_FOR_TEST "Build the native Catch2 tests" OFF)
option(ENABLE_COVERAGE "Instrument the project with GCC coverage" OFF)

if(ENABLE_COVERAGE)
  add_compile_options(--coverage -O0)
  add_link_options(--coverage)
endif()

add_library(SymSpellCpp STATIC library.cpp unicode.cpp third_party/utf8proc/utf8proc.c)
target_include_directories(SymSpellCpp PUBLIC "${PROJECT_SOURCE_DIR}/include")
target_include_directories(SymSpellCpp PRIVATE "${PROJECT_SOURCE_DIR}/third_party/utf8proc")
target_compile_definitions(SymSpellCpp PRIVATE UTF8PROC_STATIC)
target_compile_features(SymSpellCpp PUBLIC cxx_std_20)
set_target_properties(SymSpellCpp PROPERTIES CXX_EXTENSIONS OFF POSITION_INDEPENDENT_CODE ON)
if(MSVC)
  target_compile_options(SymSpellCpp PUBLIC /utf-8)
endif()

if(SKBUILD OR BUILD_FOR_PYTHON)
  find_package(Python 3.10 REQUIRED COMPONENTS Interpreter Development.Module
    OPTIONAL_COMPONENTS Development.SABIModule)
  find_package(nanobind 3.1 CONFIG REQUIRED)
  nanobind_add_module(_core STABLE_ABI NB_STATIC SymSpellCppPy.cpp)
  target_link_libraries(_core PRIVATE SymSpellCpp)
  set_target_properties(_core PROPERTIES CXX_EXTENSIONS OFF)

  nanobind_add_stub(symspell_stub MODULE _core OUTPUT _core.pyi
    PYTHON_PATH $<TARGET_FILE_DIR:_core> DEPENDS _core MARKER_FILE py.typed)
  install(TARGETS _core LIBRARY DESTINATION SymSpellCppPy)
  install(FILES "${CMAKE_CURRENT_BINARY_DIR}/_core.pyi"
    "${CMAKE_CURRENT_BINARY_DIR}/py.typed" DESTINATION SymSpellCppPy)
  install(FILES resources/frequency_dictionary_en_82_765.txt
    resources/frequency_bigramdictionary_en_243_342.txt DESTINATION SymSpellCppPy)
endif()

if(BUILD_FOR_TEST)
  include(CTest)
  include(FetchContent)
  FetchContent_Declare(Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.8.1
    GIT_SHALLOW TRUE)
  FetchContent_MakeAvailable(Catch2)
  add_executable(Catch2Test tests/CatchMain.cpp)
  target_link_libraries(Catch2Test PRIVATE SymSpellCpp Catch2::Catch2WithMain)
  add_test(NAME Catch2Test COMMAND Catch2Test WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}")
endif()
