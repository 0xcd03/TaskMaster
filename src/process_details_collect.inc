; Caller owns both blocks. No main collector/UI buffers, formatting or privilege
; changes are used here. Every process handle is held until its sample is done.
extern GetProcessId:proc, QueryPerformanceCounter:proc, QueryPerformanceFrequency:proc
extern GetProcessHandleCount:proc, ConvertSidToStringSidW:proc, GetExitCodeProcess:proc
.data
align 8
; Isolated tests may replace only access boundaries, never identity validation.
pdNativeOpenProcess dq OpenProcess
pdNativeMemoryInfo dq GetProcessMemoryInfo
.code
PdCollectNative PROC
  push rbx
  push rsi
  push rdi
  push r12
  push r13
  push r14
  push r15
  sub rsp,1A0h
  mov rbx,rcx
  mov rsi,rdx
  xor edi,edi
  xor r12d,r12d
  xor r13d,r13d
  xor r14d,r14d
  xor r15d,r15d
  mov qword ptr [rsp+128h],0
  mov eax,87
  test rbx,rbx
  jz pdcReturn
  test rsi,rsi
  jz pdcReturn
  cmp r8d,PD_SNAPSHOT_SIZE
  jb pdcSmall
  cmp dword ptr [rbx+PD_R_SIZE],PD_REQUEST_SIZE
  jne pdcReturn
  cmp dword ptr [rbx+PD_R_VERSION],PD_VERSION
  jne pdcReturn
  cmp dword ptr [rbx+PD_R_FLAGS],0
  jne pdcReturn
  cmp dword ptr [rbx+PD_R_RESERVED],0
  jne pdcReturn
  cmp qword ptr [rbx+PD_R_CONTEXT],0
  jne pdcReturn
  cmp dword ptr [rbx+PD_R_LANE],2
  je pdcUnsupported
  cmp dword ptr [rbx+PD_R_LANE],1
  jne pdcReturn
  mov rdi,rsi
  xor eax,eax
  mov ecx,PD_SNAPSHOT_SIZE/8
  rep stosq
  xor edi,edi
  mov dword ptr [rsi+PD_S_SIZE],PD_SNAPSHOT_SIZE
  mov dword ptr [rsi+PD_S_VERSION],PD_VERSION
  mov dword ptr [rsi+PD_S_LANE],1
  mov dword ptr [rsi+PD_S_TARGET],PD_TARGET_UNKNOWN
  mov rax,[rbx+PD_R_COOKIE]
  mov [rsi+PD_S_COOKIE],rax
  mov rax,[rbx+PD_R_GENERATION]
  mov [rsi+PD_S_GENERATION],rax
  mov rax,[rbx+PD_R_SEQUENCE]
  mov [rsi+PD_S_SEQUENCE],rax
  mov rax,[rbx+PD_R_EPOCH]
  mov [rsi+PD_S_EPOCH],rax
  mov eax,[rbx+PD_R_PID]
  mov [rsi+PD_S_PID],eax
  mov rax,[rbx+PD_R_BIRTH]
  mov [rsi+PD_S_BIRTH],rax
  mov ecx,12
  lea r10,[rsi+PD_S_FIELDS]
pdcFields:
  mov dword ptr [r10],PD_STATUS_NOT_REQUESTED
  add r10,PD_FIELD_SIZE
  dec ecx
  jnz pdcFields
  mov dword ptr [rsi+PD_S_FIELDS+PD_F_IDENTITY*16],PD_STATUS_UNAVAILABLE
  mov dword ptr [rsi+PD_S_FIELDS+PD_F_PATH*16],PD_STATUS_UNAVAILABLE
  mov dword ptr [rsi+PD_S_FIELDS+PD_F_SID*16],PD_STATUS_UNAVAILABLE
  mov dword ptr [rsi+PD_S_FIELDS+PD_F_HANDLES*16],PD_STATUS_UNAVAILABLE
  mov dword ptr [rsi+PD_S_FIELDS+PD_F_MEMORY*16],PD_STATUS_UNAVAILABLE
  mov dword ptr [rsi+PD_S_FIELDS+PD_F_CPU_TIMES*16],PD_STATUS_UNAVAILABLE
  mov dword ptr [rsi+PD_S_FIELDS+PD_F_IO*16],PD_STATUS_UNAVAILABLE
  mov dword ptr [rsi+PD_S_FIELDS+PD_F_LIVENESS*16],PD_STATUS_UNAVAILABLE
  lea r10,[rsi+PD_S_FIELDS]
  mov ecx,12
