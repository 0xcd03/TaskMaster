.data
startupConfirmFmt dw '%','s',13,10,'%','s',13,10,13,10,'%','s',0
.data?
startupSourceHandle dq ?
startupDestHandle dq ?
.code
QueueStartupAction PROC
  push rbx
  push rsi
  sub rsp,28h
  mov ebx,ecx
  cmp ebx,2400
  jb qsaDone
  cmp ebx,2401
  ja qsaDone
  cmp actionBusy,0
  jne qsaDone
  cmp frontMode,5
  jne qsaDone
  mov rcx,gList
  mov edx,100Ch
  mov r8,-1
  mov r9d,2
  call SendMessageW
  cmp eax,visibleCount
  jae qsaDone
  lea rdx,rowMap
  mov rsi,[rdx+rax*8]
  cmp dword ptr [rsi+3624],0
  je qsaDone
  mov eax,[rsi+3620]
  mov startupActionKind,eax
  mov rax,[rsi+3600]
  mov startupActionRoot,rax
  mov eax,[rsi+3608]
  mov startupActionView,eax
  mov eax,[rsi+3612]
  mov startupActionOnce,eax
  mov eax,[rsi+3616]
  mov startupActionDisabled,eax
  mov rax,[rsi+3640]
  mov startupActionHash,rax
  lea rcx,startupActionName
  mov rdx,rsi
  call lstrcpyW
  lea rcx,startupActionPath
  lea rdx,[rsi+2560]
  call lstrcpyW
  ; Isolated UI tests capture the production target without changing Windows.
  cmp uiRegressionMode,0
  je qsaConfirm
  mov uiStartupCapturedCode,ebx
  inc uiStartupCapturedCount
  jmp qsaDone
qsaConfirm:
  lea rcx,actionMessage
  lea rdx,startupConfirmFmt
  mov r8,rsi
  lea r9,[rsi+1024]
  lea rax,startupWarning
  mov [rsp+20h],rax
  call wsprintfW
  mov rcx,gMain
  lea rdx,actionMessage
  lea r8,szTitle
  mov r9d,134h
  call MessageBoxW
  cmp eax,6
  jne qsaDone
  mov actionMode,5
  mov actionCode,ebx
  mov actionBusy,1
  mov actionPending,1
qsaDone:
  mov rcx,refreshEvent
  call SetEvent
  add rsp,28h
  pop rsi
  pop rbx
  ret
QueueStartupAction ENDP

RunStartupAction PROC
  sub rsp,28h
  cmp startupActionKind,1
  je rsaRegistry
  cmp startupActionKind,4
  je rsaTask
  call ChangeStartupFile
  jmp rsaResult
rsaRegistry:
  call ChangeStartupRegistry
  jmp rsaResult
rsaTask:
  call ChangeStartupTask
rsaResult:
  mov actionResult,eax
  test eax,eax
  jnz rsaFail
  inc actionSucceeded
  jmp rsaDone
rsaFail:
  inc actionFailed
rsaDone:
  add rsp,28h
  ret
RunStartupAction ENDP

