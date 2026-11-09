extern GetThreadPriority:proc, GetThreadDescription:proc, LocalFree:proc
.data
align 8
desiredMode dd 0
frontMode dd 0
backMode dd 0
inspectedPid dd 0
inspectedBirth dq 0
threadName dq 0
.code
SetModeColumns PROC
  push rbx
  sub rsp,20h
  mov ebx,7
smcDelete:
  mov rcx,gList
  mov edx,101Ch
  xor r8d,r8d
  xor r9d,r9d
  call SendMessageW
  dec ebx
  jnz smcDelete
  cmp frontMode,2
  jae smcPanel
  cmp frontMode,0
  jne smcThread
  xor ecx,ecx
  lea rdx,szProcess
  mov r8d,340
  call GuiColumn
  mov ecx,1
  lea rdx,szPid
  mov r8d,90
  call GuiColumn
  mov ecx,2
  lea rdx,szParent
  mov r8d,90
  call GuiColumn
  mov ecx,3
  lea rdx,szThreads
  mov r8d,90
  call GuiColumn
  mov ecx,4
  lea rdx,privateCommitCol
  cmp treeEnabled,0
  je smcCommit
  lea rdx,treeCommitCaption
smcCommit:
  mov r8d,220
  call GuiColumn
  mov ecx,5
  lea rdx,wsCaption
  cmp treeEnabled,0
  je smcWorkingSet
  lea rdx,treeWsCaption
smcWorkingSet:
  mov r8d,150
  call GuiColumn
  jmp smcCPU
smcThread:
  xor ecx,ecx
  lea rdx,threadCaption
  mov r8d,340
  call GuiColumn
  mov ecx,1
  lea rdx,threadTid
  mov r8d,90
  call GuiColumn
  mov ecx,2
  lea rdx,threadOwner
  mov r8d,90
  call GuiColumn
  mov ecx,3
  lea rdx,threadPriority
  mov r8d,90
  call GuiColumn
  mov ecx,4
  lea rdx,threadKernel
  mov r8d,150
  call GuiColumn
  mov ecx,5
  lea rdx,threadUser
  mov r8d,150
  call GuiColumn
smcCPU:
  mov ecx,6
  lea rdx,cpuCaption
  mov r8d,90
  call GuiColumn
  jmp smcDone
smcPanel:
  call PanelColumns
smcDone:
  add rsp,20h
  pop rbx
  ret
SetModeColumns ENDP

CollectThreads PROC
  push rbx
  push rsi
  push rdi
  sub rsp,30h
  mov backCount,0
  mov ecx,1000h
  xor edx,edx
  mov r8d,inspectedPid
  call OpenProcess
  mov rsi,rax
  test rax,rax
  jz ctDone
  mov rcx,rax
  lea rdx,timeCreate
  lea r8,timeExit
  lea r9,timeKernel
  lea rax,timeUser
  mov [rsp+20h],rax
  call GetProcessTimes
  mov ebx,eax
  mov rcx,rsi
  call CloseHandle
  test ebx,ebx
  jz ctDone
  mov rax,timeCreate
  cmp rax,inspectedBirth
  jne ctDone
  mov ecx,4
  xor edx,edx
  call CreateToolhelp32Snapshot
  mov rbx,rax
  cmp rax,-1
  je ctDone
  mov dword ptr [threadEntry],28
  mov rcx,rbx
  lea rdx,threadEntry
  call Thread32First
ctNext:
  test eax,eax
  jz ctClose
  mov eax,dword ptr [threadEntry+12]
  cmp eax,inspectedPid
  jne ctAdvance
  cmp backCount,4096
  jae ctClose
  mov edi,backCount
  shl rdi,12
  add rdi,backBuffer
  mov rcx,rdi
  call ClearRow
  mov rcx,rdi
  lea rdx,threadCaption
  call lstrcpyW
  mov ecx,dword ptr [threadEntry+8]
  mov [rdi+3584],ecx
  lea rdx,[rdi+512]
  call StoreDecimal
  mov ecx,inspectedPid
  mov [rdi+3588],ecx
  lea rdx,[rdi+1024]
  call StoreDecimal
  mov ecx,dword ptr [threadEntry+16]
  lea rdx,[rdi+1536]
  call StoreDecimal
  mov qword ptr [rdi+3600],0
  mov rax,inspectedBirth
  mov [rdi+3616],rax
  lea rcx,[rdi+2048]
  lea rdx,unavailable
  call lstrcpyW
  lea rcx,[rdi+2560]
  lea rdx,unavailable
  call lstrcpyW
  lea rcx,[rdi+3072]
  lea rdx,unavailable
  call lstrcpyW
  mov ecx,40h
  xor edx,edx
  mov r8d,dword ptr [threadEntry+8]
  call OpenThread
  mov rsi,rax
  test rax,rax
  jz ctRow
  mov rcx,rax
  lea rdx,threadName
  call GetThreadDescription
  test eax,eax
  js ctTimes
  mov rax,threadName
  test rax,rax
  jz ctTimes
  cmp word ptr [rax],0
  je ctFreeName
  mov rcx,rdi
  mov rdx,rax
  mov r8d,256
  call lstrcpynW
ctFreeName:
  mov rcx,threadName
  call LocalFree
