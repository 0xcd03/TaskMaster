; Native, DPI-scaled sidebar. Real BUTTON controls retain keyboard/UIA names.
extern BeginPaint:proc, EndPaint:proc, FillRect:proc, DrawTextW:proc
extern SetTextColor:proc, SetBkMode:proc, SelectObject:proc
extern SetMapMode:proc, SetWindowExtEx:proc, SetViewportExtEx:proc
extern SaveDC:proc, RestoreDC:proc, EnableWindow:proc
.data
shellWnd dq 0
memoryButton dq 0
shellHdc dq 0
shellBrush dq 0
shellCardBrush dq 0
shellActiveBrush dq 0
shellAccentBrush dq 0
shellTrackBrush dq 0
shellNormalFont dq 0
shellTitleFont dq 0
shellLargeFont dq 0
shellIconFont dq 0
shellTextColor dd 202020h
shellMutedColor dd 706860h
shellHeight dd 620
shellCardTop dd 382
shellMemoryValid dd 0
shellButtonBusy dd -1
shellPaintMode dd -1
shellClassName dw 'T','m','x','S','i','d','e','b','a','r',0
shellIconFace dw 'S','e','g','o','e',' ','M','D','L','2',' ','A','s','s','e','t','s',0
shellPercentFmt dw '%','u','%','%',0
shellGlyphs dw 0E8A5h,0,0E713h,0,0E716h,0,0E9D9h,0,0E7E8h,0,0E81Ch,0
.data?
shellClass WNDCLASSEXW <>
shellPaint db 72 dup(?)
shellRect RECT <>
shellTextRect RECT <>
shellDirtyRect RECT <>
shellMemory db 64 dup(?)
shellLabel dw 256 dup(?)
shellButtonText dw 256 dup(?)
.code

; Shell fonts are in logical 96-DPI units; the paint DC maps them to current DPI.
UpdateShellFonts PROC
  push rbx
  sub rsp,80h
  cmp shellNormalFont,0
  jne usfDone
  xor ebx,ebx
usfNext:
  mov ecx,-14
  cmp ebx,1
  jne usfNotTitle
  mov ecx,-18
usfNotTitle:
  cmp ebx,2
  jne usfNotLarge
  mov ecx,-30
usfNotLarge:
  cmp ebx,3
  jne usfFont
  mov ecx,-19
usfFont:
  xor edx,edx
  xor r8d,r8d
  xor r9d,r9d
  mov qword ptr [rsp+20h],400
  cmp ebx,1
  jne usfWeight
  mov qword ptr [rsp+20h],600
usfWeight:
  mov qword ptr [rsp+28h],0
  mov qword ptr [rsp+30h],0
  mov qword ptr [rsp+38h],0
  mov qword ptr [rsp+40h],1
  mov qword ptr [rsp+48h],0
  mov qword ptr [rsp+50h],0
  mov qword ptr [rsp+58h],5
  mov qword ptr [rsp+60h],0
  lea rax,fontFace
  cmp ebx,3
  jne usfFace
  lea rax,shellIconFace
usfFace:
  mov [rsp+68h],rax
  call CreateFontW
  lea rdx,shellNormalFont
  mov [rdx+rbx*8],rax
  inc ebx
  cmp ebx,4
  jb usfNext
usfDone:
  add rsp,80h
  pop rbx
  ret
UpdateShellFonts ENDP

UpdateShellTheme PROC
  push rbx
  push rsi
  sub rsp,28h
  lea rbx,shellBrush
  mov esi,5
ustDelete:
  mov rcx,[rbx]
  test rcx,rcx
  jz ustNext
  call DeleteObject
ustNext:
  add rbx,8
  dec esi
  jnz ustDelete
  mov shellTextColor,202020h
  mov shellMutedColor,706860h
  mov ecx,0FAF5EFh
  cmp themeOption,0
  je ustBackground
  mov ecx,242424h
  mov shellTextColor,0F2F2F2h
  mov shellMutedColor,0B8B8B8h
