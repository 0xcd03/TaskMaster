; Controlled provider and monotonic clock exercise the production optional
; mailbox, heap ownership, thread bounds and notification path. No user process.
extern GetStdHandle:proc, WriteFile:proc, wsprintfA:proc, ExitProcess:proc
extern GetTickCount64:proc, Sleep:proc, GetCurrentProcess:proc
extern DuplicateHandle:proc, CreateWindowExW:proc, DestroyWindow:proc
extern GetProcessHandleCount:proc
.data
pdowtStage dd 0
pdowtAssertions dd 0
pdowtBytes dd 0
pdowtCalls dd 0
pdowtNativeCalls dd 0
pdowtReleaseCount dd 0
pdowtFailAllocation dd 0
pdowtBackendError dd 0
pdowtHandles dd 0
pdowtHandlesAfter dd 0
pdowtSetupError dd 0
pdowtClassAlignment dd 0
pdowtNow dq 10000
pdowtWindow dq 0
pdowtDuplicate dq 0
pdowtNativeDuplicate dq 0
pdowtFormat db 'Process details optional worker: exit=%u assertions=%u calls=%u nativeReads=%u threads=%u blocks=%u',13,10,0
align 2 ; Win32 probes WCHAR class names with WCHAR alignment.
pdowtClass dw 'S','T','A','T','I','C',0
pdowtSetupFormat db 'Optional worker setup: error=%u classAlignment=%u',13,10,0
.data?
pdowtOutput db 256 dup(?)
align 8
pdowtRequest db PD_REQUEST_SIZE dup(?)
pdowtNativeRequest db PD_REQUEST_SIZE dup(?)
pdowtFailureCopy db PD_REQUEST_SIZE dup(?)
.code
PDOWT_ZERO MACRO
  test eax,eax
  jnz pdowtFail
  inc pdowtAssertions
ENDM
PDOWT_SUBMIT MACRO
  lea rcx,pdowtRequest
  call PdOptionalWorkerSubmit
  PDOWT_ZERO
ENDM
PDOWT_WAIT MACRO counterName,expectedCount
  lea rcx,counterName
  mov edx,expectedCount
  call PdowtWaitCount
  PDOWT_ZERO
ENDM
PDOWT_EPOCH MACRO epochNumber
  mov qword ptr [pdowtRequest+PD_R_EPOCH],epochNumber
ENDM

PdowtWaitCount PROC
  push rbx
  push rsi
  push rdi
  sub rsp,20h
  mov rbx,rcx
  mov esi,edx
  call GetTickCount64
  lea rdi,[rax+3000]
pdowtwcNext:
  cmp dword ptr [rbx],esi
  jae pdowtwcPass
  call GetTickCount64
  cmp rax,rdi
  jae pdowtwcTimeout
  mov ecx,1
  call Sleep
  jmp pdowtwcNext
pdowtwcTimeout:
  mov eax,1460
  jmp pdowtwcDone
pdowtwcPass:
  xor eax,eax
pdowtwcDone:
  add rsp,20h
  pop rdi
  pop rsi
  pop rbx
  ret
PdowtWaitCount ENDP

PdowtClock PROC
  mov rax,pdowtNow
  ret
PdowtClock ENDP

PdowtAllocate PROC
  cmp pdowtFailAllocation,0
  je HeapAlloc
  xor eax,eax
  ret
PdowtAllocate ENDP

; Echo all header identity values after a deterministic provider gate. Keeping
; the real queue/heap/PostMessage makes stale publication observable at Take.
PdowtCollector PROC
  push rbx
  push rsi
  push rdi
  sub rsp,20h
  mov rbx,rcx
  mov rdi,rdx
  mov eax,1
  lock xadd pdowtCalls,eax
  lea esi,[rax+1]
pdowtcGate:
  cmp pdowtReleaseCount,esi
  jae pdowtcRead
  mov ecx,1
  call Sleep
  jmp pdowtcGate
pdowtcRead:
  mov eax,pdowtBackendError
  test eax,eax
  jnz pdowtcDone
  mov rsi,rbx
  mov ecx,PD_REQUEST_SIZE/8
  rep movsq
  sub rdi,PD_REQUEST_SIZE
  mov dword ptr [rdi+PD_S_SIZE],PD_SNAPSHOT_SIZE
  mov dword ptr [rdi+PD_S_TARGET],PD_TARGET_VERIFIED
  xor eax,eax
