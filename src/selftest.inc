extern CreateProcessW:proc, VirtualAlloc:proc, GetExitCodeProcess:proc
.data
align 8
targetCommand dw 'o','u','t','p','u','t','s','\','t','e','s','t','-','t','a','r','g','e','t','.','e','x','e',0
.data?
align 8
startInfo db 104 dup(?)
.data
.data?
align 8
processInfo db 24 dup(?)
.data
testFailure dd 0
testMemory dq 0
.code
TargetThread PROC
  sub rsp,28h
ttLoop:
  mov ecx,100
  call Sleep
  jmp ttLoop
TargetThread ENDP

TargetEntry PROC
  sub rsp,38h
  xor ecx,ecx
  mov edx,67108864
  mov r8d,3000h
  mov r9d,4
  call VirtualAlloc
  mov testMemory,rax
  test rax,rax
  jz targetExit
  mov ecx,16384
touchMemory:
  mov byte ptr [rax],1
  add rax,4096
  dec ecx
  jnz touchMemory
  xor ecx,ecx
  xor edx,edx
  lea r8,TargetThread
  xor r9d,r9d
  mov qword ptr [rsp+20h],0
  mov qword ptr [rsp+28h],0
  call CreateThread
  mov rcx,rax
  call CloseHandle
  call TargetThread
targetExit:
  mov ecx,1
  call ExitProcess
TargetEntry ENDP

SelfTest PROC
  push rbx
  sub rsp,70h
  mov dword ptr [startInfo],104
  call GetCurrentProcessId
  mov actionPid,eax
  mov ecx,1000h
  xor edx,edx
  mov r8d,eax
  call OpenProcess
  mov [rsp+60h],rax
  mov rcx,rax
  mov edx,actionPid
  call GuardProcess
  mov [rsp+68h],eax
  mov rcx,[rsp+60h]
  call CloseHandle
  cmp dword ptr [rsp+68h],0
  jne stLaunchFail
  xor ecx,ecx
  lea rdx,targetCommand
  xor r8d,r8d
  xor r9d,r9d
  mov qword ptr [rsp+20h],0
  mov qword ptr [rsp+28h],8000000h
  mov qword ptr [rsp+30h],0
  mov qword ptr [rsp+38h],0
  lea rax,startInfo
  mov [rsp+40h],rax
  lea rax,processInfo
  mov [rsp+48h],rax
  call CreateProcessW
  test eax,eax
  jz stLaunchFail
  mov rbx,qword ptr [processInfo]
  mov eax,dword ptr [processInfo+16]
  mov actionPid,eax
  mov ecx,300
  call Sleep
  mov rcx,rbx
  lea rdx,checkCreate
  lea r8,checkExit
  lea r9,checkKernel
  lea rax,checkUser
  mov [rsp+20h],rax
  call GetProcessTimes
  mov rax,checkCreate
  mov actionBirth,rax
  ; stale identity must be rejected without touching target
  inc actionBirth
  mov actionCode,2001
  mov actionPending,1
  call RunQueuedAction
  cmp actionSucceeded,0
  jne stIdentityFail
  dec actionBirth
  ; pause => at least one owned thread handle
  mov actionCode,2002
  mov actionPending,1
  call RunQueuedAction
  cmp heldHandles,0
  je stPauseFail
  mov actionCode,2003
  mov actionPending,1
  call RunQueuedAction
  cmp actionSucceeded,0
  je stResumeFail
  cmp heldHandles,0
  jne stResumeFail
  ; inspect real TID from our process pause ledger, test isolated thread APIs
  mov actionCode,2002
  mov actionPending,1
  call RunQueuedAction
  mov ecx,dword ptr [heldTids]
  mov dword ptr [rsp+58h],ecx
  mov actionCode,2003
  mov actionPending,1
  call RunQueuedAction
  mov eax,actionPid
  mov actionOwner,eax
  mov rax,actionBirth
  mov actionOwnerBirth,rax
  mov ecx,40h
  xor edx,edx
  mov r8d,dword ptr [rsp+58h]
  call OpenThread
  mov qword ptr [rsp+60h],rax
  mov rcx,rax
  lea rdx,checkCreate
  lea r8,checkExit
  lea r9,checkKernel
  lea rax,checkUser
  mov [rsp+20h],rax
  call GetThreadTimes
  mov rax,checkCreate
  mov actionBirth,rax
  mov rcx,qword ptr [rsp+60h]
  call CloseHandle
  mov eax,dword ptr [rsp+58h]
  mov actionPid,eax
  mov actionMode,1
  mov actionCode,2002
  mov actionPending,1
  call RunQueuedAction
  cmp actionSucceeded,1
  jne stThreadPauseFail
  mov actionCode,2003
  mov actionPending,1
  call RunQueuedAction
  cmp actionSucceeded,0
  je stResumeFail
  ; An independent suspend must survive our pause/resume pair.
  mov ecx,2
  xor edx,edx
  mov r8d,actionPid
  call OpenThread
  mov [rsp+60h],rax
  test rax,rax
  jz stOwnedFail
  mov rcx,rax
  call SuspendThread
  cmp eax,-1
  je stOwnedFail
  mov [rsp+68h],eax
  mov actionCode,2002
  mov actionPending,1
  call RunQueuedAction
  mov actionCode,2003
  mov actionPending,1
  call RunQueuedAction
  mov rcx,[rsp+60h]
  call ResumeThread
  mov edx,[rsp+68h]
  inc edx
  cmp eax,edx
  jne stOwnedFail
  mov rcx,[rsp+60h]
  call CloseHandle
  mov eax,actionOwner
  mov actionPid,eax
  mov rax,actionOwnerBirth
  mov actionBirth,rax
  mov actionMode,0
  ; trim and priority target only
  mov actionCode,2004
  mov actionPending,1
  call RunQueuedAction
  cmp actionSucceeded,1
  jne stTrimFail
  mov actionCode,2007
  mov actionPending,1
  call RunQueuedAction
  cmp actionSucceeded,1
  jne stPriorityFail
  ; terminate target, wait for actual exit
  mov actionCode,2001
  mov actionPending,1
  call RunQueuedAction
  cmp actionSucceeded,1
  jne stKillFail
  mov rcx,rbx
  mov edx,2000
  call WaitForSingleObject
  test eax,eax
  jne stKillFail
  xor eax,eax
  jmp stFinish
stIdentityFail:
  mov eax,12
  jmp stFinish
stOwnedFail:
  mov eax,18
  jmp stFinish
stThreadPauseFail:
  mov eax,actionResult
  add eax,100
  jmp stFinish
stPauseFail:
  mov eax,13
  jmp stFinish
stResumeFail:
  mov eax,14
  jmp stFinish
stTrimFail:
  mov eax,15
  jmp stFinish
stPriorityFail:
  mov eax,16
  jmp stFinish
stKillFail:
  mov eax,17
stFinish:
  mov testFailure,eax
  call ReleaseSuspensions
  mov rcx,rbx
  mov edx,1
  call TerminateProcess
  mov rcx,rbx
  call CloseHandle
  mov rcx,qword ptr [processInfo+8]
  call CloseHandle
  mov ecx,testFailure
  call ExitProcess
stLaunchFail:
  mov ecx,11
  call ExitProcess
SelfTest ENDP


