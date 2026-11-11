; One application-global native reader. Requests are copied values, never UI
; pointers. At most one active request, one pending request and one result.
extern AcquireSRWLockExclusive:proc, ReleaseSRWLockExclusive:proc
extern GetProcessHeap:proc, HeapAlloc:proc, HeapFree:proc
extern CreateEventW:proc, CreateThread:proc, SetEvent:proc
extern WaitForSingleObject:proc, CloseHandle:proc, GetLastError:proc
extern PostMessageW:proc
.data
align 8
pdWorkerLock dq 0
pdWorkerEvent dq 0
pdWorkerThread dq 0
pdWorkerResult dq 0
pdWorkerCollector dq PdCollectNative
pdWorkerAllocate dq HeapAlloc ; owned allocation-failure fixtures replace only this boundary
pdWorkerFailure dd 0
pdWorkerStopping dd 0
pdWorkerPendingReady dd 0
pdWorkerThreadsCreated dd 0
pdWorkerCompleted dd 0
pdWorkerPublished dd 0
pdWorkerDropped dd 0
pdWorkerBlocks dd 0
pdWorkerActive dd 0
.data?
align 8
pdWorkerPending db PD_REQUEST_SIZE dup(?)
pdWorkerDesired db PD_REQUEST_SIZE dup(?)
pdWorkerFailureRequest db PD_REQUEST_SIZE dup(?)
.code

; Startup is idempotent. A stopped service cannot spawn a replacement thread.
PdWorkerEnsure PROC
  push rbx
  sub rsp,30h
  xor ebx,ebx
  lea rcx,pdWorkerLock
  call AcquireSRWLockExclusive
  cmp pdWorkerStopping,0
  jne pdweStopped
  cmp pdWorkerThread,0
  jne pdweUnlock
  xor ecx,ecx
  xor edx,edx
  xor r8d,r8d
  xor r9d,r9d
  call CreateEventW
  mov pdWorkerEvent,rax
  test rax,rax
  jz pdweError
  xor ecx,ecx
  xor edx,edx
  lea r8,PdWorkerMain
  xor r9d,r9d
  mov qword ptr [rsp+20h],0
  mov qword ptr [rsp+28h],0
  call CreateThread
  mov pdWorkerThread,rax
  test rax,rax
  jnz pdweCreated
  call GetLastError
  mov ebx,eax
  mov rcx,pdWorkerEvent
  call CloseHandle
  mov pdWorkerEvent,0
  jmp pdweUnlock
pdweCreated:
  inc pdWorkerThreadsCreated
  jmp pdweUnlock
pdweError:
  call GetLastError
  mov ebx,eax
  test ebx,ebx
  jnz pdweUnlock
  mov ebx,8
  jmp pdweUnlock
pdweStopped:
  mov ebx,1115
pdweUnlock:
  lea rcx,pdWorkerLock
  call ReleaseSRWLockExclusive
  mov eax,ebx
  add rsp,30h
  pop rbx
  ret
PdWorkerEnsure ENDP

; RCX request, copied before return. Only the native lane is implemented here.
PdWorkerSubmit PROC
  push rbx
  push rsi
  push rdi
  push r12
  sub rsp,28h
  mov rbx,rcx
  mov r12d,87
  test rbx,rbx
  jz pdwsDone
  cmp dword ptr [rbx+PD_R_SIZE],PD_REQUEST_SIZE
  jne pdwsDone
  cmp dword ptr [rbx+PD_R_VERSION],PD_VERSION
  jne pdwsDone
  cmp dword ptr [rbx+PD_R_FLAGS],0
  jne pdwsDone
  cmp dword ptr [rbx+PD_R_RESERVED],0
  jne pdwsDone
  cmp qword ptr [rbx+PD_R_COOKIE],0
  je pdwsDone
  cmp qword ptr [rbx+PD_R_GENERATION],0
  je pdwsDone
  cmp qword ptr [rbx+PD_R_SEQUENCE],0
  je pdwsDone
  cmp qword ptr [rbx+PD_R_NOTIFY],0
  je pdwsDone
  ; Borrowed cancellation/context pointers cannot outlive a copied request.
  cmp qword ptr [rbx+PD_R_CONTEXT],0
  jne pdwsDone
  cmp qword ptr [rbx+PD_R_CANCEL],0
  jne pdwsDone
  cmp dword ptr [rbx+PD_R_LANE],1
  je pdwsStart
  mov r12d,50
  jmp pdwsDone
