; Native modal controls, closed from the initialized-dialog observer before
; display. Both OS launch and file picker boundaries are controlled fixtures.
extern IsDialogMessageW:proc, IsWindow:proc, GetSystemDirectoryW:proc
.data
newTaskDialogTestStage dd 0
newTaskDialogTestAssertions dd 0
newTaskDialogTestMode dd 0
newTaskDialogTestCalls dd 0
newTaskDialogTestOpens dd 0
newTaskDialogTestMockError dd 2
newTaskDialogTestPickResult dd 0
newTaskDialogTestWritten dd 0
newTaskDialogTestRequest dq 0
newTaskDialogTestExe dw 'C',':','\','O','w','n','e','d',' ','F','i','x','t','u','r','e','\','a','p','p','.','e','x','e',0
newTaskDialogTestArgs dw '"','t','w','o',' ','w','o','r','d','s','"',' ','&',' ','|',' ','%','T','E','M','P','%',0
newTaskDialogTestDir dw 'C',':','\','O','w','n','e','d',' ','F','i','x','t','u','r','e','\','w','o','r','k',0
newTaskDialogTestChosen dw 'C',':','\','O','w','n','e','d',' ','F','i','x','t','u','r','e','\',442h,435h,441h,442h,'.','e','x','e',0
newTaskDialogTestFormat db 'New task dialog: exit=%u assertions=%u opens=%u',13,10,0
.data?
newTaskDialogTestText dw 32769 dup(?)
newTaskDialogTestLong dw 32769 dup(?)
newTaskDialogTestCwd dw 32768 dup(?)
newTaskDialogTestMessage db 48 dup(?)
newTaskDialogTestOutput db 160 dup(?)
.code
NTDT_CLICK MACRO controlId
  mov rcx,rbx
  mov edx,controlId
  call GetDlgItem
  mov rcx,rax
  mov edx,0F5h
  xor r8d,r8d
  xor r9d,r9d
  call SendMessageW
ENDM
NTDT_SET MACRO controlId,labelName
  mov rcx,rbx
  mov edx,controlId
  lea r8,labelName
  call SetDlgItemTextW
ENDM
NTDT_CHECK_TEXT MACRO controlId,labelName
  mov rcx,rbx
  mov edx,controlId
  lea r8,newTaskDialogTestText
  mov r9d,32769
  call GetDlgItemTextW
  lea rcx,newTaskDialogTestText
  lea rdx,labelName
  call lstrcmpW
  test eax,eax
  jnz ntdthFail
  inc newTaskDialogTestAssertions
ENDM

NewTaskDialogTestLaunch PROC
  inc newTaskDialogTestCalls
  mov newTaskDialogTestRequest,rcx
  mov eax,newTaskDialogTestMockError
  ret
NewTaskDialogTestLaunch ENDP

NewTaskDialogTestPicker PROC
  push rbx
  sub rsp,20h
  mov rbx,rcx
  test dword ptr [rbx+96],2000000h ; do not add executable to recent documents
  jz ntdtpBad
  inc newTaskDialogTestAssertions
  ; Simulate a legacy picker changing only this isolated test process's cwd.
  lea rcx,newTaskDialogTestText
  mov edx,32769
  call GetSystemDirectoryW
  test eax,eax
  jz ntdtpBad
  cmp eax,32768
  jae ntdtpBad
  lea rcx,newTaskDialogTestText
  lea rdx,newTaskDialogTestCwd
  call lstrcmpW
  test eax,eax
  jz ntdtpBad
  lea rcx,newTaskDialogTestText
  call SetCurrentDirectoryW
  test eax,eax
  jz ntdtpBad
  inc newTaskDialogTestAssertions
  mov rcx,[rbx+48]
  lea rdx,newTaskDialogTestChosen
  call lstrcpyW
  mov eax,newTaskDialogTestPickResult
  jmp ntdtpDone
