; UI-thread-owned bounded metadata store. No process APIs, clocks, waits, or
; shared formatting buffers. Hash values are descriptor-index+1; -1 tombstone.
extern GetProcessHeap:proc, HeapAlloc:proc, HeapFree:proc
.data
align 8
pmNow dq 0
pmDemandEpoch dq 1
pmFrame dq 1
pmBytes dq 0
pmCacheAllocate dq HeapAlloc
pmCacheBudget dq PM_STRING_BUDGET
pmCookieGenerator dq 0
pmSerialGenerator dq 0
pmNotify dq 0
pmDemand dd 0
pmEntryCount dd 0
pmHighWater dd 0
pmFreeHead dd -1
pmCursors dd 3 dup(0)
.data?
align 8
pmEntries db PM_CACHE_CAPACITY*PM_ENTRY_SIZE dup(?)
pmHash dd PM_HASH_CAPACITY dup(?)
.code
PmCacheSetClock PROC
 mov pmNow,rcx
 ret
PmCacheSetClock ENDP
PmCacheBeginFrame PROC
 inc pmFrame
 ret
PmCacheBeginFrame ENDP

; Internal leaf: RCX PID, RDX birth -> entry or zero; R10D insertion slot.
PmCacheFind PROC
 xor eax,eax
 test ecx,ecx
 jz pmcfDone
 test rdx,rdx
 jz pmcfDone
 mov r8,rdx
 shr r8,32
 xor r8d,edx
 xor r8d,ecx
 ; 32-bit mix; avoid a signed-immediate assembler ambiguity.
 mov eax,r8d
 shl r8d,5
 xor r8d,eax
 and r8d,PM_HASH_CAPACITY-1
 mov r10d,-1
 lea r11,pmHash
pmcfProbe:
 mov eax,[r11+r8*4]
 test eax,eax
 jz pmcfEmpty
 cmp eax,-1
 je pmcfTomb
 dec eax
 imul eax,PM_ENTRY_SIZE
 lea r9,pmEntries
 add rax,r9
 cmp ecx,[rax+PM_E_PID]
 jne pmcfAdvance
 cmp rdx,[rax+PM_E_BIRTH]
 je pmcfDone
 jmp pmcfAdvance
pmcfTomb:
 cmp r10d,-1
 jne pmcfAdvance
 mov r10d,r8d
pmcfAdvance:
 inc r8d
 and r8d,PM_HASH_CAPACITY-1
 jmp pmcfProbe
pmcfEmpty:
 cmp r10d,-1
 jne pmcfAbsent
 mov r10d,r8d
pmcfAbsent:
 xor eax,eax
pmcfDone:
 ret
PmCacheFind ENDP

PmCacheMark PROC
 sub rsp,28h
 call PmCacheFind
 test rax,rax
 jz pmcmDone
 mov rdx,pmFrame
 mov [rax+PM_E_FRAME],rdx
pmcmDone:
 add rsp,28h
 ret
PmCacheMark ENDP

PmCacheEnsure PROC
 push rbx
 push rsi
 push rdi
 sub rsp,20h
 mov ebx,ecx
 mov rsi,rdx
 test ecx,ecx
 jz pmceZero
 test rdx,rdx
 jz pmceZero
 call PmCacheFind
 test rax,rax
 jnz pmceMark
 cmp pmEntryCount,PM_CACHE_CAPACITY
 jae pmceZero
 mov edi,pmFreeHead
 cmp edi,-1
 jne pmceReuse
 mov edi,pmHighWater
 inc pmHighWater
 jmp pmceSlot
pmceReuse:
 imul eax,edi,PM_ENTRY_SIZE
 lea rdx,pmEntries
 mov eax,[rdx+rax+PM_E_RESERVED]
 mov pmFreeHead,eax