pdcPendingErrors:
  cmp dword ptr [r10],PD_STATUS_UNAVAILABLE
  jne pdcNextError
  mov dword ptr [r10+4],21 ; ERROR_NOT_READY until queried or target cause known
pdcNextError:
  add r10,16
  dec ecx
  jnz pdcPendingErrors
  call GetTickCount64
  mov [rsi+PD_S_TICK],rax
  lea rcx,[rsi+PD_S_QPC]
  call QueryPerformanceCounter
  lea rcx,[rsi+PD_S_FREQUENCY]
  call QueryPerformanceFrequency
  mov ecx,0FFFFh
  call GetActiveProcessorCount
  mov [rsi+PD_S_CAPACITY],eax
  mov rcx,[rbx+PD_R_CANCEL]
  test rcx,rcx
  jz pdcBirth
  xor edx,edx
  call WaitForSingleObject
  test eax,eax
  jnz pdcBirth
  mov dword ptr [rsi+PD_S_TARGET],PD_TARGET_CANCELLED
  mov eax,1223
  jmp pdcTargetError
pdcBirth:
  cmp qword ptr [rbx+PD_R_BIRTH],0
  jne pdcOpen
  mov dword ptr [rsi+PD_S_TARGET],PD_TARGET_MISMATCH
  mov eax,87 ; never acquire a new identity from a PID-only request
  jmp pdcTargetError
pdcOpen:
  mov ecx,101000h ; QUERY_LIMITED_INFORMATION | SYNCHRONIZE
  xor edx,edx
  mov r8d,[rbx+PD_R_PID]
  call qword ptr pdNativeOpenProcess
  mov rdi,rax
  test rax,rax
  jz pdcLimited
  mov r12d,1
  jmp pdcVerify
pdcLimited:
  mov ecx,1000h
  xor edx,edx
  mov r8d,[rbx+PD_R_PID]
  call qword ptr pdNativeOpenProcess
  mov rdi,rax
  test rax,rax
  jnz pdcVerify
  call GetLastError
  cmp eax,5
  jne pdcTargetError
  mov dword ptr [rsi+PD_S_TARGET],PD_TARGET_DENIED
  jmp pdcTargetError
pdcVerify:
  mov rcx,rdi
  lea rdx,[rsp+60h]
  lea r8,[rsp+68h]
  lea r9,[rsp+70h]
  lea rax,[rsp+78h]
  mov [rsp+20h],rax
  call GetProcessTimes
  test eax,eax
  jz pdcIdentityApiError
  mov rax,[rsp+60h]
  mov [rsi+PD_S_OBSERVED_BIRTH],rax
  cmp rax,[rbx+PD_R_BIRTH]
  jne pdcMismatch
  mov rcx,rdi
  call GetProcessId
  test eax,eax
  jz pdcIdentityApiError
  cmp eax,[rbx+PD_R_PID]
  jne pdcMismatch
  mov dword ptr [rsi+PD_S_FIELDS+PD_F_IDENTITY*16],PD_STATUS_VALID
  mov dword ptr [rsi+PD_S_FIELDS+PD_F_IDENTITY*16+4],0
  mov dword ptr [rsi+PD_S_TARGET],PD_TARGET_VERIFIED
  mov rax,[rsp+70h]
  mov [rsi+PD_S_KERNEL],rax
  mov rax,[rsp+78h]
  mov [rsi+PD_S_USER],rax
  mov dword ptr [rsi+PD_S_FIELDS+PD_F_CPU_TIMES*16],PD_STATUS_VALID
  mov dword ptr [rsi+PD_S_FIELDS+PD_F_CPU_TIMES*16+4],0
  mov rcx,rdi
  mov edx,r12d
  mov r8,rsi
  call PdNativeLiveness
  test eax,eax
  jnz pdcDone
  mov dword ptr [rsp+114h],PD_PATH_CAPACITY
  mov rcx,rdi
  xor edx,edx
  lea r8,[rsi+PD_S_PATH]
  lea r9,[rsp+114h]
  call QueryFullProcessImageNameW
  test eax,eax
  jz pdcPathError
  mov eax,[rsp+114h]
  cmp eax,PD_PATH_CAPACITY
  jae pdcPathTooLong
  mov word ptr [rsi+PD_S_PATH+rax*2],0
  mov [rsi+PD_S_FIELDS+PD_F_PATH*16+8],eax
  mov dword ptr [rsi+PD_S_FIELDS+PD_F_PATH*16],PD_STATUS_VALID
  mov dword ptr [rsi+PD_S_FIELDS+PD_F_PATH*16+4],0
  jmp pdcHandles
