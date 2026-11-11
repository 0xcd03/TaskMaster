; Native modal launch UI. Inputs are private to one dialog and never persisted.
includelib comdlg32.lib
extern GetDlgItem:proc, GetDlgItemTextW:proc, SetDlgItemTextW:proc
extern IsDlgButtonChecked:proc, GetCurrentDirectoryW:proc, SetCurrentDirectoryW:proc
extern GetOpenFileNameW:proc, CommDlgExtendedError:proc, HeapFree:proc
extern GetSysColorBrush:proc

NTD_LAUNCH equ 0
NTD_PICKER equ 8
NTD_REQUEST equ 16
NTD_OVERFLOW equ 48
NTD_INIT_ERROR equ 52
NTD_LAST_ERROR equ 56
NTD_ACTIVE equ 60
NTD_EXE equ 64
NTD_ARGS equ 65600
NTD_DIR equ 131136
NTD_CWD equ 196672
NTD_PICK equ 262208
NTD_REASON equ 327744
NTD_ERROR equ 328768
NTD_FORMAT equ 330816
NTD_SIZE equ 331840
.data
; Optional initialized-dialog observer for isolated native control regressions.
; Zero in the application. RCX HWND, RDX private state; never an execution hook.
newTaskDialogInitHook dq 0
newTaskDialogEmpty dw 0
newTaskExeFilter dw 'E','x','e','c','u','t','a','b','l','e','s',0
dw '*','.','e','x','e',';','*','.','c','o','m',0,0
.code
; RCX state, RDX launch boundary (0=real engine), R8 picker (0=native picker).
NewTaskDialogStateInit PROC
  push rbx
  push rsi
  push rdi
  sub rsp,20h
  mov rbx,rcx
  mov rsi,rdx
  mov rdi,rcx
  mov r9,r8
  mov ecx,NTD_SIZE/8
  xor eax,eax
  rep stosq
  test rsi,rsi
  jnz ntdsiLaunch
  lea rsi,LaunchNewTask
ntdsiLaunch:
  mov [rbx+NTD_LAUNCH],rsi
  test r9,r9
  jnz ntdsiPicker
  lea r9,GetOpenFileNameW
ntdsiPicker:
  mov [rbx+NTD_PICKER],r9
  mov ecx,32768
  lea rdx,[rbx+NTD_CWD]
  call GetCurrentDirectoryW
  test eax,eax
  jnz ntdsiLength
  call GetLastError
  jmp ntdsiError
ntdsiLength:
  cmp eax,32766
  jbe ntdsiDone
  mov eax,206
ntdsiError:
  mov [rbx+NTD_INIT_ERROR],eax
  mov word ptr [rbx+NTD_CWD],0
ntdsiDone:
  add rsp,20h
  pop rdi
  pop rsi
  pop rbx
  ret
NewTaskDialogStateInit ENDP

ShowNewTaskDialog PROC
  push rbx
  push rsi
  push rdi
  sub rsp,30h
  call GetProcessHeap
  mov rcx,rax
  mov edx,8
  mov r8d,NTD_SIZE
  call HeapAlloc
  mov rbx,rax
  test rax,rax
  jz sntdAllocError
  mov rcx,rax
  xor edx,edx
  xor r8d,r8d
  call NewTaskDialogStateInit
  mov rcx,gInst
  mov edx,303
  cmp appLanguage,0
  je sntdResource
  inc edx
sntdResource:
  mov r8,gMain
  lea r9,NewTaskDialogProc
  mov [rsp+20h],rbx
  call DialogBoxParamW
  mov rsi,rax
  mov rdi,rbx
  mov ecx,NTD_SIZE/8
  xor eax,eax
  rep stosq
  call GetProcessHeap
  mov rcx,rax
  xor edx,edx
  mov r8,rbx
  call HeapFree
  cmp rsi,-1
  jne sntdResult
  call NewTaskDialogOpenError
sntdResult:
  mov rax,rsi
  jmp sntdDone
sntdAllocError:
  call NewTaskDialogOpenError
  xor eax,eax
sntdDone:
  add rsp,30h
  pop rdi
  pop rsi
  pop rbx
  ret
ShowNewTaskDialog ENDP

; Allocation/resource failures happen before a usable dialog can show inline text.
NewTaskDialogOpenError PROC
  sub rsp,428h
  mov word ptr [rsp+20h],0
  mov rcx,gInst
  mov edx,50006
  cmp appLanguage,0
  je ntdoeText
  inc edx
ntdoeText:
  lea r8,[rsp+20h]
  mov r9d,512
  call LoadStringW
  mov rcx,gMain
  lea rdx,[rsp+20h]
  lea r8,szTitle
  mov r9d,10h ; MB_OK | MB_ICONERROR
  call MessageBoxW
  add rsp,428h
  ret
