; Process-name cells use geometry rather than characters to form a hierarchy.
; The native owner-data list retains text, selection, focus and keyboard access.
extern ImageList_DrawEx:proc, RoundRect:proc, SetDCPenColor:proc
extern IntersectClipRect:proc, DrawFocusRect:proc, SetFocus:proc
.code

; EAX logical pixels -> EAX device pixels, rounding to the nearest pixel.
TreeScale PROC
  imul eax,uiDpi
  add eax,48
  xor edx,edx
  mov r11d,96
  div r11d
  ret
TreeScale ENDP

; ECX visible index, RDX output (64 bytes): row RECT; box RECT;
; icon x/y; label RECT. Both painting and hit testing use this exact layout.
ProcessTreeGeometry PROC
  push rbx
  push rsi
  push rdi
  sub rsp,30h
  mov esi,ecx
  mov rbx,rdx
  cmp esi,visibleCount
  jae ptgFail
  lea rax,rowMap
  mov rdi,[rax+rsi*8]
  test rdi,rdi
  jz ptgFail
  mov dword ptr [rbx],0 ; LVIR_BOUNDS
  mov rcx,gList
  mov edx,100Eh
  mov r8d,esi
  mov r9,rbx
  call SendMessageW
  test eax,eax
  jz ptgFail
  mov rcx,gList
  mov edx,101Dh ; first column width
  xor r8d,r8d
  xor r9d,r9d
  call SendMessageW
  add eax,[rbx]
  mov [rbx+8],eax
  mov eax,8
  cmp treeEnabled,0
  je ptgIndent
  mov eax,[rdi+3704]
  cmp eax,24
  jbe ptgDepth
  mov eax,24
ptgDepth:
  imul eax,20
  add eax,8
ptgIndent:
  call TreeScale
  add eax,[rbx]
  mov [rbx+16],eax
  ; Keep deeply nested labels usable if the first column is narrow.
  mov eax,88
  call TreeScale
  mov edx,[rbx+8]
  sub edx,eax
  cmp edx,[rbx]
  jge ptgMaxIndent
  mov edx,[rbx]
ptgMaxIndent:
  cmp [rbx+16],edx
  jle ptgBox
  mov [rbx+16],edx
ptgBox:
  mov eax,14
  call TreeScale
  mov edx,[rbx+16]
  add edx,eax
  mov [rbx+24],edx
  mov edx,[rbx+12]
  sub edx,[rbx+4]
  sub edx,eax
  sar edx,1
  add edx,[rbx+4]
  mov [rbx+20],edx
  add edx,eax
  mov [rbx+28],edx
  mov eax,22
  call TreeScale
  add eax,[rbx+16]
  mov [rbx+32],eax
  mov eax,16
  call TreeScale
  mov edx,[rbx+12]
  sub edx,[rbx+4]
  sub edx,eax
  sar edx,1
  add edx,[rbx+4]
  mov [rbx+36],edx
  mov eax,44
  call TreeScale
  add eax,[rbx+16]
  mov [rbx+40],eax
  mov eax,[rbx+4]
  mov [rbx+44],eax
  mov eax,8
  call TreeScale
  mov edx,[rbx+8]
  sub edx,eax
  mov [rbx+48],edx
  mov eax,[rbx+12]
  mov [rbx+52],eax
  mov eax,1
  jmp ptgDone
ptgFail:
  xor eax,eax
ptgDone:
  add rsp,30h
  pop rdi
  pop rsi
  pop rbx
  ret
ProcessTreeGeometry ENDP

; R9 NMLVCUSTOMDRAW. This suppresses the default first-column icon/text only.
PaintProcessNameCell PROC
  push rbx
  push rsi
  push rdi
  push r12
  push r13
  sub rsp,0C0h
  mov rbx,r9
  mov ecx,[rbx+56]
  cmp ecx,visibleCount
  jae ppncDefault
  lea rax,rowMap
  mov rdi,[rax+rcx*8]
  test rdi,rdi
  jz ppncDefault
  lea rdx,[rsp+50h]
  call ProcessTreeGeometry
  test eax,eax
  jz ppncDefault
  mov rsi,[rbx+32]
  test rsi,rsi
  jz ppncDefault
  mov rcx,rsi
  call SaveDC
  mov [rsp+90h],eax
  mov rcx,rsi
  mov edx,[rsp+50h]
  mov r8d,[rsp+54h]
  mov r9d,[rsp+58h]
  mov eax,[rsp+5Ch]
  mov [rsp+20h],rax
  call IntersectClipRect
  mov r12d,0FFFFFFh
  mov r13d,202020h
  cmp themeOption,0
  je ppncSelection
  mov r12d,202020h
  mov r13d,0EEEEEEh
