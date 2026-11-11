; Supported token/account and WMI APIs only. All scratch and COM ownership is
; local to this call; blocking provider calls are never forcibly terminated.
extern LookupAccountSidW:proc, CoSetProxyBlanket:proc, VariantClear:proc
extern SystemTimeToFileTime:proc
PD_OPTIONAL_ENGINE_READY equ 1
.data
align 8
pdOptionalOpenProcess dq OpenProcess
pdOptionalLookupAccount dq LookupAccountSidW
pdOptionalClock dq GetTickCount64
pdOptionalCreateInstance dq CoCreateInstance
pdOptionalProxyBlanket dq CoSetProxyBlanket
pdOptionalClass dd 4590F811h
dw 1D3Ah,11D0h
db 89h,1Fh,0,0AAh,0,4Bh,2Eh,24h
pdOptionalIID dd 0DC12A687h
dw 737Fh,11CFh
db 88h,4Dh,0,0AAh,0,4Bh,2Eh,24h
pdOptionalNamespace dw 'R','O','O','T','\','C','I','M','V','2',0
pdOptionalWql dw 'W','Q','L',0
pdOptionalQuery dw 'S','E','L','E','C','T',' ','P','r','o','c','e','s','s','I','d',','
dw 'C','r','e','a','t','i','o','n','D','a','t','e',',','C','o','m','m','a','n','d','L','i','n','e',' '
dw 'F','R','O','M',' ','W','i','n','3','2','_','P','r','o','c','e','s','s',' '
dw 'W','H','E','R','E',' ','P','r','o','c','e','s','s','I','d','=','%','u',0
pdOptionalPidName dw 'P','r','o','c','e','s','s','I','d',0
pdOptionalBirthName dw 'C','r','e','a','t','i','o','n','D','a','t','e',0
pdOptionalCommandName dw 'C','o','m','m','a','n','d','L','i','n','e',0
.code

; Returns 0 + EDX remaining finite milliseconds, 1460 deadline, 1223 cancel.
PdOptionalCheck PROC
  push rbx
  sub rsp,20h
  mov rbx,rcx
  mov rcx,[rbx+PD_R_CANCEL]
  test rcx,rcx
  jz pocClock
  xor edx,edx
  call WaitForSingleObject
  test eax,eax
  jz pocCancelled
pocClock:
  call qword ptr pdOptionalClock
  mov rdx,[rbx+PD_R_DEADLINE]
  sub rdx,rax
  jbe pocTimeout
  cmp rdx,3000
  jbe pocReady
  mov edx,3000
pocReady:
  xor eax,eax
  jmp pocReturn
pocTimeout:
  mov eax,1460
  jmp pocReturn
pocCancelled:
  mov eax,1223
pocReturn:
  add rsp,20h
  pop rbx
  ret
PdOptionalCheck ENDP

; A held native handle is verified both before and after slow provider calls.
; R9D says SYNCHRONIZE was obtained. EAX0 only means positively live.
PdOptionalVerify PROC
  push rbx
  push rsi
  push rdi
  sub rsp,60h
  mov rbx,rcx
  mov rsi,rdx
  mov rdi,r8
  mov [rsp+50h],r9d
  mov rcx,rdi
  lea rdx,[rsp+30h]
  lea r8,[rsp+38h]
  lea r9,[rsp+40h]
  lea rax,[rsp+48h]
  mov [rsp+20h],rax
  call GetProcessTimes
  test eax,eax
  jz povError
  mov rax,[rsp+30h]
  mov [rsi+PD_S_OBSERVED_BIRTH],rax
  cmp rax,[rbx+PD_R_BIRTH]
  jne povMismatch
  mov rcx,rdi
  call GetProcessId
  test eax,eax
  jz povError
  cmp eax,[rbx+PD_R_PID]
  jne povMismatch
  mov dword ptr [rsi+PD_S_FIELDS+PD_F_IDENTITY*16],PD_STATUS_VALID
  mov dword ptr [rsi+PD_S_FIELDS+PD_F_IDENTITY*16+4],0
  mov dword ptr [rsi+PD_S_TARGET],PD_TARGET_VERIFIED
  mov dword ptr [rsi+PD_S_ERROR],0
  mov rcx,rdi
  mov edx,[rsp+50h]
  mov r8,rsi
  call PdNativeLiveness
  xor eax,eax
  cmp dword ptr [rsi+PD_S_TARGET],PD_TARGET_VERIFIED
  sete al
  xor eax,1
  jmp povReturn
