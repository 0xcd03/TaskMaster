; Reentrant table-only field readers. PD helper seams are read-only in production.
.code
PmValidateRequest PROC
  mov eax,87
  test rcx,rcx
  jz pmvrDone
  cmp dword ptr [rcx+PD_R_SIZE],PD_REQUEST_SIZE
  jne pmvrDone
  cmp dword ptr [rcx+PD_R_VERSION],PD_VERSION
  jne pmvrDone
  cmp dword ptr [rcx+PD_R_LANE],1
  jb pmvrDone
  cmp dword ptr [rcx+PD_R_LANE],3
  ja pmvrDone
  cmp dword ptr [rcx+PD_R_FLAGS],0
  jne pmvrDone
  cmp dword ptr [rcx+PD_R_RESERVED],0
  jne pmvrDone
  cmp qword ptr [rcx+PD_R_CONTEXT],0
  jne pmvrDone
  cmp qword ptr [rcx+PD_R_CANCEL],0
  jne pmvrDone
  cmp qword ptr [rcx+PD_R_COOKIE],0
  je pmvrDone
  cmp qword ptr [rcx+PD_R_GENERATION],0
  je pmvrDone
  cmp qword ptr [rcx+PD_R_SEQUENCE],0
  je pmvrDone
  cmp qword ptr [rcx+PD_R_EPOCH],0
  je pmvrDone
  cmp dword ptr [rcx+PD_R_PID],0
  je pmvrDone
  cmp qword ptr [rcx+PD_R_BIRTH],0
  je pmvrDone
  cmp qword ptr [rcx+PD_R_DEADLINE],0
  je pmvrDone
  cmp qword ptr [rcx+PD_R_NOTIFY],0
  je pmvrDone
  xor eax,eax
pmvrDone:
  ret
PmValidateRequest ENDP

PmCollectField PROC
  push rsi
  push rdi
  sub rsp,98h
  mov [rsp+80h],rdx
  mov [rsp+88h],r8d
  call PmValidateRequest
  test eax,eax
  jnz pmcfDone
  mov eax,87
  cmp qword ptr [rsp+80h],0
  je pmcfDone
  mov eax,122
  cmp dword ptr [rsp+88h],PD_SNAPSHOT_SIZE
  jb pmcfDone
  mov rsi,rcx
  lea rdi,[rsp+20h]
  mov ecx,PD_REQUEST_SIZE/8
  rep movsq
  lea rcx,[rsp+20h]
  call PdOptionalCheck
  test eax,eax
  jnz pmcfDone
  lea rcx,[rsp+20h]
  mov rdx,[rsp+80h]
  mov r8d,[rsp+88h]
  cmp dword ptr [rcx+PD_R_LANE],1
  jne pmcfOptional
  call PdCollectNative
  jmp pmcfDone
pmcfOptional:
  call PmCollectOptionalField
pmcfDone:
  add rsp,98h
  pop rdi
  pop rsi
  ret
PmCollectField ENDP