pmceSlot:
 lea rdx,pmHash
 lea eax,[rdi+1]
 mov [rdx+r10*4],eax
 imul eax,edi,PM_ENTRY_SIZE
 lea rdi,pmEntries
 add rdi,rax
 mov rdx,rdi
 xor eax,eax
 mov ecx,PM_ENTRY_SIZE/8
 rep stosq
 mov [rdx+PM_E_PID],ebx
 mov [rdx+PM_E_BIRTH],rsi
 mov dword ptr [rdx+PM_E_FLAGS],1
 inc pmCookieGenerator
 mov rax,pmCookieGenerator
 mov [rdx+PM_E_COOKIE],rax
 inc pmEntryCount
 mov rax,rdx
pmceMark:
 mov rdx,pmFrame
 mov [rax+PM_E_FRAME],rdx
 jmp pmceDone
pmceZero:
 xor eax,eax
pmceDone:
 add rsp,20h
 pop rdi
 pop rsi
 pop rbx
 ret
PmCacheEnsure ENDP

; RCX field; frees only its exact-sized owned payload.
PmCacheFreeField PROC
 push rbx
 sub rsp,20h
 mov rbx,rcx
 mov r8,[rbx+PM_F_POINTER]
 test r8,r8
 jz pmcffDone
 call GetProcessHeap
 mov rcx,rax
 xor edx,edx
 mov r8,[rbx+PM_F_POINTER]
 call HeapFree
 mov eax,[rbx+PM_F_BYTES]
 sub pmBytes,rax
 mov qword ptr [rbx+PM_F_POINTER],0
 mov dword ptr [rbx+PM_F_BYTES],0
pmcffDone:
 add rsp,20h
 pop rbx
 ret
PmCacheFreeField ENDP

PmCachePrune PROC
 push rbx
 push rsi
 sub rsp,28h
 xor ebx,ebx
pmcpLoop:
 cmp ebx,pmHighWater
 jae pmcpDone
 imul eax,ebx,PM_ENTRY_SIZE
 lea rsi,pmEntries
 add rsi,rax
 test dword ptr [rsi+PM_E_FLAGS],1
 jz pmcpNext
 mov rax,pmFrame
 cmp rax,[rsi+PM_E_FRAME]
 je pmcpNext
 lea rcx,[rsi+PM_E_FIELDS]
 call PmCacheFreeField
 lea rcx,[rsi+PM_E_FIELDS+PM_FIELD_SIZE]
 call PmCacheFreeField
 lea rcx,[rsi+PM_E_FIELDS+2*PM_FIELD_SIZE]
 call PmCacheFreeField
 ; No lookup occurs until the hash is rebuilt below, so deletion is O(1).
 mov dword ptr [rsi+PM_E_FLAGS],0
 mov eax,pmFreeHead
 mov [rsi+PM_E_RESERVED],eax
 mov pmFreeHead,ebx
 dec pmEntryCount
pmcpNext:
 inc ebx
 jmp pmcpLoop
pmcpDone:
 ; Clear tombstones when empty; at nonzero load compact via rebuild below.
 lea rcx,pmHash
 xor eax,eax
 xor edx,edx
pmcpClear:
 mov [rcx+rdx*4],eax
 inc edx
 cmp edx,PM_HASH_CAPACITY
 jb pmcpClear
 xor ebx,ebx
pmcpRehash:
 cmp ebx,pmHighWater
 jae pmcpReturn
 imul eax,ebx,PM_ENTRY_SIZE
 lea rsi,pmEntries
 add rsi,rax
 test dword ptr [rsi+PM_E_FLAGS],1
 jz pmcpReNext
 mov ecx,[rsi+PM_E_PID]
 mov rdx,[rsi+PM_E_BIRTH]
 call PmCacheFind
 lea rdx,pmHash
 lea eax,[rbx+1]
 mov [rdx+r10*4],eax
pmcpReNext:
 inc ebx
 jmp pmcpRehash
pmcpReturn:
 add rsp,28h
 pop rsi
 pop rbx
 ret
PmCachePrune ENDP

PmCacheReset PROC
 push rdi
 sub rsp,20h
 inc pmFrame
 call PmCachePrune
 lea rdi,pmEntries
 xor eax,eax
 mov ecx,PM_CACHE_CAPACITY*PM_ENTRY_SIZE/8
 rep stosq
 mov pmHighWater,0
 mov pmFreeHead,-1
 mov pmEntryCount,0
 mov pmDemand,0
 mov pmNotify,0
 mov pmBytes,0
 mov pmNow,0
 mov pmFrame,1
 inc pmDemandEpoch
 mov qword ptr pmCursors,0
 mov dword ptr [pmCursors+8],0
 add rsp,20h
 pop rdi
 ret