NewTaskDialogOpenError ENDP

; RCX dialog, EDX Win32 error. Numeric/localized status, inputs untouched.
NewTaskDialogError PROC
  push rbx
  push rsi
  push rdi
  sub rsp,40h
  mov rbx,rcx
  mov esi,edx
  mov edx,16 ; DWLP_USER on x64
  call GetWindowLongPtrW
  mov rdi,rax
  test rax,rax
  jz ntdeDone
  mov [rdi+NTD_LAST_ERROR],esi
  cmp esi,1223
  je ntdeCancelled
  mov word ptr [rdi+NTD_REASON],0
  mov ecx,1200h ; FROM_SYSTEM | IGNORE_INSERTS
  xor edx,edx
  mov r8d,esi
  mov r9d,409h
  cmp appLanguage,0
  je ntdeLocale
  mov r9d,419h
ntdeLocale:
  lea rax,[rdi+NTD_REASON]
  mov [rsp+20h],rax
  mov qword ptr [rsp+28h],512
  mov qword ptr [rsp+30h],0
  call FormatMessageW
  mov edx,50000
  cmp appLanguage,0
  je ntdeFormat
  inc edx
ntdeFormat:
  mov rcx,gInst
  lea r8,[rdi+NTD_FORMAT]
  mov r9d,512
  call LoadStringW
  lea rcx,[rdi+NTD_ERROR]
  lea rdx,[rdi+NTD_FORMAT]
  mov r8d,esi
  lea r9,[rdi+NTD_REASON]
  call wsprintfW
  jmp ntdeShow
ntdeCancelled:
  mov rcx,gInst
  mov edx,50002
  cmp appLanguage,0
  je ntdeCancelText
  inc edx
ntdeCancelText:
  lea r8,[rdi+NTD_ERROR]
  mov r9d,1024
  call LoadStringW
ntdeShow:
  mov rcx,rbx
  mov edx,4006
  lea r8,[rdi+NTD_ERROR]
  call SetDlgItemTextW
ntdeDone:
  add rsp,40h
  pop rdi
  pop rsi
  pop rbx
  ret
NewTaskDialogError ENDP

NewTaskDialogProc PROC
  push rbx
  push rsi
  sub rsp,38h
  mov rbx,rcx
  cmp edx,110h
  je ntdpInit
  cmp edx,111h
  je ntdpCommand
  cmp edx,10h
  je ntdpCancel
  cmp edx,133h
  je ntdpColor
  cmp edx,136h
  je ntdpColor
  cmp edx,138h
  je ntdpColor
  xor eax,eax
  jmp ntdpDone
ntdpInit:
  mov rsi,r9
  mov edx,16
  mov r8,rsi
  call SetWindowLongPtrW
  mov rcx,rbx
  mov edx,4001
  call GetDlgItem
  mov rcx,rax
  mov edx,0C5h ; EM_SETLIMITTEXT: maximum accepted text still fails on overflow
  mov r8d,32767
  xor r9d,r9d
  call SendMessageW
  mov rcx,rbx
  mov edx,4002
  call GetDlgItem
  mov rcx,rax
  mov edx,0C5h
  mov r8d,32767
  xor r9d,r9d
  call SendMessageW
  mov rcx,rbx
  mov edx,4003
  call GetDlgItem
  mov rcx,rax
  mov edx,0C5h
  mov r8d,32767
  xor r9d,r9d
  call SendMessageW
  mov edx,[rsi+NTD_INIT_ERROR]
  test edx,edx
  jz ntdpHook
  mov rcx,rbx
  call NewTaskDialogError
ntdpHook:
  mov rax,newTaskDialogInitHook
  test rax,rax
  jz ntdpHandled
  mov rcx,rbx
  mov rdx,rsi
  call rax
  jmp ntdpHandled
ntdpCommand:
  mov rsi,r8
  mov edx,16
  call GetWindowLongPtrW
  test rax,rax
  jz ntdpIgnored
  mov [rsp+28h],rax
  mov ecx,esi
  and ecx,0FFFFh
  cmp ecx,4001
  jb ntdpButton
  cmp ecx,4003
  ja ntdpButton
  mov edx,esi
  shr edx,16
  cmp edx,501h ; EN_MAXTEXT: the EDIT rejected part of the user's input
  je ntdpOverflow
  cmp edx,300h ; EN_CHANGE: only a shorter, corrected value clears that flag
  jne ntdpIgnored
  mov edx,ecx
  sub ecx,4001
  mov esi,1
  shl esi,cl
  mov rcx,rbx
  call GetDlgItem
  mov rcx,rax
  call GetWindowTextLengthW
  cmp eax,32767
  jae ntdpHandled
  mov rax,[rsp+28h]
  not esi
  and [rax+NTD_OVERFLOW],esi
  jmp ntdpHandled