pdwsStart:
  call PdWorkerEnsure
  mov r12d,eax
  test eax,eax
  jnz pdwsDone
  lea rcx,pdWorkerLock
  call AcquireSRWLockExclusive
  cmp pdWorkerStopping,0
  jne pdwsStopped
  mov rsi,rbx
  lea rdi,pdWorkerPending
  mov ecx,PD_REQUEST_SIZE/8
  rep movsq
  mov rsi,rbx
  lea rdi,pdWorkerDesired
  mov ecx,PD_REQUEST_SIZE/8
  rep movsq
  mov pdWorkerPendingReady,1
  mov rcx,pdWorkerEvent
  call SetEvent
  xor r12d,r12d
  jmp pdwsUnlock
pdwsStopped:
  mov r12d,1115
pdwsUnlock:
  lea rcx,pdWorkerLock
  call ReleaseSRWLockExclusive
pdwsDone:
  mov eax,r12d
  add rsp,28h
  pop r12
  pop rdi
  pop rsi
  pop rbx
  ret
PdWorkerSubmit ENDP

; Transfer the immutable result to caller. Caller validates its keys.
PdWorkerTake PROC
  push rbx
  sub rsp,20h
  lea rcx,pdWorkerLock
  call AcquireSRWLockExclusive
  mov rbx,pdWorkerResult
  mov pdWorkerResult,0
  lea rcx,pdWorkerLock
  call ReleaseSRWLockExclusive
  mov rax,rbx
  add rsp,20h
  pop rbx
  ret
PdWorkerTake ENDP

; RCX writable request copy. Nonzero error means a failure was transferred.
; This fixed mailbox reports even out-of-memory without another allocation.
PdWorkerTakeFailure PROC
  push rbx
  push rsi
  push rdi
  sub rsp,20h
  mov rdi,rcx
  mov ebx,87
  test rcx,rcx
  jz pdwtfDone
  lea rcx,pdWorkerLock
  call AcquireSRWLockExclusive
  mov ebx,pdWorkerFailure
  test ebx,ebx
  jz pdwtfUnlock
  lea rsi,pdWorkerFailureRequest
  mov ecx,PD_REQUEST_SIZE/8
  rep movsq
  mov pdWorkerFailure,0
pdwtfUnlock:
  lea rcx,pdWorkerLock
  call ReleaseSRWLockExclusive
pdwtfDone:
  mov eax,ebx
  add rsp,20h
  pop rdi
  pop rsi
  pop rbx
  ret
PdWorkerTakeFailure ENDP

PdWorkerFree PROC
  push rbx
  sub rsp,20h
  mov rbx,rcx
  test rbx,rbx
  jz pdwfDone
  call GetProcessHeap
  mov rcx,rax
  xor edx,edx
  mov r8,rbx
  call HeapFree
  test eax,eax
  jz pdwfDone
  lock dec pdWorkerBlocks
pdwfDone:
  add rsp,20h
  pop rbx
  ret
PdWorkerFree ENDP

; RCX cookie. Never waits for the active read or touches an HWND.
PdWorkerCancel PROC
  push rbx
  push rsi
  sub rsp,28h
  mov rbx,rcx
  xor esi,esi
  lea rcx,pdWorkerLock
  call AcquireSRWLockExclusive
  cmp rbx,qword ptr [pdWorkerDesired+PD_R_COOKIE]
  jne pdwcPending
  mov qword ptr [pdWorkerDesired+PD_R_COOKIE],0
pdwcPending:
  cmp rbx,qword ptr [pdWorkerPending+PD_R_COOKIE]
  jne pdwcResult
  mov pdWorkerPendingReady,0