PmCacheReset ENDP

PmCacheSetDemand PROC
 cmp ecx,PM_MASK_ALL
 ja pmcsdInvalid
 test ecx,ecx
 jz pmcsdValid
 test rdx,rdx
 jz pmcsdInvalid
pmcsdValid:
 cmp ecx,pmDemand
 jne pmcsdChange
 cmp rdx,pmNotify
 je pmcsdOk
pmcsdChange:
 mov pmDemand,ecx
 mov pmNotify,rdx
 inc pmDemandEpoch
 lea r9,pmEntries
 mov r10d,PM_CACHE_CAPACITY
pmcsdEntry:
 lea r8,[r9+PM_E_FIELDS]
 mov edx,PM_LANES
pmcsdField:
 mov qword ptr [r8+PM_F_SERIAL],0
 cmp dword ptr [r8+PM_F_STATUS],PM_STATUS_LOADING
 jne pmcsdNext
 mov dword ptr [r8+PM_F_STATUS],PM_STATUS_MISSING
 mov qword ptr [r8+PM_F_DUE],0
pmcsdNext:
 add r8,PM_FIELD_SIZE
 dec edx
 jnz pmcsdField
 add r9,PM_ENTRY_SIZE
 dec r10d
 jnz pmcsdEntry
pmcsdOk:
 xor eax,eax
 ret
pmcsdInvalid:
 mov eax,87
 ret
PmCacheSetDemand ENDP

PmCacheGet PROC
 sub rsp,28h
 mov [rsp+20h],r8d
 dec r8d
 cmp r8d,PM_LANES-1
 ja pmcgMissing
 call PmCacheFind
 test rax,rax
 jz pmcgMissing
 mov r8d,[rsp+20h]
 dec r8d
 imul r8d,PM_FIELD_SIZE
 lea r10,[rax+r8+PM_E_FIELDS]
 mov edx,[r10+PM_F_STATUS]
 mov r8d,[r10+PM_F_ERROR]
 mov r9d,[r10+PM_F_FLAGS]
 xor eax,eax
 cmp edx,PM_STATUS_VALID
 jne pmcgDone
 mov rax,pmNow
 cmp rax,[r10+PM_F_DUE]
 jae pmcgExpired
 mov rax,[r10+PM_F_POINTER]
 jmp pmcgDone
pmcgExpired:
 mov edx,PM_STATUS_LOADING
 xor eax,eax
 jmp pmcgDone
pmcgMissing:
 xor eax,eax
 xor edx,edx
 xor r8d,r8d
 xor r9d,r9d
pmcgDone:
 add rsp,28h
 ret
PmCacheGet ENDP

PmCachePrepare PROC
 push rbx
 push rsi
 push rdi
 push r12
 sub rsp,28h
 mov rdi,r9
 mov r12d,r8d
 mov eax,87
 test rdi,rdi
 jz pmcpreDone
 dec r8d
 cmp r8d,PM_LANES-1
 ja pmcpreDone
 test ecx,ecx
 jz pmcpreDone
 test rdx,rdx
 jz pmcpreDone
 call PmCacheFind
 test rax,rax
 jz pmcpreMissing
 mov rbx,rax
 mov eax,170
 test dword ptr [rbx+PM_E_FLAGS],2
 jnz pmcpreDone
 mov ecx,r12d
 dec ecx
 mov edx,1
 shl edx,cl
 test pmDemand,edx
 jz pmcpreDone
 imul ecx,PM_FIELD_SIZE
 lea rsi,[rbx+rcx+PM_E_FIELDS]
 cmp dword ptr [rsi+PM_F_STATUS],PM_STATUS_LOADING
 je pmcpreDone
 mov rdx,[rsi+PM_F_DUE]
 cmp dword ptr [rsi+PM_F_STATUS],PM_STATUS_TIMEOUT
 jne pmcpreDue
 cmp qword ptr [rsi+PM_F_SERIAL],0
 je pmcpreDue
 add rdx,PM_RETRY_MS
