; Called only with the collector's held handle after GetProcessTimes matched
; the row birth time. No additional process open or provider call is needed.
.data
align 8
pmHandleQuery dq GetProcessHandleCount
.code
PmRecordHandles PROC
  push rbx
  sub rsp,20h
  mov rbx,rcx
  mov dword ptr [rbx+PM_ROW_HANDLES],0
  mov dword ptr [rbx+PM_ROW_HANDLES_VALID],0
  cmp qword ptr [rbx+3600],0
  je pmrhInvalid
  mov rcx,rdx
  lea rdx,[rbx+PM_ROW_HANDLES]
  call qword ptr pmHandleQuery
  test eax,eax
  jz pmrhFailed
  mov dword ptr [rbx+PM_ROW_HANDLES_VALID],1
  xor eax,eax
  jmp pmrhDone
pmrhFailed:
  call GetLastError
  mov dword ptr [rbx+PM_ROW_HANDLES],0
  test eax,eax
  jnz pmrhDone
  mov eax,31
  jmp pmrhDone
pmrhInvalid:
  mov eax,87
pmrhDone:
  add rsp,20h
  pop rbx
  ret
PmRecordHandles ENDP
