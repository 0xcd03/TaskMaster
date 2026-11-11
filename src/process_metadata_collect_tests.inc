; Owned disposable child fixtures; PD helper test fixtures provide independent oracles.
.data?
align 8
pmCollectCachePointer dq ?
pmCollectExpected dw PD_PATH_CAPACITY dup(?)
.code
PMCT_CHECK32 MACRO value,expected,stage
  mov pdTestStage,stage
  mov r10d,value
  cmp r10d,expected
  jne pmctFinish
  inc pdTestAssertions
ENDM
PmCollectorTest PROC
  push rbx
  push rsi
  push rdi
  sub rsp,60h
  call GetCurrentProcessId
  mov r8d,eax
  lea rcx,pdTestReadyName
  lea rdx,pdTestReadyFmt
  call wsprintfW
  call GetCurrentProcessId
  mov r8d,eax
  lea rcx,pdTestStopName
  lea rdx,pdTestStopFmt
  call wsprintfW
  xor ecx,ecx
  mov edx,1
  xor r8d,r8d
  lea r9,pdTestReadyName
  call CreateEventW
  mov pdTestReady,rax
  test rax,rax
  jz pmctFinish
  xor ecx,ecx
  mov edx,1
  xor r8d,r8d
  lea r9,pdTestStopName
  call CreateEventW
  mov pdTestStop,rax
  test rax,rax
  jz pmctFinish
  mov ecx,32768
  lea rdx,pdTestCwd
  call GetCurrentDirectoryW
  test eax,eax
  jz pmctFinish
  lea rcx,pdTestExe
  lea rdx,pdTestExeFmt
  lea r8,pdTestCwd
  call wsprintfW
  lea rcx,pdTestCommand
  lea rdx,pdTestCmdFmt
  lea r8,pdTestExe
  lea r9,pdTestReadyName
  lea rax,pdTestStopName
  mov [rsp+20h],rax
  call wsprintfW
  mov dword ptr [pdTestSi],104
  lea rcx,pdTestExe
  lea rdx,pdTestCommand
  xor r8d,r8d
  xor r9d,r9d
  mov qword ptr [rsp+20h],0
  mov qword ptr [rsp+28h],0
  mov qword ptr [rsp+30h],0
  mov qword ptr [rsp+38h],0
  lea rax,pdTestSi
  mov [rsp+40h],rax
  lea rax,pdTestPi
  mov [rsp+48h],rax
  call CreateProcessW
  test eax,eax
  jz pmctFinish
  mov rcx,pdTestReady
  mov edx,5000
  call WaitForSingleObject
  PMCT_CHECK32 eax,0,7101
  mov rcx,qword ptr [pdTestPi]
  lea rdx,pdTestRefTimes
  lea r8,pdTestRefTimes+8
  lea r9,pdTestRefTimes+16
  lea rax,pdTestRefTimes+24
  mov [rsp+20h],rax
  call GetProcessTimes
  PMCT_CHECK32 eax,1,7102
  mov dword ptr [pdTestRequest+PD_R_SIZE],PD_REQUEST_SIZE
  mov dword ptr [pdTestRequest+PD_R_VERSION],PD_VERSION
  mov dword ptr [pdTestRequest+PD_R_LANE],1
  mov qword ptr [pdTestRequest+PD_R_COOKIE],111
  mov qword ptr [pdTestRequest+PD_R_GENERATION],222
  mov qword ptr [pdTestRequest+PD_R_SEQUENCE],333
  mov qword ptr [pdTestRequest+PD_R_EPOCH],444
  mov eax,dword ptr [pdTestPi+16]
  mov dword ptr [pdTestRequest+PD_R_PID],eax
  mov rax,qword ptr [pdTestRefTimes]
  mov qword ptr [pdTestRequest+PD_R_BIRTH],rax
  mov qword ptr [pdTestRequest+PD_R_NOTIFY],1
  call GetTickCount64
  add rax,3000
  mov qword ptr [pdTestRequest+PD_R_DEADLINE],rax
  ; Admit real source requests through the actual cache ABI, without altering
  ; processor-count capacity or observed birth in the resulting snapshots.
  call PmCacheReset
  mov ecx,PM_MASK_ALL
  mov edx,1
  call PmCacheSetDemand
  PMCT_CHECK32 eax,0,9330
  mov ecx,dword ptr [pdTestPi+16]
  mov rdx,qword ptr [pdTestRefTimes]
  call PmCacheEnsure
  mov pdTestStage,9331
  test rax,rax
  jz pmctFinish
  inc pdTestAssertions
  mov ecx,PM_LANE_PATH
  call PmCollectorPrepare
  PMCT_CHECK32 eax,0,9332
  call PmTestCollect
  PMCT_CHECK32 eax,0,9301
  PMCT_CHECK32 dword ptr [pdTestSnapshot+PD_S_TARGET],PD_TARGET_VERIFIED,9302
  PMCT_CHECK32 dword ptr [pdTestSnapshot+PD_S_FIELDS+PD_F_PATH*16],PD_STATUS_VALID,9303
  lea rcx,pdTestExe
  lea rdx,pdTestSnapshot+PD_S_PATH
  call lstrcmpW
  PMCT_CHECK32 eax,0,9304
  call PmCollectorRoundtrip
  PMCT_CHECK32 eax,0,9333
  mov ecx,PM_LANE_ACCOUNT
  call PmCollectorPrepare
  PMCT_CHECK32 eax,0,9334
  call PmTestCollect
  PMCT_CHECK32 eax,0,9305
  PMCT_CHECK32 dword ptr [pdTestSnapshot+PD_S_FIELDS+PD_F_ACCOUNT*16],PD_STATUS_VALID,9306
  PMCT_CHECK32 dword ptr [pdTestSnapshot+PD_S_FIELDS+PD_F_COMMAND*16],PD_STATUS_NOT_REQUESTED,9307
  ; Independent current token account name must match the owned child's user.
  lea rcx,pdOptTestUser
  lea rdx,pdOptTestUserLength
  call GetUserNameW
  PMCT_CHECK32 eax,1,9308
  lea rcx,pdTestSnapshot+PD_S_ACCOUNT
  lea rdx,pdOptTestUser
  call StrStrIW
  mov pdTestStage,9309
  test rax,rax
  jz pmctFinish
  inc pdTestAssertions
  call PmCollectorRoundtrip
  PMCT_CHECK32 eax,0,9335
  inc qword ptr [pdTestRequest+PD_R_BIRTH]
  call PmTestCollect
  PMCT_CHECK32 eax,0,9310
  PMCT_CHECK32 dword ptr [pdTestSnapshot+PD_S_TARGET],PD_TARGET_MISMATCH,9311
  PMCT_CHECK32 dword ptr [pdTestSnapshot+PD_S_FIELDS+PD_F_ACCOUNT*16],PD_STATUS_UNAVAILABLE,9312
  dec qword ptr [pdTestRequest+PD_R_BIRTH]
  mov qword ptr [pdTestRequest+PD_R_DEADLINE],1
  call PmTestCollect
  PMCT_CHECK32 eax,1460,9313
  mov ecx,PM_LANE_COMMAND
  call PmCollectorPrepare
  PMCT_CHECK32 eax,0,9336
  call PmTestCollect
  PMCT_CHECK32 eax,0,9314
  PMCT_CHECK32 dword ptr [pdTestSnapshot+PD_S_FIELDS+PD_F_COMMAND*16],PD_STATUS_VALID,9315
  lea rcx,pdTestCommand
  lea rdx,pdTestSnapshot+PD_S_COMMAND
  call lstrcmpW
  PMCT_CHECK32 eax,0,9316
  PMCT_CHECK32 dword ptr [pdTestSnapshot+PD_S_FIELDS+PD_F_ACCOUNT*16],PD_STATUS_NOT_REQUESTED,9317
  call PmCollectorRoundtrip
  PMCT_CHECK32 eax,0,9337
  ; Independent deterministic COM boundary tests preserve real WMI validation.
  lea rsi,pdTestRequest
  lea rdi,pdOptTestRequest
  mov ecx,PD_REQUEST_SIZE/8
  rep movsq
  call PdOptTestInstall
  mov rax,qword ptr [pdTestRequest+PD_R_BIRTH]
  mov pdOptTestTimes,rax
  call PdOptTestFormatBirth
  mov qword ptr [pdTestRequest+PD_R_DEADLINE],4000
  mov pdOptTestMode,4
  call PmTestCollect
  PMCT_CHECK32 eax,0,9318
  PMCT_CHECK32 dword ptr [pdTestSnapshot+PD_S_FIELDS+PD_F_COMMAND*16],PD_STATUS_VALID,9319
  PMCT_CHECK32 dword ptr [pdTestSnapshot+PD_S_FIELDS+PD_F_COMMAND*16+8],0,9320
  mov pdOptTestMode,5
  call PmTestCollect
  PMCT_CHECK32 dword ptr [pdTestSnapshot+PD_S_FIELDS+PD_F_COMMAND*16],PD_STATUS_UNAVAILABLE,9321
  call PdOptTestRestore
  ; An unchanged held identity with no SYNCHRONIZE right still permits the
  ; independently resolved account. Cache admission must preserve that policy.
  call PmCacheReset
  mov ecx,PM_MASK_ALL
  mov edx,1
  call PmCacheSetDemand
  mov ecx,dword ptr [pdTestPi+16]
  mov rdx,qword ptr [pdTestRefTimes]
  call PmCacheEnsure
  mov ecx,PM_LANE_ACCOUNT
  call PmCollectorPrepare
  PMCT_CHECK32 eax,0,9338
  call PdOptTestInstall
  mov pdOptTestMode,20
  call PmTestCollect
  PMCT_CHECK32 eax,0,9339
  PMCT_CHECK32 dword ptr [pdTestSnapshot+PD_S_TARGET],PD_TARGET_UNKNOWN,9340
  PMCT_CHECK32 dword ptr [pdTestSnapshot+PD_S_FIELDS+PD_F_IDENTITY*16],PD_STATUS_VALID,9341
  PMCT_CHECK32 dword ptr [pdTestSnapshot+PD_S_FIELDS+PD_F_ACCOUNT*16],PD_STATUS_VALID,9342
  call PmCollectorRoundtrip
  PMCT_CHECK32 eax,0,9343
  call PdOptTestRestore
  mov pdTestStage,0
