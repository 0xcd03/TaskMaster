; One application-global optional reader. Requests are copied values, never UI
; pointers. At most one active request, one pending request and one result.
extern AcquireSRWLockExclusive:proc, ReleaseSRWLockExclusive:proc
extern GetProcessHeap:proc, HeapAlloc:proc, HeapFree:proc
extern CreateEventW:proc, CreateThread:proc, SetEvent:proc
extern WaitForSingleObject:proc, CloseHandle:proc, GetLastError:proc
extern PostMessageW:proc, GetTickCount64:proc
.data
align 8
pdOptionalWorkerLock dq 0
pdOptionalWorkerEvent dq 0
pdOptionalWorkerThread dq 0
pdOptionalWorkerResult dq 0
pdOptionalWorkerCollector dq PdCollectOptional
pdOptionalWorkerAllocate dq HeapAlloc ; owned fixtures replace allocation only
pdOptionalWorkerClock dq GetTickCount64 ; acceptance deadline uses a monotonic clock
pdOptionalWorkerFailure dd 0
pdOptionalWorkerStopping dd 0
pdOptionalWorkerPendingReady dd 0
pdOptionalWorkerThreadsCreated dd 0
pdOptionalWorkerCompleted dd 0
pdOptionalWorkerPublished dd 0
pdOptionalWorkerDropped dd 0
pdOptionalWorkerBlocks dd 0
pdOptionalWorkerActive dd 0
.data?
align 8
pdOptionalWorkerPending db PD_REQUEST_SIZE dup(?)
pdOptionalWorkerDesired db PD_REQUEST_SIZE dup(?)
pdOptionalWorkerFailureRequest db PD_REQUEST_SIZE dup(?)
pdOptionalWorkerResultRequest db PD_REQUEST_SIZE dup(?)
.code

; Startup is idempotent. A stopped service cannot spawn a replacement thread.
PdOptionalWorkerEnsure PROC
  push rbx
  sub rsp,30h
  xor ebx,ebx
  lea rcx,pdOptionalWorkerLock
  call AcquireSRWLockExclusive
  cmp pdOptionalWorkerStopping,0
  jne pdoweStopped
  cmp pdOptionalWorkerThread,0
  jne pdoweUnlock
  xor ecx,ecx
  xor edx,edx
  xor r8d,r8d
  xor r9d,r9d
  call CreateEventW
  mov pdOptionalWorkerEvent,rax
  test rax,rax
  jz pdoweError
  xor ecx,ecx
  xor edx,edx
  lea r8,PdOptionalWorkerMain
  xor r9d,r9d
  mov qword ptr [rsp+20h],0
  mov qword ptr [rsp+28h],0
  call CreateThread
  mov pdOptionalWorkerThread,rax
  test rax,rax
  jnz pdoweCreated
  call GetLastError
  mov ebx,eax
  mov rcx,pdOptionalWorkerEvent
  call CloseHandle
  mov pdOptionalWorkerEvent,0
  jmp pdoweUnlock
pdoweCreated:
  inc pdOptionalWorkerThreadsCreated
  jmp pdoweUnlock
pdoweError:
  call GetLastError
  mov ebx,eax
  test ebx,ebx
  jnz pdoweUnlock
  mov ebx,8
  jmp pdoweUnlock
pdoweStopped:
  mov ebx,1115
pdoweUnlock:
  lea rcx,pdOptionalWorkerLock
  call ReleaseSRWLockExclusive
  mov eax,ebx
  add rsp,30h
  pop rbx
  ret
PdOptionalWorkerEnsure ENDP

