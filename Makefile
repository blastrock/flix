CFLAGS=-nostdlib -nostdinc -fno-builtin -fno-stack-protector -m32
CXXFLAGS=-nostdlib -nostdinc -fno-builtin -fno-stack-protector -m32 -std=c++0x

%.o: %.asm
	nasm -f elf32 $< -o $@

flix: boot.o main.o
	ld -m elf_i386 -Tlink.ld $+ -o $@

clean:
	rm -f flix *.o
