cmake_minimum_required(VERSION 2.8)
project(flix)

# clang doesn't print colored diagnostics when invoked from Ninja
if(UNIX AND CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND CMAKE_GENERATOR STREQUAL
"Ninja")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fcolor-diagnostics")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fcolor-diagnostics")
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-threadsafe-statics")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mno-sse -mno-mmx")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")

set(CXX_COMPILER_INCLUDE "" CACHE PATH "Compiler-provided headers, like stdint.h, stddef.h")

if(NOT EXISTS "${CXX_COMPILER_INCLUDE}/stddef.h")
  message(FATAL_ERROR "Could not find stddef.h, fix CXX_COMPILER_INCLUDE")
endif()

set(LIBKCXX_INCLUDE_DIRS ${CMAKE_SOURCE_DIR}/libkcxx/include)

set(LIBCXX_ENABLE_EXCEPTIONS OFF CACHE BOOL "Use exceptions.")
set(LIBCXX_ENABLE_RTTI OFF CACHE BOOL "Use run time type information.")
set(LIBCXX_ENABLE_ASSERTIONS ON CACHE BOOL "Enable assertions independent of build mode.")
set(LIBCXX_ENABLE_PEDANTIC ON CACHE BOOL "Compile with pedantic enabled.")
set(LIBCXX_ENABLE_WERROR OFF CACHE BOOL "Fail and stop if a warning is triggered.")
set(LIBCXX_ENABLE_CXX1Y ON CACHE BOOL "Enable -std=c++1y and use of c++1y language features if the compiler supports it.")
set(LIBCXX_ENABLE_SHARED OFF CACHE BOOL "Build libc++ as a shared library.")
set(LIBCXX_ENABLE_THREADS OFF CACHE BOOL "Build libc++ with support for threads.")
set(LIBCXX_ENABLE_LOCALE OFF CACHE BOOL "Build libc++ with support for locales.")
set(LIBCXX_ENABLE_MONOTONIC_CLOCK OFF CACHE BOOL
  "Build libc++ with support for a monotonic clock.
   This option may only be used when LIBCXX_ENABLE_THREADS=OFF.")
set(LIBCXX_CXX_ABI "libcxxabi" CACHE STRING "")
set(LIBCXX_CXX_ABI_INTREE 1)
set(LIBCXX_LIBCXXABI_INCLUDE_PATHS "${CMAKE_SOURCE_DIR}/libcxxabi/include;${CXX_COMPILER_INCLUDE}" CACHE STRING "")

set(LIBKC_INCLUDE "${PROJECT_SOURCE_DIR}/libkc/include")
set(LIBCXX_LIBKC_INCLUDE "${LIBKC_INCLUDE}")

set(LIBCXXABI_ENABLE_THREADS OFF CACHE BOOL "Build libc++abi with support for threads.")
set(LIBCXXABI_ENABLE_SHARED OFF CACHE BOOL "Build libc++abi as a shared library.")
set(LIBCXXABI_LIBCXX_INCLUDES "${CMAKE_SOURCE_DIR}/libkcxx/include" CACHE BOOL "Path to libcxx includes")

set(LIBCXX_HAS_PTHREAD_LIB 0 CACHE INTERNAL "")
set(LIBCXX_HAS_C_LIB 0 CACHE INTERNAL "")
set(LIBCXX_HAS_M_LIB 0 CACHE INTERNAL "")
set(LIBCXX_HAS_RT_LIB 0 CACHE INTERNAL "")
set(LIBCXX_HAS_GCC_S_LIB 0 CACHE INTERNAL "")

set(LIBCXXABI_HAS_C_LIB 0 CACHE INTERNAL "")
set(LIBCXXABI_HAS_PTHREAD_LIB 0 CACHE INTERNAL "")
set(LIBCXXABI_HAS_GCC_EH_LIB 0 CACHE INTERNAL "")
set(LIBCXXABI_HAS_CXA_THREAD_ATEXIT_IMPL 0 CACHE INTERNAL "")
set(LIBCXXABI_HAS_DL_LIB 0 CACHE INTERNAL "")

add_definitions(-D__GLIBC__)

add_subdirectory(libcxxabi)

# disable exception and rtti now
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-exceptions")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti")

add_subdirectory(libkc)
add_subdirectory(libkcxx)
add_subdirectory(source)
add_subdirectory(debug)
