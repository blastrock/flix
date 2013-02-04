CFLAGS=-nostdlib -nostdinc -fno-builtin -fno-stack-protector -m32
CXXFLAGS=-nostdlib -nostdinc -fno-builtin -fno-stack-protector -m32 -std=c++0x

include $(wildcard *.d)

%.o: %.asm
	nasm -f elf32 $< -o $@

%.o: %.cpp
	g++ -MD -c $(CXXFLAGS) $< -o $@

flix: boot.o main.o Screen.o
	ld -m elf_i386 -Tlink.ld $+ -o $@

clean:
	rm -f flix *.o *.d

.DEFAULT_GOAL := flix
