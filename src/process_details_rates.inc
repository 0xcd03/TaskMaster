; Private clock boundary; tests replace it only inside their own executable.
extern GetSystemTimeAdjustment:proc
.data
align 8
pdRatesClockQuery dq GetSystemTimeAdjustment
.code
; RCX previous immutable snapshot (or zero), RDX current snapshot.
; Writes only current CPU/I/O rate metadata and derived numeric values.
PdDeriveRates PROC
  test rdx,rdx
  jz pdRatesReturn
  push rbx
  push rsi
  push rdi
  push r12
  push r13
  push r14
  push r15
  sub rsp,60h
  mov rsi,rcx
  mov rbx,rdx
  ; A missing/failed baseline is unknown, never a measured idle interval.
  mov dword ptr [rbx+PD_S_FIELDS+PD_F_CPU_RATE*PD_FIELD_SIZE],PD_STATUS_UNAVAILABLE
  mov dword ptr [rbx+PD_S_FIELDS+PD_F_CPU_RATE*PD_FIELD_SIZE+4],21
  mov qword ptr [rbx+PD_S_FIELDS+PD_F_CPU_RATE*PD_FIELD_SIZE+8],0
  mov dword ptr [rbx+PD_S_FIELDS+PD_F_IO_RATE*PD_FIELD_SIZE],PD_STATUS_UNAVAILABLE
  mov dword ptr [rbx+PD_S_FIELDS+PD_F_IO_RATE*PD_FIELD_SIZE+4],21
  mov qword ptr [rbx+PD_S_FIELDS+PD_F_IO_RATE*PD_FIELD_SIZE+8],0
  mov dword ptr [rbx+PD_S_CPU_BASIS_POINTS],0
  mov qword ptr [rbx+PD_S_READ_BPS],0
  mov qword ptr [rbx+PD_S_WRITE_BPS],0
  mov qword ptr [rbx+PD_S_OTHER_BPS],0
  test rsi,rsi
  jz pdRatesDone
  ; Complete native headers and verified identity are required on both sides.
  cmp dword ptr [rsi+PD_S_SIZE],PD_SNAPSHOT_SIZE
  jne pdRatesInvalid
  cmp dword ptr [rbx+PD_S_SIZE],PD_SNAPSHOT_SIZE
  jne pdRatesInvalid
  cmp dword ptr [rsi+PD_S_VERSION],PD_VERSION
  jne pdRatesInvalid
  cmp dword ptr [rbx+PD_S_VERSION],PD_VERSION
  jne pdRatesInvalid
  cmp dword ptr [rsi+PD_S_LANE],1
  jne pdRatesInvalid
  cmp dword ptr [rbx+PD_S_LANE],1
  jne pdRatesInvalid
  mov rax,[rbx+PD_S_COOKIE]
  test rax,rax
  jz pdRatesInvalid
  cmp rax,[rsi+PD_S_COOKIE]
  jne pdRatesInvalid
  mov rax,[rbx+PD_S_GENERATION]
  test rax,rax
  jz pdRatesInvalid
  cmp rax,[rsi+PD_S_GENERATION]
  jne pdRatesInvalid
  mov eax,[rbx+PD_S_PID]
  test eax,eax
  jz pdRatesInvalid
  cmp eax,[rsi+PD_S_PID]
  jne pdRatesInvalid
  mov rax,[rbx+PD_S_BIRTH]
  test rax,rax
  jz pdRatesInvalid
  cmp rax,[rsi+PD_S_BIRTH]
  jne pdRatesInvalid
  cmp rax,[rbx+PD_S_OBSERVED_BIRTH]
  jne pdRatesInvalid
  cmp rax,[rsi+PD_S_OBSERVED_BIRTH]
  jne pdRatesInvalid
  cmp dword ptr [rsi+PD_S_TARGET],PD_TARGET_VERIFIED
  jne pdRatesDone
  cmp dword ptr [rbx+PD_S_TARGET],PD_TARGET_VERIFIED
  jne pdRatesDone
  mov r12,[rbx+PD_S_TICK]
  sub r12,[rsi+PD_S_TICK]
  jbe pdRatesInvalid
  cmp r12,30000
  ja pdRatesDone
  mov eax,[rbx+PD_S_CAPACITY]
  test eax,eax
  jz pdRatesInvalid
  cmp eax,[rsi+PD_S_CAPACITY]
  jne pdRatesInvalid
  ; Each source can fail independently. CPU failure does not erase valid I/O.
  cmp dword ptr [rsi+PD_S_FIELDS+PD_F_CPU_TIMES*PD_FIELD_SIZE],PD_STATUS_VALID
  jne pdRatesIo
  cmp dword ptr [rbx+PD_S_FIELDS+PD_F_CPU_TIMES*PD_FIELD_SIZE],PD_STATUS_VALID
  jne pdRatesIo
  mov r14,[rbx+PD_S_KERNEL]
  sub r14,[rsi+PD_S_KERNEL]
  jc pdRatesCpuInvalid
  mov rax,[rbx+PD_S_USER]
  sub rax,[rsi+PD_S_USER]
  jc pdRatesCpuInvalid
  add r14,rax
  jc pdRatesCpuInvalid
  ; Total process CPU can exceed one core. Ceiling = wall time * capacity.
  mov rax,r12
  mov ecx,10000
  mul rcx
  test rdx,rdx
  jnz pdRatesCpuInvalid
  mov ecx,[rbx+PD_S_CAPACITY]
  mul rcx
  test rdx,rdx
  jnz pdRatesCpuInvalid
  test rax,rax
  jz pdRatesCpuInvalid
  mov r13,rax
  cmp r14,r13
  jbe pdRatesCpuDivide
  ; Same quantization rule as thread metrics: two measured clock quanta,
  ; capped at 32 ms, with a conservative 31.25 ms fallback; scale per CPU.
  mov r15d,312500
  lea rcx,[rsp+20h]
  lea rdx,[rsp+24h]
  lea r8,[rsp+28h]
  call qword ptr [pdRatesClockQuery]
  test eax,eax
  jz pdRatesToleranceReady
  mov eax,[rsp+24h]
  test eax,eax
  jz pdRatesToleranceReady
  cmp eax,1000000
  ja pdRatesToleranceReady
  shl rax,1
  cmp rax,320000
  jbe pdRatesToleranceMeasured
  mov eax,320000