ChangeStartupRegistry PROC
  push rbx
  push rsi
  push rdi
  sub rsp,60h
  mov startupSourceHandle,0
  mov startupDestHandle,0
  mov eax,startupActionView
  mov startupView,eax
  mov eax,startupActionOnce
  mov startupOnce,eax
  mov eax,startupActionDisabled
  mov startupDisabled,eax
  mov eax,actionCode
  sub eax,2400
  cmp eax,startupDisabled
  jne csrAlready ; disable=0, enabled source=0; enable=1, disabled source=1
  mov rax,startupActionRoot
  mov startupRoot,rax
  call MakeStartupKey
  mov rcx,startupRoot
  lea rdx,startupKeyPath
  xor r8d,r8d
  mov r9d,startupView
  or r9d,3
  lea rax,startupSourceHandle
  mov [rsp+20h],rax
  call RegOpenKeyExW
  test eax,eax
  jnz csrFinish
  mov startupDataSize,32768
  mov rcx,startupSourceHandle
  lea rdx,startupActionName
  xor r8d,r8d
  lea r9,startupType
  lea rax,startupData
  mov [rsp+20h],rax
  lea rax,startupDataSize
  mov [rsp+28h],rax
  call RegQueryValueExW
  test eax,eax
  jnz csrFinish
  mov ecx,startupDataSize
  lea rdx,startupData
  mov r8d,startupType
  call StartupHash
  cmp rax,startupActionHash
  jne csrChanged
  xor startupDisabled,1
  call MakeStartupKey
  mov rcx,startupRoot
  lea rdx,startupKeyPath
  xor r8d,r8d
  xor r9d,r9d
  mov qword ptr [rsp+20h],0
  mov eax,startupView
  or eax,3
  mov [rsp+28h],rax
  mov qword ptr [rsp+30h],0
  lea rax,startupDestHandle
  mov [rsp+38h],rax
  mov qword ptr [rsp+40h],0
  call RegCreateKeyExW
  test eax,eax
  jnz csrFinish
  mov rcx,startupDestHandle
  lea rdx,startupActionName
  xor r8d,r8d
  xor r9d,r9d
  mov qword ptr [rsp+20h],0
  mov qword ptr [rsp+28h],0
  call RegQueryValueExW
  test eax,eax
  jz csrCollision
  cmp eax,2
  jne csrFinish
  mov rcx,startupDestHandle
  lea rdx,startupActionName
  xor r8d,r8d
  mov r9d,startupType
  lea rax,startupData
  mov [rsp+20h],rax
  mov eax,startupDataSize
  mov [rsp+28h],rax
  call RegSetValueExW
  test eax,eax
  jnz csrFinish
  mov rcx,startupSourceHandle
  lea rdx,startupActionName
  call RegDeleteValueW
  test eax,eax
  jz csrFinish
  mov ebx,eax
  mov rcx,startupDestHandle
  lea rdx,startupActionName
  call RegDeleteValueW
  mov eax,ebx
  jmp csrFinish
csrChanged:
  mov eax,1237
  jmp csrFinish
csrCollision:
  mov eax,183
  jmp csrFinish
csrAlready:
  xor eax,eax
csrFinish:
  mov ebx,eax
  mov rcx,startupSourceHandle
  test rcx,rcx
  jz csrDestClose
  call RegCloseKey
csrDestClose:
  mov rcx,startupDestHandle
  test rcx,rcx
  jz csrDone
  call RegCloseKey
csrDone:
  mov eax,ebx
  add rsp,60h
  pop rdi
  pop rsi
  pop rbx
  ret
ChangeStartupRegistry ENDP

ChangeStartupFile PROC
  push rbx
  sub rsp,40h
  mov eax,actionCode
  sub eax,2400
  cmp eax,startupActionDisabled
  jne csfiAlready
  lea rcx,startupActionPath
  lea rdx,startupFindData
  call FindFirstFileW
  cmp rax,-1
  je csfiError
  mov rcx,rax
  call FindClose
  mov rax,qword ptr [startupFindData+4]
  cmp rax,startupActionHash
  jne csfiChanged
  cmp startupTestMode,0
  je csfiProductionFolder
  lea rcx,startupActionBase
  lea rdx,startupTestFolder
  call lstrcpyW
  jmp csfiHaveFolder
csfiProductionFolder:
  xor ecx,ecx
  mov edx,7
  cmp startupActionKind,2
  je csfiFolder
  mov edx,18h
csfiFolder:
  xor r8d,r8d
  xor r9d,r9d
  lea rax,startupActionBase
  mov [rsp+20h],rax
  call SHGetFolderPathW
  test eax,eax
  js csfiDone
csfiHaveFolder:
  ; Construct the expected source from the known folder, rather than trust a path.
  lea rcx,startupPath
  lea rdx,startupActionBase
  call lstrcpyW
  cmp startupActionDisabled,0
  je csfiExpectedName
  lea rcx,startupPath
  lea rdx,disabledSuffix
  call lstrcatW
csfiExpectedName:
  lea rcx,startupPath
  lea rdx,startupSlash
  call lstrcatW
  lea rcx,startupPath
  lea rdx,startupActionName
  call lstrcatW
  lea rcx,startupPath
  lea rdx,startupActionPath
  call lstrcmpiW
  test eax,eax
  jnz csfiChanged
  lea rcx,startupPath
  lea rdx,startupActionBase
  call lstrcpyW
  cmp actionCode,2401
  je csfiDestination
  lea rcx,startupPath
  lea rdx,disabledSuffix
  call lstrcatW
  lea rcx,startupPath
  xor edx,edx
  call CreateDirectoryW
  test eax,eax
  jnz csfiDestination
  call GetLastError
  cmp eax,183
  jne csfiDone
