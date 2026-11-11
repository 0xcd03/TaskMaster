; Single producer (collector), single consumer (icon worker). Shell extensions
; never run on the UI or collector thread. Queue slots hold at most 1023 WCHAR
; paths; oversized paths use the generic EXE icon. All storage is bounded.
ICON_REQUEST_BYTES equ 2080
ICON_REQUEST_COUNT equ 32
.data
iconWorkerStarted dd 0
iconProducerCursor dd 0
iconConsumerCursor dd 0
iconQueueEvent dq 0
.data?
align 8
; slot: state+0, PID+4, birth+8, cache slot+16, path+32 (1024 WCHAR).
; state: 0 free, 1 ready, 2 consumer-owned, 3 producer-owned.
iconRequests db ICON_REQUEST_BYTES * ICON_REQUEST_COUNT dup(?)
processIconStates dd 16384 dup(?) ; 0 unused, 1 pending, 2 ready, 3 failed
iconWorkerShellInfo db 696 dup(?)
IFDEF TM_TESTS
iconTestRow db 4096 dup(?)
ENDIF
.code

; Producer only. Reserve any free slot without waiting for the consumer.
ReserveIconRequest PROC
  mov eax,iconProducerCursor
  mov ecx,ICON_REQUEST_COUNT
  lea rdx,iconRequests
rirNext:
  imul r8d,eax,ICON_REQUEST_BYTES
  lea r9,[rdx+r8]
  cmp dword ptr [r9],0
  je rirFound
  inc eax
  and eax,31
  dec ecx
  jnz rirNext
  xor eax,eax
  ret
rirFound:
  mov dword ptr [r9],3
  inc eax
  and eax,31
  mov iconProducerCursor,eax
  mov rax,r9
  ret
ReserveIconRequest ENDP

; RCX row, RDX query handle. The collector publishes immutable PID+birth cache
; identities only after obtaining a bounded path. Pending identities suppress
; duplicate requests; a full queue is retried on a later process snapshot.
CollectCachedProcessIcon PROC
  push rbx
  push rsi
  push rdi
  push r12
  push r13
  sub rsp,30h
  mov rbx,rcx
  mov rsi,rdx
  cmp iconWorkerStarted,0
  je ccpiDone
  mov r12d,[rbx+3584]
  xor r12d,dword ptr [rbx+3600]
  and r12d,16383
  mov edi,16384
ccpiProbe:
  lea rdx,processIconPids
  mov eax,[rdx+r12*4]
  test eax,eax
  jz ccpiNew
  cmp eax,[rbx+3584]
  jne ccpiNext
  lea rdx,processIconBirth
  mov rax,[rdx+r12*8]
  cmp rax,[rbx+3600]
  jne ccpiNext
  lea rdx,processIconStates
  cmp dword ptr [rdx+r12*4],2
  jne ccpiDone ; pending/failed identities keep the generic icon
  lea rdx,processIconIndices
  mov eax,[rdx+r12*4]
  mov [rbx+3880],eax
  jmp ccpiDone
ccpiNext:
  inc r12d
  and r12d,16383
  dec edi
  jnz ccpiProbe
  jmp ccpiDone
ccpiNew:
  call ReserveIconRequest
  test rax,rax
  jz ccpiDone
  mov r13,rax
  mov processIconLength,1024
  mov rcx,rsi
  xor edx,edx
  lea r8,[r13+32]
  lea r9,processIconLength
  call QueryFullProcessImageNameW
  test eax,eax
  jz ccpiRelease
  mov eax,processIconLength
  test eax,eax
  jz ccpiRelease
  cmp eax,1024
  jae ccpiRelease
  mov word ptr [r13+rax*2+32],0
  call CollectorSnapshotCurrent
  test eax,eax
  jz ccpiRelease
  ; Only the collector assigns cache identities. They are never reused while
  ; an icon request is in flight, so a completion cannot race PID recycling.
  lea rdx,processIconPids
  mov eax,[rbx+3584]
  mov [rdx+r12*4],eax
  mov [r13+4],eax
  lea rdx,processIconBirth
  mov rax,[rbx+3600]
  mov [rdx+r12*8],rax
  mov [r13+8],rax
  mov [r13+16],r12d
  lea rdx,processIconStates
  mov dword ptr [rdx+r12*4],1
  mov eax,1
  xchg dword ptr [r13],eax ; release path and identity to consumer
  mov rcx,iconQueueEvent
  call SetEvent
  jmp ccpiDone
ccpiRelease:
  xor eax,eax
  xchg dword ptr [r13],eax
ccpiDone:
  add rsp,30h
  pop r13
  pop r12
  pop rdi
  pop rsi
  pop rbx
  ret
CollectCachedProcessIcon ENDP

