; Owned, deterministic model fixtures; no process/provider calls.
.data?
pmtRequest db PD_REQUEST_SIZE dup(?)
pmtSnapshot db PD_SNAPSHOT_SIZE dup(?)
pmtSaved db PD_REQUEST_SIZE dup(?)
.data
pmtStage dd 0
pmtCookie dq 0
pmtSerial dq 0
pmtHeaderOffsets dd PD_S_VERSION,PD_S_LANE,PD_S_COOKIE,PD_S_GENERATION,PD_S_SEQUENCE,PD_S_EPOCH,PD_S_PID,PD_S_BIRTH,PD_S_CONTEXT
.code
PMT_EQ MACRO actual,expected
 cmp actual,expected
 jne pmtFail
ENDM
; Seed only the lane requested; other field statuses intentionally stay missing.
PmtSnapshot PROC
 push rsi
 push rdi
 sub rsp,28h
 lea rdi,pmtSnapshot
 xor eax,eax
 mov ecx,PD_SNAPSHOT_SIZE/8
 rep stosq
 lea rsi,pmtRequest
 lea rdi,pmtSnapshot
 mov ecx,PD_REQUEST_SIZE/8
 rep movsq
 mov dword ptr [pmtSnapshot+PD_S_SIZE],PD_SNAPSHOT_SIZE
 mov dword ptr [pmtSnapshot+PD_S_TARGET],PD_TARGET_VERIFIED
 mov dword ptr [pmtSnapshot+PD_S_ERROR],0
 mov rax,qword ptr [pmtRequest+PD_R_BIRTH]
 mov qword ptr [pmtSnapshot+PD_S_OBSERVED_BIRTH],rax
 ; Independent PD source ABI oracle: native writes DWORD processor count;
 ; optional leaves it zero. Snapshot byte extent is PD_S_SIZE alone.
 mov dword ptr [pmtSnapshot+PD_S_CAPACITY],0
 cmp dword ptr [pmtRequest+PD_R_LANE],PM_LANE_PATH
 jne pmtssCapacityReady
 mov dword ptr [pmtSnapshot+PD_S_CAPACITY],64
pmtssCapacityReady:
 mov qword ptr [pmtSnapshot+PD_S_CONTEXT],0
 mov eax,dword ptr [pmtRequest+PD_R_LANE]
 lea rdx,[pmtSnapshot+PD_S_FIELDS+PD_F_PATH*PD_FIELD_SIZE]
 cmp eax,1
 je pmtssSource
 lea rdx,[pmtSnapshot+PD_S_FIELDS+PD_F_ACCOUNT*PD_FIELD_SIZE]
 cmp eax,2
 je pmtssSource
 lea rdx,[pmtSnapshot+PD_S_FIELDS+PD_F_COMMAND*PD_FIELD_SIZE]
pmtssSource:
 mov dword ptr [rdx+PD_FIELD_STATUS],PD_STATUS_VALID
 mov dword ptr [pmtSnapshot+PD_S_FIELDS+PD_F_IDENTITY*16+PD_FIELD_STATUS],PD_STATUS_VALID
 add rsp,28h
 pop rdi
 pop rsi
 ret
PmtSnapshot ENDP
PmtPrepare PROC
 ; caller supplies lane in R8D; fixed identity.
 mov ecx,101
 mov edx,123
 lea r9,pmtRequest
 jmp PmCachePrepare
PmtPrepare ENDP
PmtComplete PROC
 lea rcx,pmtRequest
 lea rdx,pmtSnapshot
 xor r8d,r8d
 jmp PmCacheComplete
PmtComplete ENDP
PmtFailAlloc PROC
 xor eax,eax
 ret