pmcpreDue:
 cmp pmNow,rdx
 jb pmcpreDone
 mov rcx,rsi
 call PmCacheFreeField
 mov dword ptr [rsi+PM_F_STATUS],PM_STATUS_LOADING
 mov dword ptr [rsi+PM_F_ERROR],0
 mov dword ptr [rsi+PM_F_FLAGS],0
 inc pmSerialGenerator
 mov rax,pmSerialGenerator
 mov [rsi+PM_F_SERIAL],rax
 mov rax,pmNow
 add rax,PM_DEADLINE_MS
 mov [rsi+PM_F_DUE],rax
 mov rdx,rdi
 xor eax,eax
 mov ecx,PD_REQUEST_SIZE/8
 rep stosq
 mov dword ptr [rdx+PD_R_SIZE],PD_REQUEST_SIZE
 mov dword ptr [rdx+PD_R_VERSION],PD_VERSION
 mov [rdx+PD_R_LANE],r12d
 mov rax,[rbx+PM_E_COOKIE]
 mov [rdx+PD_R_COOKIE],rax
 mov rax,pmDemandEpoch
 mov [rdx+PD_R_GENERATION],rax
 mov [rdx+PD_R_EPOCH],rax
 mov rax,[rsi+PM_F_SERIAL]
 mov [rdx+PD_R_SEQUENCE],rax
 mov eax,[rbx+PM_E_PID]
 mov [rdx+PD_R_PID],eax
 mov rax,[rbx+PM_E_BIRTH]
 mov [rdx+PD_R_BIRTH],rax
 mov rax,[rsi+PM_F_DUE]
 mov [rdx+PD_R_DEADLINE],rax
 mov rax,pmNotify
 mov [rdx+PD_R_NOTIFY],rax
 xor eax,eax
 jmp pmcpreDone
pmcpreMissing:
 mov eax,1168
pmcpreDone:
 add rsp,28h
 pop r12
 pop rdi
 pop rsi
 pop rbx
 ret
PmCachePrepare ENDP

PmCacheNext PROC
 push rbx
 push rsi
 push rdi
 push r12
 sub rsp,28h
 mov rsi,rdx
 mov edi,ecx
 dec ecx
 mov eax,87
 cmp ecx,PM_LANES-1
 ja pmcnDone
 test rdx,rdx
 jz pmcnDone
 lea rax,pmCursors
 mov ebx,[rax+rcx*4]
 mov r12d,PM_CACHE_CAPACITY
pmcnLoop:
 imul eax,ebx,PM_ENTRY_SIZE
 lea rdx,pmEntries
 add rdx,rax
 test dword ptr [rdx+PM_E_FLAGS],1
 jz pmcnAdvance
 mov ecx,[rdx+PM_E_PID]
 mov rdx,[rdx+PM_E_BIRTH]
 mov r8d,edi
 mov r9,rsi
 call PmCachePrepare
 test eax,eax
 jz pmcnAccepted
pmcnAdvance:
 inc ebx
 and ebx,PM_CACHE_CAPACITY-1
 dec r12d
 jnz pmcnLoop
 mov eax,259
 jmp pmcnDone
pmcnAccepted:
 inc ebx
 and ebx,PM_CACHE_CAPACITY-1
 lea rdx,pmCursors
 mov [rdx+rdi*4-4],ebx
pmcnDone:
 add rsp,28h
 pop r12
 pop rdi
 pop rsi
 pop rbx
 ret
PmCacheNext ENDP

