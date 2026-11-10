; Hidden native-control regression: disposable three-level process tree only.
; It renders to a memory DC and sends input to its own hidden ListView.
.data
uiTreeAssertions dd 0
uiTreeFailure dd 0
.data?
uiTreeGeometry db 64 dup(?)
uiTreeDraw db 160 dup(?)
.code
UiTreeControlRegression PROC
  push rbx
  push rsi
  push rdi
  push r12
  push r13
  sub rsp,50h
  mov uiTreeFailure,150
  mov eax,uiDpi
  mov [rsp+40h],eax
  mov eax,themeOption
  mov [rsp+44h],eax
  mov frontMode,0
  mov treeEnabled,1
  mov frontCount,3
  mov collapsedUsed,0
  mov sortColumn,0
  mov sortDescending,0
  mov rememberId,-1
  mov rememberItem,-1
  mov word ptr searchBuffer,0
  mov rbx,frontBuffer
  xor esi,esi
utcrFixture:
  mov rcx,rbx
  call ClearRow
  mov rcx,rbx
  lea rdx,presentationName
  call lstrcpyW
  lea eax,[rsi+10000]
  mov [rbx+3584],eax
  test esi,esi
  jz utcrRoot
  dec eax
  mov [rbx+3588],eax
utcrRoot:
  lea eax,[rsi+1]
  mov [rbx+3600],rax
  mov eax,processDefaultIcon
  mov [rbx+3880],eax
  add rbx,4096
  inc esi
  cmp esi,3
  jb utcrFixture
  call SetModeColumns
  call BuildRowMap
  cmp visibleCount,3
  jne utcrCleanup
  mov rcx,gList
  mov edx,1013h
  xor r8d,r8d
  xor r9d,r9d
  call SendMessageW
  xor ecx,ecx
  lea rdx,uiTreeGeometry
  call ProcessTreeGeometry
  test eax,eax
  jz utcrCleanup
  ; Click the center of the independently painted root disclosure box.
  mov eax,dword ptr [uiTreeGeometry+16]
  add eax,dword ptr [uiTreeGeometry+24]
  sar eax,1
  mov r9d,eax
  mov eax,dword ptr [uiTreeGeometry+20]
  add eax,dword ptr [uiTreeGeometry+28]
  sar eax,1
  shl eax,16
  or r9d,eax
  mov rcx,gList
  mov edx,201h
  mov r8d,1
  call SendMessageW
  cmp visibleCount,1
  jne utcrCleanup
  mov rbx,frontBuffer
  cmp dword ptr [rbx+3716],1
  jne utcrCleanup
  inc uiTreeAssertions
  mov uiTreeFailure,151
  ; Right opens the branch; another Right selects its direct child.
  mov rcx,gList
  mov edx,100h
  mov r8d,27h
  xor r9d,r9d
  call SendMessageW
  cmp visibleCount,3
  jne utcrCleanup
  cmp dword ptr [rbx+3716],0
  jne utcrCleanup
  inc uiTreeAssertions
  mov uiTreeFailure,152
  mov rcx,gList
  mov edx,100h
  mov r8d,27h
  xor r9d,r9d
  call SendMessageW
  mov rcx,gList
  mov edx,100Ch
  mov r8,-1
  mov r9d,2
  call SendMessageW
  cmp eax,1
  jne utcrCleanup
  inc uiTreeAssertions
  mov uiTreeFailure,153
  mov rcx,gList
  mov edx,100h
  mov r8d,25h
  xor r9d,r9d
  call SendMessageW
  cmp visibleCount,2 ; child collapsed, root remains open
  jne utcrCleanup
  inc uiTreeAssertions
  mov uiTreeFailure,154
  mov rcx,gList
  mov edx,100h
  mov r8d,25h
  xor r9d,r9d
  call SendMessageW
  mov rcx,gList
  mov edx,100Ch
  mov r8,-1
  mov r9d,2
  call SendMessageW
  test eax,eax ; collapsed child Left moves to parent
  jnz utcrCleanup
  inc uiTreeAssertions
  mov uiTreeFailure,155
  ; Offscreen paint checks use the real native row geometry and painter.
  xor ecx,ecx
  call CreateCompatibleDC
  mov r12,rax
  test rax,rax
  jz utcrCleanup
  mov rcx,gMain
  call GetDC
  mov r13,rax
  mov rcx,rax
  mov edx,2048
  mov r8d,256
  call CreateCompatibleBitmap
  mov [rsp+30h],rax
  mov rcx,gMain
  mov rdx,r13
  call ReleaseDC
  cmp qword ptr [rsp+30h],0
  je utcrDeleteDc
  mov rcx,r12
  mov rdx,[rsp+30h]
  call SelectObject
  mov [rsp+38h],rax
  xor esi,esi
