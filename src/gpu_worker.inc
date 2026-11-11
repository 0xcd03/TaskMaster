; Single sampler owns provider state. Only copied immutable frames cross threads.
extern AcquireSRWLockExclusive:proc, ReleaseSRWLockExclusive:proc
extern GetProcessHeap:proc, HeapAlloc:proc, HeapFree:proc, GetTickCount64:proc
extern CreateEventW:proc, CreateThread:proc, SetEvent:proc, CloseHandle:proc
extern WaitForSingleObject:proc, GetLastError:proc, PostMessageW:proc
.data
align 8
gpWorkerLock dq 0
gpWorkerEvent dq 0
gpWorkerThread dq 0
gpWorkerEpoch dq 0
gpWorkerNotify dq 0
gpWorkerReady dq 0
gpWorkerSequence dq 0
gpWorkerSampler dq GpProviderSample
gpWorkerCloser dq GpProviderClose
gpWorkerAllocator dq HeapAlloc
gpWorkerClock dq GetTickCount64
gpWorkerContext dq 0
gpWorkerMask dd 0
gpWorkerStopping dd 0
gpWorkerThreads dd 0
gpWorkerBlocks dd 0
gpWorkerDropped dd 0
gpWorkerPublished dd 0
gpWorkerFailures dd 0
gpWorkerFailure dd 0
gpWorkerSummaryError dd 21
gpWorkerSummary db GP_HEADER_SIZE dup(0)
.data?
align 8
gpWorkerAdapters db GP_ADAPTER_CAPACITY*GP_ADAPTER_SIZE dup(?)
.code
; Leaf: RCX output256, EDX status, R8D error, R9 epoch. No provider/heap.
GpWorkerClearSummary PROC
  push rdi
  mov r10,rcx
  mov rdi,rcx
  mov ecx,GP_HEADER_SIZE/8
  xor eax,eax
  rep stosq
  mov dword ptr [r10+GP_S_SIZE],GP_SNAPSHOT_SIZE
  mov dword ptr [r10+GP_S_VERSION],GP_VERSION
  mov [r10+GP_S_EPOCH],r9
  mov [r10+GP_S_UTIL_STATUS],edx
  mov [r10+GP_S_UTIL_ERROR],r8d
  mov [r10+GP_S_DEDICATED_STATUS],edx
  mov [r10+GP_S_DEDICATED_ERROR],r8d
  mov [r10+GP_S_SHARED_STATUS],edx
  mov [r10+GP_S_SHARED_ERROR],r8d
  xor eax,eax
gpwcsLane:
  mov [r10+rax*8+GP_S_SOURCES],edx
  mov [r10+rax*8+GP_S_SOURCES+4],r8d
  inc eax
  cmp eax,GP_SOURCES
  jb gpwcsLane
  pop rdi
  ret
GpWorkerClearSummary ENDP

; Age only measurements; retain capture tags, counts and metadata diagnostics.
GpWorkerMarkStale PROC
  mov dword ptr [rcx+GP_S_UTIL_STATUS],GP_STATUS_STALE
  mov dword ptr [rcx+GP_S_UTIL_ERROR],1460
  mov dword ptr [rcx+GP_S_DEDICATED_STATUS],GP_STATUS_STALE
  mov dword ptr [rcx+GP_S_DEDICATED_ERROR],1460
  mov dword ptr [rcx+GP_S_SHARED_STATUS],GP_STATUS_STALE
  mov dword ptr [rcx+GP_S_SHARED_ERROR],1460
  mov dword ptr [rcx+GP_S_UTIL],0
  mov qword ptr [rcx+GP_S_DEDICATED],0
  mov qword ptr [rcx+GP_S_SHARED],0
  xor eax,eax
gpwmsSource:
  mov dword ptr [rcx+rax*8+GP_S_SOURCES],GP_STATUS_STALE
  mov dword ptr [rcx+rax*8+GP_S_SOURCES+4],1460
  inc eax
  cmp eax,GP_SOURCES
  jb gpwmsSource
  ret
GpWorkerMarkStale ENDP