pmctFinish:
  call PdOptTestRestore
  call PmCacheReset
  lea rax,OpenProcess
  mov pdNativeOpenProcess,rax
  lea rax,GetProcessMemoryInfo
  mov pdNativeMemoryInfo,rax
  mov rcx,pdTestSid
  test rcx,rcx
  jz pmctTokenClose
  call LocalFree
pmctTokenClose:
  mov rcx,pdTestToken
  test rcx,rcx
  jz pmctSignalStop
  call CloseHandle
pmctSignalStop:
  mov rcx,pdTestStop
  test rcx,rcx
  jz pmctCloseChild
  call SetEvent
pmctCloseChild:
  mov rcx,qword ptr [pdTestPi]
  test rcx,rcx
  jz pmctEvents
  mov edx,5000
  call WaitForSingleObject
  mov rcx,qword ptr [pdTestPi]
  call CloseHandle
  mov rcx,qword ptr [pdTestPi+8]
  call CloseHandle
pmctEvents:
  mov rcx,pdTestReady
  test rcx,rcx
  jz pmctStopClose
  call CloseHandle
pmctStopClose:
  mov rcx,pdTestStop
  test rcx,rcx
  jz pmctReport
  call CloseHandle
pmctReport:
  lea rcx,pdTestOutput
  lea rdx,pdTestFormat
  mov r8d,pdTestStage
  mov r9d,pdTestAssertions
  call wsprintfA
  mov ebx,eax
  mov ecx,-11
  call GetStdHandle
  mov rcx,rax
  lea rdx,pdTestOutput
  mov r8d,ebx
  lea r9,pdTestWritten
  mov qword ptr [rsp+20h],0
  call WriteFile
  mov ecx,pdTestStage
  call ExitProcess
