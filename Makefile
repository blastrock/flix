CXX=clang++
#CXX=g++
CFLAGS=-nostdlib -nostdinc -fno-builtin -fno-stack-protector
CXXFLAGS=-g -O0 -Ilibkcxx -nostdlib -nostdinc -nostdinc++ -fno-builtin -fno-stack-protector -std=c++0x -fno-exceptions -Wall -Wextra -pedantic -mno-sse -mno-mmx -fPIC -m64
NASMFLAGS=-felf64
#LDFLAGS=-nostdlib -belf64-x86-64 #-melf_i386
LDFLAGS=-nostdlib -melf_x86_64 -z max-page-size=0x1000
OBJECTS=boot.o main.o Screen.o IntUtil.o
#OBJECTS=boot.o main.o Screen.o DescTables.o dt_set.o Isr.o interrupt.o Timer.o KHeap.o Paging.o Memory.o IntUtil.o
#SETUPOBJS=boot.setup.o bootstrap.setup.o DescTables.setup.o Isr.setup.o interrupt.setup.o Paging.setup.o KHeap.setup.o dt_set.setup.o
#OBJECTS=main64.o Screen.o IntUtil.o
HD_SIZE=4

include $(wildcard *.d)

%.o: %.asm
	nasm $(NASMFLAGS) $< -o $@

%.o: %.cpp
	$(CXX) -MD -c $(CXXFLAGS) $< -o $@

flix: $(OBJECTS) flix.ld
	ld -Tflix.ld $(LDFLAGS) $+ -o $@

#flix.img: flix
#	dd if=/dev/zero of=$@.tmp bs=1M count=$(HD_SIZE) 2>/dev/null
#	/sbin/mkfs.vfat $@.tmp
#	syslinux $@.tmp
#	mcopy -i $@.tmp /usr/lib/syslinux/mboot.c32 ::mboot.c32
#	mcopy -i $@.tmp flix ::flix
#	echo -e "TIMEOUT 1\nDEFAULT mboot.c32 flix" | mcopy -i $@.tmp - ::syslinux.cfg
#	mv $@.tmp $@
flix.img: flix
	mkdir -p img/boot/grub
	echo "set timeout=0\nset default=0\nmenuentry "flix" { multiboot2 /flix }" > img/boot/grub/grub.cfg
	cp -f flix img/flix
	grub-mkrescue -o flix.img img

run: flix.img
	qemu-system-x86_64 -hda flix.img -m 64 -monitor stdio -enable-kvm
.PHONY: run

debug: flix.img
	qemu-system-x86_64 -hda flix.img -m 64 -s -S -daemonize
	gdb flix
.PHONY: debug

clean:
	rm -f bootstrap flix flix64 $(SETUPOBJS) $(OBJECTS) *.d flix.img
.PHONY: clean

.DEFAULT_GOAL := flix