ntdtpBad:
  mov newTaskDialogTestStage,4040
  xor eax,eax
ntdtpDone:
  add rsp,20h
  pop rbx
  ret
NewTaskDialogTestPicker ENDP

; RCX actual initialized dialog, RDX its private production context.
NewTaskDialogTestInit PROC
  push rbx
  push rsi
  push rdi
  sub rsp,30h
  mov rbx,rcx
  mov rsi,rdx
  inc newTaskDialogTestOpens
  lea rax,NewTaskDialogTestLaunch
  mov [rsi+NTD_LAUNCH],rax
  lea rax,NewTaskDialogTestPicker
  mov [rsi+NTD_PICKER],rax
  cmp newTaskDialogTestMode,2
  je ntdthCancelButton
  cmp newTaskDialogTestMode,1
  je ntdthEscape
  mov newTaskDialogTestStage,4001
  mov edi,4001
ntdthControl:
  mov rcx,rbx
  mov edx,edi
  call GetDlgItem
  test rax,rax
  jz ntdthFail
  inc edi
  cmp edi,4008
  jb ntdthControl
  mov rcx,rbx
  lea rdx,newTaskDialogTestText
  mov r8d,32769
  call GetWindowTextW
  cmp appLanguage,0
  jne ntdthRuTitle
  cmp word ptr newTaskDialogTestText,'R'
  jne ntdthFail
  jmp ntdthTitle
ntdthRuTitle:
  cmp word ptr newTaskDialogTestText,417h ; З
  jne ntdthFail
ntdthTitle:
  inc newTaskDialogTestAssertions
  mov rcx,rbx
  mov edx,4005
  call IsDlgButtonChecked
  test eax,eax
  jnz ntdthFail
  inc newTaskDialogTestAssertions

  mov newTaskDialogTestStage,4002
  NTDT_CLICK 1
  cmp newTaskDialogTestCalls,0
  jne ntdthFail
  cmp dword ptr [rsi+NTD_LAST_ERROR],87
  jne ntdthFail
  inc newTaskDialogTestAssertions
  NTDT_SET 4001,newTaskDialogTestExe
  NTDT_SET 4002,newTaskDialogTestArgs
  mov newTaskDialogTestStage,4003
  mov newTaskDialogTestMockError,2
  NTDT_CLICK 1
  cmp newTaskDialogTestCalls,1
  jne ntdthFail
  cmp dword ptr [rsi+NTD_LAST_ERROR],2
  jne ntdthFail
  mov rdi,newTaskDialogTestRequest
  mov rcx,[rdi]
  lea rdx,newTaskDialogTestExe
  call lstrcmpW
  test eax,eax
  jnz ntdthFail
  mov rcx,[rdi+8]
  lea rdx,newTaskDialogTestArgs
  call lstrcmpW
  test eax,eax
  jnz ntdthFail
  mov rcx,[rdi+16]
  lea rdx,[rsi+NTD_CWD]
  call lstrcmpW
  test eax,eax
  jnz ntdthFail
  cmp qword ptr [rdi+24],0 ; normal rights and reserved DWORD zero
  jne ntdthFail
  inc newTaskDialogTestAssertions
  NTDT_CHECK_TEXT 4001,newTaskDialogTestExe
  NTDT_CHECK_TEXT 4002,newTaskDialogTestArgs

  mov newTaskDialogTestStage,4004
  mov rcx,rbx
  mov edx,4005
  call GetDlgItem
  mov rcx,rax
  mov edx,0F1h ; BM_SETCHECK
  mov r8d,1
  xor r9d,r9d
  call SendMessageW
  mov newTaskDialogTestMockError,1223
  NTDT_CLICK 1
  cmp newTaskDialogTestCalls,2
  jne ntdthFail
  cmp dword ptr [rsi+NTD_LAST_ERROR],1223
  jne ntdthFail
  cmp dword ptr [rdi+24],1
  jne ntdthFail
  cmp dword ptr [rdi+28],0
  jne ntdthFail
  inc newTaskDialogTestAssertions
  NTDT_CHECK_TEXT 4001,newTaskDialogTestExe
  NTDT_CHECK_TEXT 4002,newTaskDialogTestArgs
  mov rcx,gInst
  mov edx,50002
  add edx,appLanguage
  lea r8,newTaskDialogTestText
  mov r9d,32769
  call LoadStringW
  mov rcx,rbx
  mov edx,4006
  lea r8,[rsi+NTD_PICK]
  mov r9d,32768
  call GetDlgItemTextW
  lea rcx,[rsi+NTD_PICK]
  lea rdx,newTaskDialogTestText
  call lstrcmpW
  test eax,eax
  jnz ntdthFail
  inc newTaskDialogTestAssertions

  mov newTaskDialogTestStage,4005
  NTDT_SET 4003,newTaskDialogTestDir
  mov newTaskDialogTestMockError,5
  NTDT_CLICK 1
  cmp newTaskDialogTestCalls,3
  jne ntdthFail
  mov rcx,[rdi+16]
  lea rdx,newTaskDialogTestDir
  call lstrcmpW
  test eax,eax
  jnz ntdthFail
  inc newTaskDialogTestAssertions
  NTDT_CHECK_TEXT 4003,newTaskDialogTestDir

  ; The actual EDIT truncates EM_REPLACESEL and emits EN_MAXTEXT. Never let
  ; the resulting prefix be launched as if it were the user's complete input.
  mov newTaskDialogTestStage,4006
  mov edi,4001