; RCX request, copied before return. Sequence belongs to the native lane and
; is deliberately ignored here; epoch and absolute deadline are mandatory.
PdOptionalWorkerSubmit PROC
  push rbx
  push rsi
  push rdi
  push r12
  sub rsp,28h
  mov rbx,rcx
  mov r12d,87
  test rbx,rbx
  jz pdowsDone
  cmp dword ptr [rbx+PD_R_SIZE],PD_REQUEST_SIZE
  jne pdowsDone
  cmp dword ptr [rbx+PD_R_VERSION],PD_VERSION
  jne pdowsDone
  cmp dword ptr [rbx+PD_R_FLAGS],0
  jne pdowsDone
  cmp dword ptr [rbx+PD_R_RESERVED],0
  jne pdowsDone
  cmp qword ptr [rbx+PD_R_COOKIE],0
  je pdowsDone
  cmp qword ptr [rbx+PD_R_GENERATION],0
  je pdowsDone
  cmp qword ptr [rbx+PD_R_EPOCH],0
  je pdowsDone
  cmp qword ptr [rbx+PD_R_DEADLINE],0
  je pdowsDone
  cmp qword ptr [rbx+PD_R_NOTIFY],0
  je pdowsDone
  ; Borrowed cancellation/context pointers cannot outlive a copied request.
  cmp qword ptr [rbx+PD_R_CONTEXT],0
  jne pdowsDone
  cmp qword ptr [rbx+PD_R_CANCEL],0
  jne pdowsDone
  cmp dword ptr [rbx+PD_R_LANE],2
  je pdowsStart
  mov r12d,50
  jmp pdowsDone
pdowsStart:
  call PdOptionalWorkerEnsure
  mov r12d,eax
  test eax,eax
  jnz pdowsDone
  lea rcx,pdOptionalWorkerLock
  call AcquireSRWLockExclusive
  cmp pdOptionalWorkerStopping,0
  jne pdowsStopped
  mov rsi,rbx
  lea rdi,pdOptionalWorkerPending
  mov ecx,PD_REQUEST_SIZE/8
  rep movsq
  mov rsi,rbx
  lea rdi,pdOptionalWorkerDesired
  mov ecx,PD_REQUEST_SIZE/8
  rep movsq
  mov pdOptionalWorkerPendingReady,1
  mov rcx,pdOptionalWorkerEvent
  call SetEvent
  xor r12d,r12d
  jmp pdowsUnlock
pdowsStopped:
  mov r12d,1115
pdowsUnlock:
  lea rcx,pdOptionalWorkerLock
  call ReleaseSRWLockExclusive
pdowsDone:
  mov eax,r12d
  add rsp,28h
  pop r12
  pop rdi
  pop rsi
  pop rbx
  ret
PdOptionalWorkerSubmit ENDP

; Transfer the immutable result to caller. Caller validates identity/epoch again.
; An unread result can expire while queued, so its copied request is retained.
; On expiration Take returns zero and exposes fixed timeout via TakeFailure.
PdOptionalWorkerTake PROC
  push rbx
  push rsi
  push rdi
  push r12
  sub rsp,28h
  xor r12d,r12d
  lea rcx,pdOptionalWorkerLock
  call AcquireSRWLockExclusive
  mov rbx,pdOptionalWorkerResult
  test rbx,rbx
  jz pdowtUnlock
  call qword ptr pdOptionalWorkerClock
  cmp rax,qword ptr [pdOptionalWorkerResultRequest+PD_R_DEADLINE]
  jb pdowtTransfer
  mov r12,rbx
  xor ebx,ebx
  lea rsi,pdOptionalWorkerResultRequest
  lea rdi,pdOptionalWorkerFailureRequest
  mov ecx,PD_REQUEST_SIZE/8
  rep movsq
  mov pdOptionalWorkerFailure,1460
pdowtTransfer:
  mov pdOptionalWorkerResult,0
pdowtUnlock:
  lea rcx,pdOptionalWorkerLock
  call ReleaseSRWLockExclusive
  mov rcx,r12
  call PdOptionalWorkerFree
  mov rax,rbx
  add rsp,28h
  pop r12
  pop rdi
  pop rsi
  pop rbx
  ret
PdOptionalWorkerTake ENDP

; RCX writable request copy. Nonzero error means a failure was transferred.
; This fixed mailbox reports even out-of-memory without another allocation.
PdOptionalWorkerTakeFailure PROC
  push rbx
  push rsi
  push rdi
  sub rsp,20h
  mov rdi,rcx
  mov ebx,87
  test rcx,rcx
  jz pdowtfDone
  lea rcx,pdOptionalWorkerLock
  call AcquireSRWLockExclusive
  mov ebx,pdOptionalWorkerFailure
  test ebx,ebx
  jz pdowtfUnlock
  lea rsi,pdOptionalWorkerFailureRequest
  mov ecx,PD_REQUEST_SIZE/8
  rep movsq
  mov pdOptionalWorkerFailure,0