pdowtcDone:
  add rsp,20h
  pop rdi
  pop rsi
  pop rbx
  ret
PdowtCollector ENDP

PdowtNativeCollector PROC
  push rsi
  push rdi
  sub rsp,28h
  lock inc pdowtNativeCalls
  mov rsi,rcx
  mov rdi,rdx
  mov ecx,PD_REQUEST_SIZE/8
  rep movsq
  sub rdi,PD_REQUEST_SIZE
  mov dword ptr [rdi+PD_S_SIZE],PD_SNAPSHOT_SIZE
  mov dword ptr [rdi+PD_S_TARGET],PD_TARGET_VERIFIED
  xor eax,eax
  add rsp,28h
  pop rdi
  pop rsi
  ret
PdowtNativeCollector ENDP

PdOptionalWorkerTest PROC
  push rbx
  push rsi
  push rdi
  push r12
  sub rsp,68h
  mov pdowtStage,7400
  lea rax,PdowtCollector
  mov pdOptionalWorkerCollector,rax
  lea rax,PdowtClock
  mov pdOptionalWorkerClock,rax
  lea rax,PdowtAllocate
  mov pdOptionalWorkerAllocate,rax
  lea rax,PdowtNativeCollector
  mov pdWorkerCollector,rax
  call PdOptionalWorkerEnsure
  PDOWT_ZERO
  call PdOptionalWorkerEnsure
  PDOWT_ZERO
  call PdWorkerEnsure
  PDOWT_ZERO
  cmp pdOptionalWorkerThreadsCreated,1
  jne pdowtFail
  cmp pdWorkerThreadsCreated,1
  jne pdowtFail
  inc pdowtAssertions
  xor ecx,ecx
  lea rdx,pdowtClass
  xor r8d,r8d
  xor r9d,r9d
  mov qword ptr [rsp+20h],0
  mov qword ptr [rsp+28h],0
  mov qword ptr [rsp+30h],0
  mov qword ptr [rsp+38h],0
  mov qword ptr [rsp+40h],-3
  mov qword ptr [rsp+48h],0
  mov qword ptr [rsp+50h],0
  mov qword ptr [rsp+58h],0
  call CreateWindowExW
  mov pdowtWindow,rax
  test rax,rax
  jnz pdowtWindowReady
  ; Capture at the failing API boundary before Stop/Destroy/reporting can
  ; replace this thread's last error. Diagnostic layout preserves class parity.
  call GetLastError
  mov pdowtSetupError,eax
  lea rax,pdowtClass
  and eax,1
  mov pdowtClassAlignment,eax
  jmp pdowtFail