ppncSelection:
  test dword ptr [rbx+64],1
  jz ppncBackground
  mov r12d,0F4E5D6h
  cmp themeOption,0
  je ppncBackground
  mov r12d,493D30h
ppncBackground:
  mov rcx,rsi
  mov edx,r12d
  call SetDCBrushColor
  mov ecx,18 ; DC_BRUSH
  call GetStockObject
  mov r8,rax
  mov rcx,rsi
  lea rdx,[rsp+50h]
  call FillRect
  cmp treeEnabled,0
  je ppncIcon
  cmp qword ptr [rdi+3696],0
  je ppncIcon
  mov ecx,18
  call GetStockObject
  mov rcx,rsi
  mov rdx,rax
  call SelectObject
  mov ecx,19 ; DC_PEN
  call GetStockObject
  mov rcx,rsi
  mov rdx,rax
  call SelectObject
  mov rcx,rsi
  mov edx,0E3E0DCh
  cmp themeOption,0
  je ppncBoxInk
  mov edx,605C58h
ppncBoxInk:
  call SetDCPenColor
  mov rcx,rsi
  mov edx,[rsp+60h]
  mov r8d,[rsp+64h]
  mov r9d,[rsp+68h]
  mov eax,[rsp+6Ch]
  mov [rsp+20h],rax
  mov qword ptr [rsp+28h],3
  mov qword ptr [rsp+30h],3
  call RoundRect
  ; A true painted +/- mark, centered independently of font metrics.
  mov rcx,rsi
  mov edx,r13d
  call SetDCBrushColor
  mov eax,[rsp+60h]
  add eax,4
  mov [rsp+98h],eax
  mov eax,[rsp+68h]
  sub eax,4
  mov [rsp+0A0h],eax
  mov eax,[rsp+64h]
  add eax,[rsp+6Ch]
  sar eax,1
  mov [rsp+9Ch],eax
  inc eax
  mov [rsp+0A4h],eax
  mov ecx,18
  call GetStockObject
  mov r8,rax
  mov rcx,rsi
  lea rdx,[rsp+98h]
  call FillRect
  cmp dword ptr [rdi+3716],0
  je ppncIcon
  mov eax,[rsp+60h]
  add eax,[rsp+68h]
  sar eax,1
  mov [rsp+98h],eax
  inc eax
  mov [rsp+0A0h],eax
  mov eax,[rsp+64h]
  add eax,4
  mov [rsp+9Ch],eax
  mov eax,[rsp+6Ch]
  sub eax,4
  mov [rsp+0A4h],eax
  mov ecx,18
  call GetStockObject
  mov r8,rax
  mov rcx,rsi
  lea rdx,[rsp+98h]
  call FillRect
ppncIcon:
  mov eax,16
  call TreeScale
  mov [rsp+28h],rax
  mov [rsp+30h],rax
  mov eax,[rsp+74h]
  mov [rsp+20h],rax
  mov qword ptr [rsp+38h],-1 ; CLR_NONE
  mov qword ptr [rsp+40h],-1
  mov qword ptr [rsp+48h],1 ; ILD_TRANSPARENT
  mov rcx,processImageList
  test rcx,rcx
  jz ppncText
  mov edx,[rdi+3880]
  mov r8,rsi
  mov r9d,[rsp+70h]
  call ImageList_DrawEx
ppncText:
  mov rcx,rsi
  mov edx,r13d
  call SetTextColor
  mov rcx,rsi
  mov edx,1
  call SetBkMode
  mov rcx,rsi
  mov rdx,appFont
  call SelectObject
  mov rcx,rdi
  xor edx,edx
  call ProcessDisplayText
  mov rdx,rax
  mov rcx,rsi
  mov r8d,-1
  lea r9,[rsp+78h]
  mov qword ptr [rsp+20h],8824h ; VCENTER | SINGLELINE | ELLIPSIS | NOPREFIX
  call DrawTextW
  test dword ptr [rbx+64],10h ; CDIS_FOCUS
  jz ppncRestore
  mov rcx,rsi
  lea rdx,[rsp+78h]
  call DrawFocusRect
ppncRestore:
  mov rcx,rsi
  mov edx,[rsp+90h]
  call RestoreDC
  mov eax,4 ; CDRF_SKIPDEFAULT
  jmp ppncDone
ppncDefault:
  xor eax,eax