PmCollectorTest ENDP
PmTestCollect PROC
  lea rcx,pdTestRequest
  lea rdx,pdTestSnapshot
  mov r8d,PD_SNAPSHOT_SIZE
  jmp PmCollectField
PmTestCollect ENDP

; UI admission prepares the exact immutable request consumed by the source.
PmCollectorPrepare PROC
  push rbx
  sub rsp,20h
  mov ebx,ecx
  call GetTickCount64
  mov rcx,rax
  call PmCacheSetClock
  mov ecx,dword ptr [pdTestPi+16]
  mov rdx,qword ptr [pdTestRefTimes]
  mov r8d,ebx
  lea r9,pdTestRequest
  call PmCachePrepare
  add rsp,20h
  pop rbx
  ret
PmCollectorPrepare ENDP

; Complete a real source result, then erase its entire scratch snapshot and
; verify Get still returns the same independently owned immutable string.
PmCollectorRoundtrip PROC
  push rbx
  push rsi
  push rdi
  sub rsp,20h
  lea rdx,pdTestSnapshot+PD_S_PATH
  mov eax,dword ptr [pdTestRequest+PD_R_LANE]
  cmp eax,PM_LANE_PATH
  je pmcrtCopy
  lea rdx,pdTestSnapshot+PD_S_ACCOUNT
  cmp eax,PM_LANE_ACCOUNT
  je pmcrtCopy
  lea rdx,pdTestSnapshot+PD_S_COMMAND
