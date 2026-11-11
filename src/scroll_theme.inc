; Public Win32 non-client scrollbar paint. Native hit testing, dragging,
; accessibility and keyboard behavior stay in DefSubclassProc. We paint after
; native processing using GetScrollBarInfo's exact range-independent geometry.
extern GetWindowDC:proc, GetWindowRect:proc, GetScrollBarInfo:proc
extern ReleaseDC:proc, SaveDC:proc, RestoreDC:proc, SetDCBrushColor:proc
extern SetTextColor:proc, SetBkMode:proc, SelectObject:proc
extern GetStockObject:proc, FillRect:proc, DrawTextW:proc
extern SetWindowSubclass:proc, DefSubclassProc:proc
.data
scrollPaintBusy dd 0
scrollUpGlyph dw 25B4h,0
scrollDownGlyph dw 25BEh,0
scrollLeftGlyph dw 25C2h,0
scrollRightGlyph dw 25B8h,0
.code

; RCX HDC, RDX SCROLLBARINFO, R8 window RECT (screen), R9D=1 vertical.
; Exposed separately so exact colors/geometry can be verified offscreen.
RenderDarkScrollbar PROC
  push rbx
  push rsi
  push rdi
  push r12
  push r13
  sub rsp,80h
  mov rbx,rcx
  mov rsi,rdx
  mov r12d,r9d
  mov r13,r8
  test dword ptr [rsi+36],18000h ; INVISIBLE | OFFSCREEN
  jnz rdsDone
  mov eax,[rsi+12]
  cmp eax,[rsi+4]
  jle rdsDone
  mov eax,[rsi+16]
  cmp eax,[rsi+8]
  jle rdsDone
  call SaveDC
  mov [rsp+70h],eax
  mov ecx,18 ; DC_BRUSH avoids one allocation per paint
  call GetStockObject
  mov rdi,rax
  mov rcx,rbx
  mov edx,202020h
  call SetDCBrushColor
  ; Translate full track rectangle to the window DC's coordinate system.
  mov eax,[rsi+4]
  sub eax,[r13]
  mov [rsp+30h],eax
  mov eax,[rsi+8]
  sub eax,[r13+4]
  mov [rsp+34h],eax
  mov eax,[rsi+12]
  sub eax,[r13]
  mov [rsp+38h],eax
  mov eax,[rsi+16]
  sub eax,[r13+4]
  mov [rsp+3Ch],eax
  mov rax,[rsp+30h]
  mov [rsp+40h],rax
  mov rax,[rsp+38h]
  mov [rsp+48h],rax
  mov rcx,rbx
  lea rdx,[rsp+30h]
  mov r8,rdi
  call FillRect
  ; xyThumbTop/Bottom are relative to rcScrollBar, as verified on Win32.
  mov eax,[rsi+28]
  cmp eax,[rsi+24]
  jle rdsArrows
  test r12d,r12d
  jz rdsHorizontalThumb
  mov eax,[rsi+24]
  add eax,[rsp+44h]
  mov [rsp+34h],eax
  mov eax,[rsi+28]
  add eax,[rsp+44h]
  mov [rsp+3Ch],eax
  add dword ptr [rsp+30h],4
  sub dword ptr [rsp+38h],4
  jmp rdsThumb
rdsHorizontalThumb:
  mov eax,[rsi+24]
  add eax,[rsp+40h]
  mov [rsp+30h],eax
  mov eax,[rsi+28]
  add eax,[rsp+40h]
  mov [rsp+38h],eax
  add dword ptr [rsp+34h],4
  sub dword ptr [rsp+3Ch],4
rdsThumb:
  mov rcx,rbx
  mov edx,707070h
  test dword ptr [rsi+48],8 ; STATE_SYSTEM_PRESSED, thumb child 3
  jz rdsThumbInk
  mov edx,0A0A0A0h
rdsThumbInk:
  call SetDCBrushColor
  mov rcx,rbx
  lea rdx,[rsp+30h]
  mov r8,rdi
  call FillRect