ntdpOverflow:
  sub ecx,4001
  mov edx,1
  shl edx,cl
  or [rax+NTD_OVERFLOW],edx
  mov rcx,rbx
  mov edx,206
  call NewTaskDialogError
  jmp ntdpHandled
ntdpButton:
  shr esi,16
  test esi,esi ; only BN_CLICKED/default keyboard commands activate buttons
  jnz ntdpIgnored
  cmp ecx,2
  je ntdpCancel
  cmp ecx,4004
  je ntdpBrowse
  cmp ecx,1
  jne ntdpIgnored
  mov rcx,rbx
  mov rdx,[rsp+28h]
  call NewTaskDialogRun
  jmp ntdpHandled
ntdpBrowse:
  mov rcx,rbx
  mov rdx,[rsp+28h]
  call NewTaskDialogBrowse
  jmp ntdpHandled
ntdpColor:
  mov [rsp+28h],r8
  mov rcx,r8
  mov edx,shellTextColor
  call SetTextColor
  mov rcx,[rsp+28h]
  mov edx,0FFFFFFh
  cmp themeOption,0
  je ntdpBackground
  mov edx,2E2E2Eh
ntdpBackground:
  call SetBkColor
  mov rax,shellCardBrush
  test rax,rax
  jnz ntdpDone
  mov ecx,15
  call GetSysColorBrush
  jmp ntdpDone
ntdpCancel:
  mov rcx,rbx
  xor edx,edx
  call EndDialog
ntdpHandled:
  mov eax,1
  jmp ntdpDone
ntdpIgnored:
  xor eax,eax
ntdpDone:
  add rsp,38h
  pop rsi
  pop rbx
  ret
NewTaskDialogProc ENDP

; RCX HWND, RDX state. Validate lengths before copying, then delegate launch.
NewTaskDialogRun PROC
  push rbx
  push rsi
  push rdi
  push r12
  sub rsp,38h
  mov rbx,rcx
  mov rsi,rdx
  cmp dword ptr [rsi+NTD_ACTIVE],0
  jne ntdrDone
  cmp dword ptr [rsi+NTD_OVERFLOW],0
  jne ntdrTooLong
  mov edx,4001
  call GetDlgItem
  mov rcx,rax
  call GetWindowTextLengthW
  test eax,eax
  jz ntdrInvalid
  cmp eax,32766
  ja ntdrTooLong
  mov r12d,eax
  mov rcx,rbx
  mov edx,4002
  call GetDlgItem
  mov rcx,rax
  call GetWindowTextLengthW
  cmp eax,32766
  ja ntdrTooLong
  mov [rsp+28h],eax
  mov rcx,rbx
  mov edx,4003
  call GetDlgItem
  mov rcx,rax
  call GetWindowTextLengthW
  cmp eax,32766
  ja ntdrTooLong
  mov edi,eax
  mov rcx,rbx
  mov edx,4001
  lea r8,[rsi+NTD_EXE]
  mov r9d,32768
  call GetDlgItemTextW
  cmp eax,r12d
  jne ntdrInvalid
  mov rcx,rbx
  mov edx,4002
  lea r8,[rsi+NTD_ARGS]
  mov r9d,32768
  call GetDlgItemTextW
  cmp eax,[rsp+28h]
  jne ntdrInvalid
  mov rcx,rbx
  mov edx,4003
  lea r8,[rsi+NTD_DIR]
  mov r9d,32768
  call GetDlgItemTextW
  cmp eax,edi
  jne ntdrInvalid
  lea rax,[rsi+NTD_EXE]
  mov [rsi+NTD_REQUEST],rax
  lea rax,[rsi+NTD_ARGS]
  mov [rsi+NTD_REQUEST+8],rax
  lea rax,[rsi+NTD_DIR]
  test edi,edi
  jnz ntdrDirectory
  mov edx,[rsi+NTD_INIT_ERROR]
  test edx,edx
  jnz ntdrError
  lea rax,[rsi+NTD_CWD]
ntdrDirectory:
  mov [rsi+NTD_REQUEST+16],rax
  mov qword ptr [rsi+NTD_REQUEST+24],0
  mov rcx,rbx
  mov edx,4005
  call IsDlgButtonChecked
  cmp eax,1
  jne ntdrCommandLength
  mov dword ptr [rsi+NTD_REQUEST+24],1
  jmp ntdrLaunch
ntdrCommandLength:
  lea eax,[r12+3] ; executable quotes and terminating NUL
  mov edx,[rsp+28h]
  add eax,edx
  test edx,edx
  jz ntdrCommandBound
  inc eax ; separator before nonempty literal arguments
