; Visible, unowned top-level application windows define UI roots. Other edges are
; the verified OS parent tree; this does not infer ownership from executable names.
extern EnumWindows:proc, IsWindowVisible:proc, GetWindow:proc, GetWindowThreadProcessId:proc
.data?
applicationPids dd 16384 dup(?)
applicationPid dd ?
.code
CollectApplicationRoots PROC
  push rdi
  sub rsp,20h
  lea rdi,applicationPids
  xor eax,eax
  mov ecx,8192
  rep stosq
  lea rcx,ApplicationWindowCallback
  xor edx,edx
  call EnumWindows
  add rsp,20h
  pop rdi
  ret
CollectApplicationRoots ENDP

ApplicationWindowCallback PROC
  push rbx
  sub rsp,20h
  mov rbx,rcx
  call IsWindowVisible
  test eax,eax
  jz awcDone
  mov rcx,rbx
  mov edx,4 ; GW_OWNER
  call GetWindow
  test rax,rax
  jnz awcDone
  mov rcx,rbx
  lea rdx,applicationPid
  call GetWindowThreadProcessId
  mov ecx,applicationPid
  test ecx,ecx
  jz awcDone
  lea rdx,applicationPids
  mov eax,ecx
  and eax,16383
awcHash:
  cmp dword ptr [rdx+rax*4],0
  je awcSet
  cmp [rdx+rax*4],ecx
  je awcDone
  inc eax
  and eax,16383
  jmp awcHash
awcSet:
  mov [rdx+rax*4],ecx
awcDone:
  mov eax,1
  add rsp,20h
  pop rbx
  ret
ApplicationWindowCallback ENDP

IsApplicationRoot PROC
  mov eax,ecx
  and eax,16383
  lea rdx,applicationPids
iarProbe:
  cmp dword ptr [rdx+rax*4],0
  je iarNo
  cmp [rdx+rax*4],ecx
  je iarYes
  inc eax
  and eax,16383
  jmp iarProbe
iarYes:
  mov eax,1
  ret
iarNo:
  xor eax,eax
  ret
IsApplicationRoot ENDP