GpWorkerEnsure PROC
  push rbx
  sub rsp,30h
  xor ebx,ebx
  lea rcx,gpWorkerLock
  call AcquireSRWLockExclusive
  cmp gpWorkerStopping,0
  jne gpweStopped
  cmp gpWorkerThread,0
  jne gpweUnlock
  xor ecx,ecx
  xor edx,edx
  xor r8d,r8d
  xor r9d,r9d
  call CreateEventW
  test rax,rax
  jz gpweError
  mov gpWorkerEvent,rax
  xor ecx,ecx
  xor edx,edx
  lea r8,GpWorkerMain
  xor r9d,r9d
  mov qword ptr [rsp+20h],0
  mov qword ptr [rsp+28h],0
  call CreateThread
  test rax,rax
  jz gpweThreadError
  mov gpWorkerThread,rax
  inc gpWorkerThreads
  jmp gpweUnlock
gpweThreadError:
  call GetLastError
  mov ebx,eax
  mov rcx,gpWorkerEvent
  call CloseHandle
  mov gpWorkerEvent,0
  jmp gpweUnlock
gpweError:
  call GetLastError
  mov ebx,eax
  test ebx,ebx
  jnz gpweUnlock
  mov ebx,8
  jmp gpweUnlock
gpweStopped:
  mov ebx,1115
gpweUnlock:
  lea rcx,gpWorkerLock
  call ReleaseSRWLockExclusive
  mov eax,ebx
  add rsp,30h
  pop rbx
  ret
GpWorkerEnsure ENDP

GpWorkerFree PROC
  push rbx
  sub rsp,20h
  mov rbx,rcx
  test rbx,rbx
  jz gpwfDone
  call GetProcessHeap
  mov rcx,rax
  xor edx,edx
  mov r8,rbx
  call HeapFree
  test eax,eax
  jz gpwfDone
  lock dec gpWorkerBlocks
gpwfDone:
  add rsp,20h
  pop rbx
  ret
GpWorkerFree ENDP

; Demand transitions invalidate ready/error/summary before returning to UI.
GpWorkerSetDemand PROC
  push rbx
  push rsi
  push rdi
  sub rsp,20h
  mov ebx,ecx
  mov rsi,rdx
  xor edi,edi
  cmp ebx,GP_MASK_ALL
  ja gpwsdArgument
  test ebx,ebx
  jz gpwsdLock
  test ebx,GP_MASK_GLOBAL
  jz gpwsdArgument
  test rsi,rsi
  jz gpwsdArgument
  call GpWorkerEnsure
  test eax,eax
  jnz gpwsdError
gpwsdLock:
  lea rcx,gpWorkerLock
  call AcquireSRWLockExclusive
  cmp gpWorkerStopping,0
  jne gpwsdStopped
  cmp ebx,gpWorkerMask
  jne gpwsdChange
  cmp rsi,gpWorkerNotify
  je gpwsdUnchanged
gpwsdChange:
  mov gpWorkerMask,ebx
  mov gpWorkerNotify,rsi
  inc gpWorkerEpoch
  jnz gpwsdEpoch
  inc gpWorkerEpoch
gpwsdEpoch:
  mov rdi,gpWorkerReady
  mov gpWorkerReady,0
  mov gpWorkerFailure,0
  mov gpWorkerSummaryError,21
  lea rcx,gpWorkerSummary
  mov edx,GP_STATUS_MISSING
  mov r8d,21
  mov r9,gpWorkerEpoch
  call GpWorkerClearSummary
  mov rcx,gpWorkerEvent
  test rcx,rcx
  jz gpwsdUnchanged
  call SetEvent
gpwsdUnchanged:
  mov rbx,gpWorkerEpoch
  xor esi,esi
  jmp gpwsdUnlock
gpwsdStopped:
  mov rbx,gpWorkerEpoch
  mov esi,1115
gpwsdUnlock:
  lea rcx,gpWorkerLock
  call ReleaseSRWLockExclusive
  mov rcx,rdi
  call GpWorkerFree
  mov rax,rbx
  mov edx,esi
  jmp gpwsdDone
