; Collector-only, bounded O(1) thread CPU observations. Direct-map collisions
; evict a baseline and therefore yield N/A, never another thread's CPU delta.
extern GetActiveProcessorCount:proc, GetSystemTimeAdjustment:proc
.data
align 8
threadMetricsEpoch dq 0
threadMetricsOwnerBirth dq 0
threadMetricsOwner dd 0
threadMetricsGeneration dd 0
threadMetricsProcessorCount dd 0
threadMetricsClockIncrement dd 0
threadMetricsClockAdjustment dd 0
threadMetricsClockDisabled dd 0
threadMetricsTolerance100ns dq 312500 ; conservative two standard 15.625ms quanta
.data?
align 8
; 8192 slots x 64 bytes: birth,total,tick,epoch,TID,CPUs,owner,reserved,
; owner birth,valid. No process history, persistence or UI-thread access.
threadMetricsCache db 524288 dup(?)
.code
; ECX owner PID, RDX owner birth, R8D navigation generation, R9D active CPUs.
; Also used by deterministic tests with explicit clocks/topology.
SetThreadMetricsContext PROC
  cmp ecx,threadMetricsOwner
  jne stmcChanged
  cmp rdx,threadMetricsOwnerBirth
  jne stmcChanged
  cmp r8d,threadMetricsGeneration
  jne stmcChanged
  cmp threadMetricsEpoch,0
  jne stmcStore
stmcChanged:
  inc threadMetricsEpoch
  jnz stmcStore
  ; Epoch wrap is practically unreachable, but must not revive old entries.
  push rdi
  push rcx
  lea rdi,threadMetricsCache
  xor eax,eax
  mov ecx,65536
  rep stosq
  pop rcx
  pop rdi
  mov threadMetricsEpoch,1
stmcStore:
  mov threadMetricsOwner,ecx
  mov threadMetricsOwnerBirth,rdx
  mov threadMetricsGeneration,r8d
  mov threadMetricsProcessorCount,r9d
  ret
SetThreadMetricsContext ENDP

BeginThreadMetrics PROC
  sub rsp,28h
  mov threadMetricsTolerance100ns,312500
  lea rcx,threadMetricsClockAdjustment
  lea rdx,threadMetricsClockIncrement
  lea r8,threadMetricsClockDisabled
  call GetSystemTimeAdjustment
  test eax,eax
  jz btmProcessorCount
  mov eax,threadMetricsClockIncrement
  test eax,eax
  jz btmProcessorCount
  cmp eax,1000000 ; reject implausible >100ms resolution rather than hide spikes
  ja btmProcessorCount
  shl rax,1
  cmp rax,320000 ; keep the allowance bounded to 32ms on unusual clock settings
  jbe btmClockTolerance
  mov eax,320000
btmClockTolerance:
  mov threadMetricsTolerance100ns,rax
btmProcessorCount:
  mov ecx,0FFFFh ; ALL_PROCESSOR_GROUPS, normalized to total machine CPU capacity
  call GetActiveProcessorCount
  mov r9d,eax
  mov ecx,collectInspectedPid
  mov rdx,collectInspectedBirth
  mov r8d,collectGeneration
  call SetThreadMetricsContext
  add rsp,28h
  ret
BeginThreadMetrics ENDP

ResetThreadCpuRow PROC
  mov dword ptr [rcx+3672],0
  mov dword ptr [rcx+3900],0
  add rcx,3072
  lea rdx,unavailable
  jmp lstrcpyW
ResetThreadCpuRow ENDP

; RCX row. A failed access/time/owner query breaks continuity for this slot.
InvalidateThreadCpu PROC
  mov eax,[rcx+3584]
  shr eax,2
  and eax,8191
  shl eax,6
  lea rdx,threadMetricsCache
  mov qword ptr [rdx+rax+56],0
  jmp ResetThreadCpuRow
InvalidateThreadCpu ENDP

