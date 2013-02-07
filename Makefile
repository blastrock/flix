CXX=clang++
CFLAGS=-nostdlib -nostdinc -fno-builtin -fno-stack-protector -m32
CXXFLAGS=-nostdlib -nostdinc -fno-builtin -fno-stack-protector -m32 -std=c++0x -fno-exceptions -Wall -Wextra -pedantic
OBJECTS=boot.o main.o Screen.o DescTables.o dt_set.o Isr.o interrupt.o Timer.o

include $(wildcard *.d)

%.o: %.asm
	nasm -f elf32 $< -o $@

%.o: %.cpp
	$(CXX) -MD -c $(CXXFLAGS) $< -o $@

flix: $(OBJECTS)
	ld -m elf_i386 -Tlink.ld $+ -o $@

clean:
	rm -f flix $(OBJECTS) *.d

.DEFAULT_GOAL := flix