gpwsdArgument:
  mov eax,87
gpwsdError:
  mov edx,eax
  mov rax,gpWorkerEpoch
gpwsdDone:
  add rsp,20h
  pop rdi
  pop rsi
  pop rbx
  ret
GpWorkerSetDemand ENDP

GpWorkerTake PROC
  push rbx
  push rsi
  push rdi
  sub rsp,20h
  mov rdi,rcx
  xor ebx,ebx
  xor esi,esi
  test rdi,rdi
  jz gpwtDone
  mov dword ptr [rdi],0
  lea rcx,gpWorkerLock
  call AcquireSRWLockExclusive
  mov rbx,gpWorkerReady
  test rbx,rbx
  jz gpwtFailure
  mov gpWorkerReady,0
  mov esi,1
  jmp gpwtUnlock
gpwtFailure:
  mov eax,gpWorkerFailure
  test eax,eax
  jz gpwtUnlock
  mov [rdi],eax
  mov gpWorkerFailure,0
  mov esi,1
gpwtUnlock:
  lea rcx,gpWorkerLock
  call ReleaseSRWLockExclusive
gpwtDone:
  mov rax,rbx
  mov edx,esi
  add rsp,20h
  pop rdi
  pop rsi
  pop rbx
  ret
GpWorkerTake ENDP

GpWorkerCopySummary PROC
  push rbx
  push rsi
  push rdi
  push r12
  sub rsp,28h
  mov rbx,rcx
  mov r12d,87
  test rbx,rbx
  jz gpwcDone
  lea rcx,gpWorkerLock
  call AcquireSRWLockExclusive
  mov rdi,rbx
  lea rsi,gpWorkerSummary
  mov ecx,GP_HEADER_SIZE/8
  rep movsq
  mov edi,gpWorkerSummaryError
  lea rcx,gpWorkerLock
  call ReleaseSRWLockExclusive
  test edi,edi
  jnz gpwcResult
  call qword ptr gpWorkerClock
  sub rax,[rbx+GP_S_TICK]
  jc gpwcStale
  cmp rax,GP_STALE_MS
  jbe gpwcResult
gpwcStale:
  mov edi,1460
  mov rcx,rbx
  call GpWorkerMarkStale
gpwcResult:
  mov r12d,edi
gpwcDone:
  mov eax,r12d
  add rsp,28h
  pop r12
  pop rdi
  pop rsi
  pop rbx
  ret
GpWorkerCopySummary ENDP

; Small copied adapter cache; neither consumer nor worker retains framepointer.
GpWorkerCopyAdapters PROC
  push rbx
  push rsi
  push rdi
  push r12
  push r13
  sub rsp,20h
  mov rbx,rcx
  mov r12d,87
  test rbx,rbx
  jz gpwcaDone
  mov rdi,rbx
  xor eax,eax
  mov ecx,GP_ADAPTER_COPY_SIZE/8
  rep stosq
  lea rcx,gpWorkerLock
  call AcquireSRWLockExclusive
  mov rdi,rbx
  lea rsi,gpWorkerSummary
  mov ecx,GP_HEADER_SIZE/8
  rep movsq
  mov r13d,[rbx+GP_S_ADAPTER_COUNT]
  cmp r13d,GP_ADAPTER_CAPACITY
  ja gpwcaInvalid
  lea rsi,gpWorkerAdapters
  mov ecx,r13d
  shl ecx,6 ; 512 / 8
  rep movsq
  mov edi,gpWorkerSummaryError
  lea rcx,gpWorkerLock
  call ReleaseSRWLockExclusive
  test edi,edi
  jnz gpwcaResult
  call qword ptr gpWorkerClock
  sub rax,[rbx+GP_S_TICK]
  jc gpwcaStale
  cmp rax,GP_STALE_MS
  jbe gpwcaResult
gpwcaStale:
  mov edi,1460
  mov rcx,rbx
  call GpWorkerMarkStale
  lea rdx,[rbx+GP_HEADER_SIZE]
  mov ecx,r13d