povMismatch:
  mov dword ptr [rsi+PD_S_TARGET],PD_TARGET_MISMATCH
  mov eax,13
  jmp povStore
povError:
  call GetLastError
  mov dword ptr [rsi+PD_S_TARGET],PD_TARGET_UNKNOWN
povStore:
  mov [rsi+PD_S_ERROR],eax
  mov dword ptr [rsi+PD_S_FIELDS+PD_F_IDENTITY*16],PD_STATUS_UNAVAILABLE
  mov [rsi+PD_S_FIELDS+PD_F_IDENTITY*16+4],eax
  mov eax,1
povReturn:
  add rsp,60h
  pop rdi
  pop rsi
  pop rbx
  ret
PdOptionalVerify ENDP

PdCollectOptional PROC
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
  jz pcoReturn
  test rsi,rsi
  jz pcoReturn
  cmp r8d,PD_SNAPSHOT_SIZE
  jb pcoSmall
  cmp dword ptr [rbx+PD_R_SIZE],PD_REQUEST_SIZE
  jne pcoReturn
  cmp dword ptr [rbx+PD_R_VERSION],PD_VERSION
  jne pcoReturn
  cmp dword ptr [rbx+PD_R_FLAGS],0
  jne pcoReturn
  cmp dword ptr [rbx+PD_R_RESERVED],0
  jne pcoReturn
  cmp qword ptr [rbx+PD_R_CONTEXT],0
  jne pcoReturn
  cmp qword ptr [rbx+PD_R_EPOCH],0
  je pcoReturn
  cmp qword ptr [rbx+PD_R_DEADLINE],0
  je pcoReturn
  cmp dword ptr [rbx+PD_R_LANE],1
  je pcoUnsupported
  cmp dword ptr [rbx+PD_R_LANE],2
  jne pcoReturn
  mov rdi,rsi
  xor eax,eax
  mov ecx,PD_SNAPSHOT_SIZE/8
  rep stosq
  xor edi,edi
  mov dword ptr [rsi+PD_S_SIZE],PD_SNAPSHOT_SIZE
  mov dword ptr [rsi+PD_S_VERSION],PD_VERSION
  mov dword ptr [rsi+PD_S_LANE],2
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
pcoFields:
  mov dword ptr [r10],PD_STATUS_NOT_REQUESTED
  add r10,16
  dec ecx
  jnz pcoFields
  mov dword ptr [rsi+PD_S_FIELDS+PD_F_IDENTITY*16],PD_STATUS_UNAVAILABLE
  mov dword ptr [rsi+PD_S_FIELDS+PD_F_LIVENESS*16],PD_STATUS_UNAVAILABLE
  mov dword ptr [rsi+PD_S_FIELDS+PD_F_ACCOUNT*16],PD_STATUS_UNAVAILABLE
  mov dword ptr [rsi+PD_S_FIELDS+PD_F_COMMAND*16],PD_STATUS_UNAVAILABLE
  mov dword ptr [rsi+PD_S_FIELDS+PD_F_ACCOUNT*16+4],21
  mov dword ptr [rsi+PD_S_FIELDS+PD_F_COMMAND*16+4],21
  mov dword ptr [rsi+PD_S_FIELDS+PD_F_IDENTITY*16+4],21
  mov dword ptr [rsi+PD_S_FIELDS+PD_F_LIVENESS*16+4],21
  call qword ptr pdOptionalClock
  mov [rsi+PD_S_TICK],rax
  mov rcx,rbx
  call PdOptionalCheck
  test eax,eax
  jnz pcoCheckError
  cmp qword ptr [rbx+PD_R_BIRTH],0
  jne pcoOpen
  mov dword ptr [rsi+PD_S_TARGET],PD_TARGET_MISMATCH
  mov eax,87
  jmp pcoTargetError