pmcrtCopy:
  lea rcx,pmCollectExpected
  call lstrcpyW
  call GetTickCount64
  mov rcx,rax
  call PmCacheSetClock
  lea rcx,pdTestRequest
  lea rdx,pdTestSnapshot
  xor r8d,r8d
  call PmCacheComplete
  test eax,eax
  jnz pmcrtDone
  call PmCollectorGet
  mov ebx,1
  cmp edx,PM_STATUS_VALID
  jne pmcrtFail
  test rax,rax
  jz pmcrtFail
  mov pmCollectCachePointer,rax
  mov rcx,rax
  lea rdx,pmCollectExpected
  call lstrcmpW
  mov ebx,2
  test eax,eax
  jnz pmcrtFail
  lea rdi,pdTestSnapshot
  mov ecx,PD_SNAPSHOT_SIZE/8
  xor eax,eax
  rep stosq
  call PmCollectorGet
  mov ebx,3
  cmp edx,PM_STATUS_VALID
  jne pmcrtFail
  cmp rax,pmCollectCachePointer
  jne pmcrtFail
  mov rcx,rax
  lea rdx,pmCollectExpected
  call lstrcmpW
  mov ebx,4
  test eax,eax
  jnz pmcrtFail
  xor eax,eax
  jmp pmcrtDone
pmcrtFail:
  mov eax,ebx
pmcrtDone:
  add rsp,20h
  pop rdi
  pop rsi
  pop rbx
  ret
PmCollectorRoundtrip ENDP
PmCollectorGet PROC
  mov ecx,dword ptr [pdTestPi+16]
  mov rdx,qword ptr [pdTestRefTimes]
  mov r8d,dword ptr [pdTestRequest+PD_R_LANE]
  jmp PmCacheGet
PmCollectorGet ENDP