rdsArrows:
  mov ecx,17 ; DEFAULT_GUI_FONT, system glyph fallback
  call GetStockObject
  mov rcx,rbx
  mov rdx,rax
  call SelectObject
  mov rcx,rbx
  mov edx,1
  call SetBkMode
  mov rcx,rbx
  mov edx,0B0B0B0h
  test dword ptr [rsi+36],1 ; disabled scrollbar
  jz rdsArrowInk
  mov edx,606060h
rdsArrowInk:
  call SetTextColor
  mov rax,[rsp+40h]
  mov [rsp+30h],rax
  mov rax,[rsp+48h]
  mov [rsp+38h],rax
  lea rdx,scrollUpGlyph
  test r12d,r12d
  jz rdsLeftArrow
  mov eax,[rsi+20]
  add eax,[rsp+34h]
  mov [rsp+3Ch],eax
  jmp rdsFirstArrow
rdsLeftArrow:
  mov eax,[rsi+20]
  add eax,[rsp+30h]
  mov [rsp+38h],eax
  lea rdx,scrollLeftGlyph
rdsFirstArrow:
  mov rcx,rbx
  mov r8d,1
  lea r9,[rsp+30h]
  mov qword ptr [rsp+20h],825h
  call DrawTextW
  mov rax,[rsp+40h]
  mov [rsp+30h],rax
  mov rax,[rsp+48h]
  mov [rsp+38h],rax
  lea rdx,scrollDownGlyph
  test r12d,r12d
  jz rdsRightArrow
  mov eax,[rsp+4Ch] ; original track bottom
  sub eax,[rsi+20]
  mov [rsp+34h],eax
  mov eax,[rsp+4Ch]
  mov [rsp+3Ch],eax
  jmp rdsSecondArrow
rdsRightArrow:
  mov eax,[rsp+48h] ; original track right
  sub eax,[rsi+20]
  mov [rsp+30h],eax
  mov eax,[rsp+48h]
  mov [rsp+38h],eax
  lea rdx,scrollRightGlyph
rdsSecondArrow:
  mov rcx,rbx
  mov r8d,1
  lea r9,[rsp+30h]
  mov qword ptr [rsp+20h],825h
  call DrawTextW
  mov rcx,rbx
  mov edx,[rsp+70h]
  call RestoreDC
rdsDone:
  add rsp,80h
  pop r13
  pop r12
  pop rdi
  pop rsi
  pop rbx
  ret
RenderDarkScrollbar ENDP

PaintDarkScrollbars PROC
  push rbx
  push rsi
  sub rsp,0D8h
  cmp themeOption,0
  je pdsDone
  cmp scrollPaintBusy,0
  jne pdsDone
  mov scrollPaintBusy,1
  mov rbx,rcx
  mov dword ptr [rsp+40h],0
  lea rdx,[rsp+30h]
  call GetWindowRect
  test eax,eax
  jz pdsClear
  mov rcx,rbx
  call GetWindowDC
  mov rsi,rax
  test rax,rax
  jz pdsClear
  mov dword ptr [rsp+50h],60
  mov rcx,rbx
  mov edx,-5 ; OBJID_VSCROLL
  lea r8,[rsp+50h]
  call GetScrollBarInfo
  test eax,eax
  jz pdsHorizontal
  test dword ptr [rsp+74h],18000h
  jnz pdsHorizontal
  mov dword ptr [rsp+40h],1
  mov rcx,rsi
  lea rdx,[rsp+50h]
  lea r8,[rsp+30h]
  mov r9d,1
  call RenderDarkScrollbar