utcrTheme:
  lea eax,[rsi+156]
  mov uiTreeFailure,eax
  mov themeOption,esi
  lea rdi,uiTreeDraw
  xor eax,eax
  mov ecx,20
  rep stosq
  mov qword ptr [uiTreeDraw+32],r12
  mov dword ptr [uiTreeDraw+24],30001h
  lea r9,uiTreeDraw
  call ProcessListCustomDraw
  cmp eax,4
  jne utcrPaintFail
  xor ecx,ecx
  lea rdx,uiTreeGeometry
  call ProcessTreeGeometry
  ; The center of an open disclosure is a painted minus, not text glyphs.
  mov edx,dword ptr [uiTreeGeometry+16]
  add edx,dword ptr [uiTreeGeometry+24]
  sar edx,1
  mov r8d,dword ptr [uiTreeGeometry+20]
  add r8d,dword ptr [uiTreeGeometry+28]
  sar r8d,1
  mov rcx,r12
  call GetPixel
  mov edx,202020h
  test esi,esi
  jz utcrInk
  mov edx,0EEEEEEh
utcrInk:
  cmp eax,edx
  jne utcrPaintFail
  inc uiTreeAssertions
  inc esi
  cmp esi,2
  jb utcrTheme
  mov uiTreeFailure,158
  ; Selection remains a visible filled row even though name drawing is custom.
  mov dword ptr [uiTreeDraw+64],1
  lea r9,uiTreeDraw
  call ProcessListCustomDraw
  cmp eax,4
  jne utcrPaintFail
  mov rcx,r12
  mov edx,dword ptr [uiTreeGeometry]
  add edx,2
  mov r8d,dword ptr [uiTreeGeometry+4]
  add r8d,2
  call GetPixel
  cmp eax,493D30h
  jne utcrPaintFail
  inc uiTreeAssertions
  mov uiTreeFailure,159
  ; Device geometry at 200%: each nested level must add 40 pixels.
  mov uiDpi,192
  xor ecx,ecx
  lea rdx,uiTreeGeometry
  call ProcessTreeGeometry
  mov ebx,dword ptr [uiTreeGeometry+16]
  mov ecx,1
  lea rdx,uiTreeGeometry
  call ProcessTreeGeometry
  mov eax,dword ptr [uiTreeGeometry+16]
  sub eax,ebx
  cmp eax,40
  jne utcrPaintFail
  inc uiTreeAssertions
  mov uiTreeFailure,0
utcrPaintFail:
  mov rcx,r12
  mov rdx,[rsp+38h]
  call SelectObject
  mov rcx,[rsp+30h]
  call DeleteObject
utcrDeleteDc:
  mov rcx,r12
  call DeleteDC
utcrCleanup:
  mov eax,[rsp+40h]
  mov uiDpi,eax
  mov eax,[rsp+44h]
  mov themeOption,eax
  mov frontCount,0
  mov visibleCount,0
  mov collapsedUsed,0
  mov publishedListCount,0
  mov rcx,gList
  mov edx,102Fh
  xor r8d,r8d
  xor r9d,r9d
  call SendMessageW
  mov eax,uiTreeFailure
  add rsp,50h
  pop r13
  pop r12
  pop rdi
  pop rsi
  pop rbx
  ret
UiTreeControlRegression ENDP