ctTimes:
  mov rcx,rsi
  lea rdx,timeCreate
  lea r8,timeExit
  lea r9,timeKernel
  lea rax,timeUser
  mov [rsp+20h],rax
  call GetThreadTimes
  test eax,eax
  jz ctHandleClose
  mov rax,timeCreate
  mov [rdi+3600],rax
  mov rax,timeKernel
  xor edx,edx
  mov ecx,10000
  div rcx
  mov rcx,rax
  lea rdx,[rdi+2048]
  call StoreDecimal
  mov rax,timeUser
  xor edx,edx
  mov ecx,10000
  div rcx
  mov rcx,rax
  lea rdx,[rdi+2560]
  call StoreDecimal
ctHandleClose:
  mov rcx,rsi
  call CloseHandle
ctRow:
  inc backCount
ctAdvance:
  mov rcx,rbx
  lea rdx,threadEntry
  call Thread32Next
  jmp ctNext
ctClose:
  mov rcx,rbx
  call CloseHandle
ctDone:
  add rsp,30h
  pop rdi
  pop rsi
  pop rbx
  ret
CollectThreads ENDP
extern lstrcpynW:proc
extern TerminateThread:proc
extern GetProcessIdOfThread:proc
.code
RunThreadAction PROC
  push rbx
  push rsi
  push rdi
  sub rsp,30h
  mov ecx,1000h
  xor edx,edx
  mov r8d,actionOwner
  call OpenProcess
  mov rbx,rax
  test rax,rax
  jz rtaError
  mov rcx,rax
  mov edx,actionOwner
  call GuardProcess
  mov esi,eax
  test eax,eax
  jz rtaParentClose
  mov rcx,rbx
  lea rdx,checkCreate
  lea r8,checkExit
  lea r9,checkKernel
  lea rax,checkUser
  mov [rsp+20h],rax
  call GetProcessTimes
  test eax,eax
  jz rtaParentMismatch
  mov rax,checkCreate
  cmp rax,actionOwnerBirth
  je rtaParentClose
rtaParentMismatch:
  xor esi,esi
rtaParentClose:
  mov rcx,rbx
  call CloseHandle
  test esi,esi
  jz rtaBlocked
  mov ecx,63h
  xor edx,edx
  mov r8d,actionPid
  call OpenThread
  mov rbx,rax
  test rax,rax
  jz rtaError
  mov rcx,rax
  call GetProcessIdOfThread
  cmp eax,actionOwner
  jne rtaCloseBlocked
  mov rcx,rbx
  lea rdx,checkCreate
  lea r8,checkExit
  lea r9,checkKernel
  lea rax,checkUser
  mov [rsp+20h],rax
  call GetThreadTimes
  test eax,eax
  jz rtaCloseError
  mov rax,checkCreate
  cmp rax,actionBirth
  jne rtaCloseBlocked
  cmp actionCode,2001
  je rtaKill
  cmp actionCode,2002
  je rtaPause
  cmp actionCode,2003
  je rtaResume
  cmp actionCode,2006
  jb rtaCloseBlocked
  cmp actionCode,2008
  ja rtaCloseBlocked
  xor edx,edx
  cmp actionCode,2006
  je rtaPriority
  mov edx,-1
  cmp actionCode,2007
  je rtaPriority
  mov edx,1
rtaPriority:
  mov rcx,rbx
  call SetThreadPriority
  jmp rtaResult
rtaKill:
  mov rcx,rbx
  mov edx,1
  call TerminateThread
  jmp rtaResult
rtaPause:
  xor esi,esi
  lea rdi,heldTids
rtaDuplicate:
  cmp esi,4096
  jae rtaNewSuspend
  mov eax,[rdi+rsi*4]
  cmp eax,actionPid
  je rtaCloseBlocked
  inc esi
  jmp rtaDuplicate
rtaNewSuspend:
  xor esi,esi
  lea rdi,heldHandles
rtaSlot:
  cmp esi,4096
  jae rtaCloseBlocked
  cmp qword ptr [rdi+rsi*8],0
  je rtaSuspend
  inc esi
  jmp rtaSlot
rtaSuspend:
  mov rcx,rbx
  call SuspendThread
  cmp eax,-1
  je rtaCloseError
  mov [rdi+rsi*8],rbx
  lea rdx,heldPids
  mov eax,actionPid
  mov [rdx+rsi*4],eax
  lea rdx,heldBirth
  mov rax,actionBirth
  mov [rdx+rsi*8],rax
  lea rdx,heldTids
  mov eax,actionPid
  mov [rdx+rsi*4],eax
  inc actionSucceeded
  jmp rtaDone
rtaResume:
  call ResumeProcessThreads
  jmp rtaClose
rtaResult:
  test eax,eax
  jz rtaCloseError
  inc actionSucceeded
  jmp rtaClose
rtaCloseBlocked:
  mov actionResult,5
  inc actionFailed
  jmp rtaClose
rtaCloseError:
  call GetLastError
  mov actionResult,eax
  inc actionFailed
rtaClose:
  mov rcx,rbx
  call CloseHandle
  jmp rtaDone
rtaBlocked:
  mov actionResult,5
  inc actionFailed
  jmp rtaDone
rtaError:
  call GetLastError
  mov actionResult,eax
  inc actionFailed
rtaDone:
  add rsp,30h
  pop rdi
  pop rsi
  pop rbx
  ret
RunThreadAction ENDP