pdcPathTooLong:
  mov eax,122
  jmp pdcStorePathError
pdcPathError:
  call GetLastError
pdcStorePathError:
  mov [rsi+PD_S_FIELDS+PD_F_PATH*16+4],eax
  mov word ptr [rsi+PD_S_PATH],0
pdcHandles:
  mov dword ptr [rsp+11Ch],0
  mov rcx,rdi
  lea rdx,[rsp+11Ch]
  call GetProcessHandleCount
  test eax,eax
  jz pdcHandlesError
  mov eax,[rsp+11Ch]
  mov [rsi+PD_S_HANDLES],rax
  mov dword ptr [rsi+PD_S_FIELDS+PD_F_HANDLES*16],PD_STATUS_VALID
  mov dword ptr [rsi+PD_S_FIELDS+PD_F_HANDLES*16+4],0
  jmp pdcMemory
pdcHandlesError:
  call GetLastError
  mov [rsi+PD_S_FIELDS+PD_F_HANDLES*16+4],eax
pdcMemory:
  mov dword ptr [rsp+0B0h],80
  mov rcx,rdi
  lea rdx,[rsp+0B0h]
  mov r8d,80
  call qword ptr pdNativeMemoryInfo
  test eax,eax
  jnz pdcMemoryValid
  call GetLastError
  mov [rsi+PD_S_FIELDS+PD_F_MEMORY*16+4],eax
  mov ecx,410h ; optional legacy memory access only
  xor edx,edx
  mov r8d,[rbx+PD_R_PID]
  call qword ptr pdNativeOpenProcess
  mov r13,rax
  test rax,rax
  jz pdcMemoryError
  mov rcx,r13
  lea rdx,[rsp+140h]
  lea r8,[rsp+148h]
  lea r9,[rsp+150h]
  lea rax,[rsp+158h]
  mov [rsp+20h],rax
  call GetProcessTimes
  test eax,eax
  jz pdcMemoryError
  mov rax,[rsp+140h]
  cmp rax,[rbx+PD_R_BIRTH]
  jne pdcMemoryIdentityError
  mov rcx,r13
  call GetProcessId
  test eax,eax
  jz pdcMemoryError
  cmp eax,[rbx+PD_R_PID]
  jne pdcMemoryIdentityError
  mov rcx,r13
  lea rdx,[rsp+0B0h]
  mov r8d,80
  call qword ptr pdNativeMemoryInfo
  test eax,eax
  jz pdcMemoryError
pdcMemoryValid:
  mov rax,[rsp+0F8h] ; PROCESS_MEMORY_COUNTERS_EX.PrivateUsage
  mov [rsi+PD_S_PRIVATE],rax
  mov rax,[rsp+0C0h] ; WorkingSetSize
  mov [rsi+PD_S_WORKING_SET],rax
  mov dword ptr [rsi+PD_S_FIELDS+PD_F_MEMORY*16],PD_STATUS_VALID
  mov dword ptr [rsi+PD_S_FIELDS+PD_F_MEMORY*16+4],0
  jmp pdcIo
pdcMemoryIdentityError:
  mov eax,13
  jmp pdcStoreMemoryError
pdcMemoryError:
  call GetLastError
pdcStoreMemoryError:
  mov [rsi+PD_S_FIELDS+PD_F_MEMORY*16+4],eax
