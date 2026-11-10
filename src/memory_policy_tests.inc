; Deterministic policy simulation; never trims user processes or writes settings.
.code
MemoryPolicyTests PROC
  push rbx
  sub rsp,20h
  mov modelStage,1601
  mov memoryPolicyAutoEnabled,0
  mov memoryPolicyThreshold,85
  mov memoryPolicyLastRun,0
  call MemoryPolicyReset
  mov ecx,99
  mov edx,1000
  call MemoryPolicyEvaluate
  test eax,eax
  jnz mptestFail
  mov memoryPolicyAutoEnabled,1
  mov ecx,85
  mov edx,1000
  call MemoryPolicyEvaluate
  test eax,eax
  jnz mptestFail
  mov ecx,85
  mov edx,10999
  call MemoryPolicyEvaluate
  test eax,eax
  jnz mptestFail
  mov ecx,84 ; a dip restarts the ten-second observation
  mov edx,11000
  call MemoryPolicyEvaluate
  mov ecx,85
  mov edx,12000
  call MemoryPolicyEvaluate
  test eax,eax
  jnz mptestFail
  mov ecx,86
  mov edx,22000
  call MemoryPolicyEvaluate
  cmp eax,1
  jne mptestFail
  mov modelStage,1602
  mov ecx,99
  mov edx,700000 ; persistent high pressure cannot retrigger without hysteresis
  call MemoryPolicyEvaluate
  test eax,eax
  jnz mptestFail
  mov ecx,81 ; insufficient rearm dip
  mov edx,701000
  call MemoryPolicyEvaluate
  mov ecx,90
  mov edx,712000
  call MemoryPolicyEvaluate
  test eax,eax
  jnz mptestFail
  mov ecx,80
  mov edx,713000
  call MemoryPolicyEvaluate
  mov ecx,90
  mov edx,714000
  call MemoryPolicyEvaluate
  test eax,eax
  jnz mptestFail
  mov ecx,90
  mov edx,724000
  call MemoryPolicyEvaluate
  cmp eax,1
  jne mptestFail
  mov modelStage,1603
  mov ecx,60
  mov edx,725000
  call MemoryPolicyEvaluate
  mov ecx,99
  mov edx,726000
  call MemoryPolicyEvaluate
  mov ecx,99
  mov edx,740000
  call MemoryPolicyEvaluate
  test eax,eax ; cooldown prevents thrashing
  jnz mptestFail
  mov memoryPolicyThreshold,96
  mov ecx,99
  mov edx,2000000
  call MemoryPolicyEvaluate
  test eax,eax
  jnz mptestFail
  mov modelStage,1604
  ; Public observation APIs with the exact least-privilege collector handle.
  call GetCurrentProcessId
  mov r8d,eax
  mov ecx,1000h
  xor edx,edx
  call OpenProcess
  mov rbx,rax
  test rax,rax
  jz mptestFail
  mov rcx,rbx
  lea rdx,processIo
  call GetProcessIoCounters
  test eax,eax
  jz mptestCloseFail
  mov rcx,rbx
  lea rdx,memoryBuffer
  mov dword ptr [rdx],80
  mov r8d,80
  call GetProcessMemoryInfo
  test eax,eax
  jz mptestCloseFail
  cmp qword ptr [memoryBuffer+16],0
  je mptestCloseFail
  mov rcx,rbx
  call CloseHandle
  call TrimSchedulingTests
  jmp mptestDone
mptestCloseFail:
  mov rcx,rbx
  call CloseHandle
mptestFail:
  mov eax,1
mptestDone:
  mov memoryPolicyAutoEnabled,0
  mov memoryPolicyThreshold,85
  mov memoryPolicyLastRun,0
  mov memoryPolicyHighSince,0
  mov memoryPolicyArmed,1
  add rsp,20h
  pop rbx
  ret
MemoryPolicyTests ENDP