pcoOpen:
  mov ecx,101000h
  xor edx,edx
  mov r8d,[rbx+PD_R_PID]
  call qword ptr pdOptionalOpenProcess
  mov rdi,rax
  test rax,rax
  jz pcoLimited
  mov r12d,1
  jmp pcoVerify
pcoLimited:
  mov ecx,1000h
  xor edx,edx
  mov r8d,[rbx+PD_R_PID]
  call qword ptr pdOptionalOpenProcess
  mov rdi,rax
  test rax,rax
  jnz pcoVerify
  call GetLastError
  cmp eax,5
  jne pcoTargetError
  mov dword ptr [rsi+PD_S_TARGET],PD_TARGET_DENIED
  jmp pcoTargetError
pcoVerify:
  mov rcx,rbx
  mov rdx,rsi
  mov r8,rdi
  mov r9d,r12d
  call PdOptionalVerify
  cmp dword ptr [rsi+PD_S_FIELDS+PD_F_IDENTITY*16],PD_STATUS_VALID
  jne pcoFailedIdentity
  cmp dword ptr [rsi+PD_S_TARGET],PD_TARGET_EXITED
  je pcoExited
  mov rcx,rbx
  mov rdx,rsi
  mov r8,rdi
  call PdOptionalAccount
  mov rcx,rbx
  call PdOptionalCheck
  test eax,eax
  jnz pcoCheckError
  cmp dword ptr [rsi+PD_S_TARGET],PD_TARGET_VERIFIED
  jne pcoAmbiguous
  mov rcx,rbx
  mov rdx,rsi
  call PdOptionalWmi
  jmp pcoFinal
pcoAmbiguous:
  mov dword ptr [rsi+PD_S_FIELDS+PD_F_COMMAND*16+4],50
pcoFinal:
  mov rcx,rbx
  call PdOptionalCheck
  test eax,eax
  jnz pcoCheckError
  mov rcx,rbx
  mov rdx,rsi
  mov r8,rdi
  mov r9d,r12d
  call PdOptionalVerify
  cmp dword ptr [rsi+PD_S_FIELDS+PD_F_IDENTITY*16],PD_STATUS_VALID
  jne pcoFailedIdentity
  cmp dword ptr [rsi+PD_S_TARGET],PD_TARGET_EXITED
  je pcoExited
  mov rcx,rbx
  call PdOptionalCheck
  test eax,eax
  jnz pcoCheckError
  cmp dword ptr [rsi+PD_S_TARGET],PD_TARGET_VERIFIED
  je pcoDone
  ; An unchanged held identity permits account name, but cannot certify a
  ; command-line observation from an ambiguously live target.
  mov dword ptr [rsi+PD_S_FIELDS+PD_F_COMMAND*16],PD_STATUS_UNAVAILABLE
  mov dword ptr [rsi+PD_S_FIELDS+PD_F_COMMAND*16+4],50
  mov dword ptr [rsi+PD_S_FIELDS+PD_F_COMMAND*16+8],0
  mov dword ptr [rsi+PD_S_FIELDS+PD_F_COMMAND*16+12],0
  mov word ptr [rsi+PD_S_COMMAND],0
  jmp pcoDone
pcoExited:
  mov eax,1067
  jmp pcoInvalidate
pcoFailedIdentity:
  mov eax,[rsi+PD_S_ERROR]
  jmp pcoInvalidate
pcoTargetError:
  mov [rsi+PD_S_ERROR],eax
  mov [rsi+PD_S_FIELDS+PD_F_IDENTITY*16+4],eax
  mov [rsi+PD_S_FIELDS+PD_F_LIVENESS*16+4],eax
  jmp pcoInvalidate
pcoCheckError:
  mov [rsp+20h],eax
  test rdi,rdi
  jz pcoCheckedIdentity
  ; Even a late provider response must be followed by a fresh held-handle
  ; identity/liveness check before any optional value can be presented.
  mov rcx,rbx
  mov rdx,rsi
  mov r8,rdi
  mov r9d,r12d
  call PdOptionalVerify
  cmp dword ptr [rsi+PD_S_FIELDS+PD_F_IDENTITY*16],PD_STATUS_VALID
  jne pcoFailedIdentity
  cmp dword ptr [rsi+PD_S_TARGET],PD_TARGET_EXITED
  je pcoExited
