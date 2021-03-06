set(SYSCALL_ENUM syscall_enum.hxx)
set(SYSCALL_REGISTER syscall_register.hxx)
set(SYSCALL_GLIBC_DEFINES syscall_glibc_defines.h)

add_subdirectory(syscalls)

enable_language(ASM_NASM)
enable_language(ASM)

include_directories(
  SYSTEM
  ${PROJECT_SOURCE_DIR}/external/xllpnt/include
  ${PROJECT_SOURCE_DIR}/external/xlllog/include
  ${PROJECT_SOURCE_DIR}/external/variant/include
  ${PROJECT_SOURCE_DIR}/external/expected/include
  )

set(FLIX_LINKER_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/flix.ld")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Ilibkcxx -nostdinc -nostdinc++ -isystem ${CXX_COMPILER_INCLUDE} -isystem ${LIBKC_INCLUDE} -D_LIBCPP_HAS_NO_MONOTONIC_CLOCK -D_LIBCPP_HAS_NO_THREADS -D_LIBCPP_NO_EXCEPTIONS -D_LIBCPP_NO_RTTI -D_LIBCPP_BUILD_STATIC -fno-builtin -fno-stack-protector -std=c++14 -fno-exceptions -Wall -Wextra -pedantic -fPIC -m64")

set(CMAKE_ASM_NASM_FLAGS "-felf64")
set(CMAKE_EXE_LINKER_FLAGS "-T${FLIX_LINKER_SCRIPT} -nostdlib -melf_x86_64 -z max-page-size=0x1000 -O0")

set(CMAKE_CXX_LINK_EXECUTABLE "ld <LINK_FLAGS> <OBJECTS> -o <TARGET> <LINK_LIBRARIES>")
set(CMAKE_C_IMPLICIT_LINK_DIRECTORIES "")
set(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS "")

include_directories(${LIBKCXX_INCLUDE_DIRS})
include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(${PROJECT_SOURCE_DIR}/include)

add_library(flix STATIC
  ${CMAKE_CURRENT_BINARY_DIR}/syscalls/${SYSCALL_ENUM}
  ${CMAKE_CURRENT_BINARY_DIR}/syscalls/${SYSCALL_REGISTER}
  ${CMAKE_CURRENT_BINARY_DIR}/syscalls/${SYSCALL_GLIBC_DEFINES}

  Assert.cpp
  boot.asm
  CondVar.cpp
  Cpu.cpp
  Cpio.cpp
  Debug.cpp
  DescTables.cpp
  Elf.cpp
  Fs.cpp
  FileManager.cpp
  interrupt.asm
  Interrupt.cpp
  IntUtil.cpp
  Keyboard.cpp
  KHeap.cpp
  kmain.cpp
  Memory.cpp
  Multiboot.cpp
  PageHeap.cpp
  PageDirectory.cpp
  Screen.cpp
  Semaphore.cpp
  Serial.cpp
  SpinLock.cpp
  StaticMemoryPool.cpp
  Symbols.cpp
  Syscall.cpp
  syscall.S
  TaskManager.cpp
  task.S
  Timer.cpp
  Tty.cpp
)

set_source_files_properties(${CMAKE_CURRENT_BINARY_DIR}/syscalls/${SYSCALL_ENUM} PROPERTIES GENERATED 1)
set_source_files_properties(${CMAKE_CURRENT_BINARY_DIR}/syscalls/${SYSCALL_REGISTER} PROPERTIES GENERATED 1)
set_source_files_properties(${CMAKE_CURRENT_BINARY_DIR}/syscalls/${SYSCALL_GLIBC_DEFINES} PROPERTIES GENERATED 1)

add_dependencies(flix SyscallGen)

set_source_files_properties(Symbols.cpp
  PROPERTIES COMPILE_FLAGS -mcmodel=large
)

add_custom_target(flixlinkerscript DEPENDS ${FLIX_LINKER_SCRIPT})

add_dependencies(flix flixlinkerscript)

target_link_libraries(flix
  cxx
  c
)

add_executable(flixmain
  main.cpp
)

target_link_libraries(flixmain flix)

make_flix_image(flixmain WITH_MODULE)

add_subdirectory(test)