ustBackground:
  call CreateSolidBrush
  mov shellBrush,rax
  mov ecx,0FFFFFFh
  cmp themeOption,0
  je ustCard
  mov ecx,2E2E2Eh
ustCard:
  call CreateSolidBrush
  mov shellCardBrush,rax
  mov ecx,0F4E5D6h
  cmp themeOption,0
  je ustActive
  mov ecx,493D30h
ustActive:
  call CreateSolidBrush
  mov shellActiveBrush,rax
  mov ecx,0C56B00h
  call CreateSolidBrush
  mov shellAccentBrush,rax
  mov ecx,0E8E1DBh
  cmp themeOption,0
  je ustTrack
  mov ecx,505050h
ustTrack:
  call CreateSolidBrush
  mov shellTrackBrush,rax
  mov rcx,gMain
  test rcx,rcx
  jz ustShell
  mov edx,-10
  mov r8,shellCardBrush
  call SetClassLongPtrW
  mov rcx,gMain
  xor edx,edx
  mov r8d,1
  call InvalidateRect
ustShell:
  mov rcx,shellWnd
  test rcx,rcx
  jz ustDone
  xor edx,edx
  mov r8d,1
  call InvalidateRect
ustDone:
  add rsp,28h
  pop rsi
  pop rbx
  ret
UpdateShellTheme ENDP

CreateShellSidebar PROC
  sub rsp,68h
  call UpdateShellFonts
  call UpdateShellTheme
  mov shellClass.wcSize,SIZEOF WNDCLASSEXW
  lea rax,ShellWndProc
  mov shellClass.wcProc,rax
  mov rax,gInst
  mov shellClass.wcInst,rax
  lea rax,shellClassName
  mov shellClass.wcName,rax
  lea rcx,shellClass
  call RegisterClassExW
  xor ecx,ecx
  lea rdx,shellClassName
  lea r8,shellBrand
  mov r9d,52000000h
  mov qword ptr [rsp+20h],0
  mov qword ptr [rsp+28h],0
  mov qword ptr [rsp+30h],212
  mov qword ptr [rsp+38h],620
  mov rax,gMain
  mov [rsp+40h],rax
  mov qword ptr [rsp+48h],9010
  mov rax,gInst
  mov [rsp+50h],rax
  mov qword ptr [rsp+58h],0
  call CreateWindowExW
  mov shellWnd,rax
  mov rcx,rax
  mov edx,901
  mov r8d,1000
  xor r9d,r9d
  call SetTimer
  call RefreshShellMemory
  add rsp,68h
  ret
CreateShellSidebar ENDP

CreateMemoryButton PROC
  sub rsp,68h
  xor ecx,ecx
  lea rdx,szButton
  lea r8,shellMemoryButton
  mov r9d,5001000Bh
  mov qword ptr [rsp+20h],24
  mov qword ptr [rsp+28h],560
  mov qword ptr [rsp+30h],172
  mov qword ptr [rsp+38h],36
  mov rax,shellWnd
  mov [rsp+40h],rax
  mov qword ptr [rsp+48h],2005
  mov rax,gInst
  mov [rsp+50h],rax
  mov qword ptr [rsp+58h],0
  call CreateWindowExW
  mov memoryButton,rax
  add rsp,68h
  ret
CreateMemoryButton ENDP

LayoutMemoryButton PROC
  sub rsp,38h
  ; card top is expressed in logical units for paint and layout consistency.
  mov eax,gRect.rcBottom
  imul eax,96
  xor edx,edx
  div uiDpi
  mov shellHeight,eax
  sub eax,238
  cmp eax,344
  jge lmbTop
  mov eax,344