pdowtWindowReady:
  inc pdowtAssertions
  mov dword ptr [pdowtRequest+PD_R_SIZE],PD_REQUEST_SIZE
  mov dword ptr [pdowtRequest+PD_R_VERSION],PD_VERSION
  mov dword ptr [pdowtRequest+PD_R_LANE],2
  mov qword ptr [pdowtRequest+PD_R_COOKIE],1
  mov qword ptr [pdowtRequest+PD_R_GENERATION],1
  mov qword ptr [pdowtRequest+PD_R_EPOCH],1
  mov dword ptr [pdowtRequest+PD_R_PID],42
  mov qword ptr [pdowtRequest+PD_R_BIRTH],123456
  mov qword ptr [pdowtRequest+PD_R_DEADLINE],13000
  mov qword ptr [pdowtRequest+PD_R_NOTIFY],rax
  lea rsi,pdowtRequest
  lea rdi,pdowtNativeRequest
  mov ecx,PD_REQUEST_SIZE/8
  rep movsq
  mov dword ptr [pdowtNativeRequest+PD_R_LANE],1
  mov qword ptr [pdowtNativeRequest+PD_R_DEADLINE],0
  mov qword ptr [pdowtNativeRequest+PD_R_SEQUENCE],1

  ; Missing validation could retain borrowed pointers or run a wrong lane.
  mov pdowtStage,7401
  mov dword ptr [pdowtRequest+PD_R_LANE],1
  lea rcx,pdowtRequest
  call PdOptionalWorkerSubmit
  cmp eax,50
  jne pdowtFail
  inc pdowtAssertions
  mov dword ptr [pdowtRequest+PD_R_LANE],2
  mov qword ptr [pdowtRequest+PD_R_DEADLINE],0
  lea rcx,pdowtRequest
  call PdOptionalWorkerSubmit
  cmp eax,87
  jne pdowtFail
  inc pdowtAssertions
  mov qword ptr [pdowtRequest+PD_R_DEADLINE],13000
  mov qword ptr [pdowtRequest+PD_R_EPOCH],0
  lea rcx,pdowtRequest
  call PdOptionalWorkerSubmit
  cmp eax,87
  jne pdowtFail
  inc pdowtAssertions
  PDOWT_EPOCH 1
  mov qword ptr [pdowtRequest+PD_R_CONTEXT],1
  lea rcx,pdowtRequest
  call PdOptionalWorkerSubmit
  cmp eax,87
  jne pdowtFail
  inc pdowtAssertions
  mov qword ptr [pdowtRequest+PD_R_CONTEXT],0
  mov qword ptr [pdowtRequest+PD_R_CANCEL],1
  lea rcx,pdowtRequest
  call PdOptionalWorkerSubmit
  cmp eax,87
  jne pdowtFail
  inc pdowtAssertions
  mov qword ptr [pdowtRequest+PD_R_CANCEL],0

  ; 2.5 s controlled optional delay spans two independent one-second native
  ; polls. A native sequence must never invalidate an optional epoch.
  mov pdowtStage,7402
  PDOWT_SUBMIT
  PDOWT_WAIT pdowtCalls,1
  mov esi,1