PmtFailAlloc ENDP
.code
PmCacheTest PROC
  push rbx
  push rsi
  push rdi
  sub rsp,20h
  mov pmtStage,1
  call PmCacheReset
  mov ecx,101
  mov edx,123
  call PmCacheEnsure
  test rax,rax
  jz pmtFail
  mov ecx,7
  mov edx,1
  call PmCacheSetDemand
  PMT_EQ eax,0
  mov r8d,1
  call PmtPrepare
  PMT_EQ eax,0
  call PmtSnapshot
  ; path = three WCHARs, separately allocated and independent of snapshot.
  mov dword ptr [pmtSnapshot+PD_S_FIELDS+PD_F_PATH*16+PD_FIELD_LENGTH],3
  mov rax,0000006300620061h
  mov qword ptr [pmtSnapshot+PD_S_PATH],rax
  call PmtComplete
  PMT_EQ eax,0
  mov ecx,101
  mov edx,123
  mov r8d,1
  call PmCacheGet
  PMT_EQ edx,PM_STATUS_VALID
  test rax,rax
  jz pmtFail
  mov rbx,rax
  PMT_EQ word ptr [rax],61h
  PMT_EQ pmBytes,8
  mov word ptr [pmtSnapshot+PD_S_PATH],78h
  PMT_EQ word ptr [rbx],61h
  ; Valid empty command is an allocated NUL, account failure is independent.
  mov pmtStage,2
  mov r8d,3
  call PmtPrepare
  PMT_EQ eax,0
  call PmtSnapshot
  call PmtComplete
  PMT_EQ eax,0
  PMT_EQ pmBytes,10
  mov r8d,2
  call PmtPrepare
  PMT_EQ eax,0
  lea rcx,pmtRequest
  xor edx,edx
  mov r8d,5
  call PmCacheComplete
  PMT_EQ eax,0
  mov ecx,101
  mov edx,123
  mov r8d,2
  call PmCacheGet
  PMT_EQ edx,PM_STATUS_UNAVAILABLE
  PMT_EQ r8d,5
  PMT_EQ rax,0
  mov ecx,101
  mov edx,123
  mov r8d,3
  call PmCacheGet
  PMT_EQ edx,PM_STATUS_VALID
  test rax,rax
  jz pmtFail
  PMT_EQ word ptr [rax],0
  ; Retry before/at boundary; independent serial, duplicate rejection.
  mov pmtStage,3
  mov ecx,14999
  call PmCacheSetClock
  mov r8d,2
  call PmtPrepare
  PMT_EQ eax,170
  mov ecx,15000
  call PmCacheSetClock
  mov r8d,2
  call PmtPrepare
  PMT_EQ eax,0
  call PmtSnapshot
  inc qword ptr [pmtSnapshot+PD_S_SEQUENCE]
  call PmtComplete
  PMT_EQ eax,13
  dec qword ptr [pmtSnapshot+PD_S_SEQUENCE]
  call PmtComplete
  PMT_EQ eax,0
  call PmtComplete
  PMT_EQ eax,13
  ; TTL immediately before/at boundary never exposes expired text.
  mov pmtStage,4
  mov ecx,59999
  call PmCacheSetClock
  mov ecx,101
  mov edx,123
  mov r8d,1
  call PmCacheGet
  PMT_EQ edx,PM_STATUS_VALID
  mov ecx,60000
  call PmCacheSetClock
  mov ecx,101
  mov edx,123
  mov r8d,1
  call PmCacheGet
  PMT_EQ edx,PM_STATUS_LOADING
  PMT_EQ rax,0
  mov r8d,1
  call PmtPrepare
  PMT_EQ eax,0
  PMT_EQ pmBytes,4
  call PmtSnapshot
  ; Cookie mutation cannot turn an otherwise correct result into valid.
  inc qword ptr [pmtRequest+PD_R_COOKIE]
  inc qword ptr [pmtSnapshot+PD_S_COOKIE]
  call PmtComplete
  PMT_EQ eax,13
  dec qword ptr [pmtRequest+PD_R_COOKIE]
  dec qword ptr [pmtSnapshot+PD_S_COOKIE]
  mov ecx,62999
  call PmCacheSetClock
  lea rcx,pmtRequest
  call PmCacheExpire
  PMT_EQ eax,0
  mov ecx,63000
  call PmCacheSetClock
  lea rcx,pmtRequest
  call PmCacheExpire
  PMT_EQ eax,1
  call PmtComplete
  PMT_EQ eax,1460
  mov r8d,1
  call PmtPrepare
  PMT_EQ eax,170
  mov ecx,78000
  call PmCacheSetClock
  mov r8d,1
  call PmtPrepare
  PMT_EQ eax,0
  call PmtSnapshot
  ; UTF16 exact length, cap, terminator, embedded NUL and malformed header.
  mov pmtStage,5
  inc dword ptr [pmtSnapshot+PD_S_SIZE]
  call PmtComplete
  PMT_EQ eax,13
  dec dword ptr [pmtSnapshot+PD_S_SIZE]
  xor ebx,ebx