pcoCheckedIdentity:
  mov eax,[rsp+20h]
  cmp eax,1223
  jne pcoTimeout
  mov dword ptr [rsi+PD_S_TARGET],PD_TARGET_CANCELLED
  mov [rsi+PD_S_ERROR],eax
  jmp pcoInvalidate
pcoTimeout:
  mov dword ptr [rsi+PD_S_FIELDS+PD_F_ACCOUNT*16],PD_STATUS_TIMEOUT
  mov dword ptr [rsi+PD_S_FIELDS+PD_F_COMMAND*16],PD_STATUS_TIMEOUT
  jmp pcoClear
pcoInvalidate:
  mov dword ptr [rsi+PD_S_FIELDS+PD_F_ACCOUNT*16],PD_STATUS_UNAVAILABLE
  mov dword ptr [rsi+PD_S_FIELDS+PD_F_COMMAND*16],PD_STATUS_UNAVAILABLE
pcoClear:
  mov [rsi+PD_S_FIELDS+PD_F_ACCOUNT*16+4],eax
  mov [rsi+PD_S_FIELDS+PD_F_COMMAND*16+4],eax
  mov qword ptr [rsi+PD_S_FIELDS+PD_F_ACCOUNT*16+8],0
  mov qword ptr [rsi+PD_S_FIELDS+PD_F_COMMAND*16+8],0
  mov word ptr [rsi+PD_S_ACCOUNT],0
  mov word ptr [rsi+PD_S_COMMAND],0
pcoDone:
  mov rcx,rdi
  test rcx,rcx
  jz pcoSuccess
  call CloseHandle
pcoSuccess:
  xor eax,eax
  jmp pcoReturn
pcoSmall:
  mov eax,122
  jmp pcoReturn
pcoUnsupported:
  mov eax,50
pcoReturn:
  add rsp,28h
  pop r12
  pop rdi
  pop rsi
  pop rbx
  ret
PdCollectOptional ENDP

; A bounded NUL scan, -1 means no terminator within the supplied capacity.
PdOptionalWideLength PROC
  xor eax,eax
powlNext:
  cmp eax,edx
  jae powlLong
  cmp word ptr [rcx+rax*2],0
  je powlReturn
  inc eax
  jmp powlNext
powlLong:
  mov eax,-1
powlReturn:
  ret
PdOptionalWideLength ENDP

PdOptionalAccount PROC
  push rbx
  push rsi
  push rdi
  push r12
  push r13
  push r14
  push r15
  sub rsp,90h
  mov rbx,rcx
  mov rsi,rdx
  mov rdi,r8
  xor r12d,r12d
  xor r13d,r13d
  mov qword ptr [rsp+60h],0
  mov rcx,rdi
  mov edx,8
  lea r8,[rsp+60h]
  call OpenProcessToken
  test eax,eax
  jz poaError
  mov r12,[rsp+60h]
  mov dword ptr [rsp+68h],0
  mov rcx,r12
  mov edx,1
  xor r8d,r8d
  xor r9d,r9d
  lea rax,[rsp+68h]
  mov [rsp+20h],rax
  call GetTokenInformation
  call GetLastError
  cmp eax,122
  jne poaStoreError
  mov eax,[rsp+68h]
  cmp eax,16
  jb poaInvalid
  cmp eax,65536
  ja poaLong
  call GetProcessHeap
  mov rcx,rax
  mov edx,8
  mov r8d,[rsp+68h]
  add r8d,PD_ACCOUNT_CAPACITY*4
  call HeapAlloc
  mov r13,rax
  test rax,rax
  jz poaMemory
  mov eax,[rsp+68h]
  lea r14,[r13+rax]
  lea r15,[r14+PD_ACCOUNT_CAPACITY*2]
  mov rcx,r12
  mov edx,1
  mov r8,r13
  mov r9d,[rsp+68h]
  lea rax,[rsp+68h]
  mov [rsp+20h],rax
  call GetTokenInformation
  test eax,eax
  jz poaError
  mov rcx,rbx
  call PdOptionalCheck
  test eax,eax
  jnz poaStoreError
  mov dword ptr [rsp+6Ch],PD_ACCOUNT_CAPACITY
  mov dword ptr [rsp+70h],PD_ACCOUNT_CAPACITY
  xor ecx,ecx
  mov rdx,[r13]
  mov r8,r14
  lea r9,[rsp+6Ch]
  mov [rsp+20h],r15
  lea rax,[rsp+70h]
  mov [rsp+28h],rax
  lea rax,[rsp+74h]
  mov [rsp+30h],rax
  call qword ptr pdOptionalLookupAccount
  test eax,eax
  jz poaError
  mov rcx,rbx
  call PdOptionalCheck
  test eax,eax
  jnz poaStoreError
  mov rcx,r14
  mov edx,PD_ACCOUNT_CAPACITY
  call PdOptionalWideLength
  cmp eax,-1
  je poaLong
  mov [rsp+78h],eax
  mov rcx,r15
  mov edx,PD_ACCOUNT_CAPACITY
  call PdOptionalWideLength
  cmp eax,-1
  je poaLong
  mov [rsp+7Ch],eax
  add eax,[rsp+78h]
  cmp dword ptr [rsp+7Ch],0
  je poaLength
  inc eax