pdowtNativePoll:
  mov qword ptr [pdowtNativeRequest+PD_R_SEQUENCE],rsi
  lea rcx,pdowtNativeRequest
  call PdWorkerSubmit
  PDOWT_ZERO
  lea rcx,pdWorkerCompleted
  mov edx,esi
  call PdowtWaitCount
  PDOWT_ZERO
  call PdWorkerTake
  mov rbx,rax
  test rax,rax
  jz pdowtFail
  cmp qword ptr [rax+PD_S_SEQUENCE],rsi
  jne pdowtFreeNativeFail
  inc pdowtAssertions
  mov rcx,rbx
  call PdWorkerFree
  add pdowtNow,1000
  inc esi
  cmp esi,3
  jb pdowtNativePoll
  mov pdowtNow,12500
  mov pdowtReleaseCount,1
  PDOWT_WAIT pdOptionalWorkerCompleted,1
  call PdOptionalWorkerTake
  mov rbx,rax
  test rax,rax
  jz pdowtFail
  cmp qword ptr [rax+PD_S_EPOCH],1
  jne pdowtFreeFail
  cmp qword ptr [rax+PD_S_SEQUENCE],0
  jne pdowtFreeFail
  inc pdowtAssertions
  mov rcx,rbx
  call PdOptionalWorkerFree
  cmp pdowtNativeCalls,2
  jne pdowtFail
  inc pdowtAssertions

  ; Refresh invalidates the old epoch and overwrites pending, even if caller
  ; memory is immediately reused. Only the final epoch may be returned.
  mov pdowtStage,7403
  PDOWT_EPOCH 2
  mov qword ptr [pdowtRequest+PD_R_DEADLINE],15500
  PDOWT_SUBMIT
  PDOWT_WAIT pdowtCalls,2
  PDOWT_EPOCH 3
  PDOWT_SUBMIT
  PDOWT_EPOCH 4
  PDOWT_SUBMIT
  PDOWT_EPOCH 999
  mov pdowtReleaseCount,2
  PDOWT_WAIT pdOptionalWorkerCompleted,2
  PDOWT_WAIT pdowtCalls,3
  call PdOptionalWorkerTake
  test rax,rax
  jnz pdowtFreeUnexpected
  inc pdowtAssertions
  mov pdowtReleaseCount,3
  PDOWT_WAIT pdOptionalWorkerCompleted,3
  call PdOptionalWorkerTake
  mov rbx,rax
  test rax,rax
  jz pdowtFail
  cmp qword ptr [rax+PD_S_EPOCH],4
  jne pdowtFreeFail
  inc pdowtAssertions
  mov rcx,rbx
  call PdOptionalWorkerFree

  mov pdowtStage,7404
  mov pdowtReleaseCount,-1
  PDOWT_EPOCH 5
  PDOWT_SUBMIT
  PDOWT_WAIT pdOptionalWorkerCompleted,4
  PDOWT_EPOCH 6
  PDOWT_SUBMIT
  PDOWT_WAIT pdOptionalWorkerCompleted,5
  call PdOptionalWorkerTake
  mov rbx,rax
  test rax,rax
  jz pdowtFail
  cmp qword ptr [rax+PD_S_EPOCH],6
  jne pdowtFreeFail
  inc pdowtAssertions
  mov rcx,rbx
  call PdOptionalWorkerFree
  PDOWT_EPOCH 7
  mov qword ptr [pdowtRequest+PD_R_NOTIFY],1
  PDOWT_SUBMIT
  PDOWT_WAIT pdOptionalWorkerCompleted,6
  call PdOptionalWorkerTake
  test rax,rax
  jnz pdowtFreeUnexpected
  lea rcx,pdowtFailureCopy
  call PdOptionalWorkerTakeFailure
  PDOWT_ZERO
  cmp pdOptionalWorkerBlocks,0
  jne pdowtFail
  inc pdowtAssertions
  mov rax,pdowtWindow
  mov qword ptr [pdowtRequest+PD_R_NOTIFY],rax

  ; An expired pending item must skip both allocation and provider invocation.
  mov pdowtStage,7405
  mov pdowtNow,15000
  mov pdowtReleaseCount,6
  PDOWT_EPOCH 8
  mov qword ptr [pdowtRequest+PD_R_DEADLINE],18000
  PDOWT_SUBMIT
  PDOWT_WAIT pdowtCalls,7
  PDOWT_EPOCH 9
  mov qword ptr [pdowtRequest+PD_R_DEADLINE],15001
  PDOWT_SUBMIT
  mov pdowtNow,16000
  mov pdowtReleaseCount,7
  PDOWT_WAIT pdOptionalWorkerCompleted,8
  cmp pdowtCalls,7
  jne pdowtFail
  inc pdowtAssertions
  lea rcx,pdowtFailureCopy
  call PdOptionalWorkerTakeFailure
  cmp eax,1460
  jne pdowtFail
  cmp qword ptr [pdowtFailureCopy+PD_R_EPOCH],9
  jne pdowtFail
  inc pdowtAssertions

  ; Provider completes at the deadline: its late valid result becomes timeout.
  mov pdowtStage,7406
  PDOWT_EPOCH 10
  mov qword ptr [pdowtRequest+PD_R_DEADLINE],19000
  PDOWT_SUBMIT
  PDOWT_WAIT pdowtCalls,8
  mov pdowtNow,19000
  mov pdowtReleaseCount,8
  PDOWT_WAIT pdOptionalWorkerCompleted,9
  call PdOptionalWorkerTake
  test rax,rax
  jnz pdowtFreeUnexpected
  lea rcx,pdowtFailureCopy
  call PdOptionalWorkerTakeFailure
  cmp eax,1460
  jne pdowtFail
  cmp qword ptr [pdowtFailureCopy+PD_R_EPOCH],10
  jne pdowtFail
  inc pdowtAssertions

  ; A timely publication that sits unread beyond deadline cannot escape Take.
  mov pdowtStage,7407
  mov pdowtNow,20000
  mov pdowtReleaseCount,-1
  PDOWT_EPOCH 11
  mov qword ptr [pdowtRequest+PD_R_DEADLINE],23000
  PDOWT_SUBMIT
  PDOWT_WAIT pdOptionalWorkerCompleted,10
  mov pdowtNow,23000
  call PdOptionalWorkerTake
  test rax,rax
  jnz pdowtFreeUnexpected
  lea rcx,pdowtFailureCopy
  call PdOptionalWorkerTakeFailure
  cmp eax,1460
  jne pdowtFail
  cmp qword ptr [pdowtFailureCopy+PD_R_EPOCH],11
  jne pdowtFail
  cmp pdOptionalWorkerBlocks,0
  jne pdowtFail
  inc pdowtAssertions

  mov pdowtStage,7408
  mov pdowtFailAllocation,1
  PDOWT_EPOCH 12
  mov qword ptr [pdowtRequest+PD_R_DEADLINE],26000
  PDOWT_SUBMIT
  PDOWT_WAIT pdOptionalWorkerCompleted,11
  cmp pdowtCalls,9
  jne pdowtFail
  lea rcx,pdowtFailureCopy
  call PdOptionalWorkerTakeFailure
  cmp eax,8
  jne pdowtFail
  cmp qword ptr [pdowtFailureCopy+PD_R_EPOCH],12
  jne pdowtFail
  inc pdowtAssertions
  lea rcx,pdowtFailureCopy
  call PdOptionalWorkerTakeFailure
  PDOWT_ZERO
  mov pdowtFailAllocation,0
  mov pdowtBackendError,5
  PDOWT_EPOCH 13
  PDOWT_SUBMIT
  PDOWT_WAIT pdOptionalWorkerCompleted,12
  lea rcx,pdowtFailureCopy
  call PdOptionalWorkerTakeFailure
  cmp eax,5
  jne pdowtFail
  cmp qword ptr [pdowtFailureCopy+PD_R_EPOCH],13
  jne pdowtFail
  inc pdowtAssertions
  PDOWT_EPOCH 14
  PDOWT_SUBMIT
  PDOWT_WAIT pdOptionalWorkerCompleted,13
  mov ecx,1
  call PdOptionalWorkerCancel
  lea rcx,pdowtFailureCopy
  call PdOptionalWorkerTakeFailure
  PDOWT_ZERO
  mov pdowtBackendError,0
  PDOWT_EPOCH 15
  PDOWT_SUBMIT
  PDOWT_WAIT pdOptionalWorkerCompleted,14
  lea rcx,pdowtFailureCopy
  call PdOptionalWorkerTakeFailure
  PDOWT_ZERO
  call PdOptionalWorkerTake
  mov rbx,rax
  test rax,rax
  jz pdowtFail
  cmp qword ptr [rax+PD_S_EPOCH],15
  jne pdowtFreeFail
  inc pdowtAssertions
  mov rcx,rbx
  call PdOptionalWorkerFree
  mov pdowtFailAllocation,1
  mov qword ptr [pdowtRequest+PD_R_NOTIFY],1
  PDOWT_SUBMIT
  PDOWT_WAIT pdOptionalWorkerCompleted,15
  lea rcx,pdowtFailureCopy
  call PdOptionalWorkerTakeFailure
  PDOWT_ZERO ; failed notification releases scalar failure ownership, too
  mov pdowtFailAllocation,0
  mov rax,pdowtWindow
  mov qword ptr [pdowtRequest+PD_R_NOTIFY],rax

  ; One blocked provider across 100 close/reopen/Refresh cycles. No additional
  ; worker, request handle or snapshot may accumulate; native still progresses.
  mov pdowtStage,7409
  mov pdowtNow,24000
  mov pdowtReleaseCount,12
  PDOWT_EPOCH 16
  mov qword ptr [pdowtRequest+PD_R_DEADLINE],27000
  PDOWT_SUBMIT
  PDOWT_WAIT pdowtCalls,13
  call GetCurrentProcess
  mov rcx,rax
  lea rdx,pdowtHandles
  call GetProcessHandleCount
  test eax,eax
  jz pdowtFail
  inc pdowtAssertions
  mov esi,2