pmtBadHeaders:
  lea rax,pmtHeaderOffsets
  mov esi,[rax+rbx*4]
  lea rax,pmtSnapshot
  inc qword ptr [rax+rsi]
  call PmtComplete
  PMT_EQ eax,13
  lea rax,pmtSnapshot
  dec qword ptr [rax+rsi]
  inc ebx
  cmp ebx,9
  jb pmtBadHeaders
  mov dword ptr [pmtSnapshot+PD_S_FIELDS+PD_F_PATH*16+PD_FIELD_LENGTH],PD_PATH_CAPACITY
  call PmtComplete
  PMT_EQ eax,13
  mov dword ptr [pmtSnapshot+PD_S_FIELDS+PD_F_PATH*16+PD_FIELD_LENGTH],3
  mov rax,0000006300000061h
  mov qword ptr [pmtSnapshot+PD_S_PATH],rax
  call PmtComplete
  PMT_EQ eax,13
  mov rax,0064006300620061h
  mov qword ptr [pmtSnapshot+PD_S_PATH],rax
  call PmtComplete
  PMT_EQ eax,13
  mov word ptr [pmtSnapshot+PD_S_PATH+6],0
  ; Budget and allocation failures require no snapshot-sized cache object.
  mov pmtStage,6
  mov pmCacheBudget,4
  call PmtComplete
  PMT_EQ eax,0
  PMT_EQ pmBytes,4
  mov ecx,93000
  call PmCacheSetClock
  mov r8d,1
  call PmtPrepare
  PMT_EQ eax,0
  call PmtSnapshot
  lea rax,PmtFailAlloc
  mov pmCacheAllocate,rax
  mov pmCacheBudget,PM_STRING_BUDGET
  call PmtComplete
  PMT_EQ eax,0
  lea rax,HeapAlloc
  mov pmCacheAllocate,rax
  mov ecx,101
  mov edx,123
  mov r8d,1
  call PmCacheGet
  PMT_EQ edx,PM_STATUS_UNAVAILABLE
  PMT_EQ r8d,8
  ; Demand changes cancel serials without freeing valid observations.
  mov pmtStage,7
  mov ecx,108000
  call PmCacheSetClock
  mov r8d,1
  call PmtPrepare
  PMT_EQ eax,0
  call PmtSnapshot
  mov ecx,6
  mov edx,1
  call PmCacheSetDemand
  call PmtComplete
  PMT_EQ eax,13
  mov ecx,7
  mov edx,1
  call PmCacheSetDemand
  mov r8d,1
  call PmtPrepare
  PMT_EQ eax,0
  call PmtSnapshot
  ; Max accepted long path, exact 65534-byte allocation.
  lea rdi,[pmtSnapshot+PD_S_PATH]
  mov ecx,PD_PATH_CAPACITY-1
  mov eax,65h
  rep stosw
  mov word ptr [rdi],0
  mov dword ptr [pmtSnapshot+PD_S_FIELDS+PD_F_PATH*16+PD_FIELD_LENGTH],PD_PATH_CAPACITY-1
  call PmtComplete
  PMT_EQ eax,0
  PMT_EQ pmBytes,65538
  ; Timer-only timeout retains original serial/deadline, retries at +15000.
  mov r8d,2
  call PmtPrepare
  PMT_EQ eax,0
  mov ecx,111000
  call PmCacheSetClock
  lea rcx,pmtRequest
  call PmCacheExpire
  PMT_EQ eax,1
  mov ecx,125999
  call PmCacheSetClock
  mov r8d,2
  call PmtPrepare
  PMT_EQ eax,170
  mov ecx,126000
  call PmCacheSetClock
  mov r8d,2
  call PmtPrepare
  PMT_EQ eax,0
  ; Slot/cookie/serial reuse and all 5000 identities remain findable.
  mov pmtStage,8
  mov ecx,1
  lea rdx,pmtRequest
  call PmCacheNext
  PMT_EQ eax,259
  mov rax,pmCookieGenerator
  mov pmtCookie,rax
  mov rax,pmSerialGenerator
  mov pmtSerial,rax
  call PmCacheReset
  mov ecx,7
  mov edx,1
  call PmCacheSetDemand
  mov ebx,1
pmt5000:
  mov ecx,ebx
  mov edx,ebx
  shl rdx,32
  or rdx,123
  call PmCacheEnsure
  test rax,rax
  jz pmtFail
  inc ebx
  cmp ebx,5000
  jbe pmt5000
  PMT_EQ pmEntryCount,5000
  call PmCacheBeginFrame
  mov ebx,1
