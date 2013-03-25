enable_language(ASM_NASM)

set(FLIX_LINKER_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/flix.ld")

set(CMAKE_CXX_FLAGS "-g -O0 -Ilibkcxx -nostdlib -nostdinc -nostdinc++ -fno-builtin -fno-stack-protector -std=c++0x -fno-exceptions -Wall -Wextra -pedantic -mno-sse -mno-mmx -fPIC -m64")
set(CMAKE_ASM_NASM_FLAGS "-felf64")
set(CMAKE_EXE_LINKER_FLAGS "-T${FLIX_LINKER_SCRIPT} -nostdlib -melf_x86_64 -z max-page-size=0x1000")

set(CMAKE_CXX_LINK_EXECUTABLE "ld <LINK_FLAGS> <OBJECTS> -o <TARGET>")

include_directories(${LIBKCXX_INCLUDE_DIRS})

add_executable(flix
  boot.asm
  DescTables.cpp
  interrupt.asm
  Interrupt.cpp
  IntUtil.cpp
  KHeap.cpp
  main.cpp
  #Memory.cpp
  Multiboot.cpp
  Paging.cpp
  Screen.cpp
  Timer.cpp
)

# does not work
add_dependencies(flix
  ${FLIX_LINKER_SCRIPT}
)