pdwcResult:
  cmp rbx,qword ptr [pdWorkerFailureRequest+PD_R_COOKIE]
  jne pdwcSnapshot
  mov pdWorkerFailure,0
pdwcSnapshot:
  mov rax,pdWorkerResult
  test rax,rax
  jz pdwcUnlock
  cmp rbx,[rax+PD_S_COOKIE]
  jne pdwcUnlock
  mov rsi,rax
  mov pdWorkerResult,0
pdwcUnlock:
  lea rcx,pdWorkerLock
  call ReleaseSRWLockExclusive
  mov rcx,rsi
  call PdWorkerFree
  add rsp,28h
  pop rsi
  pop rbx
  ret
PdWorkerCancel ENDP

; Asynchronous application shutdown. Worker closes its service handles on exit.
PdWorkerStop PROC
  push rbx
  sub rsp,20h
  lea rcx,pdWorkerLock
  call AcquireSRWLockExclusive
  mov pdWorkerStopping,1
  mov pdWorkerPendingReady,0
  mov pdWorkerFailure,0
  mov qword ptr [pdWorkerDesired+PD_R_COOKIE],0
  mov rbx,pdWorkerResult
  mov pdWorkerResult,0
  mov rcx,pdWorkerEvent
  test rcx,rcx
  jz pdwstopUnlock
  call SetEvent
pdwstopUnlock:
  lea rcx,pdWorkerLock
  call ReleaseSRWLockExclusive
  mov rcx,rbx
  call PdWorkerFree
  add rsp,20h
  pop rbx
  ret
PdWorkerStop ENDP

PdWorkerMain PROC
  push rbx
  push rsi
  push rdi
  push r12
  push r13
  sub rsp,0B0h
pdwmWait:
  mov rcx,pdWorkerEvent
  mov edx,-1
  call WaitForSingleObject
pdwmNext:
  lea rcx,pdWorkerLock
  call AcquireSRWLockExclusive
  cmp pdWorkerStopping,0
  jne pdwmExitLocked
  cmp pdWorkerPendingReady,0
  je pdwmIdleLocked
  lea rsi,pdWorkerPending
  lea rdi,[rsp+40h]
  mov ecx,PD_REQUEST_SIZE/8
  rep movsq
  mov pdWorkerPendingReady,0
  mov pdWorkerActive,1
  lea rcx,pdWorkerLock
  call ReleaseSRWLockExclusive
  call GetProcessHeap
  mov rcx,rax
  mov edx,8
  mov r8d,PD_SNAPSHOT_SIZE
  call qword ptr pdWorkerAllocate
  mov r12,rax
  test rax,rax
  jz pdwmAllocationFailed
  lock inc pdWorkerBlocks
  lea rcx,[rsp+40h]
  mov rdx,rax
  mov r8d,PD_SNAPSHOT_SIZE
  call qword ptr pdWorkerCollector
  test eax,eax
  jnz pdwmBackendFailed
  xor r13d,r13d
  lea rcx,pdWorkerLock
  call AcquireSRWLockExclusive
  cmp pdWorkerStopping,0
  jne pdwmDropLocked
  ; A newer pending poll must not invalidate a usable active sample.
  mov rax,[rsp+40h+PD_R_COOKIE]
  cmp rax,qword ptr [pdWorkerDesired+PD_R_COOKIE]
  jne pdwmDropLocked
  mov rax,[rsp+40h+PD_R_GENERATION]
  cmp rax,qword ptr [pdWorkerDesired+PD_R_GENERATION]
  jne pdwmDropLocked
  mov eax,[rsp+40h+PD_R_PID]
  cmp eax,dword ptr [pdWorkerDesired+PD_R_PID]
  jne pdwmDropLocked
  mov rax,[rsp+40h+PD_R_BIRTH]
  cmp rax,qword ptr [pdWorkerDesired+PD_R_BIRTH]
  jne pdwmDropLocked
  mov rax,[rsp+40h+PD_R_NOTIFY]
  cmp rax,qword ptr [pdWorkerDesired+PD_R_NOTIFY]
  jne pdwmDropLocked
  mov r13,pdWorkerResult
  mov pdWorkerResult,r12
  mov pdWorkerFailure,0
  mov rcx,rax
  mov edx,PD_READY_MESSAGE
  xor r8d,r8d
  xor r9d,r9d
  call PostMessageW
  test eax,eax
  jz pdwmPostFailed
  inc pdWorkerPublished
  xor r12d,r12d
  jmp pdwmPublishedUnlock