poaLength:
  cmp eax,PD_ACCOUNT_CAPACITY
  jae poaLong
  mov [rsi+PD_S_FIELDS+PD_F_ACCOUNT*16+8],eax
  lea rdi,[rsi+PD_S_ACCOUNT]
  cmp dword ptr [rsp+7Ch],0
  je poaName
  mov rcx,rdi
  mov rdx,r15
  call lstrcpyW
  mov eax,[rsp+7Ch]
  lea rdi,[rdi+rax*2]
  mov word ptr [rdi],92
  add rdi,2
poaName:
  mov rcx,rdi
  mov rdx,r14
  call lstrcpyW
  mov dword ptr [rsi+PD_S_FIELDS+PD_F_ACCOUNT*16],PD_STATUS_VALID
  mov dword ptr [rsi+PD_S_FIELDS+PD_F_ACCOUNT*16+4],0
  jmp poaDone
poaInvalid:
  mov eax,13
  jmp poaStoreError
poaMemory:
  mov eax,8
  jmp poaStoreError
poaLong:
  mov eax,122
  mov dword ptr [rsi+PD_S_FIELDS+PD_F_ACCOUNT*16+12],2
  jmp poaStoreError
poaError:
  call GetLastError
  cmp eax,122
  jne poaStoreError
  mov dword ptr [rsi+PD_S_FIELDS+PD_F_ACCOUNT*16+12],2
poaStoreError:
  mov [rsi+PD_S_FIELDS+PD_F_ACCOUNT*16+4],eax
poaDone:
  test r13,r13
  jz poaToken
  call GetProcessHeap
  mov rcx,rax
  xor edx,edx
  mov r8,r13
  call HeapFree
poaToken:
  mov rcx,r12
  test rcx,rcx
  jz poaReturn
  call CloseHandle
poaReturn:
  add rsp,90h
  pop r15
  pop r14
  pop r13
  pop r12
  pop rdi
  pop rsi
  pop rbx
  ret
PdOptionalAccount ENDP

; Parse only concrete DMTF timestamps; no locale/time-zone state is consulted.
PdOptionalDigits PROC
  xor eax,eax
  xor r9d,r9d
podNext:
  movzx r10d,word ptr [rcx+r9*2]
  sub r10d,'0'
  cmp r10d,9
  ja podInvalid
  imul eax,eax,10
  add eax,r10d
  inc r9d
  cmp r9d,edx
  jb podNext
  ret
podInvalid:
  mov eax,-1
  ret
PdOptionalDigits ENDP
PO_DATE_PART MACRO start,count,dest
  lea rcx,[rbx+start*2]
  mov edx,count
  call PdOptionalDigits
  cmp eax,-1
  je popdInvalid
  mov word ptr [rsp+dest],ax
