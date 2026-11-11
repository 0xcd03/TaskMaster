; Deterministic inputs and independent literal expected values. No target process.
extern GetStdHandle:proc, WriteFile:proc, ExitProcess:proc, wsprintfA:proc
.data
pdRatesTestStage dd 0
pdRatesTestCases dd 0
pdRatesTestAssertions dd 0
pdRatesTestClockSuccess dd 1
pdRatesTestClockIncrement dd 156250
pdRatesTestFormat db 'PD rates cases=%u assertions=%u stage=%u',13,10,0
.data?
align 8
pdRatesTestPrevious db PD_SNAPSHOT_SIZE dup(?)
pdRatesTestCurrent db PD_SNAPSHOT_SIZE dup(?)
pdRatesTestPreviousCopy db PD_SNAPSHOT_SIZE dup(?)
pdRatesTestCurrentCopy db PD_SNAPSHOT_SIZE dup(?)
pdRatesTestOutput db 128 dup(?)
pdRatesTestWritten dd ?
.code
PdRatesTestClock PROC
  mov dword ptr [rcx],156250
  mov eax,pdRatesTestClockIncrement
  mov [rdx],eax
  mov dword ptr [r8],0
  mov eax,pdRatesTestClockSuccess
  ret
PdRatesTestClock ENDP

PdRatesTestReset PROC
  push rdi
  lea rdi,pdRatesTestPrevious
  xor eax,eax
  mov ecx,PD_SNAPSHOT_SIZE/8*2
  rep stosq
  lea r10,pdRatesTestPrevious
  lea r11,pdRatesTestCurrent
  xor ecx,ecx
pdRatesResetNext:
  mov dword ptr [r10+PD_S_SIZE],PD_SNAPSHOT_SIZE
  mov dword ptr [r10+PD_S_VERSION],PD_VERSION
  mov dword ptr [r10+PD_S_LANE],1
  mov dword ptr [r10+PD_S_TARGET],PD_TARGET_VERIFIED
  mov qword ptr [r10+PD_S_COOKIE],11
  mov qword ptr [r10+PD_S_GENERATION],12
  mov dword ptr [r10+PD_S_PID],13
  mov qword ptr [r10+PD_S_BIRTH],14
  mov qword ptr [r10+PD_S_OBSERVED_BIRTH],14
  mov dword ptr [r10+PD_S_CAPACITY],4
  mov dword ptr [r10+PD_S_FIELDS+PD_F_CPU_TIMES*PD_FIELD_SIZE],PD_STATUS_VALID
  mov dword ptr [r10+PD_S_FIELDS+PD_F_IO*PD_FIELD_SIZE],PD_STATUS_VALID
  inc ecx
  mov r10,r11
  cmp ecx,2
  jb pdRatesResetNext
  mov qword ptr [pdRatesTestPrevious+PD_S_TICK],1000
  mov qword ptr [pdRatesTestCurrent+PD_S_TICK],2000
  mov qword ptr [pdRatesTestPrevious+PD_S_KERNEL],100000
  mov qword ptr [pdRatesTestPrevious+PD_S_USER],200000
  mov qword ptr [pdRatesTestCurrent+PD_S_KERNEL],1350000
  mov qword ptr [pdRatesTestCurrent+PD_S_USER],1450000
  mov qword ptr [pdRatesTestPrevious+PD_S_READ_OPS],100
  mov qword ptr [pdRatesTestPrevious+PD_S_WRITE_OPS],200
  mov qword ptr [pdRatesTestPrevious+PD_S_OTHER_OPS],300
  mov qword ptr [pdRatesTestCurrent+PD_S_READ_OPS],101
  mov qword ptr [pdRatesTestCurrent+PD_S_WRITE_OPS],202
  mov qword ptr [pdRatesTestCurrent+PD_S_OTHER_OPS],303
  mov qword ptr [pdRatesTestPrevious+PD_S_READ_BYTES],1000
  mov qword ptr [pdRatesTestPrevious+PD_S_WRITE_BYTES],2000
  mov qword ptr [pdRatesTestPrevious+PD_S_OTHER_BYTES],3000
  mov qword ptr [pdRatesTestCurrent+PD_S_READ_BYTES],2000
  mov qword ptr [pdRatesTestCurrent+PD_S_WRITE_BYTES],4000
  mov qword ptr [pdRatesTestCurrent+PD_S_OTHER_BYTES],7000
  mov pdRatesTestClockSuccess,1
  mov pdRatesTestClockIncrement,156250
  lea rax,PdRatesTestClock
  mov pdRatesClockQuery,rax
  pop rdi
  ret
