.global jump
jump:
  mov %rdi, %rsp

  pop %r15
  pop %r14
  pop %r13
  pop %r12
  pop %rbx
  pop %rbp

  pop %r11
  pop %r10
  pop %r9
  pop %r8
  pop %rax
  pop %rcx
  pop %rdx
  pop %rsi
  pop %rdi

  iretq

/*
 * saves the current task in the struct pointed by the first argument (rdi)
 * return 0 on save and 1 when the context is restored (same as setjmp)
 */
.global task_save
task_save:
  /* context pointer is in %rdi */
  mov    %r15, 0x00(%rdi)
  mov    %r14, 0x08(%rdi)
  mov    %r13, 0x10(%rdi)
  mov    %r12, 0x18(%rdi)
  mov    %rbx, 0x20(%rdi)
  mov    %rbp, 0x28(%rdi)

  mov    %r11, 0x30(%rdi)
  mov    %r10, 0x38(%rdi)
  mov    %r9 , 0x40(%rdi)
  mov    %r8 , 0x48(%rdi)
  /* mov    %rax, 0x50(%rdi) useless, will be overwritten by return value */
  mov    %rcx, 0x58(%rdi)
  mov    %rdx, 0x60(%rdi)
  mov    %rsi, 0x68(%rdi)
  mov    %rdi, 0x70(%rdi)

  mov    $Ljumpback, %rax
  movq   %rax, 0x78(%rdi)
  mov    %cs,  0x80(%rdi)
  pushfq
  pop    %rax
  mov    %rax, 0x88(%rdi)
  mov    %rsp, 0x90(%rdi)
  mov    %ss,  0x98(%rdi)

  mov    $0x0, %rax
  ret

Ljumpback:
  mov    $0x1, %rax
  ret