gpwcaStaleAdapter:
  test ecx,ecx
  jz gpwcaResult
  mov dword ptr [rdx+GP_A_UTIL_STATUS],GP_STATUS_STALE
  mov dword ptr [rdx+GP_A_UTIL_ERROR],1460
  mov dword ptr [rdx+GP_A_DEDICATED_STATUS],GP_STATUS_STALE
  mov dword ptr [rdx+GP_A_DEDICATED_ERROR],1460
  mov dword ptr [rdx+GP_A_SHARED_STATUS],GP_STATUS_STALE
  mov dword ptr [rdx+GP_A_SHARED_ERROR],1460
  add rdx,GP_ADAPTER_SIZE
  dec ecx
  jmp gpwcaStaleAdapter
gpwcaInvalid:
  mov edi,13
  mov dword ptr [rbx+GP_S_ADAPTER_COUNT],0
gpwcaUnlock:
  lea rcx,gpWorkerLock
  call ReleaseSRWLockExclusive
gpwcaResult:
  mov r12d,edi
gpwcaDone:
  mov eax,r12d
  add rsp,20h
  pop r13
  pop r12
  pop rdi
  pop rsi
  pop rbx
  ret
GpWorkerCopyAdapters ENDP

GpWorkerStop PROC
  push rbx
  sub rsp,20h
  lea rcx,gpWorkerLock
  call AcquireSRWLockExclusive
  mov gpWorkerStopping,1
  mov gpWorkerMask,0
  mov gpWorkerNotify,0
  inc gpWorkerEpoch
  mov rbx,gpWorkerReady
  mov gpWorkerReady,0
  mov gpWorkerFailure,0
  mov gpWorkerSummaryError,1115
  lea rcx,gpWorkerSummary
  mov edx,GP_STATUS_UNAVAILABLE
  mov r8d,1115
  mov r9,gpWorkerEpoch
  call GpWorkerClearSummary
  mov rcx,gpWorkerEvent
  test rcx,rcx
  jz gpwstUnlock
  call SetEvent
gpwstUnlock:
  lea rcx,gpWorkerLock
  call ReleaseSRWLockExclusive
  mov rcx,rbx
  call GpWorkerFree
  add rsp,20h
  pop rbx
  ret
GpWorkerStop ENDP

GpWorkerMain PROC
  push rbx
  push rsi
  push rdi
  push r12
  push r13
  push r14
  push r15
  sub rsp,50h
  xor r15d,r15d ; provider's epoch
gpwmLoop:
  mov rcx,gpWorkerEvent
  xor edx,edx
  call WaitForSingleObject ; consume the demand wake before starting this job
  lea rcx,gpWorkerLock
  call AcquireSRWLockExclusive
  cmp gpWorkerStopping,0
  jne gpwmExitLocked
  mov r12,gpWorkerEpoch
  mov r13d,gpWorkerMask
  mov r14,gpWorkerNotify
  lea rcx,gpWorkerLock
  call ReleaseSRWLockExclusive
  cmp r15,r12
  je gpwmDemand
  call qword ptr gpWorkerCloser
  mov r15,r12
gpwmDemand:
  test r13d,r13d
  jz gpwmIdle
  ; Remove unread ready before allocation so even the replacement transition
  ; cannot temporarily create a fourth owned frame alongside UI front.
  lea rcx,gpWorkerLock
  call AcquireSRWLockExclusive
  mov rbx,gpWorkerReady
  mov gpWorkerReady,0
  lea rcx,gpWorkerLock
  call ReleaseSRWLockExclusive
  mov rcx,rbx
  call GpWorkerFree
  call GetProcessHeap
  mov rcx,rax
  mov edx,8
  mov r8d,GP_SNAPSHOT_SIZE
  call qword ptr gpWorkerAllocator
  mov rbx,rax
  mov esi,8
  test rbx,rbx
  jz gpwmPublish
  lock inc gpWorkerBlocks
  inc gpWorkerSequence
  mov rcx,rbx
  mov rdx,gpWorkerSequence
  mov r8,r12
  mov r9d,r13d
  call qword ptr gpWorkerSampler
  mov esi,eax
  ; A successful sampler must still return the frozen publication ABI/tags.
  test esi,esi
  jnz gpwmPublish
  cmp dword ptr [rbx+GP_S_SIZE],GP_SNAPSHOT_SIZE
  jne gpwmBadFrame
  cmp dword ptr [rbx+GP_S_VERSION],GP_VERSION
  jne gpwmBadFrame
  cmp [rbx+GP_S_EPOCH],r12
  jne gpwmBadFrame
  mov rax,gpWorkerSequence
  cmp [rbx+GP_S_SEQUENCE],rax
  jne gpwmBadFrame
  cmp dword ptr [rbx+GP_S_ADAPTER_COUNT],GP_ADAPTER_CAPACITY
  ja gpwmBadFrame
  cmp dword ptr [rbx+GP_S_PROCESS_COUNT],GP_PROCESS_CAPACITY
  ja gpwmBadFrame
  cmp dword ptr [rbx+GP_S_ENGINE_COUNT],GP_ENGINE_CAPACITY
  jbe gpwmPublish