pdowtfUnlock:
  lea rcx,pdOptionalWorkerLock
  call ReleaseSRWLockExclusive
pdowtfDone:
  mov eax,ebx
  add rsp,20h
  pop rdi
  pop rsi
  pop rbx
  ret
PdOptionalWorkerTakeFailure ENDP

PdOptionalWorkerFree PROC
  push rbx
  sub rsp,20h
  mov rbx,rcx
  test rbx,rbx
  jz pdowfDone
  call GetProcessHeap
  mov rcx,rax
  xor edx,edx
  mov r8,rbx
  call HeapFree
  test eax,eax
  jz pdowfDone
  lock dec pdOptionalWorkerBlocks
pdowfDone:
  add rsp,20h
  pop rbx
  ret
PdOptionalWorkerFree ENDP

; RCX cookie. Never waits for the active read or touches an HWND.
PdOptionalWorkerCancel PROC
  push rbx
  push rsi
  sub rsp,28h
  mov rbx,rcx
  xor esi,esi
  lea rcx,pdOptionalWorkerLock
  call AcquireSRWLockExclusive
  cmp rbx,qword ptr [pdOptionalWorkerDesired+PD_R_COOKIE]
  jne pdowcPending
  mov qword ptr [pdOptionalWorkerDesired+PD_R_COOKIE],0
pdowcPending:
  cmp rbx,qword ptr [pdOptionalWorkerPending+PD_R_COOKIE]
  jne pdowcResult
  mov pdOptionalWorkerPendingReady,0
pdowcResult:
  cmp rbx,qword ptr [pdOptionalWorkerFailureRequest+PD_R_COOKIE]
  jne pdowcSnapshot
  mov pdOptionalWorkerFailure,0
pdowcSnapshot:
  mov rax,pdOptionalWorkerResult
  test rax,rax
  jz pdowcUnlock
  cmp rbx,[rax+PD_S_COOKIE]
  jne pdowcUnlock
  mov rsi,rax
  mov pdOptionalWorkerResult,0
pdowcUnlock:
  lea rcx,pdOptionalWorkerLock
  call ReleaseSRWLockExclusive
  mov rcx,rsi
  call PdOptionalWorkerFree
  add rsp,28h
  pop rsi
  pop rbx
  ret
PdOptionalWorkerCancel ENDP

; Asynchronous application shutdown. Worker closes its service handles on exit.
PdOptionalWorkerStop PROC
  push rbx
  sub rsp,20h
  lea rcx,pdOptionalWorkerLock
  call AcquireSRWLockExclusive
  mov pdOptionalWorkerStopping,1
  mov pdOptionalWorkerPendingReady,0
  mov pdOptionalWorkerFailure,0
  mov qword ptr [pdOptionalWorkerDesired+PD_R_COOKIE],0
  mov rbx,pdOptionalWorkerResult
  mov pdOptionalWorkerResult,0
  mov rcx,pdOptionalWorkerEvent
  test rcx,rcx
  jz pdowstopUnlock
  call SetEvent
pdowstopUnlock:
  lea rcx,pdOptionalWorkerLock
  call ReleaseSRWLockExclusive
  mov rcx,rbx
  call PdOptionalWorkerFree
  add rsp,20h
  pop rbx
  ret
PdOptionalWorkerStop ENDP

PdOptionalWorkerMain PROC
  push rbx
  push rsi
  push rdi
  push r12
  push r13
  sub rsp,0B0h
pdowmWait:
  mov rcx,pdOptionalWorkerEvent
  mov edx,-1
  call WaitForSingleObject