ppncDone:
  add rsp,0C0h
  pop r13
  pop r12
  pop rdi
  pop rsi
  pop rbx
  ret
PaintProcessNameCell ENDP

; ECX visible row. Native selection determines every action target.
SelectProcessTreeRow PROC
  push rbx
  sub rsp,40h
  mov ebx,ecx
  mov dword ptr [rsp+2Ch],3
  mov dword ptr [rsp+30h],3
  mov rcx,gList
  mov edx,102Bh
  mov r8d,ebx
  lea r9,[rsp+20h]
  call SendMessageW
  mov rcx,gList
  call SetFocus
  mov rcx,gList
  mov edx,1013h ; LVM_ENSUREVISIBLE
  mov r8d,ebx
  xor r9d,r9d
  call SendMessageW
  add rsp,40h
  pop rbx
  ret
SelectProcessTreeRow ENDP

; RCX mouse LPARAM, EDX=1 ignores second click of a double click in the box.
ProcessTreeMouse PROC
  push rbx
  push rsi
  push rdi
  sub rsp,80h
  mov edi,edx
  movsx eax,cx
  mov [rsp+20h],eax
  sar ecx,16
  mov [rsp+24h],ecx
  mov rcx,gList
  mov edx,1039h ; LVM_SUBITEMHITTEST
  xor r8d,r8d
  lea r9,[rsp+20h]
  call SendMessageW
  cmp eax,visibleCount
  jae ptmNo
  cmp dword ptr [rsp+30h],0 ; iSubItem
  jne ptmNo
  mov ebx,eax
  lea rcx,rowMap
  mov rsi,[rcx+rbx*8]
  cmp qword ptr [rsi+3696],0
  je ptmNo
  mov ecx,ebx
  lea rdx,[rsp+40h]
  call ProcessTreeGeometry
  test eax,eax
  jz ptmNo
  mov eax,[rsp+20h]
  cmp eax,[rsp+50h]
  jl ptmNo
  cmp eax,[rsp+58h]
  jge ptmNo
  mov eax,[rsp+24h]
  cmp eax,[rsp+54h]
  jl ptmNo
  cmp eax,[rsp+5Ch]
  jge ptmNo
  test edi,edi
  jnz ptmYes
  mov ecx,ebx
  call SelectProcessTreeRow
  call ToggleTreeNode
ptmYes:
  mov eax,1
  jmp ptmDone
ptmNo:
  xor eax,eax
ptmDone:
  add rsp,80h
  pop rdi
  pop rsi
  pop rbx
  ret
ProcessTreeMouse ENDP

; Conventional tree keyboard navigation. Right opens/moves to first child;
; Left collapses/moves to the parent without triggering process actions.
ProcessTreeKey PROC
  push rbx
  push rsi
  push rdi
  sub rsp,20h
  mov edi,ecx
  cmp edi,25h
  je ptkSelected
  cmp edi,27h
  jne ptkNo
ptkSelected:
  mov rcx,gList
  mov edx,100Ch
  mov r8,-1
  mov r9d,2
  call SendMessageW
  cmp eax,visibleCount
  jae ptkYes
  mov ebx,eax
  lea rcx,rowMap
  mov rsi,[rcx+rbx*8]
  cmp edi,25h
  je ptkLeft
  cmp qword ptr [rsi+3696],0
  je ptkYes
  cmp dword ptr [rsi+3716],0
  jne ptkToggle
  inc ebx
  cmp ebx,visibleCount
  jae ptkYes
  lea rax,rowMap
  mov rax,[rax+rbx*8]
  cmp [rax+3680],rsi ; a filtered next root is not this node's child
  jne ptkYes
  mov ecx,ebx
  call SelectProcessTreeRow
  jmp ptkYes
ptkLeft:
  cmp qword ptr [rsi+3696],0
  je ptkParent
  cmp dword ptr [rsi+3716],0
  je ptkToggle
ptkParent:
  mov rsi,[rsi+3680]
  test rsi,rsi
  jz ptkYes
  xor ecx,ecx
  lea rdx,rowMap
ptkFind:
  cmp ecx,visibleCount
  jae ptkYes
  cmp [rdx+rcx*8],rsi
  je ptkSelect
  inc ecx
  jmp ptkFind
ptkSelect:
  call SelectProcessTreeRow
  jmp ptkYes
ptkToggle:
  call ToggleTreeNode
ptkYes:
  mov eax,1
  jmp ptkDone
ptkNo:
  xor eax,eax
ptkDone:
  add rsp,20h
  pop rdi
  pop rsi
  pop rbx
  ret
ProcessTreeKey ENDP
