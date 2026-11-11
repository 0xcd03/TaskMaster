; Actual GPU presentation lookup, identity, status, numeric and engine ordering.
; Frame is populated via real model arrays; no process or GPU API used here.
.data
align 8
gputStage dd 9200
gputAssertions dd 0
gputFrame dq 0
gputRows db 8192 dup(0)
gputArray db 512 dup(0)
align 2
gputName dw 'p','i','d','_','4','2','_','l','u','i','d','_','0','x','0','0','0','0','0','0','0','0','_'
 dw '0','x','0','0','0','0','0','0','0','1','_','p','h','y','s','_','0','_'
 dw 'e','n','g','_','2','_','e','n','g','t','y','p','e','_','3','D',0
.code
GPUT_EQ MACRO a,b
  cmp a,b
  jne gputFail
  inc gputAssertions
ENDM
GpuUiTest PROC
  push rbx
  push rsi
  push rdi
  sub rsp,30h
  call GetProcessHeap
  mov rcx,rax
  mov edx,8
  mov r8d,GP_SNAPSHOT_SIZE
  call HeapAlloc
  test rax,rax
  jz gputFail
  mov gputFrame,rax
  mov rcx,rax
  mov edx,7
  mov r8d,8
  mov r9d,1000
  call GpModelInit
  lea rdi,gputArray
  lea rsi,gputName
  lea rax,[rdi+24]
  mov [rdi],rax
gputCopyName:
  mov dx,[rsi]
  mov [rax],dx
  add rsi,2
  add rax,2
  test dx,dx
  jnz gputCopyName
  mov rax,4039000000000000h ;25.0 percent
  mov [rdi+16],rax
  mov rcx,gputFrame
  xor edx,edx
  mov r8,rdi
  mov r9d,512
  mov dword ptr [rsp+20h],1
  call GpModelArray
  GPUT_EQ eax,0
  mov rcx,gputFrame
  call GpModelFinalize
  mov rcx,gputFrame
  mov edx,42
  call GpFindProcess
  test rax,rax
  jz gputFail
  mov rbx,rax
  mov qword ptr [rbx+GP_P_BIRTH],100
  mov rax,4294967298
  mov qword ptr [rbx+GP_P_DEDICATED],rax
  mov dword ptr [rbx+GP_P_DEDICATED_STATUS],GP_STATUS_VALID
  mov dword ptr [gputRows+3584],42
  mov qword ptr [gputRows+3600],100
  mov dword ptr [gputRows+4096+3584],42
  mov qword ptr [gputRows+4096+3600],101
  mov rax,gputFrame
  mov gpUiFront,rax
  mov gpUiMask,GP_MASK_ALL
  mov gpUiEpoch,8
  mov gpUiFresh,1
  lea rcx,gputRows
  mov edx,PC_GPU
  call GpUiRowValue
  GPUT_EQ eax,2500
  GPUT_EQ edx,GP_STATUS_VALID
  lea rcx,gputRows
  mov edx,PC_GPU_DEDICATED
  call GpUiSortMetric
  mov rdx,4294967298
  GPUT_EQ rax,rdx
  GPUT_EQ ecx,1
  lea rcx,gputRows+4096
  mov edx,PC_GPU
  call GpUiRowValue
  GPUT_EQ edx,GP_STATUS_MISSING
  mov gputStage,9201
  mov gpUiFresh,0
  lea rcx,gputRows
  mov edx,PC_GPU
  call GpUiRowValue
  GPUT_EQ edx,GP_STATUS_STALE
  mov gpUiFresh,1
  mov dword ptr [rbx+GP_P_UTIL_STATUS],GP_STATUS_WARMUP
  lea rcx,gputRows
  mov edx,PC_GPU
  call GpUiSortMetric
  GPUT_EQ ecx,0
  mov dword ptr [rbx+GP_P_UTIL_STATUS],GP_STATUS_VALID
  mov dword ptr [rbx+GP_P_UTIL],0
  lea rcx,gputRows
  mov edx,PC_GPU
  call GpUiSortMetric
  GPUT_EQ ecx,1
  GPUT_EQ eax,0
  mov gputStage,9202
  ; Empty source failure must remain unavailable/error, not absent PID.
  mov dword ptr [gputRows+3584],43
  mov rax,gputFrame
  mov dword ptr [rax+GP_S_SOURCES],GP_STATUS_UNAVAILABLE
  mov dword ptr [rax+GP_S_SOURCES+4],6789
  lea rcx,gputRows
  mov edx,PC_GPU
  call GpUiRowValue
  GPUT_EQ edx,GP_STATUS_UNAVAILABLE
  GPUT_EQ r8d,6789
  mov rax,gputFrame
  mov dword ptr [rax+GP_S_SOURCES],GP_STATUS_VALID
  lea rcx,gputRows
  mov edx,PC_GPU
  call GpUiRowValue
  GPUT_EQ edx,GP_STATUS_MISSING
  mov gpUiFront,0
  mov rcx,gputFrame
  call GetProcessHeap
  mov rcx,rax
  xor edx,edx
  mov r8,gputFrame
  call HeapFree
  xor ecx,ecx
  call ExitProcess
gputFail:
  mov ecx,gputStage
  call ExitProcess
GpuUiTest ENDP