ENDM
; RCX string, EDX exact length, R8 output QWORD; no write on invalid source.
PdOptionalParseDmtf PROC
  push rbx
  push rsi
  push rdi
  push r12
  push r13
  sub rsp,40h
  mov rbx,rcx
  mov rsi,r8
  cmp edx,25
  jne popdInvalid
  cmp word ptr [rbx+28],'.'
  jne popdInvalid
  movzx r13d,word ptr [rbx+42]
  cmp r13d,'+'
  je popdParts
  cmp r13d,'-'
  jne popdInvalid
popdParts:
  mov qword ptr [rsp+20h],0
  mov qword ptr [rsp+28h],0
  PO_DATE_PART 0,4,20h
  PO_DATE_PART 4,2,22h
  PO_DATE_PART 6,2,26h
  PO_DATE_PART 8,2,28h
  PO_DATE_PART 10,2,2Ah
  PO_DATE_PART 12,2,2Ch
  lea rcx,[rbx+30]
  mov edx,6
  call PdOptionalDigits
  cmp eax,-1
  je popdInvalid
  mov edi,eax
  lea rcx,[rbx+44]
  mov edx,3
  call PdOptionalDigits
  cmp eax,-1
  je popdInvalid
  mov r12d,eax
  lea rcx,[rsp+20h]
  lea rdx,[rsp+30h]
  call SystemTimeToFileTime
  test eax,eax
  jz popdInvalid
  mov rax,[rsp+30h]
  imul rdi,rdi,10
  add rax,rdi
  jc popdInvalid
  imul r12,r12,600000000
  cmp r13d,'+'
  jne popdNegative
  sub rax,r12
  jc popdInvalid
  jmp popdStore
popdNegative:
  add rax,r12
  jc popdInvalid
popdStore:
  mov [rsi],rax
  xor eax,eax
  jmp popdReturn
popdInvalid:
  mov eax,13
popdReturn:
  add rsp,40h
  pop r13
  pop r12
  pop rdi
  pop rsi
  pop rbx
  ret
PdOptionalParseDmtf ENDP

PO_WMI_CHECK MACRO
  mov rcx,rbx
  call PdOptionalCheck
  test eax,eax
  jnz powmiWinError
ENDM
PO_WMI_GET MACRO property,variant
  mov rcx,r14
  lea rdx,property
  xor r8d,r8d
  lea r9,[rsp+variant]
  lea rax,[rsp+78h]
  mov [rsp+20h],rax
  mov qword ptr [rsp+28h],0
  mov rax,[rcx]
  call qword ptr [rax+20h]
  test eax,eax
  js powmiHresult
  PO_WMI_CHECK
ENDM
PdOptionalWmi PROC
  push rbx
  push rsi
  push rdi
  push r12
  push r13
  push r14
  push r15
  sub rsp,210h
  mov rbx,rcx
  mov rsi,rdx
  xor edi,edi
  xor r12d,r12d
  xor r13d,r13d
  xor r14d,r14d
  xor r15d,r15d
  lea r10,[rsp+60h]
  mov ecx,52
  xor eax,eax
powmiZero:
  mov [r10],rax
  add r10,8
  dec ecx
  jnz powmiZero
  PO_WMI_CHECK
  xor ecx,ecx
  xor edx,edx ; MTA on the dedicated worker
  call CoInitializeEx
  test eax,eax
  jns powmiComOwned
  cmp eax,80010106h
  jne powmiHresult
  jmp powmiStrings
powmiComOwned:
  mov r15d,1 ; S_OK and S_FALSE both add a reference
