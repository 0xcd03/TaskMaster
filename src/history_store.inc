; Worker-owned, versioned, bounded history. Atomic replacement, strict length checks.
; Samples only our observations. Never fabricates history from before first observation.
extern ReadFile:proc, WriteFile:proc, FlushFileBuffers:proc, GetFileSizeEx:proc
extern MoveFileExW:proc, DeleteFileW:proc, GetSystemTimeAsFileTime:proc
.data
historyHeader dd 32584D54h,2,4096,0 ; TMX2, schema, record size, count
historyChecksum dq 0
historyDirty dd 0
historyLoaded dd 0
historyLastSave dq 0
historyLastSample dq 0
historyNow dq 0
historyFileSize dq 0
historyTransferred dd 0
historyLoadError dd 0
historySaveError dd 0
historyReplace dd 0
historyLock dq 0
historyLockName dw '\','h','i','s','t','o','r','y','.','l','o','c','k',0
historyFileName dw '\','h','i','s','t','o','r','y','.','b','i','n',0
historyTempName dw '\','h','i','s','t','o','r','y','.','t','m','p',0
.data?
historyPath dw 1024 dup(?)
historyTemp dw 1024 dup(?)
historyReadHeader db 24 dup(?)
historyLockPath dw 1024 dup(?)
.code
InitHistoryStore PROC
  push rbx
  sub rsp,40h
  cmp configReady,0
  je ihDone
  mov historyLoadError,0
  cmp historyLock,0
  jne ihHaveLock
  lea rcx,historyLockPath
  lea rdx,storageDir
  call lstrcpyW
  lea rcx,historyLockPath
  lea rdx,historyLockName
  call lstrcatW
  lea rcx,historyLockPath
  mov edx,0C0000000h
  xor r8d,r8d
  xor r9d,r9d
  mov qword ptr [rsp+20h],4
  mov qword ptr [rsp+28h],80h
  mov qword ptr [rsp+30h],0
  call CreateFileW
  cmp rax,-1
  je ihLockError
  mov historyLock,rax
ihHaveLock:
  lea rcx,historyPath
  lea rdx,storageDir
  call lstrcpyW
  lea rcx,historyPath
  lea rdx,historyFileName
  call lstrcatW
  lea rcx,historyTemp
  lea rdx,storageDir
  call lstrcpyW
  lea rcx,historyTemp
  lea rdx,historyTempName
  call lstrcatW
  mov historyLoaded,1
  lea rcx,historyPath
  mov edx,80000000h
  mov r8d,1
  xor r9d,r9d
  mov qword ptr [rsp+20h],3
  mov qword ptr [rsp+28h],80h
  mov qword ptr [rsp+30h],0
  ; seventh argument requires 38h total: caller reserves it below.
  call CreateFileW
  mov rbx,rax
  cmp rax,-1
  jne ihRead
  call GetLastError
  cmp eax,2
  je ihDone
  mov historyLoadError,eax
  jmp ihDone
ihRead:
  mov rcx,rbx
  lea rdx,historyFileSize
  call GetFileSizeEx
  test eax,eax
  jz ihInvalid
  mov rcx,rbx
  lea rdx,historyReadHeader
  mov r8d,24
  lea r9,historyTransferred
  mov qword ptr [rsp+20h],0
  call ReadFile
  test eax,eax
  jz ihInvalid
  cmp historyTransferred,24
  jne ihInvalid
  cmp dword ptr [historyReadHeader],32584D54h
  jne ihInvalid
  cmp dword ptr [historyReadHeader+4],2
  jne ihInvalid
  cmp dword ptr [historyReadHeader+8],4096
  jne ihInvalid
  mov eax,dword ptr [historyReadHeader+12]
  cmp eax,8192
  ja ihInvalid
  shl rax,12
  lea rdx,[rax+24]
  cmp rdx,historyFileSize
  jne ihInvalid
  mov [rsp+28h],eax
  mov rcx,rbx
  lea rdx,historyRows
  mov r8d,eax
  lea r9,historyTransferred
  mov qword ptr [rsp+20h],0
  call ReadFile
  test eax,eax
  jz ihInvalid
  mov eax,[rsp+28h]
  cmp eax,historyTransferred
  jne ihInvalid
  mov ecx,eax
  lea rdx,historyRows
  xor r8d,r8d
  call StartupHash
  cmp rax,qword ptr [historyReadHeader+16]
  jne ihInvalid
  mov eax,dword ptr [historyReadHeader+12]
  mov historyCount,eax
  ; Enforce termination in every untrusted disk name; only fixed numeric fields used.
  lea rdx,historyRows
  mov ecx,eax
