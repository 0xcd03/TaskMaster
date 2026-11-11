; Optional presentation is independent of native metrics and their baseline.
.data
align 8
pdUiOptionalSnapshot dq 0
pdUiOptionalPending dd 0
pdUiOptionalStatus dd PD_STATUS_NOT_REQUESTED
pdUiOptionalError dd 0
align 8
pdUiOptionalClock dq GetTickCount64
pdUiOptionalEpochCounter dq 0
.data?
align 8
pdUiOptionalRequest db PD_REQUEST_SIZE dup(?)
pdUiOptionalFailureRequest db PD_REQUEST_SIZE dup(?)
pdUiOptionalFallback db PD_FIELD_SIZE dup(?)
.code
; New epochs are explicit: timer/resize/native completions never call this.
PdUiRefreshOptional PROC
  push rbx
  push rsi
  push rdi
  sub rsp,20h
  call PdUiCancelOptional
  cmp pdWindow,0
  je pduroDone
  cmp pdUiStopped,0
  jne pduroRender
  mov rbx,pdUiOptionalEpochCounter
  inc rbx
  jz pduroOverflow
  mov pdUiOptionalEpochCounter,rbx
  lea rsi,pdUiRequest
  lea rdi,pdUiOptionalRequest
  mov ecx,PD_REQUEST_SIZE/8
  rep movsq
  mov dword ptr [pdUiOptionalRequest+PD_R_LANE],2
  mov qword ptr [pdUiOptionalRequest+PD_R_SEQUENCE],0
  mov qword ptr [pdUiOptionalRequest+PD_R_EPOCH],rbx
  call qword ptr pdUiOptionalClock
  add rax,3000
  jc pduroOverflow
  mov qword ptr [pdUiOptionalRequest+PD_R_DEADLINE],rax
  mov pdUiOptionalPending,1
  mov pdUiOptionalStatus,PD_STATUS_LOADING
  mov pdUiOptionalError,0
  lea rcx,pdUiOptionalRequest
  call PdOptionalWorkerSubmit
  test eax,eax
  jz pduroRender
  mov pdUiOptionalPending,0
  mov pdUiOptionalStatus,PD_STATUS_UNAVAILABLE
  mov pdUiOptionalError,eax
  jmp pduroRender
pduroOverflow:
  mov pdUiOptionalStatus,PD_STATUS_UNAVAILABLE
  mov pdUiOptionalError,534
pduroRender:
  call PdUiRenderOptional
pduroDone:
  add rsp,20h
  pop rdi
  pop rsi
  pop rbx
  ret
PdUiRefreshOptional ENDP

; Validate copied keys, independent of the native sample sequence.
; RCX shared identity header -> EAX boolean. Request/snapshot key offsets match.
PdUiOptionalKeys PROC
  xor eax,eax
  cmp pdWindow,0
  je pduokDone
  cmp pdUiOptionalPending,1
  jne pduokDone
  cmp pdUiStopped,0
  jne pduokDone
  mov rdx,[rcx+PD_R_COOKIE]
  cmp rdx,qword ptr [pdUiRequest+PD_R_COOKIE]
  jne pduokDone
  mov rdx,[rcx+PD_R_GENERATION]
  cmp rdx,qword ptr [pdUiRequest+PD_R_GENERATION]
  jne pduokDone
  mov edx,[rcx+PD_R_PID]
  cmp edx,dword ptr [pdUiRequest+PD_R_PID]
  jne pduokDone
  mov rdx,[rcx+PD_R_BIRTH]
  cmp rdx,qword ptr [pdUiRequest+PD_R_BIRTH]
  jne pduokDone
  mov rdx,[rcx+PD_R_EPOCH]
  cmp rdx,qword ptr [pdUiOptionalRequest+PD_R_EPOCH]
  jne pduokDone
  test rdx,rdx
  jz pduokDone
  mov eax,1
pduokDone:
  ret
PdUiOptionalKeys ENDP

; RCX metadata, RDX string, R8D capacity. Never render a truncated/malformed value.
PdUiOptionalStringValid PROC
  xor eax,eax
  cmp dword ptr [rcx],PD_STATUS_NOT_REQUESTED
  ja pduosvDone
  cmp dword ptr [rcx],PD_STATUS_VALID
  jne pduosvNonvalue
  cmp dword ptr [rcx+PD_FIELD_ERROR],0
  jne pduosvDone
  cmp dword ptr [rcx+PD_FIELD_FLAGS],0
  jne pduosvDone
  mov r9d,[rcx+PD_FIELD_LENGTH]
  cmp r9d,r8d
  jae pduosvDone
  cmp word ptr [rdx+r9*2],0
  jne pduosvDone
  xor r8d,r8d
pduosvScan:
  cmp r8d,r9d
  jae pduosvYes
  cmp word ptr [rdx+r8*2],0
  je pduosvDone
  inc r8d
  jmp pduosvScan
pduosvNonvalue:
  cmp dword ptr [rcx+PD_FIELD_LENGTH],0
  jne pduosvDone
  test dword ptr [rcx+PD_FIELD_FLAGS],0FFFFFFFCh
  jnz pduosvDone