pmtMark:
  mov ecx,ebx
  mov edx,ebx
  shl rdx,32
  or rdx,123
  call PmCacheMark
  test rax,rax
  jz pmtFail
  inc ebx
  cmp ebx,5000
  jbe pmtMark
  call PmCachePrune
  PMT_EQ pmEntryCount,5000
  ; Fill to exact descriptor cap, reject extra without clobbering hash.
  mov pmtStage,9
  mov ebx,5001
pmtFill:
  mov ecx,ebx
  mov edx,ebx
  shl rdx,32
  or rdx,123
  call PmCacheEnsure
  test rax,rax
  jz pmtFail
  inc ebx
  cmp ebx,PM_CACHE_CAPACITY
  jbe pmtFill
  mov ecx,10000
  mov edx,10000
  call PmCacheEnsure
  PMT_EQ rax,0
  PMT_EQ pmEntryCount,PM_CACHE_CAPACITY
  ; A removed identity cannot consume an old copied request after slot reuse.
  mov ecx,1
  lea rdx,pmtRequest
  call PmCacheNext
  PMT_EQ eax,0
  mov rax,qword ptr [pmtRequest+PD_R_SEQUENCE]
  cmp rax,pmtSerial
  jbe pmtFail
  call PmtSnapshot
  call PmCacheBeginFrame
  call PmCachePrune
  PMT_EQ pmEntryCount,0
  mov ecx,dword ptr [pmtRequest+PD_R_PID]
  mov rdx,qword ptr [pmtRequest+PD_R_BIRTH]
  call PmCacheEnsure
  test rax,rax
  jz pmtFail
  mov rcx,[rax+PM_E_COOKIE]
  cmp rcx,pmtCookie
  jbe pmtFail
  call PmtComplete
  PMT_EQ eax,13
  ; Reused identity can obtain a new request; source terminal clears all lanes.
  mov ecx,1
  lea rdx,pmtRequest
  call PmCacheNext
  PMT_EQ eax,0
  call PmtSnapshot
  inc qword ptr [pmtRequest+PD_R_EPOCH]
  inc qword ptr [pmtSnapshot+PD_S_EPOCH]
  call PmtComplete
  PMT_EQ eax,13
  dec qword ptr [pmtRequest+PD_R_EPOCH]
  dec qword ptr [pmtSnapshot+PD_S_EPOCH]
  inc qword ptr [pmtRequest+PD_R_SEQUENCE]
  inc qword ptr [pmtSnapshot+PD_S_SEQUENCE]
  call PmtComplete
  PMT_EQ eax,13
  dec qword ptr [pmtRequest+PD_R_SEQUENCE]
  dec qword ptr [pmtSnapshot+PD_S_SEQUENCE]
  inc qword ptr [pmtRequest+PD_R_DEADLINE]
  call PmtComplete
  PMT_EQ eax,13
  dec qword ptr [pmtRequest+PD_R_DEADLINE]
  mov dword ptr [pmtSnapshot+PD_S_TARGET],PD_TARGET_EXITED
  call PmtComplete
  PMT_EQ eax,0
  mov ecx,1
  lea rdx,pmtSaved
  call PmCacheNext
  PMT_EQ eax,259
  PMT_EQ pmBytes,0
  ; Malformed prepare leaves every caller byte unchanged.
  mov pmtStage,10
  lea rdi,pmtSaved
  mov eax,0A5h
  mov ecx,PD_REQUEST_SIZE
  rep stosb
  mov ecx,101
  mov edx,123
  mov r8d,4
  lea r9,pmtSaved
  call PmCachePrepare
  PMT_EQ eax,87
  lea rsi,pmtSaved
  mov ecx,PD_REQUEST_SIZE