ihSanitize:
  test ecx,ecx
  jz ihClose
  mov word ptr [rdx+510],0
  mov qword ptr [rdx+3712],0
  mov qword ptr [rdx+3736],0 ; runtime CPU baseline, warm up after restart
  add rdx,4096
  dec ecx
  jmp ihSanitize
ihInvalid:
  mov historyCount,0
  mov historyLoadError,13
ihClose:
  mov rcx,rbx
  call CloseHandle
  jmp ihDone
ihLockError:
  call GetLastError
  mov historySaveError,eax
  mov historyLoaded,0
ihDone:
  add rsp,40h
  pop rbx
  ret
InitHistoryStore ENDP

SaveHistoryStore PROC
  push rbx
  sub rsp,40h
  cmp historyLoaded,0
  je shDone
  cmp historyDirty,0
  je shDone
  lea rcx,historyTemp
  mov edx,40000000h
  xor r8d,r8d
  xor r9d,r9d
  mov qword ptr [rsp+20h],2
  mov qword ptr [rsp+28h],80h
  mov qword ptr [rsp+30h],0
  call CreateFileW
  mov rbx,rax
  cmp rax,-1
  je shError
  mov eax,historyCount
  mov [historyHeader+12],eax
  shl eax,12
  mov ecx,eax
  lea rdx,historyRows
  xor r8d,r8d
  call StartupHash
  mov historyChecksum,rax
  mov rcx,rbx
  lea rdx,historyHeader
  mov r8d,24
  lea r9,historyTransferred
  mov qword ptr [rsp+20h],0
  call WriteFile
  test eax,eax
  jz shCloseError
  cmp historyTransferred,24
  jne shShort
  mov eax,historyCount
  shl eax,12
  mov [rsp+38h],eax
  mov r8d,eax
  mov rcx,rbx
  lea rdx,historyRows
  lea r9,historyTransferred
  mov qword ptr [rsp+20h],0
  call WriteFile
  test eax,eax
  jz shCloseError
  mov eax,[rsp+38h]
  cmp eax,historyTransferred
  jne shShort
  mov rcx,rbx
  call FlushFileBuffers
  test eax,eax
  jz shCloseError
  mov rcx,rbx
  call CloseHandle
  lea rcx,historyTemp
  lea rdx,historyPath
  mov r8d,9 ; REPLACE_EXISTING | WRITE_THROUGH
  call MoveFileExW
  test eax,eax
  jz shError
  mov historySaveError,0
  mov historyDirty,0
  call GetTickCount64
  mov historyLastSave,rax
  jmp shDone
shShort:
  mov historySaveError,29
  jmp shClose
shCloseError:
  call GetLastError
  mov historySaveError,eax
shClose:
  mov rcx,rbx
  call CloseHandle
  jmp shDone
shError:
  call GetLastError
  mov historySaveError,eax
shDone:
  add rsp,40h
  pop rbx
  ret
SaveHistoryStore ENDP

HistoryCheckpoint PROC
  sub rsp,28h
  call GetTickCount64
  sub rax,historyLastSave
  cmp rax,30000
  jb hcDone
  call SaveHistoryStore
hcDone:
  add rsp,28h
  ret
HistoryCheckpoint ENDP