PmCollectOptionalField PROC
  push rbx
  push rsi
  push rdi
  push r12
  sub rsp,28h
  mov rbx,rcx
  mov rsi,rdx
  xor edi,edi
  xor r12d,r12d
  mov eax,87
  test rbx,rbx
  jz pmofReturn
  test rsi,rsi
  jz pmofReturn
  cmp r8d,PD_SNAPSHOT_SIZE
  jb pmofSmall
  cmp dword ptr [rbx+PD_R_SIZE],PD_REQUEST_SIZE
  jne pmofReturn
  cmp dword ptr [rbx+PD_R_VERSION],PD_VERSION
  jne pmofReturn
  cmp dword ptr [rbx+PD_R_FLAGS],0
  jne pmofReturn
  cmp dword ptr [rbx+PD_R_RESERVED],0
  jne pmofReturn
  cmp qword ptr [rbx+PD_R_CONTEXT],0
  jne pmofReturn
  cmp qword ptr [rbx+PD_R_EPOCH],0
  je pmofReturn
  cmp qword ptr [rbx+PD_R_DEADLINE],0
  je pmofReturn
  mov rdi,rsi
  xor eax,eax
  mov ecx,PD_SNAPSHOT_SIZE/8
  rep stosq
  xor edi,edi
  mov dword ptr [rsi+PD_S_SIZE],PD_SNAPSHOT_SIZE
  mov dword ptr [rsi+PD_S_VERSION],PD_VERSION
  mov eax,[rbx+PD_R_LANE]
  mov [rsi+PD_S_LANE],eax
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
pmofFields:
  mov dword ptr [r10],PD_STATUS_NOT_REQUESTED
  add r10,16
  dec ecx
  jnz pmofFields
  mov dword ptr [rsi+PD_S_FIELDS+PD_F_IDENTITY*16],PD_STATUS_UNAVAILABLE
  mov dword ptr [rsi+PD_S_FIELDS+PD_F_LIVENESS*16],PD_STATUS_UNAVAILABLE
  mov eax,PD_F_ACCOUNT*16
  cmp dword ptr [rbx+PD_R_LANE],2
  je pmofInitSource
  mov eax,PD_F_COMMAND*16
pmofInitSource:
  mov dword ptr [rsi+PD_S_FIELDS+rax],PD_STATUS_UNAVAILABLE
  mov dword ptr [rsi+PD_S_FIELDS+rax+4],21
  mov dword ptr [rsi+PD_S_FIELDS+PD_F_IDENTITY*16+4],21
  mov dword ptr [rsi+PD_S_FIELDS+PD_F_LIVENESS*16+4],21
  call qword ptr pdOptionalClock
  mov [rsi+PD_S_TICK],rax
  mov rcx,rbx
  call PdOptionalCheck
  test eax,eax
  jnz pmofCheckError
  cmp qword ptr [rbx+PD_R_BIRTH],0
  jne pmofOpen
  mov dword ptr [rsi+PD_S_TARGET],PD_TARGET_MISMATCH
  mov eax,87
  jmp pmofTargetError
pmofOpen:
  mov ecx,101000h
  xor edx,edx
  mov r8d,[rbx+PD_R_PID]
  call qword ptr pdOptionalOpenProcess
  mov rdi,rax
  test rax,rax
  jz pmofLimited
  mov r12d,1
  jmp pmofVerify
pmofLimited:
  mov ecx,1000h
  xor edx,edx
  mov r8d,[rbx+PD_R_PID]
  call qword ptr pdOptionalOpenProcess
  mov rdi,rax
  test rax,rax
  jnz pmofVerify
  call GetLastError
  cmp eax,5
  jne pmofTargetError
  mov dword ptr [rsi+PD_S_TARGET],PD_TARGET_DENIED
  jmp pmofTargetError
pmofVerify:
  mov rcx,rbx
  mov rdx,rsi
  mov r8,rdi
  mov r9d,r12d
  call PdOptionalVerify
  cmp dword ptr [rsi+PD_S_FIELDS+PD_F_IDENTITY*16],PD_STATUS_VALID
  jne pmofFailedIdentity
  cmp dword ptr [rsi+PD_S_TARGET],PD_TARGET_EXITED
  je pmofExited
  cmp dword ptr [rbx+PD_R_LANE],3
  je pmofCommand
  mov rcx,rbx
  mov rdx,rsi
  mov r8,rdi
  call PdOptionalAccount
  jmp pmofFinal
pmofCommand:
  cmp dword ptr [rsi+PD_S_TARGET],PD_TARGET_VERIFIED
  jne pmofAmbiguous
  mov rcx,rbx
  mov rdx,rsi
  call PdOptionalWmi
  jmp pmofFinal
pmofAmbiguous:
  mov dword ptr [rsi+PD_S_FIELDS+PD_F_COMMAND*16+4],50
