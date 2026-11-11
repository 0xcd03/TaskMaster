; Programmatic color-path regression on the hidden test's own controls.
; Paints into an offscreen compatible bitmap, never captures or drives the desktop.
; Assertions cover colors/dispatch only and make no claim about visual layout.
extern CreateCompatibleDC:proc, CreateCompatibleBitmap:proc, DeleteDC:proc
extern GetPixel:proc, GetObjectW:proc, GetTextColor:proc, GetBkColor:proc
.data
uiThemeStage dd 0
uiThemeAssertions dd 0
uiThemeExpectedEdit dd 0FFFFFFh,2E2E2Eh
uiThemeExpectedText dd 202020h,0F2F2F2h
uiThemeExpectedHeader dd 0F7F7F7h,202020h
uiThemeExpectedMenu dd 0FFFFFFh,202020h
uiThemeExpectedSelected dd 0F4E5D6h,493D30h
uiThemeDc dq 0
uiThemeBitmap dq 0
uiThemeOldBitmap dq 0
uiThemeHeaderResized dd 0
uiThemeTailPixel dd 0
uiThemeDiagnosticBytes dd 0
uiThemeDiagnosticFormat db 'Header paint: theme=%u, lastRight=%d, clientRight=%d, clientBottom=%d, pixel=%08X',13,10,0
.data?
align 8
uiThemeDiagnostic db 256 dup(?)
uiThemeSavedHeaderRect RECT <>
uiThemeHeaderRect RECT <>
uiThemeLastRect RECT <>
uiThemeBrushInfo db 16 dup(?)
uiThemeHeaderDraw db 80 dup(?)
uiThemeMenuInfo db 80 dup(?)
uiThemeMenuDraw db 64 dup(?)
.code
UiThemeRegression PROC
  push rbx
  push rsi
  push rdi
  sub rsp,40h
  mov uiThemeStage,130
  ; The test window can be narrower than the DPI-scaled columns. Give this
  ; hidden header a deliberate tail, preserving its original size for cleanup.
  mov rcx,processHeader
  lea rdx,uiThemeSavedHeaderRect
  call GetWindowRect
  test eax,eax
  jz utrCleanup
  mov rcx,processHeader
  mov edx,1200h ; HDM_GETITEMCOUNT
  xor r8d,r8d
  xor r9d,r9d
  call SendMessageW
  test eax,eax
  jle utrCleanup
  lea r8d,[eax-1]
  mov rcx,processHeader
  mov edx,1207h ; HDM_GETITEMRECT
  lea r9,uiThemeLastRect
  call SendMessageW
  test eax,eax
  jz utrCleanup
  mov rcx,processHeader
  xor edx,edx
  xor r8d,r8d
  xor r9d,r9d
  mov eax,uiThemeLastRect.rcRight
  add eax,64
  mov [rsp+20h],eax
  mov eax,uiThemeSavedHeaderRect.rcBottom
  sub eax,uiThemeSavedHeaderRect.rcTop
  mov [rsp+28h],eax
  mov qword ptr [rsp+30h],16h ; NOMOVE | NOZORDER | NOACTIVATE
  call SetWindowPos
  test eax,eax
  jz utrCleanup
  mov uiThemeHeaderResized,1
  mov rcx,processHeader
  lea rdx,uiThemeHeaderRect
  call GetClientRect
  test eax,eax
  jz utrCleanup
  mov rcx,gMain
  call GetDC
  mov rbx,rax
  test rax,rax
  jz utrCleanup
  mov rcx,rax
  call CreateCompatibleDC
  mov uiThemeDc,rax
  mov rcx,rbx
  mov edx,uiThemeHeaderRect.rcRight
  mov eax,256 ; synthetic item/menu callbacks below cover a 256 x 40 rect
  cmp edx,eax
  cmovl edx,eax
  mov r8d,uiThemeHeaderRect.rcBottom
  mov eax,40
  cmp r8d,eax
  cmovl r8d,eax
  call CreateCompatibleBitmap
  mov uiThemeBitmap,rax
  mov rcx,gMain
  mov rdx,rbx
  call ReleaseDC
  cmp uiThemeDc,0
  je utrCleanup
  cmp uiThemeBitmap,0
  je utrCleanup
  mov rcx,uiThemeDc
  mov rdx,uiThemeBitmap
  call SelectObject
  mov uiThemeOldBitmap,rax
  test rax,rax
  jz utrCleanup
  cmp rax,-1
  je utrCleanup
  xor esi,esi
