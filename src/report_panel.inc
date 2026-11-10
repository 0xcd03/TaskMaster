.data
reportWnd dq 0
reportClassName dw 'T','m','x','R','e','p','o','r','t',0
.data?
reportClass WNDCLASSEXW <>
.code
CreateReportPanel PROC
  sub rsp,68h
  mov reportClass.wcSize,SIZEOF WNDCLASSEXW
  lea rax,ReportWndProc
  mov reportClass.wcProc,rax
  mov rax,gInst
  mov reportClass.wcInst,rax
  lea rax,reportClassName
  mov reportClass.wcName,rax
  lea rcx,reportClass
  call RegisterClassExW
  xor ecx,ecx
  lea rdx,reportClassName
  xor r8d,r8d
  mov r9d,42000000h ; hidden child, CLIPCHILDREN
  mov qword ptr [rsp+20h],0
  mov qword ptr [rsp+28h],0
  mov qword ptr [rsp+30h],400
  mov qword ptr [rsp+38h],60
  mov rax,gMain
  mov [rsp+40h],rax
  mov qword ptr [rsp+48h],2603
  mov rax,gInst
  mov [rsp+50h],rax
  mov qword ptr [rsp+58h],0
  call CreateWindowExW
  mov reportWnd,rax
  add rsp,68h
  ret
CreateReportPanel ENDP

ReportWndProc PROC
  sub rsp,88h
  cmp edx,111h
  je rwpForward
  cmp edx,2Bh
  je rwpForward
  cmp edx,138h
  je rwpForward
  cmp edx,133h
  je rwpForward
  cmp edx,0Fh
  je rwpPaint
  cmp edx,14h
  je rwpErase
  call DefWindowProcW
  jmp rwpDone
rwpForward:
  mov rcx,gMain
  call SendMessageW
  jmp rwpDone
rwpPaint:
  mov [rsp+78h],rcx
  lea rdx,[rsp+30h]
  call BeginPaint
  mov [rsp+80h],rax
  mov rcx,[rsp+78h]
  lea rdx,[rsp+20h]
  call GetClientRect
  mov rcx,[rsp+80h]
  lea rdx,[rsp+20h]
  mov r8,shellCardBrush
  call FillRect
  mov rcx,[rsp+80h]
  lea rdx,[rsp+20h]
  mov r8,menuThemeLineBrush
  call FrameRect
  mov rcx,[rsp+78h]
  lea rdx,[rsp+30h]
  call EndPaint
rwpErase:
  mov eax,1
rwpDone:
  add rsp,88h
  ret
ReportWndProc ENDP

LayoutReportChildren PROC
  sub rsp,48h
  mov rcx,reportWnd
  lea rdx,[rsp+30h]
  call GetClientRect
  mov eax,uiInset
  imul eax,10
  mov [rsp+40h],eax ; button width scales with DPI
  mov rcx,actionBanner
  mov edx,uiInset
  mov r8d,uiInset
  mov r9d,[rsp+38h]
  sub r9d,eax
  mov eax,uiInset
  imul eax,3
  sub r9d,eax
  mov eax,uiStatusHeight
  mov [rsp+20h],rax
  mov qword ptr [rsp+28h],1
  call MoveWindow
  mov rcx,actionBannerDetails
  mov edx,[rsp+38h]
  sub edx,uiInset
  sub edx,[rsp+40h]
  mov r8d,uiInset
  mov r9d,[rsp+40h]
  mov eax,uiStatusHeight
  mov [rsp+20h],rax
  mov qword ptr [rsp+28h],1
  call MoveWindow
  mov rcx,actionDetailsText
  mov edx,uiInset
  mov r8d,uiStatusHeight
  add r8d,uiInset
  mov r9d,[rsp+38h]
  mov eax,uiInset
  add eax,eax
  sub r9d,eax
  mov eax,[rsp+3Ch]
  sub eax,r8d
  sub eax,uiInset
  cmp eax,1
  jge lrcHeight
  mov eax,1
lrcHeight:
  mov [rsp+20h],rax
  mov qword ptr [rsp+28h],1
  call MoveWindow
  add rsp,48h
  ret
LayoutReportChildren ENDP