ntdthOverflow:
  mov rcx,rbx
  mov edx,edi
  lea r8,newTaskDialogEmpty
  call SetDlgItemTextW
  mov rcx,rbx
  mov edx,edi
  call GetDlgItem
  mov rcx,rax
  mov edx,0C2h ; EM_REPLACESEL, real typing/paste limit behavior
  mov r8d,1
  lea r9,newTaskDialogTestLong
  call SendMessageW
  mov ecx,edi
  sub ecx,4001
  mov eax,1
  shl eax,cl
  test [rsi+NTD_OVERFLOW],eax
  jz ntdthFail
  inc newTaskDialogTestAssertions
  NTDT_CLICK 1
  cmp newTaskDialogTestCalls,3
  jne ntdthFail
  cmp dword ptr [rsi+NTD_LAST_ERROR],206
  jne ntdthFail
  inc newTaskDialogTestAssertions
  cmp edi,4001
  jne ntdthOverflowArgs
  NTDT_SET 4001,newTaskDialogTestExe
  jmp ntdthOverflowNext
ntdthOverflowArgs:
  cmp edi,4002
  jne ntdthOverflowDir
  NTDT_SET 4002,newTaskDialogTestArgs
  jmp ntdthOverflowNext
ntdthOverflowDir:
  NTDT_SET 4003,newTaskDialogTestDir
