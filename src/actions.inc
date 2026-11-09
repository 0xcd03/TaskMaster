extern CreateMenu:proc, CreatePopupMenu:proc, AppendMenuW:proc, SetMenu:proc
extern GetCurrentProcessId:proc, IsProcessCritical:proc, ProcessIdToSessionId:proc
extern Thread32First:proc, Thread32Next:proc, OpenThread:proc
extern GetThreadTimes:proc, SetPriorityClass:proc, EmptyWorkingSet:proc
extern wsprintfW:proc, QueryFullProcessImageNameW:proc, ShellExecuteW:proc
.data
align 8
actionBusy dd 0
actionPending dd 0
actionCode dd 0
actionPid dd 0
actionBirth dq 0
actionOwner dd 0
actionOwnerBirth dq 0
actionMode dd 0
actionResult dd 0
actionSucceeded dd 0
actionFailed dd 0
.data?
align 8
actionMessage dw 2048 dup(?)
.data
mainMenu dq 0
processMenu dq 0
.data?
align 8
threadEntry db 28 dup(?)
.data
.data?
align 8
heldHandles dq 4096 dup(?)
.data
.data?
align 8
heldPids dd 4096 dup(?)
.data
.data?
align 8
heldBirth dq 4096 dup(?)
.data
.data?
align 8
heldTids dd 4096 dup(?)
.data
checkCritical dd 0
checkSession dd 0
checkCreate dq 0
checkExit dq 0
checkKernel dq 0
checkUser dq 0
menuProc dw 'P','r','o','c','e','s','s',0
menuKill dw 'E','n','d',' ','p','r','o','c','e','s','s',0
menuPause dw 'P','a','u','s','e',' ','p','r','o','c','e','s','s',0
menuResume dw 'R','e','s','u','m','e',' ','o','u','r',' ','p','a','u','s','e','s',0
menuTrim dw 'T','r','i','m',' ','s','e','l','e','c','t','e','d',' ','w','o','r','k','i','n','g',' ','s','e','t',0
menuAll dw 'T','r','i','m',' ','a','l','l',' ','e','l','i','g','i','b','l','e',' ','a','p','p','s',0
menuNormal dw 'N','o','r','m','a','l',' ','p','r','i','o','r','i','t','y',0
menuLow dw 'B','e','l','o','w',' ','n','o','r','m','a','l',' ','p','r','i','o','r','i','t','y',0
menuHigh dw 'A','b','o','v','e',' ','n','o','r','m','a','l',' ','p','r','i','o','r','i','t','y',0
menuThreads dw 'I','n','s','p','e','c','t',' ','t','h','r','e','a','d','s',0
menuBack dw 'B','a','c','k',' ','t','o',' ','p','r','o','c','e','s','s','e','s',0
.code
BuildMenus PROC
  sub rsp,28h
  mov rcx,gInst
  mov edx,101
  cmp appLanguage,0
  je bmLoad
  mov edx,102
bmLoad:
  call LoadMenuW
  mov mainMenu,rax
  mov rcx,gMain
  mov rdx,rax
  call SetMenu
  add rsp,28h
  ret
if 0
  call CreateMenu
  mov mainMenu,rax
  call CreatePopupMenu
  mov processMenu,rax
  mov rcx,mainMenu
  mov edx,10h
  mov r8,rax
  lea r9,menuProc
  call AppendMenuW
  mov rcx,processMenu
  xor edx,edx
  mov r8d,2001
  lea r9,menuKill
  call AppendMenuW
  mov rcx,processMenu
  xor edx,edx
  mov r8d,2002
  lea r9,menuPause
  call AppendMenuW
  mov rcx,processMenu
  xor edx,edx
  mov r8d,2003
  lea r9,menuResume
  call AppendMenuW
  mov rcx,processMenu
  xor edx,edx
  mov r8d,2004
  lea r9,menuTrim
  call AppendMenuW
  mov rcx,processMenu
  xor edx,edx
  mov r8d,2005
  lea r9,menuAll
  call AppendMenuW
  mov rcx,processMenu
  xor edx,edx
  mov r8d,2006
  lea r9,menuNormal
  call AppendMenuW
  mov rcx,processMenu
  xor edx,edx
  mov r8d,2007
  lea r9,menuLow
  call AppendMenuW
  mov rcx,processMenu
  xor edx,edx
  mov r8d,2008
  lea r9,menuHigh
  call AppendMenuW
  mov rcx,processMenu
  xor edx,edx
  mov r8d,2010
  lea r9,menuThreads
  call AppendMenuW
  mov rcx,processMenu
  xor edx,edx
  mov r8d,2011
  lea r9,menuBack
  call AppendMenuW
  mov rcx,gMain
  mov rdx,mainMenu
  call SetMenu
  call BuildPanelMenu
  add rsp,28h
  ret
