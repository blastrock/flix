%macro ISR_NOERRCODE 1  ; define a macro, taking one parameter
  isr%1:
    sub rsp, 9
    mov qword [rsp+1], 0
    mov byte [rsp], %1
    jmp isr_common_stub
%endmacro

%macro ISR_ERRCODE 1
  isr%1:
    sub rsp, 1
    mov byte [rsp], %1
    jmp isr_common_stub
%endmacro

ISR_NOERRCODE 0
ISR_NOERRCODE 1
ISR_NOERRCODE 2
ISR_NOERRCODE 3
ISR_NOERRCODE 4
ISR_NOERRCODE 5
ISR_NOERRCODE 6
ISR_NOERRCODE 7
ISR_ERRCODE 8
ISR_NOERRCODE 9
ISR_ERRCODE 10
ISR_ERRCODE 11
ISR_ERRCODE 12
ISR_ERRCODE 13
ISR_ERRCODE 14
ISR_NOERRCODE 15
ISR_NOERRCODE 16
ISR_NOERRCODE 17
ISR_NOERRCODE 18
ISR_NOERRCODE 19
ISR_NOERRCODE 20
ISR_NOERRCODE 21
ISR_NOERRCODE 22
ISR_NOERRCODE 23
ISR_NOERRCODE 24
ISR_NOERRCODE 25
ISR_NOERRCODE 26
ISR_NOERRCODE 27
ISR_NOERRCODE 28
ISR_NOERRCODE 29
ISR_NOERRCODE 30
ISR_NOERRCODE 31

ISR_NOERRCODE 32
ISR_NOERRCODE 33
ISR_NOERRCODE 34
ISR_NOERRCODE 35
ISR_NOERRCODE 36
ISR_NOERRCODE 37
ISR_NOERRCODE 38
ISR_NOERRCODE 39
ISR_NOERRCODE 40
ISR_NOERRCODE 41
ISR_NOERRCODE 42
ISR_NOERRCODE 43
ISR_NOERRCODE 44
ISR_NOERRCODE 45
ISR_NOERRCODE 46
ISR_NOERRCODE 47
ISR_NOERRCODE 128

[GLOBAL intVectors]
intVectors:
%assign $i 0
%rep 48
  dq isr%[$i]
%assign $i $i+1
%endrep
  dq isr128

[EXTERN intHandler]

; This is our common ISR stub. It saves the processor state, sets
; up for kernel mode segments, calls the C-level fault handler,
; and finally restores the stack frame.
isr_common_stub:
  push rdi
  push rsi
  push rdx
  push rcx
  push rax
  push r8
  push r9
  push r10
  push r11

  push rbp
  push rbx
  push r12
  push r13
  push r14
  push r15

  ;mov ax, ds               ; Lower 16-bits of eax = ds.
  ;push eax                 ; save the data segment descriptor

  ;mov ax, 0x10  ; load the kernel data segment descriptor
  ;mov ds, ax
  ;mov es, ax
  ;mov fs, ax
  ;mov gs, ax

  ; pass the stack pointer as an argument so that the handler has access to the
  ; machine state before the interrupt and to the interrupt number and error
  ; code
  mov rdi, rsp

  call intHandler

  ;pop eax        ; reload the original data segment descriptor
  ;mov ds, ax
  ;mov es, ax
  ;mov fs, ax
  ;mov gs, ax

  add rsp, 6*8
  pop r11
  pop r10
  pop r9
  pop r8
  pop rax
  pop rcx
  pop rdx
  pop rsi
  pop rdi
  add rsp, 9     ; Cleans up the pushed error code and pushed ISR number
  iretq           ; pops 5 things at once: CS, EIP, EFLAGS, SS, and ESP