utrTheme:
  mov themeOption,esi
  call ApplyTheme
  ; A real parent WM_CTLCOLOREDIT must select the edit's themed colors/brush.
  mov uiThemeStage,131
  mov rcx,gMain
  mov edx,133h
  mov r8,uiThemeDc
  mov r9,searchWnd
  call SendMessageW
  test rax,rax
  jz utrCleanup
  mov rcx,rax
  mov edx,16
  lea r8,uiThemeBrushInfo
  call GetObjectW
  cmp eax,16
  jne utrCleanup
  lea rdx,uiThemeExpectedEdit
  mov eax,[rdx+rsi*4]
  cmp dword ptr [uiThemeBrushInfo+4],eax
  jne utrCleanup
  mov rcx,uiThemeDc
  call GetBkColor
  lea rdx,uiThemeExpectedEdit
  cmp eax,[rdx+rsi*4]
  jne utrCleanup
  mov rcx,uiThemeDc
  call GetTextColor
  lea rdx,uiThemeExpectedText
  cmp eax,[rdx+rsi*4]
  jne utrCleanup
  inc uiThemeAssertions
  ; Drive actual header NM_CUSTOMDRAW routing through the list subclass.
  mov uiThemeStage,132
  lea rdi,uiThemeHeaderDraw
  xor eax,eax
  mov ecx,10
  rep stosq
  mov rax,processHeader
  mov qword ptr [uiThemeHeaderDraw],rax
  mov dword ptr [uiThemeHeaderDraw+16],-12
  mov dword ptr [uiThemeHeaderDraw+24],10001h
  mov rax,uiThemeDc
  mov qword ptr [uiThemeHeaderDraw+32],rax
  mov dword ptr [uiThemeHeaderDraw+48],256
  mov dword ptr [uiThemeHeaderDraw+52],40
  mov rcx,gList
  mov edx,4Eh
  xor r8d,r8d
  lea r9,uiThemeHeaderDraw
  call SendMessageW
  cmp eax,4 ; CDRF_SKIPDEFAULT
  jne utrCleanup
  mov rcx,uiThemeDc
  mov edx,2
  mov r8d,2
  call GetPixel
  lea rdx,uiThemeExpectedHeader
  cmp eax,[rdx+rsi*4]
  jne utrCleanup
  inc uiThemeAssertions
  ; Render the complete native header, including the formerly white tail.
  ; Unlike the synthetic item callback above, this runs the real subclass.
  mov uiThemeStage,135
  mov rcx,processHeader
  mov edx,318h ; WM_PRINTCLIENT
  mov r8,uiThemeDc
  mov r9d,4 ; PRF_CLIENT
  call SendMessageW
  mov edx,uiThemeLastRect.rcRight
  add edx,3
  cmp edx,uiThemeHeaderRect.rcRight
  jae utrCleanup
  mov rcx,uiThemeDc
  mov r8d,2
  call GetPixel
  mov uiThemeTailPixel,eax
  lea rdx,uiThemeExpectedHeader
  cmp eax,[rdx+rsi*4]
  jne utrCleanup
  inc uiThemeAssertions
  mov uiThemeStage,136
  mov rcx,mainMenu
  mov edx,4
  call GetSubMenu
  mov rcx,rax
  mov edx,2302
  add edx,esi ; light/dark current option
  xor r8d,r8d ; MF_BYCOMMAND
  call GetMenuState
  cmp eax,-1
  je utrCleanup
  test eax,8 ; MF_CHECKED
  jz utrCleanup
  inc uiThemeAssertions
  ; Obtain the real command's owner data, then exercise main WM_DRAWITEM.
  mov uiThemeStage,133
  lea rdi,uiThemeMenuInfo
  xor eax,eax
  mov ecx,10
  rep stosq
  mov dword ptr [uiThemeMenuInfo],80
  mov dword ptr [uiThemeMenuInfo+4],20h ; MIIM_DATA
  mov rcx,mainMenu
  mov edx,2001
  xor r8d,r8d
  lea r9,uiThemeMenuInfo
  call GetMenuItemInfoW
  test eax,eax
  jz utrCleanup
  mov rax,qword ptr [uiThemeMenuInfo+48]
  test rax,rax
  jz utrCleanup
  mov qword ptr [uiThemeMenuDraw+56],rax
  mov dword ptr [uiThemeMenuDraw],1 ; ODT_MENU
  mov dword ptr [uiThemeMenuDraw+8],2001
  mov dword ptr [uiThemeMenuDraw+12],1 ; ODA_DRAWENTIRE
  mov rax,mainMenu
  mov qword ptr [uiThemeMenuDraw+24],rax
  mov rax,uiThemeDc
  mov qword ptr [uiThemeMenuDraw+32],rax
  mov dword ptr [uiThemeMenuDraw+48],256
  mov dword ptr [uiThemeMenuDraw+52],40
  xor ebx,ebx
