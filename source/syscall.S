.section .bss

Lrsp_save: .skip 8

.section .text

.global syscall_entry
syscall_entry:
  mov    %rsp, Lrsp_save
  mov    kernelStack, %rsp
  sub    $0x8, %rsp /* ss */
  pushq  (Lrsp_save)
  push   %r11 /* rflags */
  sub    $0x8, %rsp /* cs */
  push   %rcx /* rip */

  mov    %cs, %rcx
  mov    %rcx, 0x8(%rsp)
  mov    %ss, %rcx
  mov    %rcx, 0x20(%rsp)

  sub    $0x9, %rsp /* intNo + errCode */

  push   %rdi
  push   %rsi
  push   %rdx
  push   %rcx
  push   %rax
  push   %r8
  push   %r9
  push   %r10
  push   %r11

  push   %rbp
  push   %rbx
  push   %r12
  push   %r13
  push   %r14
  push   %r15

  mov    %rsp,%rdi /* pass this as argument to the handler */

  /* align to 16 bytes */
  mov    %rsp, %rcx
  and    $0xf, %rcx
  sub    %rcx, %rsp

  sub    $0x200, %rsp
  fxsave (%rsp)

  push   %rcx

  callq  syscallHandler

  pop    %rcx

  fxrstor (%rsp)

  add    $0x200, %rsp
  add    %rcx, %rsp

  add    $6 * 8, %rsp

  pop    %r11
  pop    %r10
  pop    %r9
  pop    %r8
  pop    %rax
  pop    %rcx
  pop    %rdx
  pop    %rsi
  pop    %rdi

  add    $0x9,%rsp

  pop    %rcx
  add    $0x8,%rsp
  pop    %r11
  pop    %rsp /* this will pop rsp_save, so no need to execute the last line */
  /* add %rsp, $8 */
  sysretq
