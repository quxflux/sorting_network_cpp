cmake_minimum_required(VERSION 3.20 FATAL_ERROR)

project(sorting_network_cpp)

option(SORTING_NETWORK_CPP_BUILD_BENCHMARK "Build the benchmark" OFF)
option(SORTING_NETWORK_CPP_BUILD_TESTS "Build the tests" OFF)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(cmake/CPM.cmake)

add_library(project_options INTERFACE)
target_compile_features(project_options INTERFACE cxx_std_17)

if (${CMAKE_CXX_COMPILER_ID} STREQUAL MSVC)
    target_compile_options(project_options INTERFACE $<$<NOT:$<CONFIG:Debug>>: /W3 /O2 /fp:fast> /bigobj)
    target_link_options(project_options INTERFACE $<$<NOT:$<CONFIG:Debug>>: /OPT:REF /GL>)
elseif (${CMAKE_CXX_COMPILER_ID} STREQUAL GNU)
    target_compile_options(project_options INTERFACE $<$<NOT:$<CONFIG:Debug>>: -O3 -ffast-math>)
elseif (${CMAKE_CXX_COMPILER_ID} STREQUAL Clang)
    target_compile_options(project_options INTERFACE $<$<NOT:$<CONFIG:Debug>>: -O3 -ffast-math>)
endif()

add_library(sorting_network_cpp INTERFACE)
target_include_directories(sorting_network_cpp INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/include")

if (SORTING_NETWORK_CPP_BUILD_BENCHMARK)
    set(SN_BENCHMARK_EXECUTABLE_NAME ${PROJECT_NAME}_benchmark)

    add_executable(${SN_BENCHMARK_EXECUTABLE_NAME}
        "src/benchmark_main.cpp"
        "src/benchmark_int16.cpp"
        "src/benchmark_int32.cpp"
        "src/benchmark_uint32.cpp"
        "src/benchmark_int64.cpp"
        "src/benchmark_float.cpp"
        "src/benchmark_double.cpp"
        "src/benchmark_vec2i.cpp"
    )
    target_link_libraries(${SN_BENCHMARK_EXECUTABLE_NAME} PRIVATE project_options sorting_network_cpp)
endif()

if(SORTING_NETWORK_CPP_BUILD_TESTS)
    set(METAL_BUILD_DOC OFF)
    set(METAL_BUILD_EXAMPLES OFF)
    set(METAL_BUILD_TESTS OFF)
    CPMAddPackage("gh:brunocodutra/metal#v2.1.4")
    CPMAddPackage("gh:google/googletest#release-1.12.0")

    include(GoogleTest)

    enable_testing()

    set(SN_TESTS_EXECUTABLE_NAME ${PROJECT_NAME}_tests)

    add_executable(${SN_TESTS_EXECUTABLE_NAME}
        "test/test_base.h"
        "test/test_batcher_odd_even_merge_sort.cpp"
        "test/test_bitonic_merge_sort.cpp"
        "test/test_bose_nelson_sort.cpp"
        "test/test_bubble_sort.cpp"
        "test/test_insertion_sort.cpp"
        "test/test_size_optimized_sort.cpp"
    )
    target_link_libraries(${SN_TESTS_EXECUTABLE_NAME} GTest::gtest GTest::gmock GTest::gtest_main Metal project_options sorting_network_cpp)
    gtest_discover_tests(${SN_TESTS_EXECUTABLE_NAME})
endif()