pdowtReopen:
  mov rcx,qword ptr [pdowtRequest+PD_R_COOKIE]
  call PdOptionalWorkerCancel
  mov qword ptr [pdowtRequest+PD_R_COOKIE],rsi
  PDOWT_SUBMIT
  cmp pdOptionalWorkerThreadsCreated,1
  jne pdowtFail
  cmp pdOptionalWorkerBlocks,1
  jne pdowtFail
  cmp pdOptionalWorkerActive,1
  jne pdowtFail
  cmp pdOptionalWorkerPendingReady,1
  jne pdowtFail
  inc pdowtAssertions
  inc esi
  cmp esi,102
  jb pdowtReopen
  call GetCurrentProcess
  mov rcx,rax
  lea rdx,pdowtHandlesAfter
  call GetProcessHandleCount
  test eax,eax
  jz pdowtFail
  mov eax,pdowtHandles
  cmp eax,pdowtHandlesAfter
  jne pdowtFail
  inc pdowtAssertions
  mov qword ptr [pdowtNativeRequest+PD_R_SEQUENCE],3
  lea rcx,pdowtNativeRequest
  call PdWorkerSubmit
  PDOWT_ZERO
  PDOWT_WAIT pdWorkerCompleted,3
  call PdWorkerTake
  mov rbx,rax
  test rax,rax
  jz pdowtFail
  cmp qword ptr [rax+PD_S_SEQUENCE],3
  jne pdowtFreeNativeFail
  inc pdowtAssertions
  mov rcx,rbx
  call PdWorkerFree
  mov qword ptr [pdowtRequest+PD_R_COOKIE],102
  mov qword ptr [pdowtRequest+PD_R_GENERATION],2
  mov qword ptr [pdowtRequest+PD_R_BIRTH],654321
  PDOWT_EPOCH 17
  PDOWT_SUBMIT
  mov pdowtReleaseCount,13
  PDOWT_WAIT pdOptionalWorkerCompleted,16
  PDOWT_WAIT pdowtCalls,14
  call PdOptionalWorkerTake
  test rax,rax
  jnz pdowtFreeUnexpected
  inc pdowtAssertions
  mov pdowtReleaseCount,14
  PDOWT_WAIT pdOptionalWorkerCompleted,17
  call PdOptionalWorkerTake
  mov rbx,rax
  test rax,rax
  jz pdowtFail
  cmp qword ptr [rax+PD_S_COOKIE],102
  jne pdowtFreeFail
  cmp qword ptr [rax+PD_S_GENERATION],2
  jne pdowtFreeFail
  cmp qword ptr [rax+PD_S_BIRTH],654321
  jne pdowtFreeFail
  cmp qword ptr [rax+PD_S_EPOCH],17
  jne pdowtFreeFail
  inc pdowtAssertions
  mov rcx,rbx
  call PdOptionalWorkerFree

  ; Birth must be a publication key, even when HWND/cookie/gen/epoch coincide.
  mov pdowtStage,7410
  PDOWT_EPOCH 18
  PDOWT_SUBMIT
  PDOWT_WAIT pdowtCalls,15
  mov qword ptr [pdowtRequest+PD_R_BIRTH],987654
  PDOWT_SUBMIT
  mov pdowtReleaseCount,15
  PDOWT_WAIT pdOptionalWorkerCompleted,18
  PDOWT_WAIT pdowtCalls,16
  call PdOptionalWorkerTake
  test rax,rax
  jnz pdowtFreeUnexpected
  inc pdowtAssertions
  mov pdowtReleaseCount,16
  PDOWT_WAIT pdOptionalWorkerCompleted,19
  call PdOptionalWorkerTake
  mov rbx,rax
  test rax,rax
  jz pdowtFail
  cmp qword ptr [rax+PD_S_BIRTH],987654
  jne pdowtFreeFail
  inc pdowtAssertions
  mov rcx,rbx
  call PdOptionalWorkerFree

  ; Error publication needs the epoch check too. A late failure from the old
  ; Refresh must not replace the new epoch's loading state with an error.
  mov pdowtStage,7412
  mov pdowtBackendError,5
  PDOWT_EPOCH 19
  PDOWT_SUBMIT
  PDOWT_WAIT pdowtCalls,17
  PDOWT_EPOCH 20
  PDOWT_SUBMIT
  mov pdowtReleaseCount,17
  PDOWT_WAIT pdOptionalWorkerCompleted,20
  PDOWT_WAIT pdowtCalls,18
  lea rcx,pdowtFailureCopy
  call PdOptionalWorkerTakeFailure
  PDOWT_ZERO
  mov pdowtReleaseCount,18
  PDOWT_WAIT pdOptionalWorkerCompleted,21
  lea rcx,pdowtFailureCopy
  call PdOptionalWorkerTakeFailure
  cmp eax,5
  jne pdowtFail
  cmp qword ptr [pdowtFailureCopy+PD_R_EPOCH],20
  jne pdowtFail
  inc pdowtAssertions
  mov pdowtBackendError,0

  mov pdowtStage,7411
  PDOWT_EPOCH 21
  PDOWT_SUBMIT
  PDOWT_WAIT pdowtCalls,19
  call GetCurrentProcess
  mov r12,rax
  mov rcx,rax
  mov rdx,pdOptionalWorkerThread
  mov r8,rax
  lea r9,pdowtDuplicate
  mov qword ptr [rsp+20h],0
  mov qword ptr [rsp+28h],0
  mov qword ptr [rsp+30h],2
  call DuplicateHandle
  test eax,eax
  jz pdowtFail
  mov rcx,r12
  mov rdx,pdWorkerThread
  mov r8,r12
  lea r9,pdowtNativeDuplicate
  mov qword ptr [rsp+20h],0
  mov qword ptr [rsp+28h],0
  mov qword ptr [rsp+30h],2
  call DuplicateHandle
  test eax,eax
  jz pdowtFail
  inc pdowtAssertions
  call GetTickCount64
  mov r12,rax
  call PdOptionalWorkerStop
  call PdWorkerStop
  call GetTickCount64
  sub rax,r12
  cmp rax,1000
  jae pdowtFail
  inc pdowtAssertions
  call PdOptionalWorkerEnsure
  cmp eax,1115
  jne pdowtFail
  inc pdowtAssertions
  mov pdowtReleaseCount,-1
  mov rcx,pdowtDuplicate
  mov edx,3000
  call WaitForSingleObject
  PDOWT_ZERO
  mov rcx,pdowtNativeDuplicate
  mov edx,3000
  call WaitForSingleObject
  PDOWT_ZERO
  cmp pdOptionalWorkerBlocks,0
  jne pdowtFail
  cmp pdWorkerBlocks,0
  jne pdowtFail
  cmp pdOptionalWorkerThread,0
  jne pdowtFail
  cmp pdOptionalWorkerEvent,0
  jne pdowtFail
  cmp pdOptionalWorkerThreadsCreated,1
  jne pdowtFail
  inc pdowtAssertions
  mov pdowtStage,0
  jmp pdowtReport
