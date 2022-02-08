cmake_minimum_required(VERSION 3.20 FATAL_ERROR)

project(SortingNetworkCpp)

option(BUILD_BENCHMARK "Build the benchmark" ON)
option(BUILD_TESTS "Build the tests" OFF)

include(cmake/Conan.cmake)
run_conan()

set(SN_BENCHMARK_EXECUTABLE_NAME "sorting_network_cpp_benchmark")
set(SN_TESTS_EXECUTABLE_NAME "sorting_network_cpp_tests")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(project_options INTERFACE)
target_compile_features(project_options INTERFACE cxx_std_17)

if (${CMAKE_CXX_COMPILER_ID} STREQUAL MSVC)
  set(CMAKE_CXX_FLAGS_RELEASE "/W3 /O2 /fp:fast")
  add_compile_options("-bigobj")
elseif (${CMAKE_CXX_COMPILER_ID} STREQUAL GNU)
  set(CMAKE_CXX_FLAGS_RELEASE "-O3 -ffast-math")
elseif (${CMAKE_CXX_COMPILER_ID} STREQUAL Clang)
  set(CMAKE_CXX_FLAGS_RELEASE "-O3 -ffast-math")
endif()

include_directories("include")

add_library(metal INTERFACE)
target_include_directories(metal INTERFACE "extern/metal/include")

if (BUILD_BENCHMARK)
  add_executable(${SN_BENCHMARK_EXECUTABLE_NAME} "src/benchmark.cpp")
  target_link_libraries(${SN_BENCHMARK_EXECUTABLE_NAME} PRIVATE project_options)
endif()

if(BUILD_TESTS)
  find_package(GTest REQUIRED)
  include(GoogleTest)
  enable_testing()

  add_executable(${SN_TESTS_EXECUTABLE_NAME} "test/test_case_gen.h" "test/sorting_network_tests.cpp")
  target_link_libraries(${SN_TESTS_EXECUTABLE_NAME} GTest::gtest GTest::gtest_main metal project_options)

  gtest_discover_tests(${SN_TESTS_EXECUTABLE_NAME})
endif()
