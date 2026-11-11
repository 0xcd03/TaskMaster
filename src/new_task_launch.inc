; Launch ABI: executable*, literal arguments*, directory*, runAs DWORD, reserved.
; EAX is a Win32 error, zero on successful initiation. No settings are changed.
extern HeapFree:proc
NEW_TASK_ENGINE_READY equ 1
.data
newTaskEmpty dw 0
newTaskRunAs dw 'r','u','n','a','s',0
; Only the OS elevation boundary is replaceable by isolated native tests.
newTaskShellExecute dq ShellExecuteExW
.code
; RCX NUL-terminated UTF16. Return length, or -1 beyond32766 characters.
NewTaskStringLength PROC
  xor eax,eax
ntslNext:
  cmp word ptr [rcx+rax*2],0
  je ntslDone
  cmp eax,32766
  jae ntslTooLong
  inc eax
  jmp ntslNext
ntslTooLong:
  mov eax,-1
ntslDone:
  ret
NewTaskStringLength ENDP

; Strict fully-qualified drive/UNC/extended drive/extended UNC paths.
; Root-relative \file and drive-relative C:file are deliberately rejected.
NewTaskAbsolutePath PROC
  mov r10,rcx
  xor r8d,r8d
ntapQuotes:
  movzx eax,word ptr [r10+r8*2]
  test eax,eax
  jz ntapRoot
  cmp eax,34
  je ntapNo
  cmp eax,32
  jb ntapNo
  inc r8d
  jmp ntapQuotes
ntapRoot:
  cmp r8d,3
  jb ntapNo
  cmp word ptr [r10],92
  jne ntapDrive
  cmp word ptr [r10+2],92
  jne ntapNo
  cmp r8d,5
  jb ntapNo
  cmp word ptr [r10+4],63
  jne ntapUncStart
  cmp word ptr [r10+6],92
  jne ntapNo
  add r10,8
  cmp r8d,7
  jb ntapNo
  cmp word ptr [r10+2],58
  je ntapDrive
  cmp r8d,10
  jb ntapNo
  movzx eax,word ptr [r10]
  or eax,32
  cmp eax,'u'
  jne ntapNo
  movzx eax,word ptr [r10+2]
  or eax,32
  cmp eax,'n'
  jne ntapNo
  movzx eax,word ptr [r10+4]
  or eax,32
  cmp eax,'c'
  jne ntapNo
  cmp word ptr [r10+6],92
  jne ntapNo
  add r10,8
  jmp ntapUncServer
ntapUncStart:
  add r10,4
ntapUncServer:
  cmp word ptr [r10],0
  je ntapNo
  cmp word ptr [r10],92
  je ntapNo
  cmp word ptr [r10],'.'
  je ntapNo ; reject device namespace \\.\ and malformed dot servers
ntapServerNext:
  movzx eax,word ptr [r10]
  test eax,eax
  jz ntapNo
  add r10,2
  cmp eax,92
  jne ntapServerNext
  cmp word ptr [r10],0
  je ntapNo
  cmp word ptr [r10],92
  je ntapNo
  mov eax,1
  ret
ntapDrive:
  movzx eax,word ptr [r10]
  or eax,32
  cmp eax,'a'
  jb ntapNo
  cmp eax,'z'
  ja ntapNo
  cmp word ptr [r10+2],58
  jne ntapNo
  cmp word ptr [r10+4],92
  je ntapYes
  cmp word ptr [r10+4],47
  jne ntapNo
ntapYes:
  mov eax,1
  ret
ntapNo:
  xor eax,eax
  ret
NewTaskAbsolutePath ENDP

LaunchNewTask PROC
  push rbx
  push rsi
  push rdi
  push r12
  push r13
  push r14
  push r15
  sub rsp,1A0h
  mov rbx,rcx
  xor r13d,r13d
  xor r15d,r15d
  mov r14d,87
  test rbx,rbx
  jz lntDone
  cmp dword ptr [rbx+24],1
  ja lntDone
  cmp dword ptr [rbx+28],0
  jne lntDone
  mov rsi,[rbx]
  test rsi,rsi
  jz lntDone
  mov rdi,[rbx+8]
  test rdi,rdi
  jnz lntArguments
  lea rdi,newTaskEmpty
lntArguments:
  mov r12,[rbx+16]
  mov rcx,rsi
  call NewTaskStringLength
  cmp eax,-1
  je lntTooLong
  test eax,eax
  jz lntDone
  mov [rsp+180h],eax
  mov rcx,rdi
  call NewTaskStringLength
  cmp eax,-1
  je lntTooLong
  mov [rsp+184h],eax
  add eax,[rsp+180h]
  add eax,3 ; two executable quotes and terminating NUL
  cmp dword ptr [rsp+184h],0
  je lntCommandLength
  inc eax ; exactly one separator before literal arguments
lntCommandLength:
  cmp eax,32767
  ja lntTooLong
  mov [rsp+188h],eax
  mov rcx,rsi
  call NewTaskAbsolutePath
  test eax,eax
  jz lntDone
  test r12,r12
  jz lntCheckExecutable
  mov rcx,r12
  call NewTaskStringLength
  cmp eax,-1
  je lntTooLong
  test eax,eax
  jz lntDone
  mov rcx,r12
  call NewTaskAbsolutePath
  test eax,eax
  jz lntDone