pdowtFreeNativeFail:
  mov rcx,rbx
  call PdWorkerFree
  jmp pdowtFail
pdowtFreeUnexpected:
  mov rbx,rax
pdowtFreeFail:
  mov rcx,rbx
  call PdOptionalWorkerFree
pdowtFail:
  mov pdowtReleaseCount,-1
pdowtReport:
  call PdOptionalWorkerStop
  call PdWorkerStop
  mov rcx,pdowtDuplicate
  test rcx,rcx
  jz pdowtCloseNative
  call CloseHandle
pdowtCloseNative:
  mov rcx,pdowtNativeDuplicate
  test rcx,rcx
  jz pdowtDestroy
  call CloseHandle
pdowtDestroy:
  mov rcx,pdowtWindow
  test rcx,rcx
  jz pdowtWrite
  call DestroyWindow
pdowtWrite:
  lea rcx,pdowtOutput
  lea rdx,pdowtSetupFormat
  mov r8d,pdowtSetupError
  mov r9d,pdowtClassAlignment
  call wsprintfA
  mov [rsp+60h],eax
  mov ecx,-11
  call GetStdHandle
  mov rcx,rax
  lea rdx,pdowtOutput
  mov r8d,[rsp+60h]
  lea r9,pdowtBytes
  mov qword ptr [rsp+20h],0
  call WriteFile
  lea rcx,pdowtOutput
  lea rdx,pdowtFormat
  mov r8d,pdowtStage
  mov r9d,pdowtAssertions
  mov eax,pdowtCalls
  mov [rsp+20h],rax
  mov eax,pdowtNativeCalls
  mov [rsp+28h],rax
  mov eax,pdOptionalWorkerThreadsCreated
  add eax,pdWorkerThreadsCreated
  mov [rsp+30h],rax
  mov eax,pdOptionalWorkerBlocks
  add eax,pdWorkerBlocks
  mov [rsp+38h],rax
  call wsprintfA
  mov [rsp+60h],eax
  mov ecx,-11
  call GetStdHandle
  mov rcx,rax
  lea rdx,pdowtOutput
  mov r8d,[rsp+60h]
  lea r9,pdowtBytes
  mov qword ptr [rsp+20h],0
  call WriteFile
  mov ecx,pdowtStage
  call ExitProcess
PdOptionalWorkerTest ENDP