; RCX row, RDX monotonic ms, R8 kernel 100ns, R9 user 100ns.
; Only a complete current identity and a valid previous sample yield a number.
RecordThreadCpu PROC
  push rbx
  push rsi
  push rdi
  push r12
  sub rsp,28h
  mov rdi,rcx
  mov rbx,rdx
  mov r12,r8
  add r12,r9
  jc rtcOverflow
  call ResetThreadCpuRow
  mov eax,[rdi+3584]
  shr eax,2
  and eax,8191
  shl eax,6
  lea rsi,threadMetricsCache
  add rsi,rax
  cmp threadMetricsProcessorCount,0
  je rtcInvalidate
  test rbx,rbx
  jz rtcInvalidate
  cmp dword ptr [rdi+3584],0
  je rtcInvalidate
  cmp qword ptr [rdi+3600],0
  je rtcInvalidate
  mov eax,[rdi+3588]
  cmp eax,threadMetricsOwner
  jne rtcInvalidate
  mov rax,[rdi+3616]
  cmp rax,threadMetricsOwnerBirth
  jne rtcInvalidate
  cmp qword ptr [rsi+56],1
  jne rtcBaseline
  mov rax,threadMetricsEpoch
  cmp rax,[rsi+24]
  jne rtcBaseline
  mov eax,[rdi+3584]
  cmp eax,[rsi+32]
  jne rtcBaseline
  mov rax,[rdi+3600]
  cmp rax,[rsi]
  jne rtcBaseline
  mov eax,[rdi+3588]
  cmp eax,[rsi+40]
  jne rtcBaseline
  mov rax,[rdi+3616]
  cmp rax,[rsi+48]
  jne rtcBaseline
  mov eax,threadMetricsProcessorCount
  cmp eax,[rsi+36]
  jne rtcBaseline
  mov rax,rbx
  sub rax,[rsi+16]
  jbe rtcBaseline ; equal clock or rollback
  cmp rax,30000
  ja rtcBaseline ; a long gap starts a fresh baseline
  mov ecx,10000
  mul rcx
  test rdx,rdx
  jnz rtcBaseline
  mov r10,rax ; elapsed wall time in 100ns
  mov rax,r12
  sub rax,[rsi+8]
  jc rtcBaseline
  cmp rax,r10
  jbe rtcCpuPlausible
  ; CPU accounting and GetTickCount64 each round independently. Accept at most
  ; two measured clock quanta of excess, clamping to one CPU's elapsed time.
  ; Larger/impossible deltas remain unknown and start a new baseline.
  mov rcx,rax
  sub rcx,r10
  cmp rcx,threadMetricsTolerance100ns
  ja rtcBaseline
  mov rax,r10
rtcCpuPlausible:
  mov [rdi+3664],rax
  mov rax,r10
  mov ecx,threadMetricsProcessorCount
  mul rcx
  test rdx,rdx
  jnz rtcBaseline
  test rax,rax
  jz rtcBaseline
  mov [rdi+4000],rax
  mov dword ptr [rdi+3672],1
  mov rcx,rdi
  ; Shared calculation checks 128-bit delta*1000/divisor and formats RU/EN %.
  call CalculateProcessCpu
rtcBaseline:
  mov rax,[rdi+3600]
  mov [rsi],rax
  mov [rsi+8],r12
  mov [rsi+16],rbx
  mov rax,threadMetricsEpoch
  mov [rsi+24],rax
  mov eax,[rdi+3584]
  mov [rsi+32],eax
  mov eax,threadMetricsProcessorCount
  mov [rsi+36],eax
  mov eax,[rdi+3588]
  mov [rsi+40],eax
  mov rax,[rdi+3616]
  mov [rsi+48],rax
  mov qword ptr [rsi+56],1
  jmp rtcDone
rtcInvalidate:
  mov qword ptr [rsi+56],0
  jmp rtcDone
rtcOverflow:
  mov rcx,rdi
  call InvalidateThreadCpu
rtcDone:
  add rsp,28h
  pop r12
  pop rdi
  pop rsi
  pop rbx
  ret
RecordThreadCpu ENDP
