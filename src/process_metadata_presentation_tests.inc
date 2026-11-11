; Presentation oracle uses immutable owned rows; no settings or foreign PID API.
.data
align 2
pmpExpectedCount dw '4','2',0
pmpAlpha dw 'a','l','p','h','a','1','0',0
pmpZeta dw 'z','e','t','a','9',0
pmpEmpty dw 0
.data?
align 8
pmpRows db 8192 dup(?)
pmpUnknown db 4096 dup(?)
pmpRequest db PD_REQUEST_SIZE dup(?)
pmpSnapshot db PD_SNAPSHOT_SIZE dup(?)
.code
; Synthetic immutable source envelopes exercise display keys without any PID API.
PmpSeed PROC
  push rbx
  push rsi
  push rdi
  push r12
  push r13
  sub rsp,30h
  mov rbx,rcx
  mov r12,r8
  call PmColumnLane
  mov r13d,eax
  mov ecx,[rbx+3584]
  mov rdx,[rbx+3600]
  mov r8d,r13d
  lea r9,pmpRequest
  call PmCachePrepare
  test eax,eax
  jnz pmpsDone
  lea rdi,pmpSnapshot
  xor eax,eax
  mov ecx,PD_SNAPSHOT_SIZE/8
  rep stosq
  lea rsi,pmpRequest
  lea rdi,pmpSnapshot
  mov ecx,8
  rep movsq
  mov dword ptr [pmpSnapshot+PD_S_SIZE],PD_SNAPSHOT_SIZE
  mov dword ptr [pmpSnapshot+PD_S_TARGET],PD_TARGET_VERIFIED
  mov rax,[rbx+3600]
  mov qword ptr [pmpSnapshot+PD_S_OBSERVED_BIRTH],rax
  mov ecx,PD_F_PATH
  lea rdi,pmpSnapshot+PD_S_PATH
  cmp r13d,PM_LANE_PATH
  je pmpsSource
  mov ecx,PD_F_ACCOUNT
  lea rdi,pmpSnapshot+PD_S_ACCOUNT
  cmp r13d,PM_LANE_ACCOUNT
  je pmpsSource
  mov ecx,PD_F_COMMAND
  lea rdi,pmpSnapshot+PD_S_COMMAND
pmpsSource:
  shl ecx,4
  lea rsi,pmpSnapshot+PD_S_FIELDS
  add rsi,rcx
  mov rcx,r12
  call lstrlenW
  mov [rsi+PD_FIELD_LENGTH],eax
  mov dword ptr [rsi+PD_FIELD_STATUS],PD_STATUS_VALID
  mov rcx,rdi
  mov rdx,r12
  call lstrcpyW
  lea rcx,pmpRequest
  lea rdx,pmpSnapshot
  xor r8d,r8d
  call PmCacheComplete
pmpsDone:
  add rsp,30h
  pop r13
  pop r12
  pop rdi
  pop rsi
  pop rbx
  ret
PmpSeed ENDP

ProcessMetadataPresentationTest PROC
  push rbx
  push rsi
  push rdi
  sub rsp,20h
  mov ebx,8501
  lea rcx,pmpRows
  mov dword ptr [rcx+3904],42
  mov dword ptr [rcx+3908],1
  mov edx,15
  call PcSortMetric
  cmp ecx,1
  jne pmpDone
  cmp rax,42
  jne pmpDone
  inc ebx
  lea rcx,pmpRows
  mov edx,15
  call PcDisplayExtra
  mov rcx,rax
  lea rdx,pmpExpectedCount
  call lstrcmpW
  test eax,eax
  jnz pmpDone
  inc ebx
  mov dword ptr [pmpRows+3908],0
  lea rcx,pmpRows
  mov edx,15
  call PcSortMetric
  test ecx,ecx
  jnz pmpDone
  inc ebx
  xor ecx,ecx
  call GetModuleHandleW
  mov gInst,rax
  mov appLanguage,0
  mov ecx,12
  call PcColumnCaption
  cmp word ptr [rax],'U'
  jne pmpDone
  mov ebx,8510
  call PmCacheReset
  mov ecx,100
  call PmCacheSetClock
  mov ecx,7
  mov edx,1
  call PmCacheSetDemand
  mov dword ptr [pmpRows+3584],101
  mov qword ptr [pmpRows+3600],1
  mov dword ptr [pmpRows+4096+3584],102
  mov qword ptr [pmpRows+4096+3600],2
  mov dword ptr [pmpUnknown+3584],1
  mov qword ptr [pmpUnknown+3600],9
  mov ecx,101
  mov edx,1
  call PmCacheEnsure
  mov ecx,102
  mov edx,2
  call PmCacheEnsure
  mov frontMode,0
  mov esi,PC_ACCOUNT
pmpTextLane:
  lea rcx,pmpRows
  mov edx,esi
  lea r8,pmpZeta
  call PmpSeed
  test eax,eax
  jnz pmpDone
  lea rcx,pmpRows+4096
  mov edx,esi
  lea r8,pmpAlpha
  cmp esi,PC_COMMAND
  jne pmpTextB
  lea r8,pmpEmpty
pmpTextB:
  call PmpSeed
  test eax,eax
  jnz pmpDone
  mov sortColumn,esi
  xor edi,edi
pmpDirections:
  mov sortDescending,edi
  lea rcx,pmpRows
  lea rdx,pmpRows+4096
  call CompareRows
  test edi,edi
  jnz pmpDescending
  test eax,eax
  jle pmpDone
  jmp pmpUnknownCheck
pmpDescending:
  test eax,eax
  jns pmpDone
pmpUnknownCheck:
  lea rcx,pmpRows
  lea rdx,pmpUnknown
  call CompareRows
  test eax,eax
  jns pmpDone
  inc edi
  cmp edi,2
  jb pmpDirections
  lea rcx,pmpRows
  mov edx,esi
  call PcDisplayExtra
  mov rcx,rax
  lea rdx,pmpZeta
  call lstrcmpW
  test eax,eax
  jnz pmpDone
  inc ebx
  inc esi
  cmp esi,PC_COMMAND
  jbe pmpTextLane
  mov ebx,8514
  lea rcx,pmpRows+4096
  mov edx,PC_COMMAND
  call PcDisplayExtra
  cmp word ptr [rax],0
  jne pmpDone
  mov ecx,60100
  call PmCacheSetClock
  lea rcx,pmpRows
  mov edx,PC_ACCOUNT
  call PmRowValue
  cmp edx,PM_STATUS_LOADING
  jne pmpDone
  test rax,rax
  jnz pmpDone
  call PmCacheReset
  mov sortDescending,0
  xor ebx,ebx
pmpDone:
  mov ecx,ebx
  call ExitProcess
ProcessMetadataPresentationTest ENDP