pmofFinal:
  mov rcx,rbx
  call PdOptionalCheck
  test eax,eax
  jnz pmofCheckError
  mov rcx,rbx
  mov rdx,rsi
  mov r8,rdi
  mov r9d,r12d
  call PdOptionalVerify
  cmp dword ptr [rsi+PD_S_FIELDS+PD_F_IDENTITY*16],PD_STATUS_VALID
  jne pmofFailedIdentity
  cmp dword ptr [rsi+PD_S_TARGET],PD_TARGET_EXITED
  je pmofExited
  mov rcx,rbx
  call PdOptionalCheck
  test eax,eax
  jnz pmofCheckError
  cmp dword ptr [rbx+PD_R_LANE],2
  je pmofDone
  cmp dword ptr [rsi+PD_S_TARGET],PD_TARGET_VERIFIED
  je pmofDone
  ; An unchanged held identity permits account name, but cannot certify a
  ; command-line observation from an ambiguously live target.
  mov dword ptr [rsi+PD_S_FIELDS+PD_F_COMMAND*16],PD_STATUS_UNAVAILABLE
  mov dword ptr [rsi+PD_S_FIELDS+PD_F_COMMAND*16+4],50
  mov dword ptr [rsi+PD_S_FIELDS+PD_F_COMMAND*16+8],0
  mov dword ptr [rsi+PD_S_FIELDS+PD_F_COMMAND*16+12],0
  mov word ptr [rsi+PD_S_COMMAND],0
  jmp pmofDone
pmofExited:
  mov eax,1067
  jmp pmofInvalidate
pmofFailedIdentity:
  mov eax,[rsi+PD_S_ERROR]
  jmp pmofInvalidate
pmofTargetError:
  mov [rsi+PD_S_ERROR],eax
  mov [rsi+PD_S_FIELDS+PD_F_IDENTITY*16+4],eax
  mov [rsi+PD_S_FIELDS+PD_F_LIVENESS*16+4],eax
  jmp pmofInvalidate
pmofCheckError:
  mov [rsp+20h],eax
  test rdi,rdi
  jz pmofCheckedIdentity
  ; Even a late provider response must be followed by a fresh held-handle
  ; identity/liveness check before any optional value can be presented.
  mov rcx,rbx
  mov rdx,rsi
  mov r8,rdi
  mov r9d,r12d
  call PdOptionalVerify
  cmp dword ptr [rsi+PD_S_FIELDS+PD_F_IDENTITY*16],PD_STATUS_VALID
  jne pmofFailedIdentity
  cmp dword ptr [rsi+PD_S_TARGET],PD_TARGET_EXITED
  je pmofExited
pmofCheckedIdentity:
  mov eax,[rsp+20h]
  cmp eax,1223
  jne pmofTimeout
  mov dword ptr [rsi+PD_S_TARGET],PD_TARGET_CANCELLED
  mov [rsi+PD_S_ERROR],eax
  jmp pmofInvalidate
pmofTimeout:
  mov edx,PD_STATUS_TIMEOUT
  jmp pmofClear
pmofInvalidate:
  mov edx,PD_STATUS_UNAVAILABLE
pmofClear:
  mov ecx,PD_F_ACCOUNT*16
  mov r8d,PD_S_ACCOUNT
  cmp dword ptr [rbx+PD_R_LANE],2
  je pmofClearField
  mov ecx,PD_F_COMMAND*16
  mov r8d,PD_S_COMMAND
pmofClearField:
  mov dword ptr [rsi+PD_S_FIELDS+rcx],edx
  mov dword ptr [rsi+PD_S_FIELDS+rcx+4],eax
  mov qword ptr [rsi+PD_S_FIELDS+rcx+8],0
  mov word ptr [rsi+r8],0
pmofDone:
  mov rcx,rdi
  test rcx,rcx
  jz pmofSuccess
  call CloseHandle
pmofSuccess:
  xor eax,eax
  jmp pmofReturn
pmofSmall:
  mov eax,122
  jmp pmofReturn
pmofUnsupported:
  mov eax,50
pmofReturn:
  add rsp,28h
  pop r12
  pop rdi
  pop rsi
  pop rbx
  ret
PmCollectOptionalField ENDP