pduosvYes:
  mov eax,1
pduosvDone:
  ret
PdUiOptionalStringValid ENDP

PdUiAcceptOptional PROC
  push rbx
  sub rsp,20h
  call PdUiCheckOptionalDeadline
  call PdOptionalWorkerTake
  mov rbx,rax
  test rax,rax
  jz pduaoFailure
  cmp dword ptr [rbx+PD_S_SIZE],PD_SNAPSHOT_SIZE
  jne pduaoFree
  cmp dword ptr [rbx+PD_S_VERSION],PD_VERSION
  jne pduaoFree
  cmp dword ptr [rbx+PD_S_LANE],2
  jne pduaoFree
  mov rcx,rbx ; request/snapshot key offsets are deliberately identical
  call PdUiOptionalKeys
  test eax,eax
  jz pduaoFree
  cmp dword ptr [rbx+PD_S_TARGET],PD_TARGET_EXITED
  je pduaoIdentityFailure
  cmp dword ptr [rbx+PD_S_TARGET],PD_TARGET_MISMATCH
  je pduaoIdentityFailure
  ; A failed identity may report errors, but cannot carry valid optional text.
  cmp dword ptr [rbx+PD_S_FIELDS+PD_F_ACCOUNT*16],PD_STATUS_VALID
  je pduaoIdentity
  cmp dword ptr [rbx+PD_S_FIELDS+PD_F_COMMAND*16],PD_STATUS_VALID
  jne pduaoStrings
pduaoIdentity:
  cmp dword ptr [rbx+PD_S_FIELDS+PD_F_IDENTITY*16],PD_STATUS_VALID
  jne pduaoFree
  mov rax,[rbx+PD_S_OBSERVED_BIRTH]
  test rax,rax
  jz pduaoFree
  cmp rax,qword ptr [pdUiRequest+PD_R_BIRTH]
  jne pduaoFree
pduaoStrings:
  lea rcx,[rbx+PD_S_FIELDS+PD_F_ACCOUNT*16]
  lea rdx,[rbx+PD_S_ACCOUNT]
  mov r8d,PD_ACCOUNT_CAPACITY
  call PdUiOptionalStringValid
  test eax,eax
  jz pduaoFree
  lea rcx,[rbx+PD_S_FIELDS+PD_F_COMMAND*16]
  lea rdx,[rbx+PD_S_COMMAND]
  mov r8d,PD_COMMAND_CAPACITY
  call PdUiOptionalStringValid
  test eax,eax
  jz pduaoFree
  ; Recheck after validation/copy work: an expired epoch can never become valid.
  call qword ptr pdUiOptionalClock
  cmp rax,qword ptr [pdUiOptionalRequest+PD_R_DEADLINE]
  jae pduaoExpired
  mov rcx,pdUiOptionalSnapshot
  call PdOptionalWorkerFree
  mov pdUiOptionalSnapshot,rbx
  mov pdUiOptionalPending,0
  mov pdUiOptionalStatus,PD_STATUS_VALID
  mov pdUiOptionalError,0
  call PdUiRenderOptional
  jmp pduaoDone
pduaoIdentityFailure:
  call qword ptr pdUiOptionalClock
  cmp rax,qword ptr [pdUiOptionalRequest+PD_R_DEADLINE]
  jae pduaoExpired
  mov pdUiOptionalPending,0
  mov pdUiOptionalStatus,PD_STATUS_UNAVAILABLE
  mov eax,[rbx+PD_S_ERROR]
  mov pdUiOptionalError,eax
  call PdUiRenderOptional
  jmp pduaoFree
pduaoExpired:
  call PdUiCheckOptionalDeadline
pduaoFree:
  mov rcx,rbx
  call PdOptionalWorkerFree
  jmp pduaoDone
pduaoFailure:
  lea rcx,pdUiOptionalFailureRequest
  call PdOptionalWorkerTakeFailure
  test eax,eax
  jz pduaoDone
  mov ebx,eax
  cmp dword ptr [pdUiOptionalFailureRequest+PD_R_SIZE],PD_REQUEST_SIZE
  jne pduaoDone
  cmp dword ptr [pdUiOptionalFailureRequest+PD_R_VERSION],PD_VERSION
  jne pduaoDone
  cmp dword ptr [pdUiOptionalFailureRequest+PD_R_LANE],2
  jne pduaoDone
  mov rax,qword ptr [pdUiOptionalFailureRequest+PD_R_NOTIFY]
  cmp rax,pdWindow
  jne pduaoDone
  lea rcx,pdUiOptionalFailureRequest
  call PdUiOptionalKeys
  test eax,eax
  jz pduaoDone
  call qword ptr pdUiOptionalClock
  cmp rax,qword ptr [pdUiOptionalRequest+PD_R_DEADLINE]
  jb pduaoFailureOnTime
  call PdUiCheckOptionalDeadline
  jmp pduaoDone
pduaoFailureOnTime:
  mov pdUiOptionalPending,0
  mov pdUiOptionalStatus,PD_STATUS_UNAVAILABLE
  cmp ebx,1460
  jne pduaoError
  mov pdUiOptionalStatus,PD_STATUS_TIMEOUT
