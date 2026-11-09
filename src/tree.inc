; UI-owned process topology. Parent birth must precede child birth (PID reuse).
; Every OS process remains an actionable row. Aggregates are sums, not unique RAM.
.data
treeEnabled dd 1
treeRoot dq 0
.data?
align 8
treeHash dq 16384 dup(?)
treeOrder dq 8192 dup(?)
treeMap dq 8192 dup(?)
treeLabels db 4194304 dup(?)
collapsedPid dd 8192 dup(?)
collapsedBirth dq 8192 dup(?)
.data
treeCount dd 0
treeIndex dd 0
collapsedUsed dd 0
treeSumFmt dw '%','I','6','4','u',0
treePartialFmt dw '>','=',' ','%','I','6','4','u',0
treeCountFmt dw ' ','(','%','I','6','4','u',')',0
.code
BuildProcessTree PROC
  push rbx
  push rsi
  push rdi
  push r12
  push r13
  push r14
  push r15
  sub rsp,20h
  lea rdi,treeHash
  xor eax,eax
  mov ecx,16384
  rep stosq
  mov treeRoot,0
  lea rbx,rowMap
  xor esi,esi
btHash:
  cmp esi,visibleCount
  jae btParents
  mov rdi,[rbx+rsi*8]
  mov qword ptr [rdi+3680],0
  mov qword ptr [rdi+3688],0
  mov qword ptr [rdi+3696],0
  mov dword ptr [rdi+3704],0
  mov dword ptr [rdi+3708],0
  mov dword ptr [rdi+3712],0
  cmp dword ptr [rdi+3656],0
  jne btMemoryKnown
  mov dword ptr [rdi+3712],1
btMemoryKnown:
  mov rax,[rdi+3624]
  mov [rdi+3720],rax
  mov rax,[rdi+3632]
  mov [rdi+3728],rax
  mov qword ptr [rdi+3736],1
  mov eax,[rdi+3584]
  and eax,16383
  lea rdx,treeHash
btProbe:
  cmp qword ptr [rdx+rax*8],0
  je btInsert
  inc eax
  and eax,16383
  jmp btProbe
btInsert:
  mov [rdx+rax*8],rdi
  inc esi
  jmp btHash
btParents:
  mov esi,visibleCount
btParentNext:
  test esi,esi
  jz btFlatten
  dec esi
  mov rdi,[rbx+rsi*8]
  cmp dword ptr [rdi+3676],0
  jne btRoot
  mov ecx,[rdi+3588]
  mov eax,ecx
  and eax,16383
  lea rdx,treeHash
btLookup:
  mov r12,[rdx+rax*8]
  test r12,r12
  jz btRoot
  cmp ecx,[r12+3584]
  je btValidate
  inc eax
  and eax,16383
  jmp btLookup
btValidate:
  mov rax,[r12+3600]
  test rax,rax
  jz btRoot
  cmp rax,[rdi+3600]
  jae btRoot
  mov [rdi+3680],r12
  mov rax,[r12+3696]
  mov [rdi+3688],rax
  mov [r12+3696],rdi
  jmp btParentNext
btRoot:
  mov rax,treeRoot
  mov [rdi+3688],rax
  mov treeRoot,rdi
  jmp btParentNext
btFlatten:
  mov rdi,treeRoot
  xor esi,esi
  lea r12,treeOrder
btWalk:
  test rdi,rdi
  jz btAggregate
  mov [r12+rsi*8],rdi
  inc esi
  mov rax,[rdi+3696]
  test rax,rax
  jz btAscend
  mov ecx,[rdi+3704]
  inc ecx
  mov [rax+3704],ecx
  mov rdi,rax
  jmp btWalk
btAscend:
  mov rax,[rdi+3688]
  test rax,rax
  jnz btSibling
  mov rdi,[rdi+3680]
  test rdi,rdi
  jnz btAscend
  jmp btAggregate
btSibling:
  mov ecx,[rdi+3704]
  mov [rax+3704],ecx
  mov rdi,rax
  jmp btWalk
btAggregate:
  mov treeCount,esi
  mov edi,esi
btSum:
  test edi,edi
  jz btFilter
  dec edi
  mov rax,[r12+rdi*8]
  mov rdx,[rax+3680]
  test rdx,rdx
  jz btSum
  mov rcx,[rax+3720]
  add [rdx+3720],rcx
  mov rcx,[rax+3728]
  add [rdx+3728],rcx
  mov rcx,[rax+3736]
  add [rdx+3736],rcx
  mov ecx,[rax+3712]
  add [rdx+3712],ecx
  jmp btSum
btFilter:
  xor esi,esi
btMatch:
  cmp esi,treeCount
  jae btVisible
  mov rdi,[r12+rsi*8]
  cmp word ptr [searchBuffer],0
  je btMark
  mov rcx,rdi
  lea rdx,searchBuffer
  call StrStrIW
  test rax,rax
  jnz btMark
  lea rcx,[rdi+512]
  lea rdx,searchBuffer
  call StrStrIW
  test rax,rax
  jz btMatchNext
btMark:
  mov rax,rdi
btMarkParent:
  mov dword ptr [rax+3708],1
  mov rax,[rax+3680]
  test rax,rax
  jnz btMarkParent
btMatchNext:
  inc esi
  jmp btMatch