endif
BuildMenus ENDP

QueueAction PROC
  push rbx
  sub rsp,30h
  mov ebx,ecx
  cmp actionBusy,0
  jne qaDone
  cmp ebx,2200
  jb qaNotService
  cmp ebx,2203
  ja qaDone
  cmp frontMode,2
  jne qaDone
  mov rcx,gList
  mov edx,100Ch
  mov r8,-1
  mov r9d,2
  call SendMessageW
  cmp eax,visibleCount
  jae qaDone
  lea rdx,rowMap
  mov rdx,[rdx+rax*8]
  lea rcx,actionService
  mov r8d,256
  call lstrcpynW
  jmp qaConfirm
qaNotService:
  cmp ebx,2100
  jb qaNotPanel
  cmp ebx,2106
  ja qaDone
  sub ebx,2100
  mov desiredMode,ebx
  jmp qaDone
qaNotPanel:
  cmp frontMode,2
  jb qaAllowedMode
  cmp ebx,2011
  jne qaDone
qaAllowedMode:
  cmp ebx,2011
  je qaProcesses
  mov eax,frontMode
  mov actionMode,eax
  cmp ebx,2005
  jne qaNotGlobal
  test eax,eax
  jnz qaDone
qaNotGlobal:
  cmp ebx,2001
  jb qaDone
  cmp ebx,2022
  ja qaDone
  cmp ebx,2005
  je qaConfirm
  mov rcx,gList
  mov edx,100Ch
  mov r8,-1
  mov r9d,2
  call SendMessageW
  cmp eax,visibleCount
  jae qaDone
  lea rdx,rowMap
  mov rax,[rdx+rax*8]
  mov [rsp+28h],rax
  lea rcx,actionTargetName
  mov rdx,rax
  mov r8d,256
  call lstrcpynW
  mov rax,[rsp+28h]
  mov ecx,[rax+3584]
  mov actionPid,ecx
  mov rcx,[rax+3600]
  mov actionBirth,rcx
  mov ecx,[rax+3588]
  mov actionOwner,ecx
  mov rcx,[rax+3616]
  mov actionOwnerBirth,rcx
  cmp ebx,2019
  jb qaSingleTarget
  cmp frontMode,0
  jne qaDone
  cmp treeEnabled,0
  je qaDone
  mov rcx,rax
  call CaptureTreeTargets
qaSingleTarget:
  cmp ebx,2010
  je qaThreads
qaConfirm:
  cmp ebx,2200
  jae qaServiceConfirm
  cmp ebx,2005
  je qaGenericConfirm
  lea rcx,actionMessage
  lea rdx,actionConfirmFmt
  lea r8,actionTargetName
  mov r9d,actionPid
  lea rax,actionWarning
  mov [rsp+20h],rax
  call wsprintfW
  lea rdx,actionMessage
  jmp qaShowConfirm
qaServiceConfirm:
  lea rcx,actionMessage
  lea rdx,startupConfirmFmt
  lea r8,actionService
  lea r9,serviceNameText
  lea rax,actionWarning
  mov [rsp+20h],rax
  call wsprintfW
  lea rdx,actionMessage
  jmp qaShowConfirm
qaGenericConfirm:
  lea rdx,actionWarning
qaShowConfirm:
  mov rcx,gMain
  lea r8,szTitle
  mov r9d,134h
  call MessageBoxW
  cmp eax,6
  jne qaDone
  cmp actionMode,1
  jne qaQueue
  cmp ebx,2001
  jne qaQueue
  mov rcx,gMain
  lea rdx,threadKillWarning
  lea r8,szTitle
  mov r9d,134h
  call MessageBoxW
  cmp eax,6
  jne qaDone
qaQueue:
  mov actionBusy,1
  mov actionCode,ebx
  mov actionPending,1
  jmp qaDone
qaThreads:
  cmp frontMode,0
  jne qaDone
  mov ecx,actionPid
  mov inspectedPid,ecx
  mov rcx,actionBirth
  mov inspectedBirth,rcx
  mov desiredMode,1
  jmp qaDone
qaProcesses:
  mov desiredMode,0
qaDone:
  mov rcx,refreshEvent
  call SetEvent
  add rsp,30h
  pop rbx
  ret
QueueAction ENDP

; RCX process handle, EDX PID. Fail closed on critical/session query failure.
GuardProcess PROC
  push rbx
  push rsi
  sub rsp,28h
  mov rbx,rcx
  mov esi,edx
  cmp esi,8
  jb gpDenied
  call GetCurrentProcessId
  cmp eax,esi
  je gpDenied
  mov rcx,rbx
  lea rdx,checkCritical
  call IsProcessCritical
  test eax,eax
  jz gpDenied
  cmp checkCritical,0
  jne gpDenied
  mov ecx,esi
  lea rdx,checkSession
  call ProcessIdToSessionId
  test eax,eax
  jz gpDenied
  cmp checkSession,0
  je gpDenied
  mov eax,1
  jmp gpEnd
