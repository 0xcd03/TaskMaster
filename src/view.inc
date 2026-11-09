extern StrStrIW:proc, StrCmpLogicalW:proc, StrToInt64ExW:proc
.data
align 8
.data?
align 8
rowMap dq 8192 dup(?)
.data
visibleCount dd 0
sortColumn dd 0
sortDescending dd 0
searchWnd dq 0
.data?
align 8
searchBuffer dw 256 dup(?)
.data
editClass dw 'E','D','I','T',0
searchHint dw 'S','e','a','r','c','h',' ','n','a','m','e',' ','o','r',' ','P','I','D',0
rememberId dd -1
rememberBirth dq 0
rememberMode dd -1
.data?
align 8
restoreItem db 88 dup(?)
.data
.code
CreateSearch PROC
  sub rsp,68h
  mov ecx,200h
  lea rdx,editClass
  xor r8d,r8d
  mov r9d,50810080h
  mov qword ptr [rsp+20h],200
  mov qword ptr [rsp+28h],12
  mov qword ptr [rsp+30h],400
  mov qword ptr [rsp+38h],28
  mov rax,gMain
  mov [rsp+40h],rax
  mov qword ptr [rsp+48h],9001
  mov rax,gInst
  mov [rsp+50h],rax
  mov qword ptr [rsp+58h],0
  call CreateWindowExW
  mov searchWnd,rax
  mov rcx,rax
  mov edx,1501h
  mov r8d,1
  lea r9,searchHint
  call SendMessageW
  add rsp,68h
  ret
CreateSearch ENDP

RememberSelection PROC
  sub rsp,28h
  mov rememberId,-1
  mov eax,frontMode
  mov rememberMode,eax
  mov rcx,gList
  mov edx,100Ch
  mov r8,-1
  mov r9d,2
  call SendMessageW
  cmp eax,visibleCount
  jae remDone
  lea rdx,rowMap
  mov rax,[rdx+rax*8]
  mov edx,[rax+3584]
  mov rememberId,edx
  mov rax,[rax+3600]
  mov rememberBirth,rax
remDone:
  add rsp,28h
  ret
RememberSelection ENDP

BuildRowMap PROC
  push rbx
  push rsi
  push rdi
  push r12
  push r13
  sub rsp,30h
  mov rcx,searchWnd
  lea rdx,searchBuffer
  mov r8d,256
  call GetWindowTextW
  xor esi,esi
  mov rdi,frontBuffer
  mov visibleCount,0
  lea rbx,rowMap
brmNext:
  cmp esi,frontCount
  jae brmSort
  cmp frontMode,0
  jne brmFilter
  cmp treeEnabled,0
  jne brmAdd
brmFilter:
  cmp word ptr [searchBuffer],0
  je brmAdd
  mov rcx,rdi
  lea rdx,searchBuffer
  call StrStrIW
  test rax,rax
  jnz brmAdd
  lea rcx,[rdi+512]
  lea rdx,searchBuffer
  call StrStrIW
  test rax,rax
  jz brmAdvance
brmAdd:
  mov eax,visibleCount
  mov [rbx+rax*8],rdi
  inc visibleCount
brmAdvance:
  inc esi
  add rdi,4096
  jmp brmNext
brmSort:
  mov r12d,visibleCount
  shr r12d,1
brmGap:
  test r12d,r12d
  jz brmPublish
  mov esi,r12d
brmOuter:
  cmp esi,visibleCount
  jae brmNextGap
  mov rdi,[rbx+rsi*8]
  mov r13d,esi
brmInner:
  cmp r13d,r12d
  jb brmPlace
  mov eax,r13d
  sub eax,r12d
  mov rcx,[rbx+rax*8]
  mov rdx,rdi
  call CompareRows
  test eax,eax
  jle brmPlace
  mov eax,r13d
  sub eax,r12d
  mov rax,[rbx+rax*8]
  mov [rbx+r13*8],rax
  sub r13d,r12d
  jmp brmInner
brmPlace:
  mov [rbx+r13*8],rdi
  inc esi
  jmp brmOuter
brmNextGap:
  shr r12d,1
  jmp brmGap
brmPublish:
  cmp frontMode,0
  jne brmFlat
  cmp treeEnabled,0
  je brmFlat
  call BuildProcessTree
brmFlat:
  mov dword ptr [restoreItem+12],0
  mov dword ptr [restoreItem+16],3
  mov rcx,gList
  mov edx,102Bh
  mov r8,-1
  lea r9,restoreItem
  call SendMessageW
  mov rcx,gList
  mov edx,102Fh
  mov r8d,visibleCount
  mov r9d,3
  call SendMessageW
  xor esi,esi
  mov eax,frontMode
  cmp eax,rememberMode
  jne brmDone
  cmp eax,1
  ja brmDone
brmRestore:
  cmp esi,visibleCount
  jae brmDone
  mov rax,[rbx+rsi*8]
  mov edx,[rax+3584]
  cmp edx,rememberId
  jne brmRestoreNext
  mov rax,[rax+3600]
  cmp rax,rememberBirth
  jne brmRestoreNext
  mov dword ptr [restoreItem+12],3
  mov rcx,gList
  mov edx,102Bh
  mov r8d,esi
  lea r9,restoreItem
  call SendMessageW
  jmp brmDone
brmRestoreNext:
  inc esi
  jmp brmRestore
brmDone:
  mov rcx,gList
  xor edx,edx
  xor r8d,r8d
  call InvalidateRect
  add rsp,30h
  pop r13
  pop r12
  pop rdi
  pop rsi
  pop rbx
  ret
BuildRowMap ENDP

CompareRows PROC
  sub rsp,28h
  mov eax,sortColumn
  shl eax,9
  add rcx,rax
  add rdx,rax
  call StrCmpLogicalW
  cmp sortDescending,0
  je cmpDone
  neg eax
cmpDone:
  add rsp,28h
  ret
CompareRows ENDP

