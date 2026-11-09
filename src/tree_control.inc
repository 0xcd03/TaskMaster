.data
treeTargetCount dd 0
actionBatch dd 0
treeCommand dd 0
treeTotalSuccess dd 0
treeTotalFail dd 0
treeLastError dd 0
.data?
treeTargetPid dd 8192 dup(?)
treeTargetBirth dq 8192 dup(?)
.code
; RCX root row. Capture every descendant even when a branch is collapsed/filtered.
CaptureTreeTargets PROC
  mov treeTargetCount,0
  mov r11,rcx
  xor r8d,r8d
  mov r9,frontBuffer
cttNext:
  cmp r8d,frontCount
  jae cttDone
  mov rax,r9
  mov edx,frontCount
cttAncestor:
  cmp rax,r11
  je cttAdd
  mov rax,[rax+3680]
  test rax,rax
  jz cttAdvance
  dec edx
  jnz cttAncestor
  jmp cttAdvance
cttAdd:
  mov ecx,treeTargetCount
  lea rdx,treeTargetPid
  mov eax,[r9+3584]
  mov [rdx+rcx*4],eax
  lea rdx,treeTargetBirth
  mov rax,[r9+3600]
  mov [rdx+rcx*8],rax
  inc treeTargetCount
cttAdvance:
  inc r8d
  add r9,4096
  jmp cttNext
cttDone:
  ret
CaptureTreeTargets ENDP

RunTreeAction PROC
  push rbx
  push rsi
  sub rsp,28h
  mov eax,actionCode
  mov treeCommand,eax
  sub eax,18 ; 2019..2022 -> 2001..2004
  mov esi,eax
  mov treeTotalSuccess,0
  mov treeTotalFail,0
  mov treeLastError,0
  mov actionBatch,1
  ; Reverse capture order generally handles children before parents; every identity
  ; is revalidated by the same guarded action path, not trusted from the snapshot.
  mov ebx,treeTargetCount
rtaNext:
  test ebx,ebx
  jz rtaDone
  dec ebx
  lea rdx,treeTargetPid
  mov eax,[rdx+rbx*4]
  mov actionPid,eax
  lea rdx,treeTargetBirth
  mov rax,[rdx+rbx*8]
  mov actionBirth,rax
  mov actionCode,esi
  mov actionMode,0
  mov actionPending,1
  call RunQueuedAction
  mov eax,actionSucceeded
  add treeTotalSuccess,eax
  mov eax,actionFailed
  add treeTotalFail,eax
  mov eax,actionResult
  test eax,eax
  jz rtaNext
  mov treeLastError,eax
  jmp rtaNext
rtaDone:
  mov actionBatch,0
  mov eax,treeCommand
  mov actionCode,eax
  mov eax,treeTotalSuccess
  mov actionSucceeded,eax
  mov eax,treeTotalFail
  mov actionFailed,eax
  mov eax,treeLastError
  mov actionResult,eax
  add rsp,28h
  pop rsi
  pop rbx
  ret
RunTreeAction ENDP