pdowmNext:
  lea rcx,pdOptionalWorkerLock
  call AcquireSRWLockExclusive
  cmp pdOptionalWorkerStopping,0
  jne pdowmExitLocked
  cmp pdOptionalWorkerPendingReady,0
  je pdowmIdleLocked
  lea rsi,pdOptionalWorkerPending
  lea rdi,[rsp+40h]
  mov ecx,PD_REQUEST_SIZE/8
  rep movsq
  mov pdOptionalWorkerPendingReady,0
  mov pdOptionalWorkerActive,1
  lea rcx,pdOptionalWorkerLock
  call ReleaseSRWLockExclusive
  ; Do not call an expensive provider for an already expired pending epoch.
  call qword ptr pdOptionalWorkerClock
  cmp rax,[rsp+40h+PD_R_DEADLINE]
  jae pdowmTimedOut
  call GetProcessHeap
  mov rcx,rax
  mov edx,8
  mov r8d,PD_SNAPSHOT_SIZE
  call qword ptr pdOptionalWorkerAllocate
  mov r12,rax
  test rax,rax
  jz pdowmAllocationFailed
  lock inc pdOptionalWorkerBlocks
  lea rcx,[rsp+40h]
  mov rdx,rax
  mov r8d,PD_SNAPSHOT_SIZE
  call qword ptr pdOptionalWorkerCollector
  test eax,eax
  jnz pdowmBackendFailed
  xor r13d,r13d
  lea rcx,pdOptionalWorkerLock
  call AcquireSRWLockExclusive
  cmp pdOptionalWorkerStopping,0
  jne pdowmDropLocked
  ; Completion time limits acceptance, never thread lifetime. Provider calls
  ; may exceed the deadline; their active heap block remains owned until return.
  call qword ptr pdOptionalWorkerClock
  cmp rax,[rsp+40h+PD_R_DEADLINE]
  jae pdowmTimedOutLocked
  ; Native sequence changes cannot invalidate a usable optional epoch.
  mov rax,[rsp+40h+PD_R_COOKIE]
  cmp rax,qword ptr [pdOptionalWorkerDesired+PD_R_COOKIE]
  jne pdowmDropLocked
  mov rax,[rsp+40h+PD_R_GENERATION]
  cmp rax,qword ptr [pdOptionalWorkerDesired+PD_R_GENERATION]
  jne pdowmDropLocked
  mov eax,[rsp+40h+PD_R_PID]
  cmp eax,dword ptr [pdOptionalWorkerDesired+PD_R_PID]
  jne pdowmDropLocked
  mov rax,[rsp+40h+PD_R_BIRTH]
  cmp rax,qword ptr [pdOptionalWorkerDesired+PD_R_BIRTH]
  jne pdowmDropLocked
  mov rax,[rsp+40h+PD_R_EPOCH]
  cmp rax,qword ptr [pdOptionalWorkerDesired+PD_R_EPOCH]
  jne pdowmDropLocked
  mov rax,[rsp+40h+PD_R_NOTIFY]
  cmp rax,qword ptr [pdOptionalWorkerDesired+PD_R_NOTIFY]
  jne pdowmDropLocked
  mov r13,pdOptionalWorkerResult
  mov pdOptionalWorkerResult,r12
  mov pdOptionalWorkerFailure,0
  lea rsi,[rsp+40h]
  lea rdi,pdOptionalWorkerResultRequest
  mov ecx,PD_REQUEST_SIZE/8
  rep movsq
  mov rcx,rax
  mov edx,PD_OPTIONAL_READY_MESSAGE
  xor r8d,r8d
  xor r9d,r9d
  call PostMessageW
  test eax,eax
  jz pdowmPostFailed
  inc pdOptionalWorkerPublished
  xor r12d,r12d
  jmp pdowmPublishedUnlock
pdowmPostFailed:
  mov pdOptionalWorkerResult,0
pdowmDropLocked:
  inc pdOptionalWorkerDropped
pdowmPublishedUnlock:
  lea rcx,pdOptionalWorkerLock
  call ReleaseSRWLockExclusive
  mov rcx,r13
  call PdOptionalWorkerFree
pdowmDiscard:
  mov rcx,r12
  call PdOptionalWorkerFree
  jmp pdowmComplete
pdowmTimedOutLocked:
  lea rcx,pdOptionalWorkerLock
  call ReleaseSRWLockExclusive
  mov rcx,r12
  call PdOptionalWorkerFree
pdowmTimedOut:
  mov eax,1460
  jmp pdowmFailure