pdsHorizontal:
  mov dword ptr [rsp+90h],60
  mov rcx,rbx
  mov edx,-6 ; OBJID_HSCROLL
  lea r8,[rsp+90h]
  call GetScrollBarInfo
  test eax,eax
  jz pdsRelease
  mov rcx,rsi
  lea rdx,[rsp+90h]
  lea r8,[rsp+30h]
  xor r9d,r9d
  call RenderDarkScrollbar
  ; Paint the resize corner when both bars are visible.
  cmp dword ptr [rsp+40h],0
  je pdsRelease
  test dword ptr [rsp+0B4h],18000h
  jnz pdsRelease
  mov eax,[rsp+54h]
  sub eax,[rsp+30h]
  mov [rsp+20h],eax
  mov eax,[rsp+98h]
  sub eax,[rsp+34h]
  mov [rsp+24h],eax
  mov eax,[rsp+5Ch]
  sub eax,[rsp+30h]
  mov [rsp+28h],eax
  mov eax,[rsp+0A0h]
  sub eax,[rsp+34h]
  mov [rsp+2Ch],eax
  mov rcx,rsi
  mov edx,202020h
  call SetDCBrushColor
  mov ecx,18
  call GetStockObject
  mov r8,rax
  mov rcx,rsi
  lea rdx,[rsp+20h]
  call FillRect
pdsRelease:
  mov rcx,rbx
  mov rdx,rsi
  call ReleaseDC
pdsClear:
  mov scrollPaintBusy,0
pdsDone:
  add rsp,0D8h
  pop rsi
  pop rbx
  ret
PaintDarkScrollbars ENDP

ScrollThemeSubclass PROC
  push rbx
  sub rsp,50h
  mov rbx,rcx
  cmp edx,113h ; timer exists only inside native scrollbar tracking
  jne stsNotTimer
  cmp r8d,544D5844h
  jne stsDefault
  call PaintDarkScrollbars
  xor eax,eax
  jmp stsDone
stsNotTimer:
  cmp edx,0Fh ; WM_PAINT, item count/scroll updates may redraw bars too
  je stsAfterDefault
  cmp edx,5 ; WM_SIZE
  je stsAfterDefault
  cmp edx,85h ; WM_NCPAINT
  je stsAfterDefault
  cmp edx,86h ; WM_NCACTIVATE
  je stsAfterDefault
  cmp edx,31Ah ; WM_THEMECHANGED
  je stsAfterDefault
  cmp edx,0A0h ; WM_NCMOUSEMOVE
  je stsAfterDefault
  cmp edx,0A1h ; WM_NCLBUTTONDOWN
  je stsTrack
  cmp edx,0A2h ; WM_NCLBUTTONUP
  je stsAfterDefault
  cmp edx,114h ; WM_HSCROLL
  je stsAfterDefault
  cmp edx,115h ; WM_VSCROLL
  je stsAfterDefault
  cmp edx,20Ah ; WM_MOUSEWHEEL
  je stsAfterDefault
  cmp edx,20Eh ; WM_MOUSEHWHEEL
  je stsAfterDefault
  cmp edx,2A2h ; WM_NCMOUSELEAVE
  je stsAfterDefault
  cmp edx,215h ; WM_CAPTURECHANGED
  je stsAfterDefault
stsDefault:
  call DefSubclassProc
  jmp stsDone
stsTrack:
  cmp themeOption,0
  je stsDefault
  cmp r8d,6 ; HTHSCROLL
  je stsBeginTracking
  cmp r8d,7 ; HTVSCROLL
  jne stsDefault
stsBeginTracking:
  mov [rsp+30h],r8
  mov [rsp+38h],r9
  mov edx,544D5844h
  mov r8d,16
  xor r9d,r9d
  call SetTimer
  mov rcx,rbx
  mov edx,0A1h
  mov r8,[rsp+30h]
  mov r9,[rsp+38h]
  call DefSubclassProc
  mov [rsp+20h],rax
  mov rcx,rbx
  mov edx,544D5844h
  call KillTimer
  mov rcx,rbx
  call PaintDarkScrollbars
  mov rax,[rsp+20h]
  jmp stsDone
stsAfterDefault:
  call DefSubclassProc
  mov [rsp+20h],rax
  mov rcx,rbx
  call PaintDarkScrollbars
  mov rax,[rsp+20h]