; Validate copied request without mutation. RAX entry, RDX field or zero.
PmCacheMatch PROC
 push rbx
 sub rsp,20h
 mov rbx,rcx
 test rcx,rcx
 jz pmcmaBad
 cmp dword ptr [rbx+PD_R_SIZE],PD_REQUEST_SIZE
 jne pmcmaBad
 cmp dword ptr [rbx+PD_R_VERSION],PD_VERSION
 jne pmcmaBad
 cmp dword ptr [rbx+PD_R_FLAGS],0
 jne pmcmaBad
 cmp dword ptr [rbx+PD_R_RESERVED],0
 jne pmcmaBad
 cmp qword ptr [rbx+PD_R_CONTEXT],0
 jne pmcmaBad
 cmp qword ptr [rbx+PD_R_CANCEL],0
 jne pmcmaBad
 mov rax,pmNotify
 test rax,rax
 jz pmcmaBad
 cmp [rbx+PD_R_NOTIFY],rax
 jne pmcmaBad
 mov rax,pmDemandEpoch
 cmp [rbx+PD_R_GENERATION],rax
 jne pmcmaBad
 cmp [rbx+PD_R_EPOCH],rax
 jne pmcmaBad
 mov ecx,[rbx+PD_R_LANE]
 dec ecx
 cmp ecx,PM_LANES-1
 ja pmcmaBad
 mov eax,1
 shl eax,cl
 test pmDemand,eax
 jz pmcmaBad
 mov ecx,[rbx+PD_R_PID]
 mov rdx,[rbx+PD_R_BIRTH]
 call PmCacheFind
 test rax,rax
 jz pmcmaBad
 mov rdx,[rbx+PD_R_COOKIE]
 test rdx,rdx
 jz pmcmaBad
 cmp rdx,[rax+PM_E_COOKIE]
 jne pmcmaBad
 mov edx,[rbx+PD_R_LANE]
 dec edx
 imul edx,PM_FIELD_SIZE
 lea rdx,[rax+rdx+PM_E_FIELDS]
 cmp dword ptr [rdx+PM_F_STATUS],PM_STATUS_LOADING
 je pmcmaSerial
 cmp dword ptr [rdx+PM_F_STATUS],PM_STATUS_TIMEOUT
 jne pmcmaBad
pmcmaSerial:
 mov rcx,[rbx+PD_R_SEQUENCE]
 test rcx,rcx
 jz pmcmaBad
 cmp rcx,[rdx+PM_F_SERIAL]
 jne pmcmaBad
 mov rcx,[rbx+PD_R_DEADLINE]
 test rcx,rcx
 jz pmcmaBad
 cmp rcx,[rdx+PM_F_DUE]
 jne pmcmaBad
 jmp pmcmaDone
pmcmaBad:
 xor eax,eax
 xor edx,edx
pmcmaDone:
 add rsp,20h
 pop rbx
 ret
PmCacheMatch ENDP

PmCacheExpire PROC
 sub rsp,28h
 call PmCacheMatch
 test rax,rax
 jz pmcxNo
 cmp dword ptr [rdx+PM_F_STATUS],PM_STATUS_LOADING
 jne pmcxNo
 mov rax,pmNow
 cmp rax,[rdx+PM_F_DUE]
 jb pmcxNo
 mov dword ptr [rdx+PM_F_STATUS],PM_STATUS_TIMEOUT
 mov dword ptr [rdx+PM_F_ERROR],1460
 mov eax,1
 jmp pmcxDone
pmcxNo:
 xor eax,eax
pmcxDone:
 add rsp,28h
 ret
PmCacheExpire ENDP

