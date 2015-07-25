[EXTERN syscallHandler]

[GLOBAL syscall_entry]
;TODO switch to kernel stack
syscall_entry:
  sub rsp, 8 ; ss
  push rsp
  add QWORD [rsp], 8 ; rsp
  push r11 ; rflags
  sub rsp, 8 ; cs
  push rcx ; rip

  sub rsp, 8 + 1 ; intNo + errCode

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

  mov rdi, rsp

  ; align to 16
  mov rcx, rsp
  and rcx, 0xf
  sub rsp, rcx

  sub rsp, 0x200
  fxsave [rsp]

  push rcx

  call syscallHandler

  pop rcx

  fxrstor [rsp]

  add rsp, 0x200
  add rsp, rcx

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

  add rsp, 8 + 1

  pop rcx
  add rsp, 8
  pop r11
  sub QWORD [rsp], 8
  pop rsp
  add rsp, 8

  o64 sysret