pdcIo:
  mov rcx,rdi
  lea rdx,[rsp+80h]
  call GetProcessIoCounters
  test eax,eax
  jz pdcIoError
  mov rax,[rsp+80h]
  mov [rsi+PD_S_READ_OPS],rax
  mov rax,[rsp+88h]
  mov [rsi+PD_S_WRITE_OPS],rax
  mov rax,[rsp+90h]
  mov [rsi+PD_S_OTHER_OPS],rax
  mov rax,[rsp+98h]
  mov [rsi+PD_S_READ_BYTES],rax
  mov rax,[rsp+0A0h]
  mov [rsi+PD_S_WRITE_BYTES],rax
  mov rax,[rsp+0A8h]
  mov [rsi+PD_S_OTHER_BYTES],rax
  mov dword ptr [rsi+PD_S_FIELDS+PD_F_IO*16],PD_STATUS_VALID
  mov dword ptr [rsi+PD_S_FIELDS+PD_F_IO*16+4],0
  jmp pdcToken
pdcIoError:
  call GetLastError
  mov [rsi+PD_S_FIELDS+PD_F_IO*16+4],eax
pdcToken:
  mov qword ptr [rsp+100h],0
  mov rcx,rdi
  mov edx,8 ; TOKEN_QUERY only; no account lookup on this lane
  lea r8,[rsp+100h]
  call OpenProcessToken
  test eax,eax
  jz pdcSidError
  mov r14,[rsp+100h]
  mov dword ptr [rsp+110h],0
  mov rcx,r14
  mov edx,1 ; TokenUser
  xor r8d,r8d
  xor r9d,r9d
  lea rax,[rsp+110h]
  mov [rsp+20h],rax
  call GetTokenInformation
  call GetLastError
  cmp eax,122
  jne pdcStoreSidError
  mov eax,[rsp+110h]
  cmp eax,16
  jb pdcSidInvalid
  cmp eax,65536
  ja pdcSidTooLong
  call GetProcessHeap
  mov rcx,rax
  mov edx,8
  mov r8d,[rsp+110h]
  call HeapAlloc
  mov r15,rax
  test rax,rax
  jz pdcSidMemory
  mov rcx,r14
  mov edx,1
  mov r8,r15
  mov r9d,[rsp+110h]
  lea rax,[rsp+110h]
  mov [rsp+20h],rax
  call GetTokenInformation
  test eax,eax
  jz pdcSidError
  mov rcx,[r15]
  lea rdx,[rsp+128h]
  call ConvertSidToStringSidW
  test eax,eax
  jz pdcSidError
  mov r10,[rsp+128h]
  xor eax,eax
pdcSidLength:
  cmp word ptr [r10+rax*2],0
  je pdcSidCopy
  inc eax
  cmp eax,PD_SID_CAPACITY
  jb pdcSidLength
pdcSidTooLong:
  mov eax,122
  jmp pdcStoreSidError
pdcSidCopy:
  mov [rsi+PD_S_FIELDS+PD_F_SID*16+8],eax
  lea rcx,[rsi+PD_S_SID]
  mov rdx,r10
  call lstrcpyW
  mov dword ptr [rsi+PD_S_FIELDS+PD_F_SID*16],PD_STATUS_VALID
  mov dword ptr [rsi+PD_S_FIELDS+PD_F_SID*16+4],0
  jmp pdcFinalLive
pdcSidMemory:
  mov eax,8
  jmp pdcStoreSidError
pdcSidInvalid:
  mov eax,13
  jmp pdcStoreSidError
pdcSidError:
  call GetLastError
pdcStoreSidError:
  mov [rsi+PD_S_FIELDS+PD_F_SID*16+4],eax
pdcFinalLive:
  mov rcx,rdi
  mov edx,r12d
  mov r8,rsi
  call PdNativeLiveness
  jmp pdcDone
pdcMismatch:
  mov dword ptr [rsi+PD_S_TARGET],PD_TARGET_MISMATCH
  mov eax,13
  jmp pdcTargetError
pdcIdentityApiError:
  call GetLastError
