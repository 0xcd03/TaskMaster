; Real threads with an explicitly gated owned command collector. No user targets.
.data
pmTestStage dd 9400
pmTestWritten dd 0
pmTestError dd 0
pmTestMode dd 0
pmTestGate dq 0
pmTestEntered dq 0
pmTestClockTick dq 0
pmTestFormat db 'Metadata worker: exit=%u',13,10,0
.data?
pmTestRequest db 96 dup(?)
pmTestTaken db 96 dup(?)
pmTestTimes dq 4 dup(?)
pmTestOutput db 128 dup(?)
.code
PMWT_CHECK MACRO value,expected,stage
  mov pmTestStage,stage
  mov r10d,value
  cmp r10d,expected
  jne pmwtFinish
ENDM
PmWorkerTest PROC
  push rbx
  push rsi
  push rdi
  sub rsp,30h
  mov dword ptr [pmTestRequest+PD_R_SIZE],96
  mov dword ptr [pmTestRequest+PD_R_VERSION],1
  mov qword ptr [pmTestRequest+PD_R_COOKIE],101
  mov qword ptr [pmTestRequest+PD_R_GENERATION],102
  mov qword ptr [pmTestRequest+PD_R_SEQUENCE],103
  mov qword ptr [pmTestRequest+PD_R_EPOCH],104
  mov qword ptr [pmTestRequest+PD_R_NOTIFY],1
  call GetCurrentProcessId
  mov dword ptr [pmTestRequest+PD_R_PID],eax
  call GetCurrentProcess
  mov rcx,rax
  lea rdx,pmTestTimes
  lea r8,pmTestTimes+8
  lea r9,pmTestTimes+16
  lea rax,pmTestTimes+24
  mov [rsp+20h],rax
  call GetProcessTimes
  PMWT_CHECK eax,1,9401
  mov rax,pmTestTimes
  mov qword ptr [pmTestRequest+PD_R_BIRTH],rax
  xor ecx,ecx
  mov edx,1
  xor r8d,r8d
  xor r9d,r9d
  call CreateEventW
  mov pmTestGate,rax
  xor ecx,ecx
  mov edx,1
  xor r8d,r8d
  xor r9d,r9d
  call CreateEventW
  mov pmTestEntered,rax
  lea rax,PmTestGatedCollector
  mov pmWorkerCollectorQ,rax
  lea rax,PmTestGatedFactory
  mov pdOptionalCreateInstance,rax
  mov ecx,3
  call PmTestSubmit
  PMWT_CHECK eax,0,9402
  mov rcx,pmTestEntered
  mov edx,5000
  call WaitForSingleObject
  PMWT_CHECK eax,0,9403
  lea rcx,pmTestRequest
  call PmWorkerSubmit
  PMWT_CHECK eax,170,9404
  mov ecx,1
  call PmTestSubmit
  PMWT_CHECK eax,0,9405
  mov ecx,2
  call PmTestSubmit
  PMWT_CHECK eax,0,9406
  mov ecx,1
  call PmTestWaitTake
  mov rsi,rax
  PMWT_CHECK edx,1,9407
  PMWT_CHECK pmTestError,0,9408
  PMWT_CHECK dword ptr [rsi+PD_S_FIELDS+PD_F_PATH*16],PD_STATUS_VALID,9409
  mov rcx,rsi
  call PmWorkerFree
  mov ecx,2
  call PmTestWaitTake
  mov rsi,rax
  PMWT_CHECK edx,1,9410
  PMWT_CHECK pmTestError,0,9411
  PMWT_CHECK dword ptr [rsi+PD_S_FIELDS+PD_F_ACCOUNT*16],PD_STATUS_VALID,9412
  mov rcx,rsi
  call PmWorkerFree
  mov ecx,3
  call PmWorkerBusy
  PMWT_CHECK eax,1,9413
  mov rcx,pmTestGate
  call SetEvent
  mov ecx,3
  call PmTestWaitTake
  mov rsi,rax
  PMWT_CHECK edx,1,9414
  PMWT_CHECK pmTestError,0,9415
  PMWT_CHECK dword ptr [rsi+PD_S_FIELDS+PD_F_COMMAND*16],PD_STATUS_VALID,9416
  mov rcx,rsi
  call PmWorkerFree
  ; Fixed failure slot survives a failing PostMessage and requires no allocation.
  lea rax,PmTestNoAllocation
  mov pmWorkerAllocateQ,rax
  mov ecx,1
  call PmTestSubmit
  PMWT_CHECK eax,0,9417
  mov ecx,1
  call PmTestWaitTake
  PMWT_CHECK edx,1,9418
  PMWT_CHECK eax,0,9419
  PMWT_CHECK pmTestError,8,9420
  lea rax,HeapAlloc
  mov pmWorkerAllocateQ,rax
  mov pmTestMode,1
  mov ecx,1
  call PmTestSubmit
  PMWT_CHECK eax,0,9421
  mov ecx,1
  call PmTestWaitTake
  PMWT_CHECK edx,1,9422
  PMWT_CHECK pmTestError,299,9423
  mov pmTestMode,0
  ; Pre-admission expiration never calls the backend.
  mov qword ptr [pmTestRequest+PD_R_DEADLINE],1
  lea rcx,pmTestRequest
  call PmWorkerSubmit
  PMWT_CHECK eax,0,9424
  mov ecx,1
  call PmTestWaitTake
  PMWT_CHECK edx,1,9425
  PMWT_CHECK pmTestError,1460,9426
  ; Late success is freed and converted to an immutable timeout failure.
  mov pmTestMode,2
  mov ecx,1
  call PmTestSubmit
  PMWT_CHECK eax,0,9427
  mov ecx,1
  call PmTestWaitTake
  PMWT_CHECK edx,1,9428
  PMWT_CHECK eax,0,9429
  PMWT_CHECK pmTestError,1460,9430
  mov pmTestMode,0
  lea rax,GetTickCount64
  mov pmWorkerClockQ,rax
  PMWT_CHECK dword ptr [pmWorkerContexts+PM_W_THREADS],1,9431
  PMWT_CHECK dword ptr [pmWorkerContexts+256+PM_W_THREADS],1,9432
  PMWT_CHECK dword ptr [pmWorkerContexts+512+PM_W_THREADS],1,9433
  ; Stop while provider is gated returns without waiting or restarting.
  mov rcx,pmTestGate
  call ResetEvent
  mov rcx,pmTestEntered
  call ResetEvent
  mov ecx,3
  call PmTestSubmit
  PMWT_CHECK eax,0,9434
  mov rcx,pmTestEntered
  mov edx,5000
  call WaitForSingleObject
  PMWT_CHECK eax,0,9435
  call PmWorkerStop
  lea rcx,pmTestRequest
  call PmWorkerSubmit
  PMWT_CHECK eax,1115,9436
  mov rcx,pmTestGate
  call SetEvent
  mov pmTestStage,0
