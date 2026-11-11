; Owned fixtures: real worker, gated fake sampler, fake clock/allocation boundary.
extern Sleep:proc, GetTickCount64:proc, ResetEvent:proc
.data
align 8
gpwtTick dq 1000
gpwtGate dq 0
gpwtCalls dd 0
gpwtReleased dd 0
gpwtFailAlloc dd 0
gpwtClosed dd 0
gpwtClockPublish dd 0
gpwtStage dd 9100
gpwtAssertions dd 0
gpwtError dd 0
gpwtCopy db GP_HEADER_SIZE dup(0)
.data?
align 8
gpwtAdapters db GP_ADAPTER_COPY_SIZE dup(?)
.code
GpwtClock PROC
  cmp gpwtClockPublish,0
  je gpwtClockReturn
  mov gpwtClockPublish,0
  mov qword ptr [gpWorkerSummary+GP_S_TICK],1001
gpwtClockReturn:
  mov rax,gpwtTick
  ret
GpwtClock ENDP
GpwtAllocate PROC
  cmp gpwtFailAlloc,0
  je HeapAlloc
  xor eax,eax
  ret
GpwtAllocate ENDP
GpwtClose PROC
  inc gpwtClosed
  ret
GpwtClose ENDP
GpwtSampler PROC
  push rbx
  push rsi
  sub rsp,28h
  mov rbx,rcx
  mov dword ptr [rbx+GP_S_SIZE],GP_SNAPSHOT_SIZE
  mov dword ptr [rbx+GP_S_VERSION],GP_VERSION
  mov [rbx+GP_S_SEQUENCE],rdx
  mov [rbx+GP_S_EPOCH],r8
  mov eax,r9d
  mov [rbx+GP_S_COVERAGE],eax
  mov eax,1
  lock xadd gpwtCalls,eax
  lea esi,[rax+1]
gpwtsWait:
  cmp gpwtReleased,esi
  jae gpwtsRead
  mov ecx,1
  call Sleep
  jmp gpwtsWait
gpwtsRead:
  mov rax,gpwtTick
  mov [rbx+GP_S_TICK],rax
  mov dword ptr [rbx+GP_S_UTIL_STATUS],GP_STATUS_VALID
  mov dword ptr [rbx+GP_S_UTIL],2300
  mov dword ptr [rbx+GP_S_ADAPTER_COUNT],1
  mov qword ptr [rbx+GP_S_ADAPTERS+GP_A_LUID],7
  mov dword ptr [rbx+GP_S_ADAPTERS+GP_A_FLAGS],1
  mov dword ptr [rbx+GP_S_ADAPTERS+GP_A_UTIL_STATUS],GP_STATUS_VALID
  xor eax,eax
  add rsp,28h
  pop rsi
  pop rbx
  ret
GpwtSampler ENDP
GpwtWait PROC
  push rbx
  push rsi
  push rdi
  sub rsp,20h
  mov rbx,rcx
  mov esi,edx
  call GetTickCount64
  lea rdi,[rax+5000]
gpwtwLoop:
  cmp dword ptr [rbx],esi
  jae gpwtwPass
  call GetTickCount64
  cmp rax,rdi
  jae gpwtwFail
  mov ecx,1
  call Sleep
  jmp gpwtwLoop
gpwtwFail:
  mov eax,1460
  jmp gpwtwDone
gpwtwPass:
  xor eax,eax
gpwtwDone:
  add rsp,20h
  pop rdi
  pop rsi
  pop rbx
  ret
GpwtWait ENDP
GPWT_ZERO MACRO
  test eax,eax
  jnz gpwtFail
  inc gpwtAssertions
ENDM
GPWT_WAIT MACRO var,expected
  lea rcx,var
  mov edx,expected
  call GpwtWait
  GPWT_ZERO
