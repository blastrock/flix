CXX=clang++
#CXX=g++
CFLAGS=-nostdlib -nostdinc -fno-builtin -fno-stack-protector -m32
CXXFLAGS=-g -O0 -Ilibkcxx -nostdlib -nostdinc -nostdinc++ -fno-builtin -fno-stack-protector -m32 -std=c++0x -fno-exceptions -Wall -Wextra -pedantic -mno-sse -mno-mmx
OBJECTS=boot.o main.o Screen.o DescTables.o dt_set.o Isr.o interrupt.o Timer.o KHeap.o Paging.o Memory.o IntUtil.o
HD_SIZE=4

include $(wildcard *.d)

%.o: %.asm
	nasm -f elf32 $< -o $@

%.o: %.cpp
	$(CXX) -MD -c $(CXXFLAGS) $< -o $@

flix: $(OBJECTS) link.ld
	ld -m elf_i386 -Tlink.ld $+ -o $@

flix.img: flix
	dd if=/dev/zero of=$@.tmp bs=1M count=$(HD_SIZE) 2>/dev/null
	/sbin/mkfs.vfat $@.tmp
	syslinux $@.tmp
	mcopy -i $@.tmp /usr/lib/syslinux/mboot.c32 ::mboot.c32
	mcopy -i $@.tmp flix ::flix
	echo -e "TIMEOUT 1\nDEFAULT mboot.c32 flix" | mcopy -i $@.tmp - ::syslinux.cfg
	mv $@.tmp $@

run: flix.img
	qemu-system-i386 -hda flix.img -m 64 -monitor stdio
.PHONY: run

debug: flix.img
	qemu-system-i386 -hda flix.img -m 64 -s -S -daemonize
	gdb flix
.PHONY: debug

clean:
	rm -f flix $(OBJECTS) *.d flix.img
.PHONY: clean

.DEFAULT_GOAL := flix
