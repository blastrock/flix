[EXTERN syscallHandler]

[GLOBAL syscall_entry]
syscall_entry:
  push rcx
  call syscallHandler
  pop rcx
  o64 sysret