PdRatesTestReset ENDP

; RAX previous pointer or zero. Copies both snapshots, invokes the real helper,
; then proves all previous bytes and all non-rate current bytes unchanged.
PdRatesTestInvoke PROC
  push rbx
  push rsi
  push rdi
  sub rsp,20h
  mov rbx,rax
  lea rsi,pdRatesTestPrevious
  lea rdi,pdRatesTestPreviousCopy
  mov ecx,PD_SNAPSHOT_SIZE/8
  rep movsq
  lea rsi,pdRatesTestCurrent
  lea rdi,pdRatesTestCurrentCopy
  mov ecx,PD_SNAPSHOT_SIZE/8
  rep movsq
  mov rcx,rbx
  lea rdx,pdRatesTestCurrent
  call PdDeriveRates
  lea rsi,pdRatesTestPrevious
  lea rdi,pdRatesTestPreviousCopy
  mov ecx,PD_SNAPSHOT_SIZE/8
  repe cmpsq
  jne pdRatesInvokeFail
  inc pdRatesTestAssertions
  lea rsi,pdRatesTestCurrent
  lea rdi,pdRatesTestCurrentCopy
  xor ecx,ecx
pdRatesCompareNext:
  cmp ecx,PD_S_FIELDS+PD_F_CPU_RATE*PD_FIELD_SIZE
  jb pdRatesCompareByte
  cmp ecx,PD_S_FIELDS+(PD_F_IO_RATE+1)*PD_FIELD_SIZE
  jb pdRatesCompareSkip
  cmp ecx,PD_S_CPU_BASIS_POINTS
  jb pdRatesCompareByte
  cmp ecx,PD_S_CPU_BASIS_POINTS+4
  jb pdRatesCompareSkip
  cmp ecx,PD_S_READ_BPS
  jb pdRatesCompareByte
  cmp ecx,PD_S_OTHER_BPS+8
  jb pdRatesCompareSkip
pdRatesCompareByte:
  mov al,[rsi+rcx]
  cmp al,[rdi+rcx]
  jne pdRatesInvokeFail
pdRatesCompareSkip:
  inc ecx
  cmp ecx,PD_SNAPSHOT_SIZE
  jb pdRatesCompareNext
  inc pdRatesTestAssertions
  xor eax,eax
  jmp pdRatesInvokeDone
pdRatesInvokeFail:
  mov eax,1
pdRatesInvokeDone:
  add rsp,20h
  pop rdi
  pop rsi
  pop rbx
  ret
PdRatesTestInvoke ENDP

PD_RATES_EXPECT MACRO stageNumber,cpuState,cpuValue,ioState,readRate,writeRate,otherRate,cpuError,ioError,hasPrevious:=<1>
  mov pdRatesTestStage,stageNumber
IF hasPrevious
  lea rax,pdRatesTestPrevious
ELSE
  xor eax,eax
ENDIF
  call PdRatesTestInvoke
  test eax,eax
  jnz pdRatesTestFail
  lea r10,pdRatesTestCurrent
  cmp dword ptr [r10+PD_S_FIELDS+PD_F_CPU_RATE*PD_FIELD_SIZE],cpuState
  jne pdRatesTestFail
  cmp dword ptr [r10+PD_S_FIELDS+PD_F_CPU_RATE*PD_FIELD_SIZE+4],cpuError
  jne pdRatesTestFail
  cmp qword ptr [r10+PD_S_FIELDS+PD_F_CPU_RATE*PD_FIELD_SIZE+8],0
  jne pdRatesTestFail
  cmp dword ptr [r10+PD_S_CPU_BASIS_POINTS],cpuValue
  jne pdRatesTestFail
  cmp dword ptr [r10+PD_S_FIELDS+PD_F_IO_RATE*PD_FIELD_SIZE],ioState
  jne pdRatesTestFail
  cmp dword ptr [r10+PD_S_FIELDS+PD_F_IO_RATE*PD_FIELD_SIZE+4],ioError
  jne pdRatesTestFail
  cmp qword ptr [r10+PD_S_FIELDS+PD_F_IO_RATE*PD_FIELD_SIZE+8],0
  jne pdRatesTestFail
  mov rax,readRate
  cmp [r10+PD_S_READ_BPS],rax
  jne pdRatesTestFail
  mov rax,writeRate
  cmp [r10+PD_S_WRITE_BPS],rax
  jne pdRatesTestFail
  mov rax,otherRate
  cmp [r10+PD_S_OTHER_BPS],rax
  jne pdRatesTestFail
  inc pdRatesTestCases
  add pdRatesTestAssertions,10