gpwmBadFrame:
  mov esi,13
gpwmPublish:
  call qword ptr gpWorkerClock
  mov [rsp+40h],rax
  lea rcx,gpWorkerLock
  call AcquireSRWLockExclusive
  cmp gpWorkerStopping,0
  jne gpwmDrop
  cmp r12,gpWorkerEpoch
  jne gpwmDrop
  cmp r13d,gpWorkerMask
  jne gpwmDrop
  test esi,esi
  jnz gpwmFailure
  mov gpWorkerReady,rbx
  mov rsi,rbx
  lea rdi,gpWorkerSummary
  mov ecx,GP_HEADER_SIZE/8
  rep movsq
  lea rsi,[rbx+GP_S_ADAPTERS]
  lea rdi,gpWorkerAdapters
  mov ecx,[rbx+GP_S_ADAPTER_COUNT]
  shl ecx,6
  rep movsq
  mov gpWorkerSummaryError,0
  mov gpWorkerFailure,0
  xor ebx,ebx ; mailbox now owns frame
  inc gpWorkerPublished
  jmp gpwmNotify
gpwmFailure:
  mov gpWorkerFailure,esi
  mov gpWorkerSummaryError,esi
  lea rcx,gpWorkerSummary
  mov edx,GP_STATUS_UNAVAILABLE
  mov r8d,esi
  mov r9,r12
  call GpWorkerClearSummary
  mov rax,[rsp+40h]
  mov qword ptr [gpWorkerSummary+GP_S_TICK],rax
  mov rax,gpWorkerSequence
  mov qword ptr [gpWorkerSummary+GP_S_SEQUENCE],rax
  inc gpWorkerFailures
gpwmNotify:
  lea rcx,gpWorkerLock
  call ReleaseSRWLockExclusive
  mov rcx,rbx
  call GpWorkerFree
  mov rcx,r14
  mov edx,GP_MESSAGE
  xor r8d,r8d
  xor r9d,r9d
  call PostMessageW
  mov rcx,gpWorkerEvent
  mov edx,GP_SAMPLE_MS
  call WaitForSingleObject
  jmp gpwmLoop
gpwmDrop:
  inc gpWorkerDropped
  lea rcx,gpWorkerLock
  call ReleaseSRWLockExclusive
  mov rcx,rbx
  call GpWorkerFree
  jmp gpwmLoop
gpwmIdle:
  mov rcx,gpWorkerEvent
  mov edx,-1
  call WaitForSingleObject
  jmp gpwmLoop
gpwmExitLocked:
  mov rbx,gpWorkerEvent
  mov gpWorkerEvent,0
  lea rcx,gpWorkerLock
  call ReleaseSRWLockExclusive
  call qword ptr gpWorkerCloser
  ; Keep finished thread handle for diagnostics; application process owns it.
  mov rcx,rbx
  call CloseHandle
  xor eax,eax
  add rsp,50h
  pop r15
  pop r14
  pop r13
  pop r12
  pop rdi
  pop rsi
  pop rbx
  ret
GpWorkerMain ENDP