lmbTop:
  mov shellCardTop,eax
  add eax,178
  imul eax,uiDpi
  xor edx,edx
  mov ecx,96
  div ecx
  mov r8d,eax
  mov eax,24
  imul eax,uiDpi
  xor edx,edx
  div ecx
  mov edx,eax
  mov rcx,memoryButton
  mov r9d,uiSidebarWidth
  sub r9d,edx
  mov eax,36
  imul eax,uiDpi
  mov [rsp+30h],edx
  xor edx,edx
  mov r10d,96
  div r10d
  mov [rsp+20h],rax
  mov edx,[rsp+30h]
  mov qword ptr [rsp+28h],1
  call MoveWindow
  add rsp,38h
  ret
LayoutMemoryButton ENDP

RefreshShellMemory PROC
  sub rsp,28h
  mov dword ptr [shellMemory],64
  lea rcx,shellMemory
  call GlobalMemoryStatusEx
  mov shellMemoryValid,eax
  mov rcx,memoryButton
  test rcx,rcx
  jz rsmDone
  mov eax,actionBusy
  cmp eax,shellButtonBusy
  je rsmDone
  mov shellButtonBusy,eax
  xor edx,edx
  cmp actionBusy,0
  setz dl
  call EnableWindow
  mov rcx,memoryButton
  lea rdx,shellMemoryButton
  cmp actionBusy,0
  je rsmText
  lea rdx,shellMemoryBusy
rsmText:
  call SetWindowTextW
rsmDone:
  add rsp,28h
  ret
RefreshShellMemory ENDP

InvalidateShellCard PROC
  sub rsp,28h
  mov eax,frontMode
  cmp eax,shellPaintMode
  jne iscAll
  mov eax,shellCardTop
  imul eax,uiDpi
  xor edx,edx
  mov ecx,96
  div ecx
  mov shellDirtyRect.rcLeft,0
  mov shellDirtyRect.rcTop,eax
  mov eax,uiContentLeft
  mov shellDirtyRect.rcRight,eax
  mov eax,gRect.rcBottom
  mov shellDirtyRect.rcBottom,eax
  lea rdx,shellDirtyRect
  jmp iscCall
iscAll:
  mov shellPaintMode,eax
  xor edx,edx
iscCall:
  mov rcx,shellWnd
  xor r8d,r8d
  call InvalidateRect
  add rsp,28h
  ret
InvalidateShellCard ENDP

InvalidateNavigation PROC
  push rbx
  sub rsp,20h
  xor ebx,ebx
inNext:
  lea rax,navButtons
  mov rcx,[rax+rbx*8]
  test rcx,rcx
  jz inAdvance
  xor edx,edx
  xor r8d,r8d
  call InvalidateRect
inAdvance:
  inc ebx
  cmp ebx,6
  jb inNext
  add rsp,20h
  pop rbx
  ret
InvalidateNavigation ENDP

SetupShellDC PROC
  push rbx
  sub rsp,20h
  mov rbx,rcx
  mov edx,8 ; MM_ANISOTROPIC
  call SetMapMode
  mov rcx,rbx
  mov edx,96
  mov r8d,96
  xor r9d,r9d
  call SetWindowExtEx
  mov rcx,rbx
  mov edx,uiDpi
  mov r8d,edx
  xor r9d,r9d
  call SetViewportExtEx
  mov rcx,rbx
  mov edx,1
  call SetBkMode
  mov rcx,rbx
  mov rdx,shellNormalFont
  call SelectObject
  add rsp,20h
  pop rbx
  ret
SetupShellDC ENDP

; RCX text, EDX left, R8D top, R9D width. Logical units, single line.
ShellLabel PROC
  sub rsp,28h
  mov rax,rcx
  mov shellTextRect.rcLeft,edx
  add edx,r9d
  mov shellTextRect.rcRight,edx
  mov shellTextRect.rcTop,r8d
  add r8d,24
  mov shellTextRect.rcBottom,r8d
  mov rcx,shellHdc
  mov rdx,rax
  mov r8d,-1
  lea r9,shellTextRect
  mov qword ptr [rsp+20h],8824h
  call DrawTextW
  add rsp,28h
  ret