pduaoError:
  mov pdUiOptionalError,ebx
  call PdUiRenderOptional
pduaoDone:
  add rsp,20h
  pop rbx
  ret
PdUiAcceptOptional ENDP

PdUiCheckOptionalDeadline PROC
  sub rsp,28h
  cmp pdUiOptionalPending,1
  jne pduocdDone
  call qword ptr pdUiOptionalClock
  cmp rax,qword ptr [pdUiOptionalRequest+PD_R_DEADLINE]
  jb pduocdDone
  mov pdUiOptionalPending,0
  mov pdUiOptionalStatus,PD_STATUS_TIMEOUT
  mov pdUiOptionalError,1460
  mov rcx,qword ptr [pdUiOptionalRequest+PD_R_COOKIE]
  call PdOptionalWorkerCancel
  call PdUiRenderOptional
pduocdDone:
  add rsp,28h
  ret
PdUiCheckOptionalDeadline ENDP

; Separate field formatter preserves HRESULT, loading, timeout and valid empty.
; RCX snapshot/null, EDX field, R8D control, R9D WCHAR offset.
PdUiOptionalField PROC
  push rbx
  push rsi
  push rdi
  sub rsp,20h
  mov ebx,r8d
  test rcx,rcx
  jz pduofFallback
  mov edi,r9d
  add rdi,rcx
  shl edx,4
  lea rsi,[rcx+rdx+PD_S_FIELDS]
  jmp pduofStatus
pduofFallback:
  lea rsi,pdUiOptionalFallback
  xor edi,edi
pduofStatus:
  mov eax,[rsi]
  cmp eax,PD_STATUS_VALID
  je pduofSet
  mov edx,52108
  cmp eax,PD_STATUS_LOADING
  je pduofText
  mov edx,52112
  cmp eax,PD_STATUS_TIMEOUT
  je pduofText
  mov edx,52114
  cmp eax,PD_STATUS_NOT_REQUESTED
  je pduofText
  mov edx,52040
  test dword ptr [rsi+PD_FIELD_FLAGS],1
  jz pduofErrorText
  mov edx,52110
pduofErrorText:
  call PdUiLoadText
  lea rcx,pdUiLine
  mov rdx,rax
  mov r8d,[rsi+PD_FIELD_ERROR]
  call wsprintfW
  lea rdi,pdUiLine
  jmp pduofSet
pduofText:
  call PdUiLoadText
  mov rdi,rax
pduofSet:
  mov rcx,pdWindow
  mov edx,ebx
  mov r8,rdi
  call SetDlgItemTextW
  add rsp,20h
  pop rdi
  pop rsi
  pop rbx
  ret
PdUiOptionalField ENDP

PdUiRenderOptional PROC
  sub rsp,28h
  cmp pdWindow,0
  je pduorDone
  mov eax,pdUiOptionalStatus
  mov dword ptr [pdUiOptionalFallback],eax
  mov eax,pdUiOptionalError
  mov dword ptr [pdUiOptionalFallback+PD_FIELD_ERROR],eax
  mov rcx,pdUiOptionalSnapshot
  mov edx,PD_F_ACCOUNT
  mov r8d,4106
  mov r9d,PD_S_ACCOUNT
  call PdUiOptionalField
  mov rcx,pdUiOptionalSnapshot
  mov edx,PD_F_COMMAND
  mov r8d,4107
  mov r9d,PD_S_COMMAND
  call PdUiOptionalField
  mov edx,52104
  cmp pdUiOptionalSnapshot,0
  jne pduorText
  mov edx,52100
  cmp pdUiOptionalStatus,PD_STATUS_LOADING
  je pduorText
  mov edx,52102
  cmp pdUiOptionalStatus,PD_STATUS_TIMEOUT
  je pduorText
  mov edx,52114
  cmp pdUiOptionalStatus,PD_STATUS_NOT_REQUESTED
  je pduorText
  mov edx,52106
  call PdUiLoadText
  lea rcx,pdUiLine
  mov rdx,rax
  mov r8d,pdUiOptionalError
  call wsprintfW
  lea r8,pdUiLine
  jmp pduorSet
pduorText:
  call PdUiLoadText
  mov r8,rax
pduorSet:
  mov rcx,pdWindow
  mov edx,4105
  call SetDlgItemTextW
pduorDone:
  add rsp,28h
  ret
PdUiRenderOptional ENDP

PdUiCancelOptional PROC
  sub rsp,28h
  mov pdUiOptionalPending,0
  mov pdUiOptionalStatus,PD_STATUS_UNAVAILABLE
  mov pdUiOptionalError,21
  mov rcx,qword ptr [pdUiOptionalRequest+PD_R_COOKIE]
  test rcx,rcx
  jz pducoFree
  call PdOptionalWorkerCancel
pducoFree:
  mov rcx,pdUiOptionalSnapshot
  mov pdUiOptionalSnapshot,0
  call PdOptionalWorkerFree
  call PdUiRenderOptional
  add rsp,28h
  ret
PdUiCancelOptional ENDP