ENDM
GpuWorkerTest PROC
  push rbx
  push rsi
  sub rsp,38h
  lea rax,GpwtSampler
  mov gpWorkerSampler,rax
  lea rax,GpwtAllocate
  mov gpWorkerAllocator,rax
  lea rax,GpwtClock
  mov gpWorkerClock,rax
  lea rax,GpwtClose
  mov gpWorkerCloser,rax
  mov ecx,GP_MASK_PROCESS ; invalid withoutGLOBAL
  mov edx,1
  call GpWorkerSetDemand
  cmp edx,87
  jne gpwtFail
  cmp gpWorkerThreads,0
  jne gpwtFail
  inc gpwtAssertions
  mov ecx,GP_MASK_ALL
  mov edx,1 ; fake notification recipient, no foreign interaction
  call GpWorkerSetDemand
  test edx,edx
  jnz gpwtFail
  mov rbx,rax
  GPWT_WAIT gpwtCalls,1
  cmp gpWorkerThreads,1
  jne gpwtFail
  inc gpwtAssertions
  mov gpwtStage,9101
  mov ecx,GP_MASK_ALL
  mov edx,1
  call GpWorkerSetDemand
  cmp rax,rbx
  jne gpwtFail
  test edx,edx
  jnz gpwtFail
  inc gpwtAssertions
  xor ecx,ecx
  xor edx,edx
  call GpWorkerSetDemand
  cmp rax,rbx
  jbe gpwtFail
  mov gpwtReleased,1
  GPWT_WAIT gpWorkerDropped,1
  lea rcx,gpwtError
  call GpWorkerTake
  test edx,edx
  jnz gpwtFail
  cmp gpWorkerBlocks,0
  jne gpwtFail
  inc gpwtAssertions
  mov gpwtStage,9102
  mov ecx,GP_MASK_GLOBAL
  mov edx,1
  call GpWorkerSetDemand
  GPWT_WAIT gpwtCalls,2
  mov gpwtReleased,2
  GPWT_WAIT gpWorkerPublished,1
  lea rcx,gpwtCopy
  call GpWorkerCopySummary
  GPWT_ZERO
  cmp dword ptr [gpwtCopy+GP_S_UTIL_STATUS],GP_STATUS_VALID
  jne gpwtFail
  cmp dword ptr [gpwtCopy+GP_S_UTIL],2300
  jne gpwtFail
  inc gpwtAssertions
  lea rcx,gpwtAdapters
  call GpWorkerCopyAdapters
  GPWT_ZERO
  cmp dword ptr [gpwtAdapters+GP_S_ADAPTER_COUNT],1
  jne gpwtFail
  cmp qword ptr [gpwtAdapters+GP_HEADER_SIZE+GP_A_LUID],7
  jne gpwtFail
  inc gpwtAssertions
  ; Model publication between an early clock read and cache copy. Freshness
  ; must read its clock after owning the copy, so the copied older frame is fresh.
  mov gpwtClockPublish,1
  lea rcx,gpwtCopy
  call GpWorkerCopySummary
  GPWT_ZERO
  cmp qword ptr [gpwtCopy+GP_S_TICK],1000
  jne gpwtFail
  inc gpwtAssertions
  mov qword ptr [gpWorkerSummary+GP_S_TICK],1000
  lea rcx,gpwtError
  call GpWorkerTake
  cmp edx,1
  jne gpwtFail
  test rax,rax
  jz gpwtFail
  mov rsi,rax
  lea rcx,gpwtError
  call GpWorkerTake
  test edx,edx
  jnz gpwtFail
  inc gpwtAssertions
  mov gpwtTick,4001
  lea rcx,gpwtCopy
  call GpWorkerCopySummary
  cmp eax,1460
  jne gpwtFail
  cmp dword ptr [gpwtCopy+GP_S_UTIL_STATUS],GP_STATUS_STALE
  jne gpwtFail
  inc gpwtAssertions
  lea rcx,gpwtAdapters
  call GpWorkerCopyAdapters
  cmp eax,1460
  jne gpwtFail
  cmp dword ptr [gpwtAdapters+GP_S_ADAPTER_COUNT],1
  jne gpwtFail
  cmp dword ptr [gpwtAdapters+GP_HEADER_SIZE+GP_A_UTIL_STATUS],GP_STATUS_STALE
  jne gpwtFail
  cmp dword ptr [gpwtAdapters+GP_HEADER_SIZE+GP_A_FLAGS],1
  jne gpwtFail
  inc gpwtAssertions
  mov gpwtStage,9103
  mov gpwtFailAlloc,1
  mov rcx,gpWorkerEvent
  call SetEvent
  GPWT_WAIT gpWorkerFailures,1
  lea rcx,gpwtError
  call GpWorkerTake
  cmp edx,1
  jne gpwtFail
  test rax,rax
  jnz gpwtFail
  cmp gpwtError,8
  jne gpwtFail
  inc gpwtAssertions
  lea rcx,gpwtCopy
  call GpWorkerCopySummary
  cmp eax,8
  jne gpwtFail
  cmp dword ptr [gpwtCopy+GP_S_UTIL_STATUS],GP_STATUS_UNAVAILABLE
  jne gpwtFail
  inc gpwtAssertions
  mov rcx,rsi
  call GpWorkerFree
  ; Recovery and unread replacement retain only one mailbox frame.
  mov gpwtStage,9104
  mov gpwtFailAlloc,0
  mov rcx,gpWorkerEvent
  call SetEvent
  GPWT_WAIT gpwtCalls,3
  mov gpwtReleased,3
  GPWT_WAIT gpWorkerPublished,2
  mov rcx,gpWorkerEvent
  call SetEvent
  GPWT_WAIT gpwtCalls,4
  cmp gpWorkerBlocks,1
  jne gpwtFail
  inc gpwtAssertions
  mov gpwtReleased,4
  GPWT_WAIT gpWorkerPublished,3
  lea rcx,gpwtError
  call GpWorkerTake
  cmp edx,1
  jne gpwtFail
  test rax,rax
  jz gpwtFail
  mov rcx,rax
  call GpWorkerFree
  mov rcx,gpWorkerEvent
  call SetEvent
  GPWT_WAIT gpwtCalls,5
  ; Stop must return while the owned provider is still blocked.
  mov gpwtStage,9104
  call GetTickCount64
  mov rbx,rax
  call GpWorkerStop
  call GetTickCount64
  sub rax,rbx
  cmp rax,250
  jae gpwtFail
  inc gpwtAssertions
  mov gpwtReleased,10000
  mov rcx,gpWorkerThread
  mov edx,5000
  call WaitForSingleObject
  GPWT_ZERO
  cmp gpWorkerBlocks,0
  jne gpwtFail
  cmp gpWorkerThreads,1
  jne gpwtFail
  cmp gpwtClosed,0
  je gpwtFail
  inc gpwtAssertions
  mov ecx,GP_MASK_ALL
  mov edx,1
  call GpWorkerSetDemand
  cmp edx,1115
  jne gpwtFail
  inc gpwtAssertions
  xor ecx,ecx
  call ExitProcess
gpwtFail:
  mov ecx,gpwtStage
  call ExitProcess
GpuWorkerTest ENDP
