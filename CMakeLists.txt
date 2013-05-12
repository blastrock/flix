cmake_minimum_required(VERSION 2.8)
project(flix)

set(LIBKCXX_INCLUDE_DIRS ${PROJECT_SOURCE_DIR}/libkcxx/include)

set(LIBCXX_ENABLE_EXCEPTIONS OFF CACHE BOOL "Use exceptions.")
set(LIBCXX_ENABLE_RTTI OFF CACHE BOOL "Use run time type information.")
set(LIBCXX_ENABLE_ASSERTIONS OFF CACHE BOOL "Enable assertions independent of build mode.")
set(LIBCXX_ENABLE_PEDANTIC ON CACHE BOOL "Compile with pedantic enabled.")
set(LIBCXX_ENABLE_WERROR OFF CACHE BOOL "Fail and stop if a warning is triggered.")
set(LIBCXX_ENABLE_CXX0X ON CACHE BOOL "Enable -std=c++0x and use of c++0x language features if the compiler supports it.")
set(LIBCXX_ENABLE_SHARED OFF CACHE BOOL "Build libc++ as a shared library.")
set(LIBCXX_CXX_ABI "libcxxabi" CACHE STRING "")
set(LIBCXX_LIBCXXABI_INCLUDE_PATHS "${PROJECT_SOURCE_DIR}/libkcxx/libcxxabi/include" CACHE STRING "")

add_subdirectory(libkcxx)
add_subdirectory(source)
