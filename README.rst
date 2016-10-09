flix
====

A kernel written in C++.

Why?
----

This is a kernel I wrote mostly for fun (and no profit). I tried to make use of
modern C++ features to make the code clearer and easier to write.

This kernel is a work in progress that I stopped developing in the end. I tried
to make use of most open-source code as possible. This includes a fork of libc++
for kernel-space and Android's bionic libc implementation for user-space. There
are no drivers and the VFS part is very poor in features. My plan was to make
use of open-source code for this too and maybe integrate filesystems from Linux
or some micro-kernel.

Features
--------

- Supports only x86_64 CPUs
- Works on real hardware (I could only test on one computer)
- Memory protection
- Multi-process support
- ABI-compatible syscalls with Linux
- Can run a very light busybox
- Unit-tested

How to compile
--------------

This kernel is only tested with Clang 3.8. You will need to find where your
distro has installed your compiler's headers. This includes the files
``stddef.h`` and ``stdint.h``. For me, they are in
``/usr/lib/clang/3.8/include``.

I use ninja to compile, but you can use make if you prefer.

To compile::

    $ mkdir build
    $ cd build
    $ CC=clang CXX=clang++ cmake .. -DCXX_COMPILER_INCLUDE=<path to your compiler's headers> -GNinja
    $ ninja

This will create an image that you can run in bochs, qemu or your hardware.

A module is bundled inside the image with the root filesystem, in which there is
currently a compiled busybox.

Running tests
-------------

The tests run on the bochs emulator. The one bundled in Debian doesn't have all
the features needed, so you will need to compile your's.

I compiled mine from Debian sources of bochs-2.6::

    $ apt source bochs
    $ cd bochs-2.6
    $ ./configure --prefix=$HOME/.local --with-nogui  --with-x11  --with-rfb  --with-term  --with-sdl  --with-wx  --disable-docbook  --enable-cdrom  --enable-pci  --enable-usb  --enable-usb-ohci  --enable-a20-pin  --enable-cpu-level=6  --enable-x86-64  --enable-avx  --enable-vmx=2  --enable-fpu  --enable-debugger  --enable-debugger-gui  --enable-disasm  --enable-idle-hack  --enable-all-optimizations  --enable-repeat-speedups  --enable-plugins  --enable-compressed-hd  --enable-clgd54xx  --enable-sb16  --enable-es1370  --enable-ne2000  --enable-pnic
    $ make
    $ make install

After you have compiled the kernel, you can run the unit tests with::

    $ ctest -VV

How to run
----------

You can run the compiled kernel with its module in qemu with the following
command, inside the build directory::

    $ ninja run_flixmain

You can run it in bochs with the following command, inside the source
directory::

    $ bochs -f debug/bochsrc -q

You can make a bootable USB drive with the following command, inside the build
directory::

    $ dd if=build/source/flix_flixmain.img of=/dev/<your drive> bs=1M

How to compile something for flix
---------------------------------

You need to compile a `libc<https://github.com/blastrock/platform_bionic>`_ and
then you can compile `my fork of
busybox<https://github.com/blastrock/busybox>`_.
