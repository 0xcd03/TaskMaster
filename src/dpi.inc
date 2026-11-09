extern GetDpiForSystem:proc, GetDpiForWindow:proc, SetWindowPos:proc, DeleteObject:proc
.data
uiDpi dd 96
uiBase dd 12,16,32,42,46,60,76,196,224,240,294,300,310,132,1050,700,56,24,30,18,80
uiInset dd 12
uiFontHeight dd 16
uiSearchHeight dd 28
uiButtonHeight dd 40
uiButtonStep dd 48
uiListTop dd 52
uiBottomSpace dd 68
uiSidebarWidth dd 172
uiContentLeft dd 200
uiContentSpace dd 216
uiChartHeight dd 294
uiChartSpace dd 300
uiChartBottom dd 310
uiChartMinTop dd 132
uiMinWidth dd 900
uiMinHeight dd 650
uiGraphRow dd 56
uiGraphTop dd 24
uiGraphHeight dd 30
uiGraphLabel dd 18
uiMinListHeight dd 80
.code
RecalcUiScale PROC
  xor r9d,r9d
  lea r10,uiBase
  lea r11,uiInset
rusNext:
  mov eax,[r10+r9*4]
  imul eax,uiDpi
  add eax,48
  xor edx,edx
  mov r8d,96
  div r8d
  mov [r11+r9*4],eax
  inc r9d
  cmp r9d,21
  jb rusNext
  ret
RecalcUiScale ENDP

UpdateUiDpi PROC
  push rbx
  push rsi
  sub rsp,88h
  cmp ecx,48
  jb uudDone
  cmp ecx,768
  ja uudDone
  mov uiDpi,ecx
  call RecalcUiScale
  mov rbx,appFont
  mov ecx,uiFontHeight
  neg ecx
  xor edx,edx
  xor r8d,r8d
  xor r9d,r9d
  mov qword ptr [rsp+20h],400
  mov qword ptr [rsp+28h],0
  mov qword ptr [rsp+30h],0
  mov qword ptr [rsp+38h],0
  mov qword ptr [rsp+40h],1
  mov qword ptr [rsp+48h],0
  mov qword ptr [rsp+50h],0
  mov qword ptr [rsp+58h],5
  mov qword ptr [rsp+60h],0
  lea rax,fontFace
  mov [rsp+68h],rax
  call CreateFontW
  test rax,rax
  jz uudDone
  mov appFont,rax
  xor esi,esi
uudButton:
  lea rax,navButtons
  mov rcx,[rax+rsi*8]
  test rcx,rcx
  jz uudNext
  mov edx,30h
  mov r8,appFont
  mov r9d,1
  call SendMessageW
uudNext:
  inc esi
  cmp esi,6
  jb uudButton
  mov rcx,gList
  test rcx,rcx
  jz uudSearch
  mov edx,30h
  mov r8,appFont
  mov r9d,1
  call SendMessageW
  call SetModeColumns
uudSearch:
  mov rcx,searchWnd
  test rcx,rcx
  jz uudChart
  mov edx,30h
  mov r8,appFont
  mov r9d,1
  call SendMessageW
uudChart:
  cmp shellWnd,0
  je uudChartFonts
  call UpdateShellFonts
uudChartFonts:
  cmp dwriteFactory,0
  je uudDelete
  call CreateChartFormat
  call DropChartTarget
uudDelete:
  mov rcx,rbx
  call DeleteObject
uudDone:
  add rsp,88h
  pop rsi
  pop rbx
  ret
UpdateUiDpi ENDP

LayoutSidebar PROC
  push rbx
  sub rsp,30h
  mov rcx,shellWnd
  xor edx,edx
  xor r8d,r8d
  mov r9d,uiContentLeft
  sub r9d,uiInset
  mov eax,gRect.rcBottom
  mov [rsp+20h],rax
  mov qword ptr [rsp+28h],1
  call MoveWindow
  xor ebx,ebx
lbsNext:
  lea rax,navButtons
  mov rcx,[rax+rbx*8]
  mov edx,uiInset
  mov r8d,ebx
  imul r8d,uiButtonStep
  add r8d,uiListTop
  mov r9d,uiSidebarWidth
  mov eax,uiButtonHeight
  mov [rsp+20h],rax
  mov qword ptr [rsp+28h],1
  call MoveWindow
  inc ebx
  cmp ebx,6
  jb lbsNext
  call LayoutMemoryButton
  mov rcx,shellWnd
  xor edx,edx
  mov r8d,1
  call InvalidateRect
  add rsp,30h
  pop rbx
  ret
LayoutSidebar ENDP

CreateChartFormat PROC
  sub rsp,58h
  mov rcx,chartFormat
  call ReleaseCom
  mov chartFormat,0
  mov rcx,dwriteFactory
  test rcx,rcx
  jz ccfDone
  lea rdx,fontFace
  xor r8d,r8d
  mov r9d,400
  mov qword ptr [rsp+20h],0
  mov qword ptr [rsp+28h],5
  cvtsi2ss xmm0,uiInset
  movss dword ptr [rsp+30h],xmm0
  lea rax,chartLocale
  mov [rsp+38h],rax
  lea rax,chartFormat
  mov [rsp+40h],rax
  mov rax,[rcx]
  call qword ptr [rax+120]
ccfDone:
  add rsp,58h
  ret
CreateChartFormat ENDP