powmiStrings:
  lea rcx,pdOptionalNamespace
  call SysAllocString
  mov [rsp+60h],rax
  test rax,rax
  jz powmiNoMemory
  lea rcx,pdOptionalWql
  call SysAllocString
  mov [rsp+68h],rax
  test rax,rax
  jz powmiNoMemory
  lea rcx,[rsp+0E8h]
  lea rdx,pdOptionalQuery
  mov r8d,[rbx+PD_R_PID]
  call wsprintfW
  lea rcx,[rsp+0E8h]
  call SysAllocString
  mov [rsp+70h],rax
  test rax,rax
  jz powmiNoMemory
  lea rcx,pdOptionalClass
  xor edx,edx
  mov r8d,1
  lea r9,pdOptionalIID
  lea rax,[rsp+0D0h]
  mov [rsp+20h],rax
  call qword ptr pdOptionalCreateInstance
  mov rdi,[rsp+0D0h]
  test eax,eax
  js powmiHresult
  test rdi,rdi
  jz powmiInvalid
  PO_WMI_CHECK
  mov rcx,rdi
  mov rdx,[rsp+60h]
  xor r8d,r8d
  xor r9d,r9d
  mov qword ptr [rsp+20h],0
  mov qword ptr [rsp+28h],80h ; ConnectServer's documented maximum is 2 min
  mov qword ptr [rsp+30h],0
  mov qword ptr [rsp+38h],0
  lea rax,[rsp+0D0h]
  mov qword ptr [rax],0
  mov [rsp+40h],rax
  mov rax,[rcx]
  call qword ptr [rax+18h]
  mov r12,[rsp+0D0h]
  test eax,eax
  js powmiHresult
  test r12,r12
  jz powmiInvalid
  PO_WMI_CHECK
  mov rcx,r12
  call PdOptionalSetProxy
  test eax,eax
  js powmiHresult
  PO_WMI_CHECK
  mov rcx,r12
  mov rdx,[rsp+68h]
  mov r8,[rsp+70h]
  mov r9d,30h ; RETURN_IMMEDIATELY | FORWARD_ONLY
  mov qword ptr [rsp+20h],0
  lea rax,[rsp+0D0h]
  mov qword ptr [rax],0
  mov [rsp+28h],rax
  mov rax,[rcx]
  call qword ptr [rax+0A0h]
  mov r13,[rsp+0D0h]
  test eax,eax
  js powmiHresult
  test r13,r13
  jz powmiInvalid
  PO_WMI_CHECK
  mov rcx,r13
  call PdOptionalSetProxy
  test eax,eax
  js powmiHresult
  PO_WMI_CHECK ; EDX is the finite remaining deadline
  mov rcx,r13
  mov r8d,1
  lea r9,[rsp+0D0h]
  mov qword ptr [rsp+0D0h],0
  lea rax,[rsp+0C8h]
  mov [rsp+20h],rax
  mov rax,[rcx]
  call qword ptr [rax+20h]
  mov r14,[rsp+0D0h]
  test eax,eax
  js powmiHresult
  cmp dword ptr [rsp+0C8h],1
  je powmiObject
  cmp eax,40004h
  je powmiNextTimeout
  mov eax,80041002h ; WBEM_E_NOT_FOUND
  jmp powmiHresult
powmiObject:
  test r14,r14
  jz powmiInvalid
  PO_WMI_CHECK
  PO_WMI_GET pdOptionalPidName,80h
  cmp dword ptr [rsp+78h],19 ; CIM_UINT32 transported as VT_I4 normally
  jne powmiInvalid
  cmp word ptr [rsp+80h],3
  je powmiPid
  cmp word ptr [rsp+80h],19
  jne powmiInvalid
powmiPid:
  mov eax,[rsp+88h]
  cmp eax,[rbx+PD_R_PID]
  jne powmiInvalid
  PO_WMI_GET pdOptionalBirthName,98h
  cmp dword ptr [rsp+78h],101 ; CIM_DATETIME
  jne powmiInvalid
  cmp word ptr [rsp+98h],8
  jne powmiInvalid
  mov rcx,[rsp+0A0h]
  call SysStringLen
  mov edx,eax
  mov rcx,[rsp+0A0h]
  lea r8,[rsp+0D0h]
  call PdOptionalParseDmtf
  test eax,eax
  jnz powmiWinError
  mov rax,[rsp+0D0h]
  xor edx,edx
  mov ecx,10
  div rcx
  mov r8,rax
  mov rax,[rbx+PD_R_BIRTH]
  xor edx,edx
  div rcx
  cmp rax,r8
  jne powmiInvalid
  PO_WMI_GET pdOptionalCommandName,0B0h
  cmp word ptr [rsp+0B0h],1 ; VT_NULL: unavailable, distinct from empty BSTR
  je powmiNull
  cmp word ptr [rsp+0B0h],0
  je powmiNull
  cmp word ptr [rsp+0B0h],8
  jne powmiInvalid
  cmp dword ptr [rsp+78h],8 ; CIM_STRING
  jne powmiInvalid
  mov rcx,[rsp+0B8h]
  call SysStringLen
  cmp eax,PD_COMMAND_CAPACITY
  jae powmiTooLong
  mov r8d,eax
  mov r10,[rsp+0B8h]
  xor ecx,ecx