pdwmPostFailed:
  mov pdWorkerResult,0
pdwmDropLocked:
  inc pdWorkerDropped
pdwmPublishedUnlock:
  lea rcx,pdWorkerLock
  call ReleaseSRWLockExclusive
  mov rcx,r13
  call PdWorkerFree
pdwmDiscard:
  mov rcx,r12
  call PdWorkerFree
  jmp pdwmComplete
pdwmAllocationFailed:
  mov eax,8
  jmp pdwmFailure
pdwmBackendFailed:
  mov [rsp+0A0h],eax
  mov rcx,r12
  call PdWorkerFree
  mov eax,[rsp+0A0h]
pdwmFailure:
  mov [rsp+0A0h],eax
  xor r13d,r13d
  lea rcx,pdWorkerLock
  call AcquireSRWLockExclusive
  cmp pdWorkerStopping,0
  jne pdwmFailureDrop
  mov rax,[rsp+40h+PD_R_COOKIE]
  cmp rax,qword ptr [pdWorkerDesired+PD_R_COOKIE]
  jne pdwmFailureDrop
  mov rax,[rsp+40h+PD_R_GENERATION]
  cmp rax,qword ptr [pdWorkerDesired+PD_R_GENERATION]
  jne pdwmFailureDrop
  mov eax,[rsp+40h+PD_R_PID]
  cmp eax,dword ptr [pdWorkerDesired+PD_R_PID]
  jne pdwmFailureDrop
  mov rax,[rsp+40h+PD_R_BIRTH]
  cmp rax,qword ptr [pdWorkerDesired+PD_R_BIRTH]
  jne pdwmFailureDrop
  mov rax,[rsp+40h+PD_R_NOTIFY]
  cmp rax,qword ptr [pdWorkerDesired+PD_R_NOTIFY]
  jne pdwmFailureDrop
  mov r13,pdWorkerResult
  mov pdWorkerResult,0
  lea rsi,[rsp+40h]
  lea rdi,pdWorkerFailureRequest
  mov ecx,PD_REQUEST_SIZE/8
  rep movsq
  mov eax,[rsp+0A0h]
  mov pdWorkerFailure,eax
  mov rcx,[rsp+40h+PD_R_NOTIFY]
  mov edx,PD_READY_MESSAGE
  xor r8d,r8d
  xor r9d,r9d
  call PostMessageW
  test eax,eax
  jz pdwmFailurePostFailed
  inc pdWorkerPublished
  jmp pdwmFailureUnlock
pdwmFailurePostFailed:
  mov pdWorkerFailure,0
pdwmFailureDrop:
  inc pdWorkerDropped
pdwmFailureUnlock:
  lea rcx,pdWorkerLock
  call ReleaseSRWLockExclusive
  mov rcx,r13
  call PdWorkerFree
pdwmComplete:
  lea rcx,pdWorkerLock
  call AcquireSRWLockExclusive
  mov pdWorkerActive,0
  inc pdWorkerCompleted
  lea rcx,pdWorkerLock
  call ReleaseSRWLockExclusive
  jmp pdwmNext
pdwmIdleLocked:
  lea rcx,pdWorkerLock
  call ReleaseSRWLockExclusive
  jmp pdwmWait
pdwmExitLocked:
  mov rbx,pdWorkerEvent
  mov r12,pdWorkerThread
  mov pdWorkerEvent,0
  mov pdWorkerThread,0
  mov pdWorkerActive,0
  lea rcx,pdWorkerLock
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
PdWorkerMain ENDP
