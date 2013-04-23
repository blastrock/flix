;
; boot.s -- Kernel start location. Also defines multiboot header.
; Based on Bran's kernel development tutorial file start.asm
;

MB2_HEADER_MAGIC   equ 0xE85250D6
MB2_ARCHITECTURE   equ 0
MB2_LENGTH         equ MB2_HEADER_END - MB2_HEADER_START
MB2_RESPONSE_MAGIC equ 0X36D76289
MB2_CHECKSUM       equ -(MB2_HEADER_MAGIC + MB2_ARCHITECTURE + MB2_LENGTH)

[BITS 32]
[SECTION .mbhdr]
[EXTERN _kernelStart]
[EXTERN _kernelDataEnd]
[EXTERN _kernelBssEnd]

ALIGN 8
MB2_HEADER_START:
  dd MB2_HEADER_MAGIC
  dd MB2_ARCHITECTURE
  dd MB2_LENGTH
  dd MB2_CHECKSUM
  ; info request
  ;dw 1 ; type
  ;dw 0 ; not optional
  ;dd 0 ; size
  ; section definition
  dw 2, 1
  dd 8 + 4*4
  dd MB2_HEADER_START
  dd _kernelStart
  dd _kernelDataEnd
  dd _kernelBssEnd
  ; entry address
  dw 3, 1
  dd 8 + 4
  dd start
  ; page align
  ;dw 6
  ;dw 0
  ;dd 8
  ; end
  dw 0, 0
  dd 8
MB2_HEADER_END:

[SECTION .bootstrap]
[GLOBAL start]
[EXTERN _stack]
[EXTERN Pml4]
[EXTERN Pdpt]
[EXTERN Pd]

failure:
  hlt
  jmp failure

start:
  cmp eax, $MB2_RESPONSE_MAGIC
  jne failure

  cli

  ; bss section isn't 0-filled with grub2, do it now
  mov eax, _kernelDataEnd
  cmp eax, _kernelBssEnd
  jge fillBssEnd
fillBssLoop:
  mov [eax], BYTE 0
  inc eax
  cmp eax, _kernelBssEnd
  jl fillBssLoop
fillBssEnd:

  ; setup 32 bit segmentation
  mov eax, GdtR
  lgdt [eax]

  mov eax, 0x10
  mov ds, ax
  mov ss, ax
  jmp 0x08:enableLong

enableLong:
  mov esp, _stack

  ; setup long mode pagination
  ; identity-map this code and map kernel at 0xffffffff80000000
  mov eax, Pdpt
  bts eax, 0
  mov [Pml4], eax
  mov [Pml4 + 0xFF8], eax

  mov eax, Pd
  bts eax, 0
  mov [Pdpt], eax
  mov [Pdpt + 0xFF0], eax
  ; map heap at 0xffffffffc00000000
  add eax, 0x1000
  mov [Pdpt + 0xFF8], eax

  ; this is last level, 2MB pages
  mov dword [Pd     ], 0x00000083
  mov dword [Pd +  4], 0x0
  mov dword [Pd +  8], 0x00200083
  mov dword [Pd + 12], 0x0
  mov dword [Pd + 16], 0x00400083
  mov dword [Pd + 20], 0x0
  mov dword [Pd + 24], 0x00600083
  mov dword [Pd + 28], 0x0
  mov dword [Pd + 0x1000], 0x00800083
  mov dword [Pd + 0x1004], 0x0

  ; Load CR3 with PML4
  mov eax, Pml4
  mov cr3, eax

  ; Enable PAE
  mov eax, cr4
  bts eax, 5
  mov cr4, eax

  ; Enable Long Mode in the MSR
  mov ecx, 0xC0000080
  rdmsr
  bts eax, 8
  wrmsr

  ; Enable Paging
  mov eax, cr0
  bts eax, 31
  mov cr0, eax

  jmp 0x08:enableLongSeg

enableLongSeg:
  mov eax, GdtRLong

  mov eax, 0x28
  mov ds, ax
  mov ss, ax
  jmp 0x20:high64

[BITS 64]
high64:
  mov rsp, _stack + 0xffffffff80000000
  mov rax, start64 + 0xffffffff80000000
  jmp rax

[EXTERN kmain]
start64:
  mov rdi, rbx
  call kmain+0x80000000

failure64:
  hlt
  jmp failure64

Gdt:
  dq	0x0000000000000000
  dq	0x00CF9A000000FFFF
  dq	0x00CF92000000FFFF
GdtLong:
  dq	0x0000000000000000
  dq	0x0020980000000000
  dq	0x0000920000000000
GdtEnd:

GdtR:
  dw GdtEnd - Gdt - 1
  dd Gdt

GdtRLong:
  dw GdtEnd - GdtLong - 1
  dq GdtLong + 0xffffffff8000000
