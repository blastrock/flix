#flix.img: flix
#	dd if=/dev/zero of=$@.tmp bs=1M count=$(HD_SIZE) 2>/dev/null
#	/sbin/mkfs.vfat $@.tmp
#	syslinux $@.tmp
#	mcopy -i $@.tmp /usr/lib/syslinux/mboot.c32 ::mboot.c32
#	mcopy -i $@.tmp flix ::flix
#	echo -e "TIMEOUT 1\nDEFAULT mboot.c32 flix" | mcopy -i $@.tmp - ::syslinux.cfg
#	mv $@.tmp $@
rebuild:
	$(MAKE) -C build
.PHONY: rebuild

flix.img: rebuild build/source/flix
	mkdir -p img/boot/grub
	echo -e "set timeout=0\nset default=0\nmenuentry "flix" { multiboot2 /flix }" > img/boot/grub/grub.cfg
	cp -f build/source/flix img/flix
	grub-mkrescue -o flix.img img

run: flix.img
	qemu-system-x86_64 -hda flix.img -m 64 -monitor stdio -enable-kvm
.PHONY: run

debug: flix.img
	qemu-system-x86_64 -hda flix.img -m 64 -s -daemonize
	gdb build/source/flix
.PHONY: debug

clean:
	$(MAKE) -C build clean
	rm -f flix.img
.PHONY: clean

.DEFAULT_GOAL := flix.img
