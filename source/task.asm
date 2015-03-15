[GLOBAL jump]

jump:
  mov rsp, rdi

  pop r15
  pop r14
  pop r13
  pop r12
  pop rbx
  pop rbp

  pop r11
  pop r10
  pop r9
  pop r8
  pop rax
  pop rcx
  pop rdx
  pop rsi
  pop rdi

  iretq
