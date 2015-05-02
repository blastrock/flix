;
; boot.s -- Kernel start location. Also defines multiboot header.
; Based on Bran's kernel development tutorial file start.asm
;

MB2_HEADER_MAGIC   equ 0xE85250D6
MB2_ARCHITECTURE   equ 0
MB2_LENGTH         equ MB2_HEADER_END - MB2_HEADER_START
MB2_RESPONSE_MAGIC equ 0x36D76289
MB2_CHECKSUM       equ -(MB2_HEADER_MAGIC + MB2_ARCHITECTURE + MB2_LENGTH) & 0xFFFFFFFF

[BITS 32]
[SECTION .mbhdr]
[EXTERN _kernelBootstrapStart]
[EXTERN _kernelDataEnd]
[EXTERN _kernelBssEnd]

ALIGN 8
MB2_HEADER_START:
  dd MB2_HEADER_MAGIC
  dd MB2_ARCHITECTURE
  dd MB2_LENGTH
  dd MB2_CHECKSUM
; no need to specify sections and entry point, will be read from ELF file
; info request
; this part seems to be broken, it only works if 8 < size < 12. It seems to me
; that the correct value should be 12...
;ALIGN 8
;  dw 1, 0 ; not optional
;  dd 12 ; size
;  dd 1 ; request memory mapping
;; page align
;ALIGN 8
;  dw 6
;  dw 0
;  dd 8
  ; end
ALIGN 8
  dw 0, 0
  dd 8
MB2_HEADER_END:

[SECTION .bootstrap]
[GLOBAL start]
[EXTERN Pml4]
[EXTERN Pdpt]
[EXTERN Pd]

failure:
  hlt
  jmp failure

start:
  cli

  cmp eax, $MB2_RESPONSE_MAGIC
  jne failure

  ; copy multiboot header at beginning of heap
.LMBCopyF:
  mov ecx, DWORD [ebx]
  add ecx, ebx
  mov edx, 0xc00008
.LMBCopyFLoop:
  cmp ebx, ecx
  jge .LMBCopyEnd
  mov al, [ebx]
  mov [edx], al
  inc ebx
  inc edx
  jmp .LMBCopyFLoop

.LMBCopyEnd:
  ; bss section isn't 0-filled with grub2, do it now
  mov eax, _kernelDataEnd
  cmp eax, _kernelBssEnd
  jge .LfillBssEnd
.LfillBssLoop:
  mov [eax], BYTE 0
  inc eax
  cmp eax, _kernelBssEnd
  jl .LfillBssLoop
.LfillBssEnd:

  ; setup 32 bit segmentation
  mov eax, .LGdtR
  lgdt [eax]

  mov eax, 0x10
  mov ds, ax
  mov ss, ax
  jmp 0x08:.LenableLong

.LenableLong:
  ; setup long mode pagination
  mov eax, Pdpt
  bts eax, 0
  mov [Pml4], eax
  mov [Pml4 + 0xFF8], eax

  mov eax, Pd
  bts eax, 0
  ; lower memory is identity mapped
  mov [Pdpt], eax
  ; map text, stack and heaps
  add eax, 0x1000
  mov [Pdpt + 0xFF8], eax

  ; this is last level, 2MB pages
  ; identity map first 2MB
  mov dword [Pd     ], 0x00000083
  mov dword [Pd +  4], 0x0
  ; map 2MB+ (the part after the bootstrap) to 0xffffffffc0000000
  mov dword [Pd + 0x1000], 0x00200083
  mov dword [Pd + 0x1004], 0x0
  mov dword [Pd + 0x1008], 0x00400083
  mov dword [Pd + 0x100C], 0x0
  mov dword [Pd + 0x1010], 0x00600083
  mov dword [Pd + 0x1014], 0x0
  ; map stack to 0xffffffffd0000000 (and lower)
  mov dword [Pd + 0x13F8], 0x00800083
  mov dword [Pd + 0x13FC], 0x0
  ; map heaps to 0xffffffffe0000000 and 0xfffffffff0000000
  mov dword [Pd + 0x1800], 0x00A00083
  mov dword [Pd + 0x1804], 0x0
  mov dword [Pd + 0x1C00], 0x00C00083
  mov dword [Pd + 0x1C04], 0x0

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

  jmp 0x08:.LenableLongSeg

.LenableLongSeg:
  mov eax, 0x28
  mov ds, ax
  mov ss, ax
  jmp 0x20:.Lstart64

[BITS 64]
.Lstart64:
  ; set up stack
[EXTERN _stackBase]
  mov rsp, _stackBase
  ; do static initialization
[EXTERN _initArrayBegin]
[EXTERN _initArrayEnd]
  ; r14 and r15 are callee-saved (rsi and rdi are too, but they do not work ><)
  mov r14, _initArrayBegin
  mov r15, _initArrayEnd
.LinitLoop:
  cmp r14, r15
  je .LmainStart
  mov rax, QWORD [r14]
  call rax
  add r14, 8
  jmp .LinitLoop

.LmainStart:
  ; call kmain(multiboot)
[EXTERN kmain]
  mov rdi, 0xfffffffff0000008
  push QWORD 0x0
  mov rax, kmain
  jmp rax

  cli
.Lfailure64:
  hlt
  jmp .Lfailure64

.LGdt:
  dq	0x0000000000000000
  dq	0x00CF9A000000FFFF
  dq	0x00CF92000000FFFF
.LGdtLong:
  dq	0x0000000000000000
  dq	0x0020980000000000
  dq	0x0000920000000000
.LGdtEnd:

.LGdtR:
  dw .LGdtEnd - .LGdt - 1
  dd .LGdt