lntCheckExecutable:
  mov rcx,rsi
  call GetFileAttributesW
  cmp eax,-1
  je lntLastError
  test eax,10h
  jz lntCheckDirectory
  mov r14d,193 ; ERROR_BAD_EXE_FORMAT: a directory is not an executable
  jmp lntDone
lntCheckDirectory:
  test r12,r12
  jz lntAllocate
  mov rcx,r12
  call GetFileAttributesW
  cmp eax,-1
  jne lntDirectoryAttributes
  call GetLastError
  mov r14d,eax
  cmp eax,2
  jne lntDone
  mov r14d,3 ; ERROR_PATH_NOT_FOUND for missing working directory
  jmp lntDone
lntDirectoryAttributes:
  test eax,10h
  jnz lntAllocate
  mov r14d,267 ; ERROR_DIRECTORY
  jmp lntDone
lntAllocate:
  call GetProcessHeap
  mov rcx,rax
  xor edx,edx
  mov r8d,[rsp+188h]
  shl r8d,1
  call HeapAlloc
  mov r13,rax
  test rax,rax
  jnz lntBuildCommand
  mov r14d,8
  jmp lntDone
lntBuildCommand:
  mov word ptr [r13],34
  xor ecx,ecx
  mov edx,[rsp+180h]
lntCopyExecutable:
  movzx eax,word ptr [rsi+rcx*2]
  mov [r13+rcx*2+2],ax
  inc ecx
  cmp ecx,edx
  jb lntCopyExecutable
  mov word ptr [r13+rcx*2+2],34
  lea r10,[r13+rcx*2+4]
  cmp dword ptr [rsp+184h],0
  je lntCommandTerminator
  mov word ptr [r10],32
  add r10,2
  xor ecx,ecx
lntCopyArguments:
  movzx eax,word ptr [rdi+rcx*2]
  mov [r10+rcx*2],ax
  inc ecx
  cmp ecx,[rsp+184h]
  jb lntCopyArguments
  lea r10,[r10+rcx*2]
lntCommandTerminator:
  mov word ptr [r10],0
  ; Zero only local STARTUPINFO/PROCESS_INFORMATION/SHELLEXECUTEINFO.
  lea r10,[rsp+60h]
  mov ecx,32
  xor eax,eax
lntZeroStructures:
  mov [r10],rax
  add r10,8
  dec ecx
  jnz lntZeroStructures
  cmp dword ptr [rbx+24],0
  jne lntElevated
  mov dword ptr [rsp+60h],104
  mov rcx,rsi ; explicit lpApplicationName, no executable search/ambiguity
  mov rdx,r13 ; private mutable command-line buffer
  xor r8d,r8d
  xor r9d,r9d
  mov qword ptr [rsp+20h],0 ; bInheritHandles=FALSE
  mov qword ptr [rsp+28h],0
  mov qword ptr [rsp+30h],0
  mov [rsp+38h],r12
  lea rax,[rsp+60h]
  mov [rsp+40h],rax
  lea rax,[rsp+0D0h]
  mov [rsp+48h],rax
  call CreateProcessW
  test eax,eax
  jz lntLastError
  xor r14d,r14d
  mov rcx,[rsp+0D0h]
  call CloseHandle
  mov rcx,[rsp+0D8h]
  call CloseHandle
  jmp lntDone
lntElevated:
  xor ecx,ecx
  mov edx,2 ; COINIT_APARTMENTTHREADED
  call CoInitializeEx
  test eax,eax
  jns lntComOwned
  cmp eax,80010106h ; RPC_E_CHANGED_MODE: existing COM belongs to caller
  je lntShell
  mov r14d,31
  jmp lntDone
lntComOwned:
  mov r15d,1 ; S_OK and S_FALSE both require CoUninitialize
lntShell:
  mov dword ptr [rsp+0F0h],112
  mov dword ptr [rsp+0F4h],540h ; NOCLOSEPROCESS | NOASYNC | FLAG_NO_UI
  lea rax,newTaskRunAs
  mov [rsp+100h],rax
  mov [rsp+108h],rsi
  mov [rsp+110h],rdi
  mov [rsp+118h],r12
  mov dword ptr [rsp+120h],1 ; SW_SHOWNORMAL
  lea rcx,[rsp+0F0h]
  call qword ptr newTaskShellExecute
  test eax,eax
  jz lntLastError
  xor r14d,r14d
  mov rcx,[rsp+158h]
  test rcx,rcx
  jz lntDone
  call CloseHandle
  jmp lntDone
lntTooLong:
  mov r14d,206
  jmp lntDone
lntLastError:
  call GetLastError
  mov r14d,eax
  test eax,eax
  jnz lntDone
  mov r14d,31 ; never turn an API FALSE with zero last-error into success
lntDone:
  test r15d,r15d
  jz lntFree
  call CoUninitialize
lntFree:
  test r13,r13
  jz lntReturn
  call GetProcessHeap
  mov rcx,rax
  xor edx,edx
  mov r8,r13
  call HeapFree
lntReturn:
  mov eax,r14d
  add rsp,1A0h
  pop r15
  pop r14
  pop r13
  pop r12
  pop rdi
  pop rsi
  pop rbx
  ret
LaunchNewTask ENDP