gpDenied:
  xor eax,eax
gpEnd:
  add rsp,28h
  pop rsi
  pop rbx
  ret
GuardProcess ENDP

RunQueuedAction PROC
  push rbx
  push rsi
  push rdi
  sub rsp,30h
  xor eax,eax
  xchg eax,actionPending
  test eax,eax
  jz rqaDone
  mov actionSucceeded,0
  mov actionFailed,0
  mov actionResult,0
  cmp actionCode,2400
  jb rqaNotStartup
  call RunStartupAction
  jmp rqaReport
rqaNotStartup:
  cmp actionCode,2200
  jb rqaNotService
  call RunServiceAction
  jmp rqaReport
rqaNotService:
  cmp actionMode,1
  jne rqaProcessMode
  call RunThreadAction
  jmp rqaReport
rqaProcessMode:
  cmp actionCode,2019
  jb rqaSingle
  cmp actionCode,2022
  ja rqaSingle
  call RunTreeAction
  jmp rqaReport
rqaSingle:
  cmp actionCode,2005
  je rqaAll
  cmp actionCode,2003
  je rqaResume
  cmp actionCode,2012
  jb rqaRegular
  call RunExtendedAction
  jmp rqaReport
rqaRegular:
  mov ecx,1301h
  cmp actionCode,2004
  jbe rqaOpen
  mov ecx,1200h
rqaOpen:
  xor edx,edx
  mov r8d,actionPid
  call OpenProcess
  mov rbx,rax
  test rax,rax
  jz rqaError
  mov rcx,rax
  mov edx,actionPid
  call GuardProcess
  test eax,eax
  jz rqaBlocked
  mov rcx,rbx
  lea rdx,checkCreate
  lea r8,checkExit
  lea r9,checkKernel
  lea rax,checkUser
  mov [rsp+20h],rax
  call GetProcessTimes
  test eax,eax
  jz rqaCloseError
  mov rax,checkCreate
  cmp rax,actionBirth
  jne rqaBlocked
  cmp actionCode,2001
  je rqaKill
  cmp actionCode,2002
  je rqaPause
  cmp actionCode,2004
  je rqaTrim
  mov edx,20h
  cmp actionCode,2006
  je rqaPriority
  mov edx,4000h
  cmp actionCode,2007
  je rqaPriority
  mov edx,8000h
rqaPriority:
  mov rcx,rbx
  call SetPriorityClass
  jmp rqaResult
rqaKill:
  mov rcx,rbx
  mov edx,1
  call TerminateProcess
  jmp rqaResult
rqaTrim:
  mov rcx,rbx
  call EmptyWorkingSet
  jmp rqaResult
rqaPause:
  call PauseProcessThreads
  mov rcx,rbx
  call CloseHandle
  jmp rqaReport
rqaResult:
  test eax,eax
  jz rqaCloseError
  inc actionSucceeded
  mov rcx,rbx
  call CloseHandle
  jmp rqaReport
rqaBlocked:
  mov actionResult,5
  inc actionFailed
  mov rcx,rbx
  call CloseHandle
  jmp rqaReport
rqaCloseError:
  call GetLastError
  mov actionResult,eax
  inc actionFailed
  mov rcx,rbx
  call CloseHandle
  jmp rqaReport
rqaError:
  call GetLastError
  mov actionResult,eax
  inc actionFailed
  jmp rqaReport
rqaResume:
  call ResumeProcessThreads
  jmp rqaReport
rqaAll:
  mov rsi,frontBuffer
  mov edi,frontCount
rqaAllNext:
  test edi,edi
  jz rqaReport
  mov ecx,1100h
  xor edx,edx
  mov r8d,[rsi+3584]
  call OpenProcess
  mov rbx,rax
  test rax,rax
  jz rqaAllSkip
  mov rcx,rax
  mov edx,[rsi+3584]
  call GuardProcess
  test eax,eax
  jz rqaAllClose
  mov rcx,rbx
  call EmptyWorkingSet
  test eax,eax
  jz rqaAllFail
  inc actionSucceeded
  jmp rqaAllClose
rqaAllFail:
  inc actionFailed
rqaAllClose:
  mov rcx,rbx
  call CloseHandle
rqaAllSkip:
  add rsi,4096
  dec edi
  jmp rqaAllNext
rqaReport:
  cmp actionBatch,0
  jne rqaDone
  call BuildActionReport
  mov rcx,gMain
  mov edx,8002h
  xor r8d,r8d
  xor r9d,r9d
  call PostMessageW
rqaDone:
  add rsp,30h
  pop rdi
  pop rsi
  pop rbx
  ret
RunQueuedAction ENDP