ShellLabel ENDP

PaintShell PROC
  push rbx
  sub rsp,30h
  mov rcx,shellWnd
  lea rdx,shellPaint
  call BeginPaint
  mov shellHdc,rax
  mov rcx,rax
  call SetupShellDC
  mov shellRect.rcLeft,0
  mov shellRect.rcTop,0
  mov shellRect.rcRight,212
  mov eax,shellHeight
  mov shellRect.rcBottom,eax
  mov rcx,shellHdc
  lea rdx,shellRect
  mov r8,shellBrush
  call FillRect
  mov rcx,shellHdc
  mov edx,shellTextColor
  call SetTextColor
  mov rcx,shellHdc
  mov rdx,shellTitleFont
  call SelectObject
  lea rcx,shellBrand
  mov edx,20
  mov r8d,18
  mov r9d,180
  call ShellLabel
  mov ebx,shellCardTop
  mov shellRect.rcLeft,12
  mov shellRect.rcTop,ebx
  mov shellRect.rcRight,208
  lea eax,[rbx+226]
  mov shellRect.rcBottom,eax
  mov rcx,shellHdc
  lea rdx,shellRect
  mov r8,shellCardBrush
  call FillRect
  mov rcx,shellHdc
  mov rdx,shellNormalFont
  call SelectObject
  lea rcx,shellMemoryTitle
  mov edx,24
  lea r8d,[rbx+12]
  mov r9d,174
  call ShellLabel
  cmp shellMemoryValid,0
  je psDone
  lea rcx,shellLabel
  lea rdx,shellPercentFmt
  mov r8d,dword ptr [shellMemory+4]
  call wsprintfW
  mov rcx,shellHdc
  mov rdx,shellLargeFont
  call SelectObject
  lea rcx,shellLabel
  mov edx,24
  lea r8d,[rbx+44]
  mov r9d,174
  call ShellLabel
  mov shellRect.rcLeft,24
  lea eax,[rbx+88]
  mov shellRect.rcTop,eax
  mov shellRect.rcRight,196
  add eax,6
  mov shellRect.rcBottom,eax
  mov rcx,shellHdc
  lea rdx,shellRect
  mov r8,shellTrackBrush
  call FillRect
  mov eax,dword ptr [shellMemory+4]
  imul eax,172
  xor edx,edx
  mov ecx,100
  div ecx
  add eax,24
  mov shellRect.rcRight,eax
  mov rcx,shellHdc
  lea rdx,shellRect
  mov r8,shellAccentBrush
  call FillRect
  mov rcx,shellHdc
  mov rdx,shellNormalFont
  call SelectObject
  mov rcx,shellHdc
  mov edx,shellMutedColor
  call SetTextColor
  lea rcx,shellLabel
  lea rdx,shellMemoryDetailFmt
  mov r9,qword ptr [shellMemory+8]
  mov r8,r9
  sub r8,qword ptr [shellMemory+16]
  shr r8,20
  shr r9,20
  call wsprintfW
  lea rcx,shellLabel
  mov edx,24
  lea r8d,[rbx+102]
  mov r9d,176
  call ShellLabel
  lea rcx,shellLabel
  lea rdx,shellMemoryLoadFmt
  mov r8d,dword ptr [shellMemory+4]
  call wsprintfW
  cmp memoryTrimAfterValid,0
  je psLast
  cmp memoryTrimBeforeValid,0
  je psLast
  cmp actionBusy,0
  jne psLast
  lea rcx,shellLabel
  lea rdx,shellMemoryLastFmt
  mov r8d,dword ptr [memoryTrimBefore+4]
  mov r9d,dword ptr [memoryTrimAfter+4]
  call wsprintfW