PmCacheComplete PROC
 push rbx
 push rsi
 push rdi
 push r12
 push r13
 push r14
 push r15
 sub rsp,30h
 mov rbx,rcx
 mov rsi,rdx
 mov r12d,r8d
 mov dword ptr [rsp+24h],0 ; only validated provider error records set flags
 call PmCacheMatch
 test rax,rax
 jz pmccBad
 mov r13,rax
 mov rdi,rdx
 mov rax,pmNow
 cmp rax,[rbx+PD_R_DEADLINE]
 jae pmccLate
 test r12d,r12d
 jnz pmccError
 test rsi,rsi
 jz pmccBad
 cmp dword ptr [rsi+PD_S_SIZE],PD_SNAPSHOT_SIZE
 jne pmccBad
 cmp dword ptr [rsi+PD_S_VERSION],PD_VERSION
 jne pmccBad
 ; PD_S_CAPACITY is DWORD logical-processor count used by CPU rates, not
 ; snapshot bytes. Native fills it, optional sources leave zero. Fixed source
 ; offsets/caps below and PD_S_SIZE establish this snapshot's byte schema.
 cmp qword ptr [rsi+PD_S_CONTEXT],0
 jne pmccBad
 mov eax,[rbx+PD_R_LANE]
 cmp [rsi+PD_S_LANE],eax
 jne pmccBad
 mov eax,[rbx+PD_R_PID]
 cmp [rsi+PD_S_PID],eax
 jne pmccBad
 mov rax,[rbx+PD_R_BIRTH]
 cmp [rsi+PD_S_BIRTH],rax
 jne pmccBad
 mov rax,[rbx+PD_R_COOKIE]
 cmp [rsi+PD_S_COOKIE],rax
 jne pmccBad
 mov rax,[rbx+PD_R_GENERATION]
 cmp [rsi+PD_S_GENERATION],rax
 jne pmccBad
 mov rax,[rbx+PD_R_SEQUENCE]
 cmp [rsi+PD_S_SEQUENCE],rax
 jne pmccBad
 mov rax,[rbx+PD_R_EPOCH]
 cmp [rsi+PD_S_EPOCH],rax
 jne pmccBad
 cmp dword ptr [rsi+PD_S_TARGET],PD_TARGET_CANCELLED
 ja pmccBad
 cmp dword ptr [rsi+PD_S_TARGET],PD_TARGET_EXITED
 je pmccTerminal
 cmp dword ptr [rsi+PD_S_TARGET],PD_TARGET_MISMATCH
 je pmccTerminal
 mov eax,[rbx+PD_R_LANE]
 mov r14d,PD_S_PATH
 mov r15d,PD_PATH_CAPACITY
 lea rdx,[rsi+PD_S_FIELDS+PD_F_PATH*PD_FIELD_SIZE]
 cmp eax,PM_LANE_PATH
 je pmccSource
 mov r14d,PD_S_ACCOUNT
 mov r15d,PD_ACCOUNT_CAPACITY
 lea rdx,[rsi+PD_S_FIELDS+PD_F_ACCOUNT*PD_FIELD_SIZE]
 cmp eax,PM_LANE_ACCOUNT
 je pmccSource
 mov r14d,PD_S_COMMAND
 mov r15d,PD_COMMAND_CAPACITY
 lea rdx,[rsi+PD_S_FIELDS+PD_F_COMMAND*PD_FIELD_SIZE]
pmccSource:
 mov eax,[rdx+PD_FIELD_STATUS]
 cmp eax,PD_STATUS_VALID
 je pmccText
 cmp eax,PD_STATUS_UNAVAILABLE
 je pmccSourceError
 cmp eax,PD_STATUS_TIMEOUT
 jne pmccBad
pmccSourceError:
 cmp dword ptr [rdx+PD_FIELD_LENGTH],0
 jne pmccBad
 mov ecx,[rdx+PD_FIELD_FLAGS]
 mov [rsp+24h],ecx
 mov r12d,[rdx+PD_FIELD_ERROR]
 test r12d,r12d
 jnz pmccSourceHasError
 mov r12d,13
pmccSourceHasError:
 cmp eax,PD_STATUS_TIMEOUT
 jne pmccError
 mov r12d,1460
 jmp pmccError
pmccText:
 cmp dword ptr [rsi+PD_S_TARGET],PD_TARGET_VERIFIED
 je pmccTextIdentity
 ; Native path and token account remain usable when SYNCHRONIZE is denied
 ; but the held object still verifies the full identity before/after reading.
 cmp dword ptr [rbx+PD_R_LANE],PM_LANE_COMMAND
 je pmccBad
 cmp dword ptr [rsi+PD_S_TARGET],PD_TARGET_UNKNOWN
 jne pmccBad
 cmp dword ptr [rsi+PD_S_FIELDS+PD_F_IDENTITY*PD_FIELD_SIZE+PD_FIELD_STATUS],PD_STATUS_VALID
 jne pmccBad