; RCX request, EDX image index, R8D success. Publish only a matching pending
; identity. State is released last; collector reads indices only after ready.
; Returns 1 on publication. A stale or cancelled job cannot alter a new entry.
CompleteIconRequest PROC
  mov r11d,edx
  mov r10d,r8d
  xor r8d,r8d
  mov r9d,[rcx+16]
  cmp r9d,16384
  jae cirFree
  lea rax,processIconPids
  mov edx,[rcx+4]
  cmp [rax+r9*4],edx
  jne cirFree
  lea rax,processIconBirth
  mov rdx,[rcx+8]
  cmp [rax+r9*8],rdx
  jne cirFree
  lea rax,processIconStates
  cmp dword ptr [rax+r9*4],1
  jne cirFree
  cmp collectorStop,0
  jne cirFailed
  test r10d,r10d
  jz cirFailed
  lea rdx,processIconIndices
  mov [rdx+r9*4],r11d
  mov edx,2
  xchg dword ptr [rax+r9*4],edx
  mov r8d,1
  jmp cirFree
cirFailed:
  mov edx,3
  xchg dword ptr [rax+r9*4],edx
cirFree:
  xor eax,eax
  xchg dword ptr [rcx],eax
  mov eax,r8d
  ret
CompleteIconRequest ENDP

IconWorkerMain PROC
  push rbx
  push rsi
  push rdi
  push r12
  sub rsp,38h
  xor r12d,r12d
  xor ecx,ecx
  xor edx,edx
  call CoInitializeEx
  test eax,eax
  js iwmLoop
  mov r12d,1
iwmLoop:
  cmp collectorStop,0
  jne iwmExit
  mov ebx,iconConsumerCursor
  mov edi,ICON_REQUEST_COUNT
iwmScan:
  imul eax,ebx,ICON_REQUEST_BYTES
  lea rsi,iconRequests
  add rsi,rax
  mov eax,1
  mov edx,2
  lock cmpxchg dword ptr [rsi],edx
  je iwmExtract
  inc ebx
  and ebx,31
  dec edi
  jnz iwmScan
  mov rcx,iconQueueEvent
  mov edx,250
  call WaitForSingleObject
  jmp iwmLoop
iwmExtract:
  lea rcx,[rsi+32]
  xor edx,edx
  lea r8,iconWorkerShellInfo
  mov r9d,696
  mov qword ptr [rsp+20h],4001h
  call SHGetFileInfoW
  xor r8d,r8d
  test rax,rax
  setnz r8b
  mov edx,dword ptr [iconWorkerShellInfo+8]
  mov rcx,rsi
  call CompleteIconRequest
  inc ebx
  and ebx,31
  mov iconConsumerCursor,ebx
  jmp iwmLoop
iwmExit:
  test r12d,r12d
  jz iwmDone
  call CoUninitialize
iwmDone:
  xor eax,eax
  add rsp,38h
  pop r12
  pop rdi
  pop rsi
  pop rbx
  ret
IconWorkerMain ENDP

; Start once after the generic system image list exists. The process owns this
; worker until ExitProcess; the main thread deliberately never joins a shell
; extension which may be stuck in external code.
StartIconWorker PROC
  push rbx
  sub rsp,30h
  cmp iconWorkerStarted,0
  jne siwDone
  cmp processImageList,0
  je siwDone
  xor ecx,ecx
  xor edx,edx
  xor r8d,r8d
  xor r9d,r9d
  call CreateEventW
  mov iconQueueEvent,rax
  test rax,rax
  jz siwDone
  xor ecx,ecx
  xor edx,edx
  lea r8,IconWorkerMain
  xor r9d,r9d
  mov qword ptr [rsp+20h],0
  mov qword ptr [rsp+28h],0
  call CreateThread
  test rax,rax
  jz siwEventFailed
  mov rbx,rax
  mov iconWorkerStarted,1
  mov rcx,rax
  mov edx,-1 ; THREAD_PRIORITY_BELOW_NORMAL
  call SetThreadPriority
  mov rcx,rbx
  call CloseHandle
  jmp siwDone
siwEventFailed:
  mov rcx,iconQueueEvent
  call CloseHandle
  mov iconQueueEvent,0
siwDone:
  add rsp,30h
  pop rbx
  ret
StartIconWorker ENDP

IFDEF TM_TESTS
; Isolated regression fixtures: bounded capacity, duplicate pending identities,
; cached publication, PID reuse, stale completion and shutdown cancellation.
; No worker is launched and no shell extension or real process is touched.
IconQueueTests PROC
  push rbx
  push rsi
  push rdi
  push r12
  push r13
  sub rsp,30h
  mov r12d,collectorStop
  mov r13d,iconWorkerStarted
  mov collectorStop,0
  mov iconWorkerStarted,0
  call ResetIconQueueFixtures
  xor esi,esi