pmtCanary:
  PMT_EQ byte ptr [rsi],0A5h
  inc rsi
  loop pmtCanary
  call PmCacheReset
  PMT_EQ pmBytes,0
  ; Path/account accept unchanged held identity despite ambiguous liveness;
  ; command continues to require positively VERIFIED liveness.
  mov pmtStage,11
  mov ecx,101
  mov edx,123
  call PmCacheEnsure
  mov ecx,7
  mov edx,1
  call PmCacheSetDemand
  mov r8d,1
  call PmtPrepare
  PMT_EQ eax,0
  call PmtSnapshot
  mov dword ptr [pmtSnapshot+PD_S_TARGET],PD_TARGET_UNKNOWN
  call PmtComplete
  PMT_EQ eax,0
  mov r8d,2
  call PmtPrepare
  PMT_EQ eax,0
  call PmtSnapshot
  mov dword ptr [pmtSnapshot+PD_S_TARGET],PD_TARGET_UNKNOWN
  mov dword ptr [pmtSnapshot+PD_S_FIELDS+PD_F_IDENTITY*16+PD_FIELD_STATUS],PD_STATUS_UNAVAILABLE
  call PmtComplete
  PMT_EQ eax,13
  mov dword ptr [pmtSnapshot+PD_S_FIELDS+PD_F_IDENTITY*16+PD_FIELD_STATUS],PD_STATUS_VALID
  inc qword ptr [pmtSnapshot+PD_S_OBSERVED_BIRTH]
  call PmtComplete
  PMT_EQ eax,13
  dec qword ptr [pmtSnapshot+PD_S_OBSERVED_BIRTH]
  call PmtComplete
  PMT_EQ eax,0
  mov r8d,3
  call PmtPrepare
  PMT_EQ eax,0
  call PmtSnapshot
  mov dword ptr [pmtSnapshot+PD_S_TARGET],PD_TARGET_UNKNOWN
  call PmtComplete
  PMT_EQ eax,13
  ; Provider errors retain HRESULT/toolong source flags on CacheGet.
  mov pmtStage,12
  mov dword ptr [pmtSnapshot+PD_S_TARGET],PD_TARGET_VERIFIED
  mov dword ptr [pmtSnapshot+PD_S_FIELDS+PD_F_COMMAND*16+PD_FIELD_STATUS],PD_STATUS_UNAVAILABLE
  mov dword ptr [pmtSnapshot+PD_S_FIELDS+PD_F_COMMAND*16+PD_FIELD_ERROR],80004005h
  mov dword ptr [pmtSnapshot+PD_S_FIELDS+PD_F_COMMAND*16+PD_FIELD_FLAGS],1
  call PmtComplete
  PMT_EQ eax,0
  mov ecx,101
  mov edx,123
  mov r8d,3
  call PmCacheGet
  PMT_EQ edx,PM_STATUS_UNAVAILABLE
  PMT_EQ r8d,80004005h
  PMT_EQ r9d,1
  mov ecx,15000
  call PmCacheSetClock
  mov r8d,3
  call PmtPrepare
  PMT_EQ eax,0
  call PmtSnapshot
  mov dword ptr [pmtSnapshot+PD_S_FIELDS+PD_F_COMMAND*16+PD_FIELD_STATUS],PD_STATUS_TIMEOUT
  mov dword ptr [pmtSnapshot+PD_S_FIELDS+PD_F_COMMAND*16+PD_FIELD_ERROR],1460
  mov dword ptr [pmtSnapshot+PD_S_FIELDS+PD_F_COMMAND*16+PD_FIELD_FLAGS],2
  call PmtComplete
  PMT_EQ eax,0
  mov ecx,101
  mov edx,123
  mov r8d,3
  call PmCacheGet
  PMT_EQ edx,PM_STATUS_TIMEOUT
  PMT_EQ r8d,1460
  PMT_EQ r9d,2
  ; Local allocation and API failures have no provider flags to preserve.
  mov ecx,30000
  call PmCacheSetClock
  mov r8d,3
  call PmtPrepare
  PMT_EQ eax,0
  call PmtSnapshot
  mov dword ptr [pmtSnapshot+PD_S_FIELDS+PD_F_COMMAND*16+PD_FIELD_FLAGS],1
  mov pmCacheBudget,0
  call PmtComplete
  PMT_EQ eax,0
  mov pmCacheBudget,PM_STRING_BUDGET
  mov ecx,101
  mov edx,123
  mov r8d,3
  call PmCacheGet
  PMT_EQ edx,PM_STATUS_UNAVAILABLE
  PMT_EQ r8d,8
  PMT_EQ r9d,0
  mov ecx,60000
  call PmCacheSetClock
  mov r8d,2
  call PmtPrepare
  PMT_EQ eax,0
  lea rcx,pmtRequest
  xor edx,edx
  mov r8d,5
  call PmCacheComplete
  PMT_EQ eax,0
  mov ecx,101
  mov edx,123
  mov r8d,2
  call PmCacheGet
  PMT_EQ edx,PM_STATUS_UNAVAILABLE
  PMT_EQ r8d,5
  PMT_EQ r9d,0
  call PmCacheReset
  PMT_EQ pmBytes,0
  xor eax,eax
  jmp pmtDone
pmtFail:
  mov eax,pmtStage
pmtDone:
  add rsp,20h
  pop rdi
  pop rsi
  pop rbx
  ret
PmCacheTest ENDP