ntdthOverflowNext:
  inc edi
  cmp edi,4004
  jb ntdthOverflow
  cmp dword ptr [rsi+NTD_OVERFLOW],0
  jne ntdthFail

  ; Each field fits, but quotes, separator and NUL overflow CreateProcess's
  ; complete command line. Preserve the input and reject before the engine.
  mov newTaskDialogTestStage,4009
  mov rcx,rbx
  mov edx,4005
  call GetDlgItem
  mov rcx,rax
  mov edx,0F1h
  xor r8d,r8d
  xor r9d,r9d
  call SendMessageW
  mov word ptr [newTaskDialogTestLong+32750*2],0
  NTDT_SET 4002,newTaskDialogTestLong
  mov word ptr [newTaskDialogTestLong+32750*2],'A'
  cmp dword ptr [rsi+NTD_OVERFLOW],0
  jne ntdthFail
  NTDT_CLICK 1
  cmp newTaskDialogTestCalls,3
  jne ntdthFail
  cmp dword ptr [rsi+NTD_LAST_ERROR],206
  jne ntdthFail
  mov rcx,rbx
  mov edx,4002
  call GetDlgItem
  mov rcx,rax
  call GetWindowTextLengthW
  cmp eax,32750
  jne ntdthFail
  inc newTaskDialogTestAssertions
  NTDT_SET 4002,newTaskDialogTestArgs

  mov newTaskDialogTestStage,4007
  mov newTaskDialogTestPickResult,0
  NTDT_CLICK 4004
  cmp newTaskDialogTestStage,4007
  jne ntdthFail
  NTDT_CHECK_TEXT 4001,newTaskDialogTestExe
  call NewTaskDialogTestCheckCwd
  test eax,eax
  jnz ntdthFail
  inc newTaskDialogTestAssertions
  mov newTaskDialogTestPickResult,1
  NTDT_CLICK 4004
  cmp newTaskDialogTestStage,4007
  jne ntdthFail
  NTDT_CHECK_TEXT 4001,newTaskDialogTestChosen
  call NewTaskDialogTestCheckCwd
  test eax,eax
  jnz ntdthFail
  inc newTaskDialogTestAssertions

  mov newTaskDialogTestStage,4008
  NTDT_SET 4001,newTaskDialogTestExe
  NTDT_SET 4003,newTaskDialogEmpty
  mov rcx,rbx
  mov edx,4005
  call GetDlgItem
  mov rcx,rax
  mov edx,0F1h
  xor r8d,r8d
  xor r9d,r9d
  call SendMessageW
  mov newTaskDialogTestMockError,0
  mov edi,0Dh ; native dialog Enter -> default Run button
  call NewTaskDialogTestKey
  cmp newTaskDialogTestCalls,4
  jne ntdthFail
  inc newTaskDialogTestAssertions
  mov newTaskDialogTestStage,0
  jmp ntdthDone
ntdthEscape:
  mov edi,1Bh
  call NewTaskDialogTestKey
  jmp ntdthDone
ntdthCancelButton:
  NTDT_CLICK 2
  jmp ntdthDone
ntdthFail:
  mov rcx,rbx
  xor edx,edx
  call EndDialog
ntdthDone:
  add rsp,30h
  pop rdi
  pop rsi
  pop rbx
  ret
NewTaskDialogTestInit ENDP

; RBX dialog, EDI key. Feed the real dialog manager's keyboard route.
NewTaskDialogTestKey PROC
  sub rsp,28h
  mov rcx,rbx
  mov edx,4001
  call GetDlgItem
  mov qword ptr [newTaskDialogTestMessage],rax
  mov dword ptr [newTaskDialogTestMessage+8],100h
  mov qword ptr [newTaskDialogTestMessage+16],rdi
  mov qword ptr [newTaskDialogTestMessage+24],0
  mov rcx,rbx
  lea rdx,newTaskDialogTestMessage
  call IsDialogMessageW
  add rsp,28h
  ret
NewTaskDialogTestKey ENDP

NewTaskDialogTestCheckCwd PROC
  sub rsp,28h
  mov ecx,32768
  lea rdx,newTaskDialogTestText
  call GetCurrentDirectoryW
  test eax,eax
  jz ntdtccFailed
  cmp eax,32768
  jae ntdtccFailed
  lea rcx,newTaskDialogTestText
  lea rdx,newTaskDialogTestCwd
  call lstrcmpW
  jmp ntdtccDone
ntdtccFailed:
  mov eax,1
ntdtccDone:
  add rsp,28h
  ret
NewTaskDialogTestCheckCwd ENDP