iqtReserve:
  call ReserveIconRequest
  test rax,rax
  jz iqtFail
  cmp dword ptr [rax],3
  jne iqtFail
  inc esi
  cmp esi,ICON_REQUEST_COUNT
  jb iqtReserve
  call ReserveIconRequest
  test rax,rax
  jnz iqtFail
  call ResetIconQueueFixtures
  lea rbx,iconRequests
  mov dword ptr [rbx],2
  mov dword ptr [rbx+4],90123
  mov qword ptr [rbx+8],8192
  mov dword ptr [rbx+16],123
  lea rax,processIconPids
  mov dword ptr [rax+123*4],90123
  lea rax,processIconBirth
  mov qword ptr [rax+123*8],8192
  lea rax,processIconStates
  mov dword ptr [rax+123*4],1
  mov rcx,rbx
  mov edx,77
  mov r8d,1
  call CompleteIconRequest
  cmp eax,1
  jne iqtFail
  cmp dword ptr [rbx],0
  jne iqtFail
  lea rax,processIconStates
  cmp dword ptr [rax+123*4],2
  jne iqtFail
  lea rax,processIconIndices
  cmp dword ptr [rax+123*4],77
  jne iqtFail
  ; An old completion cannot update the same PID's later birth.
  mov dword ptr [rbx],2
  lea rax,processIconBirth
  mov qword ptr [rax+123*8],8193
  lea rax,processIconIndices
  mov dword ptr [rax+123*4],88
  lea rax,processIconStates
  mov dword ptr [rax+123*4],1
  mov rcx,rbx
  mov edx,99
  mov r8d,1
  call CompleteIconRequest
  test eax,eax
  jnz iqtFail
  lea rax,processIconIndices
  cmp dword ptr [rax+123*4],88
  jne iqtFail
  lea rax,processIconStates
  cmp dword ptr [rax+123*4],1
  jne iqtFail
  ; Shutdown releases the request and marks its matching identity failed.
  mov collectorStop,1
  mov dword ptr [rbx],2
  mov qword ptr [rbx+8],8193
  mov rcx,rbx
  mov edx,99
  mov r8d,1
  call CompleteIconRequest
  test eax,eax
  jnz iqtFail
  cmp dword ptr [rbx],0
  jne iqtFail
  lea rax,processIconStates
  cmp dword ptr [rax+123*4],3
  jne iqtFail
  mov collectorStop,0
  call ResetIconQueueFixtures
  mov ebx,90123
  xor ebx,8192
  and ebx,16383
  lea rax,processIconPids
  mov dword ptr [rax+rbx*4],90123
  lea rax,processIconBirth
  mov qword ptr [rax+rbx*8],8192
  lea rax,processIconStates
  mov dword ptr [rax+rbx*4],1
  lea rcx,iconTestRow
  call ClearRow
  mov dword ptr [iconTestRow+3584],90123
  mov qword ptr [iconTestRow+3600],8192
  mov dword ptr [iconTestRow+3880],42
  mov iconWorkerStarted,1
  lea rcx,iconTestRow
  xor edx,edx
  call CollectCachedProcessIcon
  cmp iconProducerCursor,0
  jne iqtFail ; duplicate pending request must not reserve another slot
  cmp dword ptr [iconTestRow+3880],42
  jne iqtFail
  lea rax,processIconIndices
  mov dword ptr [rax+rbx*4],99
  lea rax,processIconStates
  mov dword ptr [rax+rbx*4],2
  lea rcx,iconTestRow
  xor edx,edx
  call CollectCachedProcessIcon
  cmp dword ptr [iconTestRow+3880],99
  jne iqtFail
  ; Reused PID with a new birth must retain the generic icon on lookup miss.
  mov qword ptr [iconTestRow+3600],8193
  mov dword ptr [iconTestRow+3880],42
  lea rcx,iconTestRow
  xor edx,edx ; invalid query handle: no process or shell handler is used
  call CollectCachedProcessIcon
  cmp dword ptr [iconTestRow+3880],42
  jne iqtFail
  xor ebx,ebx
  jmp iqtRestore
iqtFail:
  mov ebx,1
iqtRestore:
  call ResetIconQueueFixtures
  mov collectorStop,r12d
  mov iconWorkerStarted,r13d
  mov eax,ebx
  add rsp,30h
  pop r13
  pop r12
  pop rdi
  pop rsi
  pop rbx
  ret
IconQueueTests ENDP

ResetIconQueueFixtures PROC
  push rdi
  xor eax,eax
  lea rdi,iconRequests
  mov ecx,ICON_REQUEST_BYTES * ICON_REQUEST_COUNT / 8
  rep stosq
  lea rdi,processIconPids
  mov ecx,8192
  rep stosq
  lea rdi,processIconBirth
  mov ecx,16384
  rep stosq
  lea rdi,processIconIndices
  mov ecx,8192
  rep stosq
  lea rdi,processIconStates
  mov ecx,8192
  rep stosq
  mov iconProducerCursor,0
  mov iconConsumerCursor,0
  pop rdi
  ret
ResetIconQueueFixtures ENDP
ENDIF