pmwtFinish:
  call PmWorkerStop
  mov rcx,pmTestGate
  test rcx,rcx
  jz pmwtReport
  call SetEvent
pmwtReport:
  lea rcx,pmTestOutput
  lea rdx,pmTestFormat
  mov r8d,pmTestStage
  call wsprintfA
  mov ebx,eax
  mov ecx,-11
  call GetStdHandle
  mov rcx,rax
  lea rdx,pmTestOutput
  mov r8d,ebx
  lea r9,pmTestWritten
  mov qword ptr [rsp+20h],0
  call WriteFile
  mov ecx,pmTestStage
  call ExitProcess
PmWorkerTest ENDP

PmTestSubmit PROC
  sub rsp,28h
  mov dword ptr [pmTestRequest+PD_R_LANE],ecx
  call GetTickCount64
  add rax,3000
  mov qword ptr [pmTestRequest+PD_R_DEADLINE],rax
  inc qword ptr [pmTestRequest+PD_R_SEQUENCE]
  lea rcx,pmTestRequest
  call PmWorkerSubmit
  add rsp,28h
  ret
PmTestSubmit ENDP

PmTestWaitTake PROC
  push rbx
  push rsi
  sub rsp,28h
  mov ebx,ecx
  mov esi,5000
pmwttLoop:
  mov ecx,ebx
  lea rdx,pmTestTaken
  lea r8,pmTestError
  call PmWorkerTake
  test edx,edx
  jnz pmwttDone
  mov ecx,1
  call Sleep
  dec esi
  jnz pmwttLoop
  xor eax,eax
  xor edx,edx
pmwttDone:
  add rsp,28h
  pop rsi
  pop rbx
  ret
PmTestWaitTake ENDP

PmTestNoAllocation PROC
  xor eax,eax
  ret
PmTestNoAllocation ENDP
PmTestClock PROC
  mov rax,pmTestClockTick
  ret
PmTestClock ENDP
PmTestGatedCollector PROC
  push rbx
  push rsi
  push rdi
  sub rsp,20h
  mov rbx,rcx
  mov rsi,rdx
  mov edi,r8d
pmwgcRead:
  mov eax,299
  cmp pmTestMode,1
  je pmwgcDone
  mov rcx,rbx
  mov rdx,rsi
  mov r8d,edi
  call PmCollectField
  cmp pmTestMode,2
  jne pmwgcDone
  mov rdx,[rbx+PD_R_DEADLINE]
  mov pmTestClockTick,rdx
  lea rdx,PmTestClock
  mov pmWorkerClockQ,rdx
pmwgcDone:
  add rsp,20h
  pop rdi
  pop rsi
  pop rbx
  ret
PmTestGatedCollector ENDP

; Gate at the real provider boundary, after held-handle identity verification.
PmTestGatedFactory PROC
  sub rsp,68h
  mov [rsp+30h],rcx
  mov [rsp+38h],rdx
  mov [rsp+40h],r8
  mov [rsp+48h],r9
  mov rcx,pmTestEntered
  call SetEvent
  mov rcx,pmTestGate
  mov edx,10000
  call WaitForSingleObject
  mov rcx,[rsp+30h]
  mov rdx,[rsp+38h]
  mov r8,[rsp+40h]
  mov r9,[rsp+48h]
  mov rax,[rsp+90h]
  mov [rsp+20h],rax
  call CoCreateInstance
  add rsp,68h
  ret
PmTestGatedFactory ENDP