NewTaskDialogTest PROC
  push rdi
  sub rsp,30h
  xor ecx,ecx
  call GetModuleHandleW
  mov gInst,rax
  mov ecx,32768
  lea rdx,newTaskDialogTestCwd
  call GetCurrentDirectoryW
  lea rdi,newTaskDialogTestLong
  mov ecx,32768
  mov ax,'A'
  rep stosw
  mov word ptr [rdi],0
  lea rax,NewTaskDialogTestInit
  mov newTaskDialogInitHook,rax
  mov appLanguage,0
ntdtLanguage:
  mov newTaskDialogTestMode,0
  mov newTaskDialogTestCalls,0
  mov newTaskDialogTestStage,4000
  call ShowNewTaskDialog
  cmp newTaskDialogTestStage,0
  jne ntdtReport
  cmp eax,1
  jne ntdtUnexpected
  inc newTaskDialogTestAssertions
  mov newTaskDialogTestMode,1
  mov newTaskDialogTestCalls,0
  call ShowNewTaskDialog
  test eax,eax
  jnz ntdtUnexpected
  cmp newTaskDialogTestCalls,0
  jne ntdtUnexpected
  inc newTaskDialogTestAssertions
  mov newTaskDialogTestMode,2
  call ShowNewTaskDialog
  test eax,eax
  jnz ntdtUnexpected
  cmp newTaskDialogTestCalls,0
  jne ntdtUnexpected
  inc newTaskDialogTestAssertions
  inc appLanguage
  cmp appLanguage,2
  jb ntdtLanguage
  mov newTaskDialogTestStage,0
  jmp ntdtReport
ntdtUnexpected:
  mov newTaskDialogTestStage,4050
ntdtReport:
  mov newTaskDialogInitHook,0
  lea rcx,newTaskDialogTestOutput
  lea rdx,newTaskDialogTestFormat
  mov r8d,newTaskDialogTestStage
  mov r9d,newTaskDialogTestAssertions
  mov eax,newTaskDialogTestOpens
  mov [rsp+20h],rax
  call wsprintfA
  mov [rsp+28h],eax
  mov ecx,-11
  call GetStdHandle
  mov rcx,rax
  lea rdx,newTaskDialogTestOutput
  mov r8d,[rsp+28h]
  lea r9,newTaskDialogTestWritten
  mov qword ptr [rsp+20h],0
  call WriteFile
  mov ecx,newTaskDialogTestStage
  call ExitProcess
NewTaskDialogTest ENDP

; Parent UI regression calls this only on its own hidden initialized main.
; It reaches ShowNewTaskDialog through the real menu route while idle/busy.
NewTaskDialogRoutingTests PROC
  push rbx
  push rsi
  sub rsp,38h
  mov rax,newTaskDialogInitHook
  mov [rsp+28h],rax
  mov eax,actionBusy
  mov [rsp+30h],eax
  lea rax,NewTaskDialogTestInit
  mov newTaskDialogInitHook,rax
  mov newTaskDialogTestMode,2
  mov newTaskDialogTestCalls,0
  xor esi,esi
ntdrtNext:
  mov ebx,newTaskDialogTestOpens
  inc ebx
  mov actionBusy,esi
  mov rcx,gMain
  mov edx,111h
  mov r8d,2500
  xor r9d,r9d
  call SendMessageW
  cmp newTaskDialogTestOpens,ebx
  jne ntdrtFail
  cmp newTaskDialogTestCalls,0
  jne ntdrtFail
  cmp actionBusy,esi
  jne ntdrtFail
  inc newTaskDialogTestAssertions
  inc esi
  cmp esi,2
  jb ntdrtNext
  xor ebx,ebx
  jmp ntdrtDone
ntdrtFail:
  mov ebx,4051
ntdrtDone:
  mov rax,[rsp+28h]
  mov newTaskDialogInitHook,rax
  mov eax,[rsp+30h]
  mov actionBusy,eax
  mov eax,ebx
  add rsp,38h
  pop rsi
  pop rbx
  ret
NewTaskDialogRoutingTests ENDP