ENDM

PdRatesTest PROC
  sub rsp,38h
  mov pdRatesTestCases,0
  mov pdRatesTestAssertions,0
  call PdRatesTestReset
  ; (1.25m kernel + 1.25m user) / (10m wall * 4 CPUs) = 6.25%.
  PD_RATES_EXPECT 6101,1,625,1,1000,2000,4000,0,0
  call PdRatesTestReset
  PD_RATES_EXPECT 6102,2,0,2,0,0,0,21,21,0
  call PdRatesTestReset
  mov qword ptr [pdRatesTestCurrent+PD_S_KERNEL],100000
  mov qword ptr [pdRatesTestCurrent+PD_S_USER],200000
  mov qword ptr [pdRatesTestCurrent+PD_S_READ_BYTES],1000
  mov qword ptr [pdRatesTestCurrent+PD_S_WRITE_BYTES],2000
  mov qword ptr [pdRatesTestCurrent+PD_S_OTHER_BYTES],3000
  PD_RATES_EXPECT 6103,1,0,1,0,0,0,0,0
  call PdRatesTestReset
  ; A process using 1.5 cores on four CPUs is 37.5%, not a one-core error.
  mov qword ptr [pdRatesTestCurrent+PD_S_KERNEL],10100000
  mov qword ptr [pdRatesTestCurrent+PD_S_USER],5200000
  PD_RATES_EXPECT 6104,1,3750,1,1000,2000,4000,0,0
  call PdRatesTestReset
  mov qword ptr [pdRatesTestCurrent+PD_S_KERNEL],20100000
  mov qword ptr [pdRatesTestCurrent+PD_S_USER],20200000
  PD_RATES_EXPECT 6105,1,10000,1,1000,2000,4000,0,0
  call PdRatesTestReset
  mov qword ptr [pdRatesTestCurrent+PD_S_KERNEL],99999
  PD_RATES_EXPECT 6106,2,0,1,1000,2000,4000,13,0
  call PdRatesTestReset
  mov qword ptr [pdRatesTestCurrent+PD_S_USER],199999
  PD_RATES_EXPECT 6107,2,0,1,1000,2000,4000,13,0
  call PdRatesTestReset
  mov qword ptr [pdRatesTestPrevious+PD_S_KERNEL],0
  mov qword ptr [pdRatesTestPrevious+PD_S_USER],0
  mov rax,0FFFFFFFFFFFFFFFFh
  mov qword ptr [pdRatesTestCurrent+PD_S_KERNEL],rax
  mov qword ptr [pdRatesTestCurrent+PD_S_USER],1
  PD_RATES_EXPECT 6108,2,0,1,1000,2000,4000,13,0
  call PdRatesTestReset
  mov dword ptr [pdRatesTestCurrent+PD_S_CAPACITY],8
  PD_RATES_EXPECT 6109,2,0,2,0,0,0,13,13
  call PdRatesTestReset
  mov dword ptr [pdRatesTestCurrent+PD_S_CAPACITY],0
  mov dword ptr [pdRatesTestPrevious+PD_S_CAPACITY],0
  PD_RATES_EXPECT 6110,2,0,2,0,0,0,13,13
  call PdRatesTestReset
  mov qword ptr [pdRatesTestCurrent+PD_S_TICK],1000
  PD_RATES_EXPECT 6111,2,0,2,0,0,0,13,13
  call PdRatesTestReset
  mov qword ptr [pdRatesTestCurrent+PD_S_TICK],999
  PD_RATES_EXPECT 6112,2,0,2,0,0,0,13,13
  call PdRatesTestReset
  mov qword ptr [pdRatesTestCurrent+PD_S_TICK],31001
  PD_RATES_EXPECT 6113,2,0,2,0,0,0,21,21
  call PdRatesTestReset
  mov qword ptr [pdRatesTestCurrent+PD_S_TICK],31000
  PD_RATES_EXPECT 6114,1,20,1,33,66,133,0,0
  call PdRatesTestReset
  mov qword ptr [pdRatesTestCurrent+PD_S_COOKIE],99
  PD_RATES_EXPECT 6115,2,0,2,0,0,0,13,13
  call PdRatesTestReset
  mov qword ptr [pdRatesTestCurrent+PD_S_GENERATION],99
  PD_RATES_EXPECT 6116,2,0,2,0,0,0,13,13
  call PdRatesTestReset
  mov dword ptr [pdRatesTestCurrent+PD_S_PID],99
  PD_RATES_EXPECT 6117,2,0,2,0,0,0,13,13
  call PdRatesTestReset
  mov qword ptr [pdRatesTestCurrent+PD_S_BIRTH],99
  PD_RATES_EXPECT 6118,2,0,2,0,0,0,13,13
  call PdRatesTestReset
  mov qword ptr [pdRatesTestCurrent+PD_S_OBSERVED_BIRTH],99
  PD_RATES_EXPECT 6119,2,0,2,0,0,0,13,13
  call PdRatesTestReset
  mov dword ptr [pdRatesTestCurrent+PD_S_TARGET],PD_TARGET_DENIED
  PD_RATES_EXPECT 6120,2,0,2,0,0,0,21,21
  call PdRatesTestReset
  mov dword ptr [pdRatesTestCurrent+PD_S_FIELDS+PD_F_CPU_TIMES*PD_FIELD_SIZE],PD_STATUS_UNAVAILABLE
  PD_RATES_EXPECT 6121,2,0,1,1000,2000,4000,21,0
  call PdRatesTestReset
  mov dword ptr [pdRatesTestPrevious+PD_S_FIELDS+PD_F_IO*PD_FIELD_SIZE],PD_STATUS_UNAVAILABLE
  PD_RATES_EXPECT 6122,1,625,2,0,0,0,0,21
  call PdRatesTestReset
  mov qword ptr [pdRatesTestCurrent+PD_S_READ_OPS],99
  PD_RATES_EXPECT 6123,1,625,2,0,0,0,0,13
  call PdRatesTestReset
  mov qword ptr [pdRatesTestCurrent+PD_S_READ_BYTES],999
  PD_RATES_EXPECT 6124,1,625,2,0,0,0,0,13
  call PdRatesTestReset
  mov qword ptr [pdRatesTestCurrent+PD_S_WRITE_BYTES],1999
  PD_RATES_EXPECT 6125,1,625,2,0,0,0,0,13
  call PdRatesTestReset
  mov qword ptr [pdRatesTestCurrent+PD_S_OTHER_BYTES],2999
  PD_RATES_EXPECT 6126,1,625,2,0,0,0,0,13
  call PdRatesTestReset
  mov qword ptr [pdRatesTestCurrent+PD_S_KERNEL],100000
  mov qword ptr [pdRatesTestCurrent+PD_S_USER],200000
  mov qword ptr [pdRatesTestCurrent+PD_S_TICK],1001
  mov qword ptr [pdRatesTestPrevious+PD_S_READ_BYTES],0
  mov rax,0FFFFFFFFFFFFFFFFh
  mov qword ptr [pdRatesTestCurrent+PD_S_READ_BYTES],rax
  PD_RATES_EXPECT 6127,1,0,2,0,0,0,0,13
  call PdRatesTestReset
  ; The 64-bit product overflows, but the correct 128-bit quotient fits.
  mov qword ptr [pdRatesTestPrevious+PD_S_READ_BYTES],0
  mov rax,0FFFFFFFFFFFFFFFFh
  mov qword ptr [pdRatesTestCurrent+PD_S_READ_BYTES],rax
  PD_RATES_EXPECT 6128,1,625,1,0FFFFFFFFFFFFFFFFh,2000,4000,0,0
  call PdRatesTestReset
  mov qword ptr [pdRatesTestCurrent+PD_S_KERNEL],100000
  mov qword ptr [pdRatesTestCurrent+PD_S_USER],200000
  mov qword ptr [pdRatesTestCurrent+PD_S_TICK],1003
  mov qword ptr [pdRatesTestCurrent+PD_S_READ_BYTES],1010
  mov qword ptr [pdRatesTestCurrent+PD_S_WRITE_BYTES],2020
  mov qword ptr [pdRatesTestCurrent+PD_S_OTHER_BYTES],3030
  PD_RATES_EXPECT 6129,1,0,1,3333,6666,10000,0,0
  call PdRatesTestReset
  mov dword ptr [pdRatesTestPrevious+PD_S_CAPACITY],1
  mov dword ptr [pdRatesTestCurrent+PD_S_CAPACITY],1
  mov qword ptr [pdRatesTestCurrent+PD_S_KERNEL],10256250
  mov qword ptr [pdRatesTestCurrent+PD_S_USER],200000
  PD_RATES_EXPECT 6130,1,10000,1,1000,2000,4000,0,0
  call PdRatesTestReset
  mov dword ptr [pdRatesTestPrevious+PD_S_CAPACITY],1
  mov dword ptr [pdRatesTestCurrent+PD_S_CAPACITY],1
  mov qword ptr [pdRatesTestCurrent+PD_S_KERNEL],10412500
  mov qword ptr [pdRatesTestCurrent+PD_S_USER],200000
  PD_RATES_EXPECT 6131,1,10000,1,1000,2000,4000,0,0
  inc qword ptr [pdRatesTestCurrent+PD_S_KERNEL]
  PD_RATES_EXPECT 6132,2,0,1,1000,2000,4000,13,0
  call PdRatesTestReset
  mov pdRatesTestClockIncrement,200000
  mov dword ptr [pdRatesTestPrevious+PD_S_CAPACITY],1
  mov dword ptr [pdRatesTestCurrent+PD_S_CAPACITY],1
  mov qword ptr [pdRatesTestCurrent+PD_S_KERNEL],10420000
  mov qword ptr [pdRatesTestCurrent+PD_S_USER],200000
  PD_RATES_EXPECT 6133,1,10000,1,1000,2000,4000,0,0
  inc qword ptr [pdRatesTestCurrent+PD_S_KERNEL]
  PD_RATES_EXPECT 6134,2,0,1,1000,2000,4000,13,0
  call PdRatesTestReset
  mov pdRatesTestClockSuccess,0
  mov dword ptr [pdRatesTestPrevious+PD_S_CAPACITY],1
  mov dword ptr [pdRatesTestCurrent+PD_S_CAPACITY],1
  mov qword ptr [pdRatesTestCurrent+PD_S_KERNEL],10412500
  mov qword ptr [pdRatesTestCurrent+PD_S_USER],200000
  PD_RATES_EXPECT 6135,1,10000,1,1000,2000,4000,0,0
  inc qword ptr [pdRatesTestCurrent+PD_S_KERNEL]
  PD_RATES_EXPECT 6136,2,0,1,1000,2000,4000,13,0
  call PdRatesTestReset
  mov pdRatesTestClockIncrement,0
  mov dword ptr [pdRatesTestPrevious+PD_S_CAPACITY],1
  mov dword ptr [pdRatesTestCurrent+PD_S_CAPACITY],1
  mov qword ptr [pdRatesTestCurrent+PD_S_KERNEL],10412500
  mov qword ptr [pdRatesTestCurrent+PD_S_USER],200000
  PD_RATES_EXPECT 6137,1,10000,1,1000,2000,4000,0,0
  call PdRatesTestReset
  mov pdRatesTestClockIncrement,1000001
  mov dword ptr [pdRatesTestPrevious+PD_S_CAPACITY],1
  mov dword ptr [pdRatesTestCurrent+PD_S_CAPACITY],1
  mov qword ptr [pdRatesTestCurrent+PD_S_KERNEL],10412500
  mov qword ptr [pdRatesTestCurrent+PD_S_USER],200000
  PD_RATES_EXPECT 6138,1,10000,1,1000,2000,4000,0,0
  call PdRatesTestReset
  ; Tolerance scales with all four CPUs at the total-capacity ceiling.
  mov qword ptr [pdRatesTestCurrent+PD_S_KERNEL],41350000
  mov qword ptr [pdRatesTestCurrent+PD_S_USER],200000
  PD_RATES_EXPECT 6139,1,10000,1,1000,2000,4000,0,0
  inc qword ptr [pdRatesTestCurrent+PD_S_KERNEL]
  PD_RATES_EXPECT 6140,2,0,1,1000,2000,4000,13,0
  call PdRatesTestReset
  mov dword ptr [pdRatesTestPrevious+PD_S_CAPACITY],0FFFFFFFFh
  mov dword ptr [pdRatesTestCurrent+PD_S_CAPACITY],0FFFFFFFFh
  mov qword ptr [pdRatesTestCurrent+PD_S_TICK],31000
  mov rax,1288490188500100000
  mov qword ptr [pdRatesTestCurrent+PD_S_KERNEL],rax
  mov qword ptr [pdRatesTestCurrent+PD_S_USER],200000
  PD_RATES_EXPECT 6141,1,10000,1,33,66,133,0,0
  call PdRatesTestReset
  mov dword ptr [pdRatesTestCurrent+PD_S_SIZE],PD_SNAPSHOT_SIZE-1
  PD_RATES_EXPECT 6142,2,0,2,0,0,0,13,13
  call PdRatesTestReset
  mov dword ptr [pdRatesTestCurrent+PD_S_VERSION],2
  PD_RATES_EXPECT 6143,2,0,2,0,0,0,13,13
  call PdRatesTestReset
  mov dword ptr [pdRatesTestCurrent+PD_S_LANE],2
  PD_RATES_EXPECT 6144,2,0,2,0,0,0,13,13
  call PdRatesTestReset
  mov qword ptr [pdRatesTestPrevious+PD_S_COOKIE],0
  mov qword ptr [pdRatesTestCurrent+PD_S_COOKIE],0
  PD_RATES_EXPECT 6145,2,0,2,0,0,0,13,13
  call PdRatesTestReset
  mov qword ptr [pdRatesTestPrevious+PD_S_BIRTH],0
  mov qword ptr [pdRatesTestCurrent+PD_S_BIRTH],0
  PD_RATES_EXPECT 6146,2,0,2,0,0,0,13,13
  call PdRatesTestReset
  mov dword ptr [pdRatesTestPrevious+PD_S_TARGET],PD_TARGET_EXITED
  PD_RATES_EXPECT 6147,2,0,2,0,0,0,21,21
  call PdRatesTestReset
  mov dword ptr [pdRatesTestPrevious+PD_S_FIELDS+PD_F_CPU_TIMES*PD_FIELD_SIZE],PD_STATUS_UNAVAILABLE
  PD_RATES_EXPECT 6148,2,0,1,1000,2000,4000,21,0
  call PdRatesTestReset
  mov dword ptr [pdRatesTestCurrent+PD_S_FIELDS+PD_F_IO*PD_FIELD_SIZE],PD_STATUS_UNAVAILABLE
  PD_RATES_EXPECT 6149,1,625,2,0,0,0,0,21
  call PdRatesTestReset
  mov qword ptr [pdRatesTestCurrent+PD_S_WRITE_OPS],199
  PD_RATES_EXPECT 6150,1,625,2,0,0,0,0,13
  call PdRatesTestReset
  mov qword ptr [pdRatesTestCurrent+PD_S_OTHER_OPS],299
  PD_RATES_EXPECT 6151,1,625,2,0,0,0,0,13
  call PdRatesTestReset
  mov dword ptr [pdRatesTestPrevious+PD_S_VERSION],2
  PD_RATES_EXPECT 6152,2,0,2,0,0,0,13,13
  call PdRatesTestReset
  mov qword ptr [pdRatesTestPrevious+PD_S_GENERATION],0
  mov qword ptr [pdRatesTestCurrent+PD_S_GENERATION],0
  PD_RATES_EXPECT 6153,2,0,2,0,0,0,13,13
  call PdRatesTestReset
  mov dword ptr [pdRatesTestPrevious+PD_S_PID],0
  mov dword ptr [pdRatesTestCurrent+PD_S_PID],0
  PD_RATES_EXPECT 6154,2,0,2,0,0,0,13,13
  call PdRatesTestReset
  mov qword ptr [pdRatesTestPrevious+PD_S_OBSERVED_BIRTH],99
  PD_RATES_EXPECT 6155,2,0,2,0,0,0,13,13
  call PdRatesTestReset
  ; Poll sequence and optional epoch never become a second clock/baseline.
  mov qword ptr [pdRatesTestPrevious+PD_S_SEQUENCE],4
  mov qword ptr [pdRatesTestCurrent+PD_S_SEQUENCE],5
  mov qword ptr [pdRatesTestPrevious+PD_S_EPOCH],1
  mov qword ptr [pdRatesTestCurrent+PD_S_EPOCH],2
  PD_RATES_EXPECT 6156,1,625,1,1000,2000,4000,0,0
  call PdRatesTestReset
  ; A later overflowing source must not publish the already computed read rate.
  mov qword ptr [pdRatesTestCurrent+PD_S_KERNEL],100000
  mov qword ptr [pdRatesTestCurrent+PD_S_USER],200000
  mov qword ptr [pdRatesTestCurrent+PD_S_TICK],1001
  mov qword ptr [pdRatesTestPrevious+PD_S_WRITE_BYTES],0
  mov rax,0FFFFFFFFFFFFFFFFh
  mov qword ptr [pdRatesTestCurrent+PD_S_WRITE_BYTES],rax
  PD_RATES_EXPECT 6157,1,0,2,0,0,0,0,13
  call PdRatesTestReset
  mov qword ptr [pdRatesTestCurrent+PD_S_KERNEL],100000
  mov qword ptr [pdRatesTestCurrent+PD_S_USER],200000
  mov qword ptr [pdRatesTestCurrent+PD_S_TICK],1001
  mov qword ptr [pdRatesTestPrevious+PD_S_OTHER_BYTES],0
  mov rax,0FFFFFFFFFFFFFFFFh
  mov qword ptr [pdRatesTestCurrent+PD_S_OTHER_BYTES],rax
  PD_RATES_EXPECT 6158,1,0,2,0,0,0,0,13
  call PdRatesTestReset
  lea rax,GetSystemTimeAdjustment
  mov pdRatesClockQuery,rax
  mov dword ptr [pdRatesTestPrevious+PD_S_CAPACITY],1
  mov dword ptr [pdRatesTestCurrent+PD_S_CAPACITY],1
  mov qword ptr [pdRatesTestCurrent+PD_S_KERNEL],10100001
  mov qword ptr [pdRatesTestCurrent+PD_S_USER],200000
  PD_RATES_EXPECT 6159,1,10000,1,1000,2000,4000,0,0
  call PdRatesTestReset
  mov qword ptr [pdRatesTestCurrent+PD_S_FIELDS+PD_F_CPU_RATE*PD_FIELD_SIZE],-1
  mov qword ptr [pdRatesTestCurrent+PD_S_FIELDS+PD_F_CPU_RATE*PD_FIELD_SIZE+8],-1
  mov qword ptr [pdRatesTestCurrent+PD_S_FIELDS+PD_F_IO_RATE*PD_FIELD_SIZE],-1
  mov qword ptr [pdRatesTestCurrent+PD_S_FIELDS+PD_F_IO_RATE*PD_FIELD_SIZE+8],-1
  mov dword ptr [pdRatesTestCurrent+PD_S_CPU_BASIS_POINTS],-1
  mov qword ptr [pdRatesTestCurrent+PD_S_READ_BPS],-1
  mov qword ptr [pdRatesTestCurrent+PD_S_WRITE_BPS],-1
  mov qword ptr [pdRatesTestCurrent+PD_S_OTHER_BPS],-1
  PD_RATES_EXPECT 6160,1,625,1,1000,2000,4000,0,0
  call PdRatesTestReset
  mov qword ptr [pdRatesTestCurrent+PD_S_TICK],1001
  mov qword ptr [pdRatesTestCurrent+PD_S_KERNEL],140000
  mov qword ptr [pdRatesTestCurrent+PD_S_USER],200000
  PD_RATES_EXPECT 6161,1,10000,1,1000000,2000000,4000000,0,0
  call PdRatesTestReset
  mov qword ptr [pdRatesTestPrevious+PD_S_TICK],-1
  mov qword ptr [pdRatesTestCurrent+PD_S_TICK],1
  PD_RATES_EXPECT 6162,2,0,2,0,0,0,13,13
  mov pdRatesTestStage,0
  jmp pdRatesTestReport
pdRatesTestFail:
pdRatesTestReport:
  lea rcx,pdRatesTestOutput
  lea rdx,pdRatesTestFormat
  mov r8d,pdRatesTestCases
  mov r9d,pdRatesTestAssertions
  mov eax,pdRatesTestStage
  mov [rsp+20h],rax
  call wsprintfA
  mov [rsp+28h],eax
  mov ecx,-11
  call GetStdHandle
  mov rcx,rax
  lea rdx,pdRatesTestOutput
  mov r8d,[rsp+28h]
  lea r9,pdRatesTestWritten
  mov qword ptr [rsp+20h],0
  call WriteFile
  mov ecx,pdRatesTestStage
  call ExitProcess
PdRatesTest ENDP