pdRatesToleranceMeasured:
  mov r15,rax
pdRatesToleranceReady:
  mov rax,r15
  mov ecx,[rbx+PD_S_CAPACITY]
  mul rcx
  test rdx,rdx
  jnz pdRatesCpuInvalid
  mov rcx,r14
  sub rcx,r13
  cmp rcx,rax
  ja pdRatesCpuInvalid
  mov r14,r13
pdRatesCpuDivide:
  mov rax,r14
  mov ecx,10000
  mul rcx
  cmp rdx,r13
  jae pdRatesCpuInvalid
  div r13
  cmp rax,10000
  ja pdRatesCpuInvalid
  mov [rbx+PD_S_CPU_BASIS_POINTS],eax
  mov dword ptr [rbx+PD_S_FIELDS+PD_F_CPU_RATE*PD_FIELD_SIZE],PD_STATUS_VALID
  mov dword ptr [rbx+PD_S_FIELDS+PD_F_CPU_RATE*PD_FIELD_SIZE+4],0
  jmp pdRatesIo
pdRatesCpuInvalid:
  mov dword ptr [rbx+PD_S_FIELDS+PD_F_CPU_RATE*PD_FIELD_SIZE+4],13
pdRatesIo:
  cmp dword ptr [rsi+PD_S_FIELDS+PD_F_IO*PD_FIELD_SIZE],PD_STATUS_VALID
  jne pdRatesDone
  cmp dword ptr [rbx+PD_S_FIELDS+PD_F_IO*PD_FIELD_SIZE],PD_STATUS_VALID
  jne pdRatesDone
  ; One group status covers all six counters; any rollback breaks continuity.
  mov edi,PD_S_READ_OPS
pdRatesIoMonotonic:
  mov rax,[rbx+rdi]
  cmp rax,[rsi+rdi]
  jb pdRatesIoInvalid
  add edi,8
  cmp edi,PD_S_OTHER_BYTES+8
  jb pdRatesIoMonotonic
  xor edi,edi
pdRatesIoDivide:
  mov rax,[rbx+rdi+PD_S_READ_BYTES]
  sub rax,[rsi+rdi+PD_S_READ_BYTES]
  mov ecx,1000
  mul rcx
  ; DIV faults when the quotient cannot fit; test its high half first.
  cmp rdx,r12
  jae pdRatesIoInvalid
  div r12
  mov [rsp+rdi+30h],rax
  add edi,8
  cmp edi,24
  jb pdRatesIoDivide
  mov rax,[rsp+30h]
  mov [rbx+PD_S_READ_BPS],rax
  mov rax,[rsp+38h]
  mov [rbx+PD_S_WRITE_BPS],rax
  mov rax,[rsp+40h]
  mov [rbx+PD_S_OTHER_BPS],rax
  mov dword ptr [rbx+PD_S_FIELDS+PD_F_IO_RATE*PD_FIELD_SIZE],PD_STATUS_VALID
  mov dword ptr [rbx+PD_S_FIELDS+PD_F_IO_RATE*PD_FIELD_SIZE+4],0
  jmp pdRatesDone
pdRatesIoInvalid:
  mov dword ptr [rbx+PD_S_FIELDS+PD_F_IO_RATE*PD_FIELD_SIZE+4],13
  jmp pdRatesDone
pdRatesInvalid:
  mov dword ptr [rbx+PD_S_FIELDS+PD_F_CPU_RATE*PD_FIELD_SIZE+4],13
  mov dword ptr [rbx+PD_S_FIELDS+PD_F_IO_RATE*PD_FIELD_SIZE+4],13
pdRatesDone:
  add rsp,60h
  pop r15
  pop r14
  pop r13
  pop r12
  pop rdi
  pop rsi
  pop rbx
pdRatesReturn:
  ret
PdDeriveRates ENDP