pmccTextIdentity:
 mov rax,[rbx+PD_R_BIRTH]
 cmp [rsi+PD_S_OBSERVED_BIRTH],rax
 jne pmccBad
 cmp dword ptr [rdx+PD_FIELD_ERROR],0
 jne pmccBad
 mov ecx,[rdx+PD_FIELD_LENGTH]
 cmp ecx,r15d
 jae pmccBad
 mov eax,[rdx+PD_FIELD_FLAGS]
 mov [rsp+20h],eax
 lea r14,[rsi+r14]
 xor eax,eax
pmccScan:
 cmp eax,ecx
 jae pmccNul
 cmp word ptr [r14+rax*2],0
 je pmccBad
 inc eax
 jmp pmccScan
pmccNul:
 cmp word ptr [r14+rcx*2],0
 jne pmccBad
 lea r15d,[rcx*2+2]
 mov rax,pmBytes
 add rax,r15
 jc pmccMemory
 cmp rax,pmCacheBudget
 ja pmccMemory
 call GetProcessHeap
 mov rcx,rax
 xor edx,edx
 mov r8,r15
 call qword ptr pmCacheAllocate
 test rax,rax
 jz pmccMemory
 mov [rdi+PM_F_POINTER],rax
 mov [rdi+PM_F_BYTES],r15d
 add pmBytes,r15
 mov rcx,r15
 shr rcx,1
 mov rsi,r14
 mov rdx,rdi
 mov rdi,rax
 rep movsw
 mov rdi,rdx
 mov dword ptr [rdi+PM_F_STATUS],PM_STATUS_VALID
 mov dword ptr [rdi+PM_F_ERROR],0
 mov eax,[rsp+20h]
 mov [rdi+PM_F_FLAGS],eax
 mov rax,pmNow
 mov [rdi+PM_F_TICK],rax
 add rax,PM_TTL_MS
 mov [rdi+PM_F_DUE],rax
 mov qword ptr [rdi+PM_F_SERIAL],0
 jmp pmccAccepted
pmccMemory:
 mov r12d,8
 mov dword ptr [rsp+24h],0
pmccError:
 mov dword ptr [rdi+PM_F_STATUS],PM_STATUS_UNAVAILABLE
 cmp r12d,1460
 jne pmccErrorSet
 mov dword ptr [rdi+PM_F_STATUS],PM_STATUS_TIMEOUT
pmccErrorSet:
 mov [rdi+PM_F_ERROR],r12d
 mov eax,[rsp+24h]
 mov [rdi+PM_F_FLAGS],eax
 mov rax,pmNow
 mov [rdi+PM_F_TICK],rax
 add rax,PM_RETRY_MS
 mov [rdi+PM_F_DUE],rax
 mov qword ptr [rdi+PM_F_SERIAL],0
 jmp pmccAccepted
pmccLate:
 mov dword ptr [rdi+PM_F_STATUS],PM_STATUS_TIMEOUT
 mov dword ptr [rdi+PM_F_ERROR],1460
 mov rax,pmNow
 add rax,PM_RETRY_MS
 mov [rdi+PM_F_DUE],rax
 mov qword ptr [rdi+PM_F_SERIAL],0
 mov eax,1460
 jmp pmccDone
pmccTerminal:
 or dword ptr [r13+PM_E_FLAGS],2
 lea r14,[r13+PM_E_FIELDS]
 mov r15d,PM_LANES
pmccTerminalLoop:
 mov rcx,r14
 call PmCacheFreeField
 mov dword ptr [r14+PM_F_STATUS],PM_STATUS_UNAVAILABLE
 mov dword ptr [r14+PM_F_ERROR],1168
 mov dword ptr [r14+PM_F_FLAGS],0
 mov qword ptr [r14+PM_F_SERIAL],0
 add r14,PM_FIELD_SIZE
 dec r15d
 jnz pmccTerminalLoop
pmccAccepted:
 xor eax,eax
 jmp pmccDone
pmccBad:
 mov eax,13
pmccDone:
 add rsp,30h
 pop r15
 pop r14
 pop r13
 pop r12
 pop rdi
 pop rsi
 pop rbx
 ret
PmCacheComplete ENDP
