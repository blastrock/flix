[EXTERN syscallHandler]

[GLOBAL syscall_entry]
syscall_entry:
  push rcx

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

  pop rcx

  o64 sysret