pdcTargetError:
  mov [rsi+PD_S_ERROR],eax
  mov [rsi+PD_S_FIELDS+PD_F_IDENTITY*16+4],eax
  ; Unqueried native fields carry the target's cause, never a fabricated zero.
  mov [rsi+PD_S_FIELDS+PD_F_PATH*16+4],eax
  mov [rsi+PD_S_FIELDS+PD_F_SID*16+4],eax
  mov [rsi+PD_S_FIELDS+PD_F_HANDLES*16+4],eax
  mov [rsi+PD_S_FIELDS+PD_F_MEMORY*16+4],eax
  mov [rsi+PD_S_FIELDS+PD_F_CPU_TIMES*16+4],eax
  mov [rsi+PD_S_FIELDS+PD_F_IO*16+4],eax
  mov [rsi+PD_S_FIELDS+PD_F_LIVENESS*16+4],eax
pdcDone:
  mov rcx,[rsp+128h]
  test rcx,rcx
  jz pdcFreeToken
  call LocalFree
pdcFreeToken:
  test r15,r15
  jz pdcCloseToken
  call GetProcessHeap
  mov rcx,rax
  xor edx,edx
  mov r8,r15
  call HeapFree
pdcCloseToken:
  mov rcx,r14
  test rcx,rcx
  jz pdcCloseMemory
  call CloseHandle
pdcCloseMemory:
  mov rcx,r13
  test rcx,rcx
  jz pdcCloseMain
  call CloseHandle
pdcCloseMain:
  mov rcx,rdi
  test rcx,rcx
  jz pdcSuccess
  call CloseHandle
pdcSuccess:
  xor eax,eax
  jmp pdcReturn
pdcSmall:
  mov eax,122
  jmp pdcReturn
pdcUnsupported:
  mov eax,50
pdcReturn:
  add rsp,1A0h
  pop r15
  pop r14
  pop r13
  pop r12
  pop rdi
  pop rsi
  pop rbx
  ret
PdCollectNative ENDP

; Called only after birth/PID verification. Exit code 259 without SYNCHRONIZE
; is ambiguous: a process can legitimately exit with STILL_ACTIVE as its code.
PdNativeLiveness PROC
  push rbx
  push rsi
  push rdi
  sub rsp,30h
  mov rbx,rcx
  mov edi,edx
  mov rsi,r8
  test edi,edi
  jz pdnlExitCode
  xor edx,edx
  call WaitForSingleObject
  test eax,eax
  jz pdnlExited
  cmp eax,258
  jne pdnlError
  mov dword ptr [rsi+PD_S_FIELDS+PD_F_LIVENESS*16],PD_STATUS_VALID
  mov dword ptr [rsi+PD_S_FIELDS+PD_F_LIVENESS*16+4],0
  xor eax,eax
  jmp pdnlReturn
pdnlExitCode:
  mov rcx,rbx
  lea rdx,[rsp+20h]
  call GetExitCodeProcess
  test eax,eax
  jz pdnlError
  cmp dword ptr [rsp+20h],259
  jne pdnlExited
  mov eax,50 ; cannot prove liveness from STILL_ACTIVE alone
  jmp pdnlUnknown
pdnlError:
  call GetLastError
pdnlUnknown:
  mov dword ptr [rsi+PD_S_TARGET],PD_TARGET_UNKNOWN
  mov [rsi+PD_S_ERROR],eax
  mov dword ptr [rsi+PD_S_FIELDS+PD_F_LIVENESS*16],PD_STATUS_UNAVAILABLE
  mov [rsi+PD_S_FIELDS+PD_F_LIVENESS*16+4],eax
  xor eax,eax
  jmp pdnlReturn
pdnlExited:
  mov dword ptr [rsi+PD_S_TARGET],PD_TARGET_EXITED
  mov dword ptr [rsi+PD_S_ERROR],0
  mov dword ptr [rsi+PD_S_FIELDS+PD_F_LIVENESS*16],PD_STATUS_VALID
  mov dword ptr [rsi+PD_S_FIELDS+PD_F_LIVENESS*16+4],0
  lea r10,[rsi+PD_S_FIELDS]
  mov ecx,12
pdnlExitedFields:
  cmp dword ptr [r10],PD_STATUS_UNAVAILABLE
  jne pdnlExitedNext
  mov dword ptr [r10+4],1067 ; ERROR_PROCESS_ABORTED, unqueried exited fields
pdnlExitedNext:
  add r10,16
  dec ecx
  jnz pdnlExitedFields
  mov eax,1
pdnlReturn:
  add rsp,30h
  pop rdi
  pop rsi
  pop rbx
  ret
PdNativeLiveness ENDP
