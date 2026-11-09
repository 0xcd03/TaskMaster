.data
align 8
.data?
align 8
historyRows db 33554432 dup(?)
.data
historyCount dd 0
.code
; Worker-owned history, bounded to 8192 identities. PID + birth is the identity.
; 3656=observed CPU, 3680/3688=observed I/O; do not count gaps across restarts.
; History+3736 stores the system CPU total at this identity's own sample.
; Process+4000 is the matching interval, independent of skipped snapshots.
RecordHistory PROC
  push rbx
  push rsi
  push rdi
  sub rsp,20h
  mov rbx,rcx
  mov qword ptr [rbx+3664],0
  mov dword ptr [rbx+3672],0
  mov qword ptr [rbx+4000],0
  cmp qword ptr [rbx+3600],0
  je rhDone
  mov historyDirty,1
  lea rsi,historyRows
  xor edi,edi
rhFind:
  cmp edi,historyCount
  jae rhNew
  mov eax,[rbx+3584]
  cmp eax,[rsi+3584]
  jne rhNext
  mov rax,[rbx+3600]
  cmp rax,[rsi+3600]
  je rhUpdate
rhNext:
  inc edi
  add rsi,4096
  jmp rhFind
rhNew:
  cmp edi,8192
  jb rhAllocate
  mov edi,historyReplace
  inc historyReplace
  and historyReplace,8191
  mov esi,edi
  shl rsi,12
  lea rax,historyRows
  add rsi,rax
  mov rcx,rsi
  call ClearRow
  jmp rhCreate
rhAllocate:
  inc historyCount
rhCreate:
  mov rcx,rsi
  mov rdx,rbx
  call lstrcpyW
  mov eax,[rbx+3584]
  mov [rsi+3584],eax
  mov rax,[rbx+3600]
  mov [rsi+3600],rax
  mov qword ptr [rsi+3640],0
  mov qword ptr [rsi+3648],0
  mov qword ptr [rsi+3656],0
  mov qword ptr [rsi+3680],0
  mov qword ptr [rsi+3688],0
  mov qword ptr [rsi+3712],0
  mov qword ptr [rsi+3736],0
rhUpdate:
  ; Current bytes/sec from the same process identity and a real sample interval.
  ; These are process I/O counters, not a claim of physical disk attribution.
  cmp dword ptr [rbx+3660],0
  je rhCpuUpdate
  cmp dword ptr [rsi+3716],0
  je rhCpuUpdate
  cmp dword ptr [rsi+3712],0
  je rhCpuUpdate
  mov rax,historyNow
  sub rax,[rsi+3672]
  jbe rhCpuUpdate
  cmp rax,300000000
  ja rhCpuUpdate
  mov r10,rax
  mov rax,[rbx+3640]
  sub rax,[rsi+3696]
  jc rhCpuUpdate
  mov r11,[rbx+3648]
  sub r11,[rsi+3704]
  jc rhCpuUpdate
  add rax,r11
  jc rhCpuUpdate
  mov ecx,10000000
  mul rcx
  cmp rdx,r10
  jae rhCpuUpdate
  div r10
  mov [rbx+3888],rax
  mov dword ptr [rbx+3896],1
rhCpuUpdate:
  cmp dword ptr [rsi+3712],0
  je rhFirstSample
  mov rax,[rbx+3608]
  sub rax,[rsi+3608]
  jc rhFirstSample
  mov [rbx+3664],rax
  add [rsi+3656],rax
  ; A process skipped by cancelled or infrequent passes must use the system
  ; delta from its own previous observation, never the most recent global pass.
  mov rax,sysSampleTotal
  test rax,rax
  jz rhObservedTime
  mov rdx,[rsi+3736]
  test rdx,rdx
  jz rhObservedTime
  sub rax,rdx
  jbe rhObservedTime
  mov [rbx+4000],rax
  mov dword ptr [rbx+3672],1
rhObservedTime:
  mov rax,historyNow
  sub rax,[rsi+3672]
  jc rhFirstSample
  cmp rax,300000000
  ja rhFirstSample
  xor edx,edx
  mov ecx,10000
  div rcx
  add [rsi+3664],rax
rhFirstSample:
  mov dword ptr [rsi+3712],1
  cmp dword ptr [rbx+3660],0
  je rhPeak
  cmp dword ptr [rsi+3716],0
  je rhIoFirst
  mov rax,[rbx+3640]
  sub rax,[rsi+3696]
  jc rhIoFirst
  add [rsi+3680],rax
  mov rax,[rbx+3648]
  sub rax,[rsi+3704]
  jc rhIoFirst
  add [rsi+3688],rax
rhIoFirst:
  mov dword ptr [rsi+3716],1
  mov rax,[rbx+3640]
  mov [rsi+3696],rax
  mov rax,[rbx+3648]
  mov [rsi+3704],rax
rhPeak:
  mov rax,[rbx+3632]
  cmp rax,[rsi+3720]
  jbe rhCommit
  mov [rsi+3720],rax
rhCommit:
  mov rax,[rbx+3624]
  cmp rax,[rsi+3640]
  jbe rhCpu
  mov [rsi+3640],rax
rhCpu:
  mov rax,[rbx+3608]
  mov [rsi+3608],rax
  mov rax,sysSampleTotal
  mov [rsi+3736],rax
  inc qword ptr [rsi+3648]
  mov rax,historyNow
  mov [rsi+3672],rax
rhDone:
  add rsp,20h
  pop rdi
  pop rsi
  pop rbx
  ret
RecordHistory ENDP

CollectHistory PROC
  push rbx
  push rsi
  push rdi
  sub rsp,20h
  mov backCount,0
  lea rsi,historyRows
  mov ebx,historyCount
chNext:
  call CollectorSnapshotCurrent
  test eax,eax
  jz chDone
  test ebx,ebx
  jz chDone
  mov edi,backCount
  shl rdi,12
  add rdi,backBuffer
  mov rcx,rdi
  call ClearRow
  mov rcx,rdi
  mov rdx,rsi
  call lstrcpyW
  mov ecx,[rsi+3584]
  lea rdx,[rdi+512]
  call StoreDecimal
  mov rax,[rsi+3656]
  xor edx,edx
  mov ecx,10000
  div rcx
  mov rcx,rax
  lea rdx,[rdi+1024]
  call StoreDecimal
  mov rcx,[rsi+3680]
  shr rcx,20
  lea rdx,[rdi+2560]
  call StoreDecimal
  mov rcx,[rsi+3688]
  shr rcx,20
  lea rdx,[rdi+3072]
  call StoreDecimal
  lea rcx,[rdi+1536]
  lea rdx,historyMemoryFmt
  mov r8,[rsi+3720]
  shr r8,20
  mov r9,[rsi+3640]
  shr r9,20
  call wsprintfW
  mov rax,[rsi+3664]
  xor edx,edx
  mov ecx,1000
  div rcx
  mov rcx,rax
  lea rdx,[rdi+2048]
  call StoreDecimal
  inc backCount
  add rsi,4096
  dec ebx
  jmp chNext
chDone:
  add rsp,20h
  pop rdi
  pop rsi
  pop rbx
  ret
CollectHistory ENDP
.data
historyMemoryFmt dw '%','I','6','4','u',' ','/',' ','%','I','6','4','u',0


