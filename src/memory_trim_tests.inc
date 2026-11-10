; Exercises scheduling only on the current test thread. No user process trim,
; global memory operation, settings write or production collector is invoked.
.code
TrimSchedulingTests PROC
  push rbx
  push rsi
  sub rsp,28h
  mov modelStage,1605
  mov dword ptr [rsp+20h],1
  call GetCurrentThread
  mov rbx,rax
  mov rcx,rax
  call GetThreadPriority
  mov esi,eax
  cmp eax,7FFFFFFFh
  je tstDone
  ; A previously lowered worker must never be raised by the trim policy.
  mov rcx,rbx
  mov edx,-2 ; THREAD_PRIORITY_LOWEST
  call SetThreadPriority
  test eax,eax
  jz tstRestore
  call BeginTrimScheduling
  mov rcx,rbx
  call GetThreadPriority
  cmp eax,-2
  jne tstRestore
  call EndTrimScheduling
  mov rcx,rbx
  call GetThreadPriority
  cmp eax,-2
  jne tstRestore
  ; An ordinary worker is lowered temporarily and restored exactly afterwards.
  mov rcx,rbx
  xor edx,edx
  call SetThreadPriority
  test eax,eax
  jz tstRestore
  call BeginTrimScheduling
  mov rcx,rbx
  call GetThreadPriority
  cmp eax,-1
  jne tstRestore
  call EndTrimScheduling
  mov rcx,rbx
  call GetThreadPriority
  test eax,eax
  jnz tstRestore
  cmp memoryTrimPriorityChanged,0
  jne tstRestore
  mov dword ptr [rsp+20h],0
tstRestore:
  mov rcx,rbx
  mov edx,esi
  call SetThreadPriority
  test eax,eax
  jnz tstRestored
  mov dword ptr [rsp+20h],1
tstRestored:
  mov memoryTrimPriorityChanged,0
tstDone:
  mov eax,[rsp+20h]
  add rsp,28h
  pop rsi
  pop rbx
  ret
TrimSchedulingTests ENDP
