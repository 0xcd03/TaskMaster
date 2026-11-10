; Geometry/font regression for the hidden test window at minimum dimensions.
; No desktop input, screenshots or user settings are touched.
extern GetParent:proc
.data
uiReportAssertions dd 0
uiReportFailure dd 0
uiReportDpis dd 96,144,192
.data?
uiReportSaved db 48 dup(?)
uiReportRects db 96 dup(?) ; list, chart, panel, summary, button, report edit
.code
UiReportLayoutTests PROC
  push rbx
  push rsi
  push rdi
  push r12
  push r13
  sub rsp,40h
  mov uiReportFailure,160
  mov eax,uiDpi
  mov dword ptr [uiReportSaved],eax
  mov eax,frontMode
  mov dword ptr [uiReportSaved+4],eax
  mov eax,reportAvailable
  mov dword ptr [uiReportSaved+8],eax
  mov eax,reportExpanded
  mov dword ptr [uiReportSaved+12],eax
  mov rcx,gMain
  lea rdx,uiReportSaved+16
  call GetWindowRect
  xor esi,esi
urltDpi:
  lea rax,uiReportDpis
  mov ecx,[rax+rsi*4]
  call UpdateUiDpi
  mov rcx,gMain
  mov edx,dword ptr [uiReportSaved+16]
  mov r8d,dword ptr [uiReportSaved+20]
  mov r9d,uiMinWidth
  mov eax,uiMinHeight
  mov [rsp+20h],rax
  mov qword ptr [rsp+28h],1
  call MoveWindow
  mov reportAvailable,1
  mov reportExpanded,1
  mov frontMode,4
  call SetModeColumns
  call LayoutContent
  mov rcx,gList
  lea rdx,uiReportRects
  call GetWindowRect
  mov rcx,chartWnd
  lea rdx,uiReportRects+16
  call GetWindowRect
  mov rcx,reportWnd
  lea rdx,uiReportRects+32
  call GetWindowRect
  mov rcx,actionBanner
  lea rdx,uiReportRects+48
  call GetWindowRect
  mov rcx,actionBannerDetails
  lea rdx,uiReportRects+64
  call GetWindowRect
  mov rcx,actionDetailsText
  lea rdx,uiReportRects+80
  call GetWindowRect
  mov eax,dword ptr [uiReportRects+12] ; list.bottom <= chart.top
  cmp eax,dword ptr [uiReportRects+20]
  jg urltCleanup
  mov eax,dword ptr [uiReportRects+28] ; chart.bottom <= report.top
  cmp eax,dword ptr [uiReportRects+36]
  jg urltCleanup
  mov eax,dword ptr [uiReportRects+8]
  cmp eax,dword ptr [uiReportRects+24]
  jne urltCleanup
  cmp eax,dword ptr [uiReportRects+40]
  jne urltCleanup
  inc uiReportAssertions
  lea ebx,[rsi+161]
  mov uiReportFailure,ebx
  ; All three children stay inside the panel with positive size.
  lea rbx,uiReportRects+48
  mov edi,3
urltChild:
  mov eax,[rbx]
  cmp eax,dword ptr [uiReportRects+32]
  jl urltCleanup
  mov eax,[rbx+4]
  cmp eax,dword ptr [uiReportRects+36]
  jl urltCleanup
  mov eax,[rbx+8]
  cmp eax,dword ptr [uiReportRects+40]
  jg urltCleanup
  cmp eax,[rbx]
  jle urltCleanup
  mov eax,[rbx+12]
  cmp eax,dword ptr [uiReportRects+44]
  jg urltCleanup
  cmp eax,[rbx+4]
  jle urltCleanup
  add rbx,16
  dec edi
  jnz urltChild
  mov eax,dword ptr [uiReportRects+60] ; summary.bottom <= edit.top
  cmp eax,dword ptr [uiReportRects+84]
  jg urltCleanup
  mov eax,dword ptr [uiReportRects+76] ; button.bottom <= edit.top
  cmp eax,dword ptr [uiReportRects+84]
  jg urltCleanup
  mov eax,dword ptr [uiReportRects+56] ; summary.right <= button.left
  cmp eax,dword ptr [uiReportRects+64]
  jg urltCleanup
  inc uiReportAssertions
  mov uiReportFailure,165
  xor edi,edi
urltFont:
  mov r12,actionBanner
  test edi,edi
  jz urltFontHandle
  mov r12,actionBannerDetails
  cmp edi,1
  je urltFontHandle
  mov r12,actionDetailsText
urltFontHandle:
  mov rcx,r12
  call GetParent
  cmp rax,reportWnd
  jne urltCleanup
  mov rcx,r12
  mov edx,31h ; WM_GETFONT
  xor r8d,r8d
  xor r9d,r9d
  call SendMessageW
  cmp rax,appFont
  jne urltCleanup
  inc edi
  cmp edi,3
  jb urltFont
  inc uiReportAssertions
  ; The root-client bottom margin must contain the complete panel.
  mov rcx,gMain
  lea rdx,[rsp+30h]
  call GetClientRect
  mov rcx,gMain
  lea rdx,[rsp+38h] ; client bottom/right POINT -> screen
  call ClientToScreen
  mov eax,dword ptr [uiReportRects+44]
  cmp eax,[rsp+3Ch]
  jge urltCleanup
  inc uiReportAssertions
  inc esi
  cmp esi,3
  jb urltDpi
  mov uiReportFailure,0
urltCleanup:
  mov ecx,dword ptr [uiReportSaved]
  call UpdateUiDpi
  mov eax,dword ptr [uiReportSaved+4]
  mov frontMode,eax
  mov eax,dword ptr [uiReportSaved+8]
  mov reportAvailable,eax
  mov eax,dword ptr [uiReportSaved+12]
  mov reportExpanded,eax
  call SetModeColumns
  mov rcx,gMain
  mov edx,dword ptr [uiReportSaved+16]
  mov r8d,dword ptr [uiReportSaved+20]
  mov r9d,dword ptr [uiReportSaved+24]
  sub r9d,edx
  mov eax,dword ptr [uiReportSaved+28]
  sub eax,r8d
  mov [rsp+20h],rax
  mov qword ptr [rsp+28h],1
  call MoveWindow
  call LayoutContent
  mov eax,uiReportFailure
  add rsp,40h
  pop r13
  pop r12
  pop rdi
  pop rsi
  pop rbx
  ret
UiReportLayoutTests ENDP