stsDone:
  add rsp,50h
  pop rbx
  ret
ScrollThemeSubclass ENDP

InitScrollTheme PROC
  sub rsp,28h
  mov rcx,gList
  lea rdx,ScrollThemeSubclass
  mov r8d,2
  xor r9d,r9d
  call SetWindowSubclass
  add rsp,28h
  ret
InitScrollTheme ENDP

IFDEF TM_TESTS
.data
scrollTestWindow dd 100,200,356,240
scrollTestFill dd 0,0,256,40
scrollTestVertical dd 60,330,200,350,240,6,10,22,0,0,0,0,0,0,0
scrollTestHorizontal dd 60,100,220,300,240,10,40,80,0,0,0,0,0,0,0
.code
; RCX compatible color DC containing a >=256x40 bitmap. Pure offscreen checks;
; no desktop capture, messages, input injection or visible windows are used.
ScrollPaintTests PROC
  push rbx
  push rsi
  push rdi
  sub rsp,30h
  mov rbx,rcx
  call SaveDC
  mov esi,eax
  mov edi,1
  call ClearScrollTestSurface
  mov rcx,rbx
  mov edx,123456h
  call SetTextColor
  mov rcx,rbx
  lea rdx,scrollTestVertical
  lea r8,scrollTestWindow
  mov r9d,1
  call RenderDarkScrollbar
  mov rcx,rbx
  mov edx,240
  mov r8d,15
  call GetPixel
  cmp eax,707070h
  jne sptDone
  mov rcx,rbx
  mov edx,231
  mov r8d,25
  call GetPixel
  cmp eax,202020h
  jne sptDone
  mov rcx,rbx
  mov edx,229
  mov r8d,25
  call GetPixel
  cmp eax,0FFFFFFh
  jne sptDone
  mov rcx,rbx
  call GetTextColor
  cmp eax,123456h
  jne sptDone
  call ClearScrollTestSurface
  mov rcx,rbx
  lea rdx,scrollTestHorizontal
  lea r8,scrollTestWindow
  xor r9d,r9d
  call RenderDarkScrollbar
  mov rcx,rbx
  mov edx,55
  mov r8d,30
  call GetPixel
  cmp eax,707070h
  jne sptDone
  mov rcx,rbx
  mov edx,100
  mov r8d,21
  call GetPixel
  cmp eax,202020h
  jne sptDone
  mov dword ptr [scrollTestHorizontal+48],8
  mov rcx,rbx
  lea rdx,scrollTestHorizontal
  lea r8,scrollTestWindow
  xor r9d,r9d
  call RenderDarkScrollbar
  mov rcx,rbx
  mov edx,55
  mov r8d,30
  call GetPixel
  cmp eax,0A0A0A0h
  jne sptDone
  call ClearScrollTestSurface
  mov dword ptr [scrollTestVertical+36],8000h
  mov rcx,rbx
  lea rdx,scrollTestVertical
  lea r8,scrollTestWindow
  mov r9d,1
  call RenderDarkScrollbar
  mov rcx,rbx
  mov edx,231
  mov r8d,25
  call GetPixel
  cmp eax,0FFFFFFh
  jne sptDone
  xor edi,edi
sptDone:
  mov dword ptr [scrollTestHorizontal+48],0
  mov dword ptr [scrollTestVertical+36],0
  mov rcx,rbx
  mov edx,esi
  call RestoreDC
  mov eax,edi
  add rsp,30h
  pop rdi
  pop rsi
  pop rbx
  ret
ScrollPaintTests ENDP

; Parent test keeps the destination DC in nonvolatile RBX.
ClearScrollTestSurface PROC
  sub rsp,28h
  xor ecx,ecx ; WHITE_BRUSH
  call GetStockObject
  mov r8,rax
  mov rcx,rbx
  lea rdx,scrollTestFill
  call FillRect
  add rsp,28h
  ret
ClearScrollTestSurface ENDP
ENDIF