pdowmAllocationFailed:
  mov eax,8
  jmp pdowmFailure
pdowmBackendFailed:
  mov [rsp+0A0h],eax
  mov rcx,r12
  call PdOptionalWorkerFree
  mov eax,[rsp+0A0h]
pdowmFailure:
  mov [rsp+0A0h],eax
  xor r13d,r13d
  lea rcx,pdOptionalWorkerLock
  call AcquireSRWLockExclusive
  cmp pdOptionalWorkerStopping,0
  jne pdowmFailureDrop
  call qword ptr pdOptionalWorkerClock
  cmp rax,[rsp+40h+PD_R_DEADLINE]
  jb pdowmFailureKeys
  mov dword ptr [rsp+0A0h],1460
pdowmFailureKeys:
  mov rax,[rsp+40h+PD_R_COOKIE]
  cmp rax,qword ptr [pdOptionalWorkerDesired+PD_R_COOKIE]
  jne pdowmFailureDrop
  mov rax,[rsp+40h+PD_R_GENERATION]
  cmp rax,qword ptr [pdOptionalWorkerDesired+PD_R_GENERATION]
  jne pdowmFailureDrop
  mov eax,[rsp+40h+PD_R_PID]
  cmp eax,dword ptr [pdOptionalWorkerDesired+PD_R_PID]
  jne pdowmFailureDrop
  mov rax,[rsp+40h+PD_R_BIRTH]
  cmp rax,qword ptr [pdOptionalWorkerDesired+PD_R_BIRTH]
  jne pdowmFailureDrop
  mov rax,[rsp+40h+PD_R_EPOCH]
  cmp rax,qword ptr [pdOptionalWorkerDesired+PD_R_EPOCH]
  jne pdowmFailureDrop
  mov rax,[rsp+40h+PD_R_NOTIFY]
  cmp rax,qword ptr [pdOptionalWorkerDesired+PD_R_NOTIFY]
  jne pdowmFailureDrop
  mov r13,pdOptionalWorkerResult
  mov pdOptionalWorkerResult,0
  lea rsi,[rsp+40h]
  lea rdi,pdOptionalWorkerFailureRequest
  mov ecx,PD_REQUEST_SIZE/8
  rep movsq
  mov eax,[rsp+0A0h]
  mov pdOptionalWorkerFailure,eax
  mov rcx,[rsp+40h+PD_R_NOTIFY]
  mov edx,PD_OPTIONAL_READY_MESSAGE
  xor r8d,r8d
  xor r9d,r9d
  call PostMessageW
  test eax,eax
  jz pdowmFailurePostFailed
  inc pdOptionalWorkerPublished
  jmp pdowmFailureUnlock
pdowmFailurePostFailed:
  mov pdOptionalWorkerFailure,0
pdowmFailureDrop:
  inc pdOptionalWorkerDropped
pdowmFailureUnlock:
  lea rcx,pdOptionalWorkerLock
  call ReleaseSRWLockExclusive
  mov rcx,r13
  call PdOptionalWorkerFree
pdowmComplete:
  lea rcx,pdOptionalWorkerLock
  call AcquireSRWLockExclusive
  mov pdOptionalWorkerActive,0
  inc pdOptionalWorkerCompleted
  lea rcx,pdOptionalWorkerLock
  call ReleaseSRWLockExclusive
  jmp pdowmNext
pdowmIdleLocked:
  lea rcx,pdOptionalWorkerLock
  call ReleaseSRWLockExclusive
  jmp pdowmWait
pdowmExitLocked:
  mov rbx,pdOptionalWorkerEvent
  mov r12,pdOptionalWorkerThread
  mov pdOptionalWorkerEvent,0
  mov pdOptionalWorkerThread,0
  mov pdOptionalWorkerActive,0
  lea rcx,pdOptionalWorkerLock
  call ReleaseSRWLockExclusive
  mov rcx,rbx
  call CloseHandle
  mov rcx,r12
  call CloseHandle
  xor eax,eax
  add rsp,0B0h
  pop r13
  pop r12
  pop rdi
  pop rsi
  pop rbx
  ret
PdOptionalWorkerMain ENDP