ntdrCommandBound:
  cmp eax,32767
  ja ntdrTooLong
ntdrLaunch:
  mov dword ptr [rsi+NTD_ACTIVE],1
  mov rcx,rbx
  mov edx,1
  call GetDlgItem
  mov rcx,rax
  xor edx,edx
  call EnableWindow
  lea rcx,[rsi+NTD_REQUEST]
  call qword ptr [rsi+NTD_LAUNCH]
  mov r12d,eax
  mov dword ptr [rsi+NTD_ACTIVE],0
  mov rcx,rbx
  mov edx,1
  call GetDlgItem
  mov rcx,rax
  mov edx,1
  call EnableWindow
  test r12d,r12d
  jnz ntdrLaunchError
  mov rcx,rbx
  mov edx,1
  call EndDialog
  jmp ntdrDone
ntdrLaunchError:
  mov edx,r12d
  jmp ntdrError
ntdrTooLong:
  mov edx,206
  jmp ntdrError
ntdrInvalid:
  mov edx,87
ntdrError:
  mov rcx,rbx
  call NewTaskDialogError
ntdrDone:
  add rsp,38h
  pop r12
  pop rdi
  pop rsi
  pop rbx
  ret
NewTaskDialogRun ENDP

; RCX dialog, RDX state. Explorer picker uses its own output buffer so Cancel
; cannot replace typed input. Explicit cwd restoration covers both outcomes.
NewTaskDialogBrowse PROC
  push rbx
  push rsi
  push rdi
  sub rsp,0E0h
  mov rbx,rcx
  mov rsi,rdx
  cmp dword ptr [rsi+NTD_ACTIVE],0
  jne ntdbDone
  mov ecx,32768
  lea rdx,[rsi+NTD_DIR]
  call GetCurrentDirectoryW
  test eax,eax
  jz ntdbLastError
  cmp eax,32767
  jae ntdbTooLong
  mov rcx,rbx
  mov edx,4001
  call GetDlgItem
  mov rcx,rax
  call GetWindowTextLengthW
  cmp eax,32766
  ja ntdbTooLong
  mov rcx,rbx
  mov edx,4001
  lea r8,[rsi+NTD_PICK]
  mov r9d,32768
  call GetDlgItemTextW
  xor ecx,ecx
  mov edx,6 ; STA | DISABLE_OLE1DDE, balance S_OK/S_FALSE only
  call CoInitializeEx
  test eax,eax
  js ntdbComError
  lea rdi,[rsp+30h]
  mov ecx,19 ; OPENFILENAMEW x64 is 152 bytes
  xor eax,eax
  rep stosq
  mov dword ptr [rsp+30h],152
  mov [rsp+38h],rbx
  mov rax,gInst
  mov [rsp+40h],rax
  lea rax,newTaskExeFilter
  mov [rsp+48h],rax
  mov dword ptr [rsp+5Ch],1
  lea rax,[rsi+NTD_PICK]
  mov [rsp+60h],rax
  mov dword ptr [rsp+68h],32768
  mov dword ptr [rsp+90h],208180Ch ; EXPLORER|FILE/PATHMUSTEXIST|NOCHANGEDIR|DONTADDTORECENT
  mov rcx,gInst
  mov edx,50004
  add edx,appLanguage
  lea r8,[rsi+NTD_FORMAT]
  mov r9d,512
  call LoadStringW
  lea rax,[rsi+NTD_FORMAT]
  mov [rsp+88h],rax
  lea rcx,[rsp+30h]
  call qword ptr [rsi+NTD_PICKER]
  mov edi,eax
  mov dword ptr [rsp+0D0h],0
  test eax,eax
  jnz ntdbRestore
  call CommDlgExtendedError
  mov [rsp+0D0h],eax
ntdbRestore:
  lea rcx,[rsi+NTD_DIR]
  call SetCurrentDirectoryW
  test eax,eax
  jnz ntdbUninit
  call GetLastError
  mov [rsp+0D0h],eax
ntdbUninit:
  call CoUninitialize
  mov edx,[rsp+0D0h]
  test edx,edx
  jnz ntdbError
  test edi,edi
  jz ntdbDone
  mov rcx,rbx
  mov edx,4001
  lea r8,[rsi+NTD_PICK]
  call SetDlgItemTextW
  jmp ntdbDone
ntdbTooLong:
  mov edx,206
  jmp ntdbError
ntdbComError:
  mov edx,eax
  jmp ntdbError
ntdbLastError:
  call GetLastError
  mov edx,eax
ntdbError:
  mov rcx,rbx
  call NewTaskDialogError
ntdbDone:
  add rsp,0E0h
  pop rdi
  pop rsi
  pop rbx
  ret
NewTaskDialogBrowse ENDP