psLast:
  lea rcx,shellLabel
  mov edx,24
  lea r8d,[rbx+126]
  mov r9d,176
  call ShellLabel
  lea rcx,shellMemoryNote
  mov edx,24
  lea r8d,[rbx+150]
  mov r9d,176
  call ShellLabel
psDone:
  mov rcx,shellWnd
  lea rdx,shellPaint
  call EndPaint
  add rsp,30h
  pop rbx
  ret
PaintShell ENDP

DrawShellButton PROC
  push rbx
  push rsi
  sub rsp,38h
  mov rbx,r9
  mov rcx,[rbx+32]
  mov shellHdc,rcx
  call SaveDC
  mov [rsp+30h],eax
  mov rcx,shellHdc
  call SetupShellDC
  mov shellRect.rcLeft,0
  mov shellRect.rcTop,0
  mov shellRect.rcRight,196
  mov shellRect.rcBottom,42
  mov r8,shellBrush
  cmp dword ptr [rbx+4],2005
  jne dsbNav
  mov shellRect.rcRight,172
  mov shellRect.rcBottom,36
  mov r8,shellAccentBrush
  jmp dsbFill
dsbNav:
  mov eax,[rbx+4]
  sub eax,2100
  cmp eax,desiredMode
  je dsbSelected
  test eax,eax
  jne dsbFill
  cmp desiredMode,1
  jne dsbFill
dsbSelected:
  mov r8,shellActiveBrush
dsbFill:
  mov rcx,shellHdc
  lea rdx,shellRect
  call FillRect
  mov rcx,shellHdc
  mov edx,shellTextColor
  cmp dword ptr [rbx+4],2005
  jne dsbTextColor
  mov edx,0FFFFFFh
dsbTextColor:
  call SetTextColor
  mov rcx,[rbx+24]
  lea rdx,shellButtonText
  mov r8d,256
  call GetWindowTextW
  cmp dword ptr [rbx+4],2005
  je dsbOptimize
  mov esi,[rbx+4]
  sub esi,2100
  test esi,esi
  jz dsbGlyph
  dec esi
dsbGlyph:
  cmp esi,5
  ja dsbOptimize
  mov rcx,shellHdc
  mov rdx,shellIconFont
  call SelectObject
  lea rcx,shellGlyphs
  lea rcx,[rcx+rsi*4]
  mov edx,14
  mov r8d,9
  mov r9d,28
  call ShellLabel
  mov rcx,shellHdc
  mov rdx,shellNormalFont
  call SelectObject
  lea rcx,shellButtonText
  mov edx,48
  mov r8d,9
  mov r9d,146
  call ShellLabel
  jmp dsbDone
dsbOptimize:
  mov rcx,shellHdc
  lea rdx,shellButtonText
  mov r8d,-1
  lea r9,shellRect
  mov qword ptr [rsp+20h],825h
  call DrawTextW
dsbDone:
  mov rcx,shellHdc
  mov edx,[rsp+30h]
  call RestoreDC
  mov eax,1
  add rsp,38h
  pop rsi
  pop rbx
  ret
DrawShellButton ENDP

ShellWndProc PROC
  sub rsp,28h
  cmp edx,0Fh
  je swpPaint
  cmp edx,2Bh
  je swpDraw
  cmp edx,111h
  je swpCommand
  cmp edx,113h
  je swpTimer
  cmp edx,14h
  je swpErase
  call DefWindowProcW
  jmp swpDone
swpPaint:
  call PaintShell
  xor eax,eax
  jmp swpDone
swpDraw:
  call DrawShellButton
  jmp swpDone
swpCommand:
  mov rcx,gMain
  call PostMessageW
  jmp swpZero
swpTimer:
  cmp movingWindow,0
  jne swpZero
  call RefreshShellMemory
  call InvalidateShellCard
swpZero:
  xor eax,eax
  jmp swpDone
swpErase:
  mov eax,1
swpDone:
  add rsp,28h
  ret
ShellWndProc ENDP