PauseProcessThreads PROC
  push rbx
  push rsi
  push rdi
  sub rsp,20h
  mov ecx,4
  xor edx,edx
  call CreateToolhelp32Snapshot
  mov rbx,rax
  cmp rax,-1
  jne pptSnapshot
  call GetLastError
  mov actionResult,eax
  inc actionFailed
  jmp pptDone
pptSnapshot:
  mov dword ptr [threadEntry],28
  mov rcx,rbx
  lea rdx,threadEntry
  call Thread32First
pptNext:
  test eax,eax
  jz pptClose
  mov eax,dword ptr [threadEntry+12]
  cmp eax,actionPid
  jne pptAdvance
  xor esi,esi
  lea rdi,heldTids
pptAlready:
  cmp esi,4096
  jae pptNewSlot
  mov eax,dword ptr [threadEntry+8]
  cmp eax,[rdi+rsi*4]
  je pptAdvance
  inc esi
  jmp pptAlready
pptNewSlot:
  xor esi,esi
  lea rdi,heldHandles
pptSlot:
  cmp esi,4096
  jb pptFindSlot
  mov actionResult,8
  inc actionFailed
  jmp pptClose
pptFindSlot:
  cmp qword ptr [rdi+rsi*8],0
  je pptOpen
  inc esi
  jmp pptSlot
pptOpen:
  mov ecx,802h ; query limited + suspend; revalidate TID ownership after open
  xor edx,edx
  mov r8d,dword ptr [threadEntry+8]
  call OpenThread
  test rax,rax
  jnz pptHandle
  call GetLastError
  mov actionResult,eax
  inc actionFailed
  jmp pptAdvance
pptHandle:
  mov [rdi+rsi*8],rax
  mov rcx,rax
  call GetProcessIdOfThread
  cmp eax,actionPid
  jne pptWrongOwner
  mov rcx,[rdi+rsi*8]
  call SuspendThread
  cmp eax,-1
  je pptFailure
  inc actionSucceeded
  lea rdx,heldPids
  mov eax,actionPid
  mov [rdx+rsi*4],eax
  lea rdx,heldBirth
  mov rax,actionBirth
  mov [rdx+rsi*8],rax
  lea rdx,heldTids
  mov eax,dword ptr [threadEntry+8]
  mov [rdx+rsi*4],eax
  jmp pptAdvance
pptFailure:
  call GetLastError
  mov actionResult,eax
  jmp pptCloseFailed
pptWrongOwner:
  mov actionResult,1237
pptCloseFailed:
  mov rcx,[rdi+rsi*8]
  call CloseHandle
  mov qword ptr [rdi+rsi*8],0
  inc actionFailed
pptAdvance:
  mov rcx,rbx
  lea rdx,threadEntry
  call Thread32Next
  jmp pptNext
pptClose:
  mov rcx,rbx
  call CloseHandle
pptDone:
  add rsp,20h
  pop rdi
  pop rsi
  pop rbx
  ret
PauseProcessThreads ENDP

ResumeProcessThreads PROC
  push rbx
  push rsi
  sub rsp,28h
  xor esi,esi
  lea rbx,heldHandles
rptNext:
  cmp esi,4096
  jae rptDone
  cmp qword ptr [rbx+rsi*8],0
  je rptAdvance
  lea rdx,heldPids
  mov eax,[rdx+rsi*4]
  cmp eax,actionPid
  jne rptAdvance
  lea rdx,heldBirth
  mov rax,[rdx+rsi*8]
  cmp rax,actionBirth
  jne rptAdvance
  mov rcx,[rbx+rsi*8]
  call ResumeThread
  cmp eax,-1
  je rptFailed
  inc actionSucceeded
  jmp rptClose
rptFailed:
  call GetLastError
  mov actionResult,eax
  inc actionFailed
rptClose:
  mov rcx,[rbx+rsi*8]
  call CloseHandle
  mov qword ptr [rbx+rsi*8],0
  lea rdx,heldTids
  mov dword ptr [rdx+rsi*4],0
rptAdvance:
  inc esi
  jmp rptNext
rptDone:
  add rsp,28h
  pop rsi
  pop rbx
  ret
ResumeProcessThreads ENDP

ReleaseSuspensions PROC
  push rbx
  push rsi
  sub rsp,28h
  xor esi,esi
  lea rbx,heldHandles
rsNext:
  cmp esi,4096
  jae rsDone
  mov rcx,[rbx+rsi*8]
  test rcx,rcx
  jz rsAdvance
  call ResumeThread
  mov rcx,[rbx+rsi*8]
  call CloseHandle
  mov qword ptr [rbx+rsi*8],0
rsAdvance:
  inc esi
  jmp rsNext
rsDone:
  add rsp,28h
  pop rsi
  pop rbx
  ret
ReleaseSuspensions ENDP



.data?
actionTargetName dw 256 dup(?)
