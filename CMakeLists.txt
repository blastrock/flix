cmake_minimum_required(VERSION 2.8)
project(flix)

set(CXX_COMPILER_INCLUDE "" CACHE PATH "Compiler-provided headers, like stdint.h, stddef.h")

if("${CXX_COMPILER_INCLUDE}" STREQUAL "")
  message(FATAL_ERROR "Could not find stddef.h, fix CXX_COMPILER_INCLUDE")
endif("${CXX_COMPILER_INCLUDE}" STREQUAL "")

set(LIBKCXX_INCLUDE_DIRS ${PROJECT_SOURCE_DIR}/libkcxx/include)

set(LIBCXX_ENABLE_EXCEPTIONS OFF CACHE BOOL "Use exceptions.")
set(LIBCXX_ENABLE_RTTI ON CACHE BOOL "Use run time type information.")
set(LIBCXX_ENABLE_ASSERTIONS ON CACHE BOOL "Enable assertions independent of build mode.")
set(LIBCXX_ENABLE_PEDANTIC ON CACHE BOOL "Compile with pedantic enabled.")
set(LIBCXX_ENABLE_WERROR OFF CACHE BOOL "Fail and stop if a warning is triggered.")
set(LIBCXX_ENABLE_CXX0X ON CACHE BOOL "Enable -std=c++0x and use of c++0x language features if the compiler supports it.")
set(LIBCXX_ENABLE_SHARED OFF CACHE BOOL "Build libc++ as a shared library.")
set(LIBCXX_CXX_ABI "libcxxabi" CACHE STRING "")
set(LIBCXX_LIBCXXABI_INCLUDE_PATHS "${PROJECT_SOURCE_DIR}/libkcxx/libcxxabi/include" CACHE STRING "")

add_subdirectory(libkcxx)
add_subdirectory(source)
add_subdirectory(debug)
