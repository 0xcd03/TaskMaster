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
rememberItem dd -1
publishedListCount dd -1
.data?
align 8
restoreItem db 88 dup(?)
.data
.code
CreateSearch PROC
  sub rsp,68h
  xor ecx,ecx ; one flat native border, no light 3-D client edge
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
  mov rememberItem,-1
  mov eax,frontMode
  mov rememberMode,eax
  mov rcx,gList
  mov edx,100Ch
  mov r8,-1
  mov r9d,2
  call SendMessageW
  cmp eax,visibleCount
  jae remDone
  mov rememberItem,eax
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
  ; Process trees sort sibling lists after their aggregate metrics exist.
  ; Fresh snapshot rows do not yet contain those aggregate values, and a
  ; preliminary flat sort only repeats work without defining tree order.
  cmp frontMode,0
  jne brmSortFlat
  cmp treeEnabled,0
  jne brmPublish
brmSortFlat:
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
  ; Find the same identity before touching native selection. Clearing every
  ; owner-data row each tick floods selection/UIA notifications and redraws.
  mov r12d,-1
  xor esi,esi
  mov eax,frontMode
  cmp eax,rememberMode
  jne brmUpdateCount
  cmp eax,1
  ja brmUpdateCount
brmRestore:
  cmp esi,visibleCount
  jae brmUpdateCount
  mov rax,[rbx+rsi*8]
  mov edx,[rax+3584]
  cmp edx,rememberId
  jne brmRestoreNext
  mov rax,[rax+3600]
  cmp rax,rememberBirth
  jne brmRestoreNext
  mov r12d,esi
  jmp brmUpdateCount
brmRestoreNext:
  inc esi
  jmp brmRestore
brmUpdateCount:
  ; Stable snapshots retain count and scroll. Only structural changes need
  ; LVM_SETITEMCOUNTEX; its no-invalidate/no-scroll flags preserve viewport.
  mov eax,visibleCount
  cmp eax,publishedListCount
  je brmUpdateSelection
  mov publishedListCount,eax
  mov rcx,gList
  mov edx,102Fh
  mov r8d,eax
  mov r9d,3
  call SendMessageW
brmUpdateSelection:
  cmp r12d,rememberItem
  je brmDone
  mov dword ptr [restoreItem+16],3
  mov dword ptr [restoreItem+12],0
  mov r8d,rememberItem
  cmp r8d,-1
  je brmSelectNew
  cmp r8d,visibleCount
  jae brmSelectNew
  mov rcx,gList
  mov edx,102Bh
  lea r9,restoreItem
  call SendMessageW
brmSelectNew:
  cmp r12d,-1
  je brmDone
  mov dword ptr [restoreItem+12],3
  mov rcx,gList
  mov edx,102Bh
  mov r8d,r12d
  lea r9,restoreItem
  call SendMessageW
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
  cmp frontMode,0
  jne cmpThread
  call CompareProcessRows
  jmp cmpDirection
cmpThread:
  cmp frontMode,1
  jne cmpLegacy
  cmp sortColumn,6
  jne cmpLegacy
  call CompareThreadCpuRows
  jmp cmpDirection
cmpLegacy:
  mov eax,sortColumn
  shl eax,9
  add rcx,rax
  add rdx,rax
  call StrCmpLogicalW
cmpDirection:
  cmp sortDescending,0
  je cmpDone
  neg eax
cmpDone:
  add rsp,28h
  ret
CompareRows ENDP

; CPU column uses validity and numeric tenths, independent of RU/EN text.
; Equal CPU values have stable full thread/owner identity tie-breaks.
CompareThreadCpuRows PROC
  xor r8d,r8d
  xor r9d,r9d
  cmp dword ptr [rcx+3672],0
  setne r8b
  cmp dword ptr [rdx+3672],0
  setne r9b
  cmp r8d,r9d
  ja ctcrLess
  jb ctcrGreater
  test r8d,r8d
  jz ctcrIdentity
  mov eax,[rcx+3900]
  cmp eax,[rdx+3900]
  jb ctcrLess
  ja ctcrGreater
ctcrIdentity:
  mov eax,[rcx+3584]
  cmp eax,[rdx+3584]
  jb ctcrLess
  ja ctcrGreater
  mov rax,[rcx+3600]
  cmp rax,[rdx+3600]
  jb ctcrLess
  ja ctcrGreater
  mov eax,[rcx+3588]
  cmp eax,[rdx+3588]
  jb ctcrLess
  ja ctcrGreater
  mov rax,[rcx+3616]
  cmp rax,[rdx+3616]
  jb ctcrLess
  ja ctcrGreater
  xor eax,eax
  ret
ctcrLess:
  mov eax,-1
  ret
ctcrGreater:
  mov eax,1
  ret
CompareThreadCpuRows ENDP

