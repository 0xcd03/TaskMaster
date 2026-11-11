; Chart acceptance belongs to UI; telemetryPassTick is collector-owned while
; its published snapshot is awaiting UI acknowledgement.
.data
align 8
chartAcceptedTick dq 0
chartWasPaused dd 0
.data?
align 8
chartTimes dq 180 dup(?)
.code
ApplyTelemetryChart PROC
  sub rsp,28h
  cmp frontMode,0
  je atcMode
  cmp frontMode,4
  je atcMode
  mov chartWasPaused,1
  jmp atcDone
atcMode:
  mov rax,telemetryPassTick
  test rax,rax
  jz atcDone
  cmp rax,chartAcceptedTick
  jbe atcDone
  mov [rsp+20h],rax
  cmp chartAcceptedTick,0
  je atcSample
  cmp chartWasPaused,0
  jne atcGap
  sub rax,chartAcceptedTick
  cmp rax,GP_MAX_GAP_MS
  jbe atcSample
atcGap:
  call AddChartGap
atcSample:
  call AddChartSample
  mov rax,[rsp+20h]
  mov chartAcceptedTick,rax
  mov chartWasPaused,0
atcDone:
  add rsp,28h
  ret
ApplyTelemetryChart ENDP

AddChartGap PROC
  lea r8,chartData
  mov ecx,chartHead
  lea r10,chartTimes
  mov qword ptr [r10+rcx*8],0 ; gap has no measurement timestamp
  xor eax,eax
acgLoop:
  mov dword ptr [r8+rcx*4],-1
  add r8,720
  inc eax
  cmp eax,5
  jb acgLoop
  jmp AdvanceChartPosition
AddChartGap ENDP