utrMenuState:
  mov dword ptr [uiThemeMenuDraw+16],ebx ; normal / selected
  mov rcx,gMain
  mov edx,2Bh
  xor r8d,r8d
  lea r9,uiThemeMenuDraw
  call SendMessageW
  cmp eax,1
  jne utrCleanup
  mov rcx,uiThemeDc
  mov edx,2
  mov r8d,2
  call GetPixel
  lea rdx,uiThemeExpectedMenu
  test ebx,ebx
  jz utrMenuExpected
  lea rdx,uiThemeExpectedSelected
utrMenuExpected:
  cmp eax,[rdx+rsi*4]
  jne utrCleanup
  inc uiThemeAssertions
  inc ebx
  cmp ebx,2
  jb utrMenuState
  inc esi
  cmp esi,2
  jb utrTheme
  mov uiThemeStage,134
  mov rcx,uiThemeDc
  call ScrollPaintTests
  test eax,eax
  jnz utrCleanup
  inc uiThemeAssertions
  mov uiThemeStage,0
utrCleanup:
  cmp uiThemeStage,135
  jne utrDeleteObjects
  call UiThemeHeaderDiagnostic
utrDeleteObjects:
  mov rcx,uiThemeDc
  test rcx,rcx
  jz utrBitmap
  mov rdx,uiThemeOldBitmap
  test rdx,rdx
  jz utrDeleteDc
  cmp rdx,-1
  je utrDeleteDc
  call SelectObject
utrDeleteDc:
  mov rcx,uiThemeDc
  call DeleteDC
utrBitmap:
  mov rcx,uiThemeBitmap
  test rcx,rcx
  jz utrReturn
  call DeleteObject
utrReturn:
  cmp uiThemeHeaderResized,0
  je utrResult
  mov rcx,processHeader
  xor edx,edx
  xor r8d,r8d
  xor r9d,r9d
  mov eax,uiThemeSavedHeaderRect.rcRight
  sub eax,uiThemeSavedHeaderRect.rcLeft
  mov [rsp+20h],eax
  mov eax,uiThemeSavedHeaderRect.rcBottom
  sub eax,uiThemeSavedHeaderRect.rcTop
  mov [rsp+28h],eax
  mov qword ptr [rsp+30h],16h
  call SetWindowPos
  mov uiThemeHeaderResized,0
utrResult:
  mov eax,uiThemeStage
  add rsp,40h
  pop rdi
  pop rsi
  pop rbx
  ret
UiThemeRegression ENDP

; Failure-only evidence for the offscreen header geometry and paint assertion.
UiThemeHeaderDiagnostic PROC
  sub rsp,58h
  lea rcx,uiThemeDiagnostic
  lea rdx,uiThemeDiagnosticFormat
  mov r8d,themeOption
  mov r9d,uiThemeLastRect.rcRight
  mov eax,uiThemeHeaderRect.rcRight
  mov [rsp+20h],rax
  mov eax,uiThemeHeaderRect.rcBottom
  mov [rsp+28h],rax
  mov eax,uiThemeTailPixel
  mov [rsp+30h],rax
  call wsprintfA
  mov [rsp+40h],eax
  mov ecx,-11
  call GetStdHandle
  mov rcx,rax
  lea rdx,uiThemeDiagnostic
  mov r8d,[rsp+40h]
  lea r9,uiThemeDiagnosticBytes
  mov qword ptr [rsp+20h],0
  call WriteFile
  add rsp,58h
  ret
UiThemeHeaderDiagnostic ENDP