powmiCopy:
  cmp ecx,r8d
  jae powmiCopied
  movzx edx,word ptr [r10+rcx*2]
  test edx,edx
  jz powmiInvalid ; embedded NUL must not silently truncate a BSTR
  mov [rsi+PD_S_COMMAND+rcx*2],dx
  inc ecx
  jmp powmiCopy
powmiCopied:
  mov word ptr [rsi+PD_S_COMMAND+rcx*2],0
  mov [rsi+PD_S_FIELDS+PD_F_COMMAND*16+8],r8d
  mov dword ptr [rsi+PD_S_FIELDS+PD_F_COMMAND*16],PD_STATUS_VALID
  mov dword ptr [rsi+PD_S_FIELDS+PD_F_COMMAND*16+4],0
  jmp powmiDone
powmiNextTimeout:
  mov dword ptr [rsi+PD_S_FIELDS+PD_F_COMMAND*16],PD_STATUS_TIMEOUT
  jmp powmiHresult
powmiNoMemory:
  mov eax,8007000Eh
powmiHresult:
  mov dword ptr [rsi+PD_S_FIELDS+PD_F_COMMAND*16+12],1
  jmp powmiStoreError
powmiTooLong:
  mov dword ptr [rsi+PD_S_FIELDS+PD_F_COMMAND*16+12],2
  mov eax,122
  jmp powmiStoreError
powmiNull:
  mov eax,1168
  jmp powmiStoreError
powmiInvalid:
  mov eax,13
  jmp powmiStoreError
powmiWinError:
  cmp eax,1460
  jne powmiStoreError
  mov dword ptr [rsi+PD_S_FIELDS+PD_F_COMMAND*16],PD_STATUS_TIMEOUT
powmiStoreError:
  mov [rsi+PD_S_FIELDS+PD_F_COMMAND*16+4],eax
  mov word ptr [rsi+PD_S_COMMAND],0
  mov dword ptr [rsi+PD_S_FIELDS+PD_F_COMMAND*16+8],0
powmiDone:
  lea rcx,[rsp+80h]
  call VariantClear
  lea rcx,[rsp+98h]
  call VariantClear
  lea rcx,[rsp+0B0h]
  call VariantClear
  mov rcx,r14
  call PdOptionalRelease
  mov rcx,r13
  call PdOptionalRelease
  mov rcx,r12
  call PdOptionalRelease
  mov rcx,rdi
  call PdOptionalRelease
  mov rcx,[rsp+60h]
  call SysFreeString
  mov rcx,[rsp+68h]
  call SysFreeString
  mov rcx,[rsp+70h]
  call SysFreeString
  test r15d,r15d
  jz powmiReturn
  call CoUninitialize
powmiReturn:
  add rsp,210h
  pop r15
  pop r14
  pop r13
  pop r12
  pop rdi
  pop rsi
  pop rbx
  ret
PdOptionalWmi ENDP

PdOptionalSetProxy PROC
  sub rsp,48h
  mov edx,10 ; RPC_C_AUTHN_WINNT
  xor r8d,r8d ; RPC_C_AUTHZ_NONE
  xor r9d,r9d
  mov qword ptr [rsp+20h],3 ; RPC_C_AUTHN_LEVEL_CALL
  mov qword ptr [rsp+28h],3 ; RPC_C_IMP_LEVEL_IMPERSONATE
  mov qword ptr [rsp+30h],0
  mov qword ptr [rsp+38h],0 ; EOAC_NONE
  call qword ptr pdOptionalProxyBlanket
  add rsp,48h
  ret
PdOptionalSetProxy ENDP
PdOptionalRelease PROC
  test rcx,rcx
  jz porReturn
  sub rsp,28h
  mov rax,[rcx]
  call qword ptr [rax+10h]
  add rsp,28h
porReturn:
  ret
PdOptionalRelease ENDP
