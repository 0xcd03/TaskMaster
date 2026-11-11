; Owned-process count oracle; API failures/validzero are isolated boundaries.
.data
align 8
pmhtStage dd 8401
pmhtExpected dd 0
pmhtWritten dd 0
pmhtCalls dd 0
pmhtFormat db 'Metadata handles: exit=%u',13,10,0
.data?
align 8
pmhtRow db 4096 dup(?)
pmhtTimes dq 4 dup(?)
pmhtOutput db 80 dup(?)
.code
PmhtZero PROC
  inc pmhtCalls
  mov dword ptr [rdx],0
  mov eax,1
  ret
PmhtZero ENDP
PmhtFailure PROC
  sub rsp,28h
  inc pmhtCalls
  mov dword ptr [rdx],123456
  mov ecx,5
  call SetLastError
  xor eax,eax
  add rsp,28h
  ret
PmhtFailure ENDP
PmHandlesTest PROC
  sub rsp,58h
  call GetCurrentProcess
  mov [rsp+48h],rax
  mov rcx,rax
  lea rdx,pmhtTimes
  lea r8,pmhtTimes+8
  lea r9,pmhtTimes+16
  lea rax,pmhtTimes+24
  mov [rsp+20h],rax
  call GetProcessTimes
  test eax,eax
  jz pmhtDone
  mov rax,qword ptr pmhtTimes
  mov qword ptr [pmhtRow+3600],rax
  mov rcx,[rsp+48h]
  lea rdx,pmhtExpected
  call GetProcessHandleCount
  test eax,eax
  jz pmhtDone
  lea rcx,pmhtRow
  mov rdx,[rsp+48h]
  call PmRecordHandles
  test eax,eax
  jnz pmhtDone
  cmp dword ptr [pmhtRow+PM_ROW_HANDLES_VALID],1
  jne pmhtDone
  mov eax,pmhtExpected
  cmp dword ptr [pmhtRow+PM_ROW_HANDLES],eax
  jne pmhtDone
  mov pmhtStage,8402
  lea rax,PmhtZero
  mov pmHandleQuery,rax
  lea rcx,pmhtRow
  mov rdx,[rsp+48h]
  call PmRecordHandles
  test eax,eax
  jnz pmhtDone
  cmp dword ptr [pmhtRow+PM_ROW_HANDLES_VALID],1
  jne pmhtDone
  cmp dword ptr [pmhtRow+PM_ROW_HANDLES],0
  jne pmhtDone
  mov pmhtStage,8403
  lea rax,PmhtFailure
  mov pmHandleQuery,rax
  lea rcx,pmhtRow
  mov rdx,[rsp+48h]
  call PmRecordHandles
  cmp eax,5
  jne pmhtDone
  cmp dword ptr [pmhtRow+PM_ROW_HANDLES_VALID],0
  jne pmhtDone
  cmp dword ptr [pmhtRow+PM_ROW_HANDLES],0
  jne pmhtDone
  mov pmhtStage,8404
  mov qword ptr [pmhtRow+3600],0
  mov pmhtCalls,0
  lea rcx,pmhtRow
  mov rdx,[rsp+48h]
  call PmRecordHandles
  cmp eax,87
  jne pmhtDone
  cmp pmhtCalls,0
  jne pmhtDone
  cmp dword ptr [pmhtRow+PM_ROW_HANDLES_VALID],0
  jne pmhtDone
  mov pmhtStage,0
pmhtDone:
  lea rax,GetProcessHandleCount
  mov pmHandleQuery,rax
  lea rcx,pmhtOutput
  lea rdx,pmhtFormat
  mov r8d,pmhtStage
  call wsprintfA
  mov [rsp+40h],eax
  mov ecx,-11
  call GetStdHandle
  mov rcx,rax
  lea rdx,pmhtOutput
  mov r8d,[rsp+40h]
  lea r9,pmhtWritten
  mov qword ptr [rsp+20h],0
  call WriteFile
  mov ecx,pmhtStage
  call ExitProcess
PmHandlesTest ENDP