btVisible:
  xor esi,esi
  xor r13d,r13d
  mov r14d,-1
btOutput:
  cmp esi,treeCount
  jae btDone
  mov rdi,[r12+rsi*8]
  cmp dword ptr [rdi+3708],0
  je btOutputNext
  mov eax,[rdi+3704]
  cmp r14d,-1
  je btNoSkip
  cmp eax,r14d
  ja btOutputNext
  mov r14d,-1
btNoSkip:
  lea rdx,collapsedPid
  lea r8,collapsedBirth
  xor ecx,ecx
  xor r15d,r15d
btClosed:
  cmp ecx,collapsedUsed
  jae btLabel
  mov eax,[rdi+3584]
  cmp eax,[rdx+rcx*4]
  jne btClosedNext
  mov rax,[rdi+3600]
  cmp rax,[r8+rcx*8]
  jne btClosedNext
  mov r15d,1
  jmp btLabel
btClosedNext:
  inc ecx
  jmp btClosed
btLabel:
  mov r8,[rdi+3720]
  shr r8,20
  lea rcx,[rdi+3752]
  lea rdx,treeSumFmt
  cmp dword ptr [rdi+3712],0
  je btCommitKnown
  lea rdx,treePartialFmt
btCommitKnown:
  call wsprintfW
  mov r8,[rdi+3728]
  shr r8,20
  lea rcx,[rdi+3816]
  lea rdx,treeSumFmt
  cmp dword ptr [rdi+3712],0
  je btWsKnown
  lea rdx,treePartialFmt
btWsKnown:
  call wsprintfW
  mov eax,[rdi+3712]
  cmp rax,[rdi+3736]
  jne btSomeMemory
  lea rcx,[rdi+3752]
  lea rdx,unavailable
  call lstrcpyW
  lea rcx,[rdi+3816]
  lea rdx,unavailable
  call lstrcpyW
btSomeMemory:
  lea rdx,treeMap
  mov [rdx+r13*8],rdi
  mov eax,esi
  shl rax,9
  lea rbx,treeLabels
  add rbx,rax
  mov [rdi+3744],rbx
  mov ecx,[rdi+3704]
  cmp ecx,24
  jbe btIndent
  mov ecx,24
btIndent:
  add ecx,ecx
  xor eax,eax
btSpaces:
  cmp eax,ecx
  jae btArrow
  mov word ptr [rbx+rax*2],32
  inc eax
  jmp btSpaces
btArrow:
  lea rbx,[rbx+rax*2]
  mov edx,32
  cmp qword ptr [rdi+3696],0
  je btArrowStore
  mov edx,25BEh
  test r15d,r15d
  jz btArrowStore
  mov edx,25B8h
  cmp word ptr [searchBuffer],0
  jne btArrowStore
  mov r14d,[rdi+3704]
btArrowStore:
  mov [rbx],dx
  mov word ptr [rbx+2],32
  lea rcx,[rbx+4]
  mov rdx,rdi
  mov r8d,180
  call lstrcpynW
  cmp qword ptr [rdi+3736],1
  jbe btNoCount
  mov rcx,rbx
  add rcx,4
  call lstrlenW
  lea rcx,[rbx+rax*2+4]
  lea rdx,treeCountFmt
  mov r8,[rdi+3736]
  call wsprintfW
btNoCount:
  inc r13d
btOutputNext:
  inc esi
  jmp btOutput
btDone:
  mov visibleCount,r13d
  lea rsi,treeMap
  lea rdi,rowMap
  mov ecx,r13d
  rep movsq
  add rsp,20h
  pop r15
  pop r14
  pop r13
  pop r12
  pop rdi
  pop rsi
  pop rbx
  ret
BuildProcessTree ENDP

ToggleTreeNode PROC
  sub rsp,28h
  cmp frontMode,0
  jne ttnDone
  cmp treeEnabled,0
  je ttnDone
  mov rcx,gList
  mov edx,100Ch
  mov r8,-1
  mov r9d,2
  call SendMessageW
  cmp eax,visibleCount
  jae ttnDone
  lea rdx,rowMap
  mov r9,[rdx+rax*8]
  cmp qword ptr [r9+3696],0
  je ttnDone
  xor ecx,ecx
  mov r10d,-1
  lea rdx,collapsedPid
  lea r8,collapsedBirth
ttnFind:
  cmp ecx,collapsedUsed
  jae ttnAdd
  cmp dword ptr [rdx+rcx*4],0
  jne ttnCheck
  mov r10d,ecx
ttnCheck:
  mov eax,[r9+3584]
  cmp eax,[rdx+rcx*4]
  jne ttnNext
  mov rax,[r9+3600]
  cmp rax,[r8+rcx*8]
  jne ttnNext
  mov dword ptr [rdx+rcx*4],0
  jmp ttnRefresh
ttnNext:
  inc ecx
  jmp ttnFind
ttnAdd:
  cmp r10d,-1
  jne ttnUseFree
  mov r10d,collapsedUsed
  cmp r10d,8192
  jae ttnDone
  inc collapsedUsed
ttnUseFree:
  mov eax,[r9+3584]
  mov [rdx+r10*4],eax
  mov rax,[r9+3600]
  mov [r8+r10*8],rax
ttnRefresh:
  call RememberSelection
  call BuildRowMap
ttnDone:
  add rsp,28h
  ret
ToggleTreeNode ENDP