csfiDestination:
  lea rcx,startupPath
  lea rdx,startupSlash
  call lstrcatW
  lea rcx,startupPath
  lea rdx,startupActionName
  call lstrcatW
  lea rcx,startupActionPath
  lea rdx,startupPath
  mov r8d,8 ; write-through, never overwrite a destination
  call MoveFileExW
  test eax,eax
  jz csfiError
csfiAlready:
  xor eax,eax
  jmp csfiDone
csfiChanged:
  mov eax,1237
  jmp csfiDone
csfiError:
  call GetLastError
csfiDone:
  add rsp,40h
  pop rbx
  ret
ChangeStartupFile ENDP

ChangeStartupTask PROC
  push rbx
  sub rsp,40h
  cmp taskFolder,0
  je cstNoService
  lea rcx,startupActionPath
  call SysAllocString
  mov rbx,rax
  mov rcx,taskFolder
  mov rdx,rbx
  lea r8,[rsp+30h]
  mov rax,[rcx]
  call qword ptr [rax+104]
  mov [rsp+38h],eax
  mov rcx,rbx
  call SysFreeString
  mov eax,[rsp+38h]
  test eax,eax
  js cstDone
  mov rbx,[rsp+30h]
  mov rcx,rbx
  xor edx,edx
  cmp actionCode,2400
  je cstPut
  mov edx,-1
cstPut:
  mov rax,[rcx]
  call qword ptr [rax+88]
  mov [rsp+38h],eax
  mov rcx,rbx
  call ReleaseCom
  mov eax,[rsp+38h]
  jmp cstDone
cstNoService:
  mov eax,80004005h
cstDone:
  add rsp,40h
  pop rbx
  ret
ChangeStartupTask ENDP

; Virtual checkbox state belongs to the immutable snapshot, not native storage.
StartupCheckboxMouse PROC
  sub rsp,48h
  movsx eax,cx
  mov [rsp+20h],eax
  sar ecx,16
  mov [rsp+24h],ecx
  mov rcx,gList
  mov edx,1012h ; LVM_HITTEST
  xor r8d,r8d
  lea r9,[rsp+20h]
  call SendMessageW
  test dword ptr [rsp+28h],8 ; LVHT_ONITEMSTATEICON
  jz scmNotHandled
  mov ecx,eax
  call ToggleStartupCheckbox
  mov eax,1
  add rsp,48h
  ret
scmNotHandled:
  xor eax,eax
  add rsp,48h
  ret
StartupCheckboxMouse ENDP

ToggleStartupCheckbox PROC
  push rbx
  sub rsp,40h
  mov ebx,ecx
  cmp frontMode,5
  jne tscDone
  cmp actionBusy,0
  jne tscDone
  cmp ebx,visibleCount
  jae tscDone
  lea rax,rowMap
  mov rax,[rax+rbx*8]
  test rax,rax
  jz tscDone
  cmp dword ptr [rax+3624],0
  je tscDone
  mov ecx,2400 ; currently checked: disable
  cmp dword ptr [rax+3616],0
  je tscCommand
  mov ecx,2401 ; currently unchecked: enable
tscCommand:
  mov [rsp+38h],ecx
  mov dword ptr [rsp+20h],0
  mov dword ptr [rsp+24h],0
  mov dword ptr [rsp+28h],0
  mov dword ptr [rsp+2Ch],3 ; LVITEM.state
  mov dword ptr [rsp+30h],3 ; LVITEM.stateMask
  mov rcx,gList
  mov edx,102Bh ; LVM_SETITEMSTATE
  mov r8d,ebx
  lea r9,[rsp+20h]
  call SendMessageW
  mov ecx,[rsp+38h]
  call QueueStartupAction
  mov rcx,gList
  xor edx,edx
  xor r8d,r8d
  call InvalidateRect
tscDone:
  add rsp,40h
  pop rbx
  ret
ToggleStartupCheckbox ENDP
extern lstrcmpiW:proc
