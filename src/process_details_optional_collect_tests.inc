; Isolated optional-provider fixtures. No user processes/settings are modified.
.data
align 8
pdOptTestStage dd 8100
pdOptTestAssertions dd 0
pdOptTestWritten dd 0
pdOptTestFormat db 'Optional collector: exit=%u assertions=%u liveWmi=%u liveAccount=%u',13,10,0
align 4
pdOptTestLive dd 0
pdOptTestLiveAccount dd 0
pdOptTestUserLength dd 256
pdOptTestMode dd 0
pdOptTestRefs dd 0
pdOptTestBad dd 0
pdOptTestOpenCalls dd 0
pdOptTestFactoryCalls dd 0
pdOptTestNextMs dd 0
align 8
pdOptTestTick dq 1000
pdOptTestGuardValue dq 1122334455667788h
pdOptTestParsed dq 0
pdOptTestComOwned dd 0
pdOptTestLiveFormat db 'SKIP: live optional WMI: status=%u error=%08X flags=%u',13,10,0
pdOptTestAccountFormat db 'SKIP: live optional account: status=%u error=%08X flags=%u',13,10,0
align 2
pdOptTestNamespace dw 'R','O','O','T','\','C','I','M','V','2',0
pdOptTestWql dw 'W','Q','L',0
pdOptTestPidName dw 'P','r','o','c','e','s','s','I','d',0
pdOptTestBirthName dw 'C','r','e','a','t','i','o','n','D','a','t','e',0
pdOptTestCommandName dw 'C','o','m','m','a','n','d','L','i','n','e',0
; Independent contract literal: never derive an expected query from production.
pdOptTestQuery dw 'S','E','L','E','C','T',' ','P','r','o','c','e','s','s','I','d',','
dw 'C','r','e','a','t','i','o','n','D','a','t','e',',','C','o','m','m','a','n','d','L','i','n','e',' '
dw 'F','R','O','M',' ','W','i','n','3','2','_','P','r','o','c','e','s','s',' '
dw 'W','H','E','R','E',' ','P','r','o','c','e','s','s','I','d','=','%','u',0
pdOptTestAccount dw 'f','i','x','t','u','r','e',0
pdOptTestDomain dw 'D','O','M','A','I','N',0
pdOptTestOwner dw 'D','O','M','A','I','N','\','f','i','x','t','u','r','e',0
pdOptTestCommand dw '"','o','w','n','e','d',' ','f','i','x','t','u','r','e','"',' ',1055,1088,1080,1074,1077,1090,' ','\','p','a','t','h',0
pdOptTestEmpty dw 0
pdOptTestEmbedded dw 'a',0,'b',0
pdOptTestDateFormat dw '%','0','4','u','%','0','2','u','%','0','2','u','%','0','2','u','%','0','2','u','%','0','2','u','.','%','0','6','u','+','0','0','0',0
pdOptTestUtc dw '2','0','2','6','1','1','1','0','2','1','0','1','0','2','.','1','2','3','4','5','6','+','0','0','0',0
pdOptTestPlus dw '2','0','2','6','1','1','1','1','0','0','0','1','0','2','.','1','2','3','4','5','6','+','1','8','0',0
pdOptTestMinus dw '2','0','2','6','1','1','1','0','2','0','0','1','0','2','.','1','2','3','4','5','6','-','0','6','0',0
pdOptTestWildcard dw '2','0','2','6','1','1','1','1','0','0','0','1','0','2','.','*','*','*','*','*','*','+','1','8','0',0
align 8
pdOptTestLocatorVtbl dq PdOptTestUnexpected,PdOptTestUnexpected,PdOptTestRelease,PdOptTestConnect
pdOptTestServiceVtbl dq PdOptTestUnexpected,PdOptTestUnexpected,PdOptTestRelease,17 dup(PdOptTestUnexpected),PdOptTestExec
pdOptTestEnumVtbl dq PdOptTestUnexpected,PdOptTestUnexpected,PdOptTestRelease,PdOptTestUnexpected,PdOptTestNext
pdOptTestObjectVtbl dq PdOptTestUnexpected,PdOptTestUnexpected,PdOptTestRelease,PdOptTestUnexpected,PdOptTestGet
pdOptTestLocator dq pdOptTestLocatorVtbl
pdOptTestService dq pdOptTestServiceVtbl
pdOptTestEnum dq pdOptTestEnumVtbl
pdOptTestObject dq pdOptTestObjectVtbl
.data?
align 8
pdOptTestRequest db PD_REQUEST_SIZE dup(?)
pdOptTestRequestBefore db PD_REQUEST_SIZE dup(?)
pdOptTestSnapshot db PD_SNAPSHOT_SIZE dup(?)
pdOptTestGuard dq ?
pdOptTestTimes dq 4 dup(?)
pdOptTestOutput db 192 dup(?)
pdOptTestCreation dw 32 dup(?)
pdOptTestWrongCreation dw 32 dup(?)
pdOptTestSystemTime dw 8 dup(?)
pdOptTestExpectedQuery dw 128 dup(?)
pdOptTestLarge dw 32768 dup(?)
pdOptTestUser dw 256 dup(?)
.code
extern SysAllocStringLen:proc
extern GetUserNameW:proc
POT_CHECK32 MACRO value,expected,stage
  mov pdOptTestStage,stage
  mov r10d,value
  cmp r10d,expected
  jne potReport
  inc pdOptTestAssertions
ENDM
POT_CASE MACRO mode,status,error,flags
  mov pdOptTestMode,mode
  call PdOptTestReset
  call PdOptTestCollect
  POT_CHECK32 eax,0,8200+mode
  POT_CHECK32 dword ptr [pdOptTestSnapshot+PD_S_FIELDS+PD_F_COMMAND*16],status,8300+mode
  POT_CHECK32 dword ptr [pdOptTestSnapshot+PD_S_FIELDS+PD_F_COMMAND*16+4],error,8400+mode
  POT_CHECK32 dword ptr [pdOptTestSnapshot+PD_S_FIELDS+PD_F_COMMAND*16+12],flags,8500+mode
  POT_CHECK32 pdOptTestRefs,0,8600+mode
  POT_CHECK32 pdOptTestBad,0,8700+mode
ENDM
PdOptionalCollectorTest PROC
  push rbx
  push rsi
  push rdi
  sub rsp,60h
  mov dword ptr [pdOptTestRequest+PD_R_SIZE],PD_REQUEST_SIZE
  mov dword ptr [pdOptTestRequest+PD_R_VERSION],PD_VERSION
  mov dword ptr [pdOptTestRequest+PD_R_LANE],2
  mov qword ptr [pdOptTestRequest+PD_R_COOKIE],101
  mov qword ptr [pdOptTestRequest+PD_R_GENERATION],102
  mov qword ptr [pdOptTestRequest+PD_R_EPOCH],103
  call GetCurrentProcessId
  mov dword ptr [pdOptTestRequest+PD_R_PID],eax
  call GetCurrentProcess
  mov rcx,rax
  lea rdx,pdOptTestTimes
  lea r8,pdOptTestTimes+8
  lea r9,pdOptTestTimes+16
  lea rax,pdOptTestTimes+24
  mov [rsp+20h],rax
  call GetProcessTimes
  test eax,eax
  jz potReport
  mov rax,pdOptTestTimes
  mov qword ptr [pdOptTestRequest+PD_R_BIRTH],rax
  call GetTickCount64
  add rax,3000
  mov qword ptr [pdOptTestRequest+PD_R_DEADLINE],rax
  ; Deterministic provider objects keep real COM init/VariantClear/validation.
  IFDEF PD_OPTIONAL_ENGINE_READY
    call PdOptTestInstall
    call PdOptTestFormatBirth
    call PdOptTestReset
  ENDIF
  lea rcx,pdOptTestRequest
  lea rdx,pdOptTestSnapshot
  mov r8d,PD_SNAPSHOT_SIZE
  call PdCollectOptional
  mov pdOptTestStage,8101
  test eax,eax
  jnz potReport
  inc pdOptTestAssertions
  IFDEF PD_OPTIONAL_ENGINE_READY
    POT_CHECK32 dword ptr [pdOptTestSnapshot+PD_S_TARGET],PD_TARGET_VERIFIED,8102
    POT_CHECK32 dword ptr [pdOptTestSnapshot+PD_S_FIELDS+PD_F_ACCOUNT*16],PD_STATUS_VALID,8103
    POT_CHECK32 dword ptr [pdOptTestSnapshot+PD_S_FIELDS+PD_F_COMMAND*16],PD_STATUS_VALID,8104
    lea rcx,pdOptTestSnapshot+PD_S_ACCOUNT
    lea rdx,pdOptTestOwner
    call lstrcmpW
    POT_CHECK32 eax,0,8105
    lea rcx,pdOptTestSnapshot+PD_S_COMMAND
    lea rdx,pdOptTestCommand
    call lstrcmpW
    POT_CHECK32 eax,0,8106
    POT_CHECK32 pdOptTestRefs,0,8107
    POT_CHECK32 pdOptTestBad,0,8108
    lea rcx,pdOptTestUtc
    mov edx,25
    lea r8,pdOptTestParsed
    call PdOptionalParseDmtf
    POT_CHECK32 eax,0,8110
    mov rax,134388180621234560
    mov pdOptTestStage,8111
    cmp pdOptTestParsed,rax
    jne potReport
    inc pdOptTestAssertions
    lea rcx,pdOptTestPlus
    mov edx,25
    lea r8,pdOptTestParsed
    call PdOptionalParseDmtf
    POT_CHECK32 eax,0,8112
    mov rax,134388180621234560
    mov pdOptTestStage,8113
    cmp pdOptTestParsed,rax
    jne potReport
    inc pdOptTestAssertions
    lea rcx,pdOptTestMinus
    mov edx,25
    lea r8,pdOptTestParsed
    call PdOptionalParseDmtf
    POT_CHECK32 eax,0,8114
    mov rax,134388180621234560
    mov pdOptTestStage,8115
    cmp pdOptTestParsed,rax
    jne potReport
    inc pdOptTestAssertions
    lea rcx,pdOptTestWildcard
    mov edx,25
    lea r8,pdOptTestParsed
    call PdOptionalParseDmtf
    POT_CHECK32 eax,13,8116
    ; At most 32766 characters plus NUL fit the immutable command field.
    lea rdi,pdOptTestLarge
    mov ecx,32767
    mov ax,'x'
    rep stosw
    POT_CASE 1,2,13,0
    POT_CASE 2,2,13,0
    POT_CASE 3,2,1168,0
    POT_CASE 4,1,0,0
    POT_CHECK32 dword ptr [pdOptTestSnapshot+PD_S_FIELDS+PD_F_COMMAND*16+8],0,8117
    POT_CASE 5,2,122,2
    POT_CHECK32 dword ptr [pdOptTestSnapshot+PD_S_FIELDS+PD_F_COMMAND*16+8],0,8118
    POT_CASE 6,2,13,0
    POT_CASE 7,2,80004005h,1
    POT_CASE 8,2,80070005h,1
    POT_CASE 9,2,80004002h,1
    POT_CASE 10,2,80041010h,1
    POT_CASE 11,2,80070005h,1
    POT_CASE 12,3,40004h,1
    POT_CASE 13,1,0,0
    POT_CASE 14,2,80070005h,1
    POT_CASE 15,3,1460,0
    POT_CHECK32 dword ptr [pdOptTestSnapshot+PD_S_FIELDS+PD_F_ACCOUNT*16],PD_STATUS_TIMEOUT,8119
    POT_CHECK32 pdOptTestFactoryCalls,0,8120
    POT_CASE 16,3,1460,0
    POT_CASE 17,3,1460,0
    POT_CASE 18,1,0,0
    POT_CHECK32 dword ptr [pdOptTestSnapshot+PD_S_FIELDS+PD_F_ACCOUNT*16+4],1332,8121
    POT_CASE 19,1,0,0
    POT_CHECK32 dword ptr [pdOptTestSnapshot+PD_S_FIELDS+PD_F_ACCOUNT*16+12],2,8122
    POT_CASE 20,2,50,0
    POT_CHECK32 dword ptr [pdOptTestSnapshot+PD_S_TARGET],PD_TARGET_UNKNOWN,8123
    POT_CHECK32 dword ptr [pdOptTestSnapshot+PD_S_FIELDS+PD_F_ACCOUNT*16],PD_STATUS_VALID,8124
    POT_CHECK32 pdOptTestFactoryCalls,0,8125
    POT_CASE 21,2,5,0
    POT_CHECK32 dword ptr [pdOptTestSnapshot+PD_S_TARGET],PD_TARGET_DENIED,8126
    POT_CASE 22,1,0,0
    POT_CHECK32 pdOptTestNextMs,100,8127
    POT_CASE 23,2,13,0
    POT_CASE 24,2,13,0
    POT_CASE 25,2,13,0
    POT_CASE 26,1,0,0
    POT_CHECK32 dword ptr [pdOptTestSnapshot+PD_S_FIELDS+PD_F_COMMAND*16+8],32766,8128
    movzx eax,word ptr [pdOptTestSnapshot+PD_S_COMMAND+65532]
    POT_CHECK32 eax,0,8129
    POT_CASE 27,2,80070005h,1
    POT_CASE 28,2,80004005h,1
    POT_CASE 29,1,0,0
    POT_CASE 30,1,0,0
    POT_CASE 31,2,80041002h,1
    POT_CASE 32,1,0,0
    POT_CHECK32 dword ptr [pdOptTestSnapshot+PD_S_FIELDS+PD_F_ACCOUNT*16+4],5,8130
    POT_CASE 33,2,13,0
    POT_CASE 34,2,13,0
    ; Structural failures must not alter caller memory.
    mov pdOptTestMode,0
    call PdOptTestReset
    mov rax,pdOptTestGuardValue
    mov qword ptr [pdOptTestSnapshot],rax
    lea rcx,pdOptTestRequest
    lea rdx,pdOptTestSnapshot
    mov r8d,PD_SNAPSHOT_SIZE-1
    call PdCollectOptional
    POT_CHECK32 eax,122,8131
    mov rax,pdOptTestGuardValue
    mov pdOptTestStage,8132
    cmp qword ptr [pdOptTestSnapshot],rax
    jne potReport
    inc pdOptTestAssertions
    mov dword ptr [pdOptTestRequest+PD_R_LANE],1
    call PdOptTestCollect
    POT_CHECK32 eax,50,8133
    mov dword ptr [pdOptTestRequest+PD_R_LANE],2
    mov dword ptr [pdOptTestRequest+PD_R_FLAGS],1
    call PdOptTestCollect
    POT_CHECK32 eax,87,8134
    mov dword ptr [pdOptTestRequest+PD_R_FLAGS],0
    mov dword ptr [pdOptTestRequest+PD_R_RESERVED],1
    call PdOptTestCollect
    POT_CHECK32 eax,87,8135
    mov dword ptr [pdOptTestRequest+PD_R_RESERVED],0
    mov qword ptr [pdOptTestRequest+PD_R_CONTEXT],1
    call PdOptTestCollect
    POT_CHECK32 eax,87,8136
    mov qword ptr [pdOptTestRequest+PD_R_CONTEXT],0
    mov qword ptr [pdOptTestRequest+PD_R_EPOCH],0
    call PdOptTestCollect
    POT_CHECK32 eax,87,8137
    mov qword ptr [pdOptTestRequest+PD_R_EPOCH],103
    mov qword ptr [pdOptTestRequest+PD_R_BIRTH],0
    call PdOptTestCollect
    POT_CHECK32 eax,0,8138
    POT_CHECK32 dword ptr [pdOptTestSnapshot+PD_S_TARGET],PD_TARGET_MISMATCH,8139
    POT_CHECK32 pdOptTestOpenCalls,0,8140
    call PdOptTestReset
    inc qword ptr [pdOptTestRequest+PD_R_BIRTH]
    call PdOptTestCollect
    POT_CHECK32 eax,0,8141
    POT_CHECK32 dword ptr [pdOptTestSnapshot+PD_S_TARGET],PD_TARGET_MISMATCH,8142
    POT_CHECK32 pdOptTestFactoryCalls,0,8143
    ; Both COM-owned S_FALSE and caller-owned STA survive a collector call.
    call PdOptTestReset
    xor ecx,ecx
    xor edx,edx
    call CoInitializeEx
    POT_CHECK32 eax,0,8144
    call PdOptTestCollect
    POT_CHECK32 eax,0,8145
    call CoUninitialize
    xor ecx,ecx
    mov edx,2
    call CoInitializeEx
    POT_CHECK32 eax,0,8146
    call PdOptTestCollect
    POT_CHECK32 eax,0,8147
    xor ecx,ecx
    mov edx,2
    call CoInitializeEx
    POT_CHECK32 eax,1,8148
    call CoUninitialize
    call CoUninitialize
    xor ecx,ecx
    xor edx,edx
    call CoInitializeEx
    POT_CHECK32 eax,0,8149
    call CoUninitialize
    ; Live optional coverage reads only this controlled runner child.
    call PdOptTestRestore
    call GetTickCount64
    add rax,3000
    mov qword ptr [pdOptTestRequest+PD_R_DEADLINE],rax
    call PdOptTestCollect
    POT_CHECK32 eax,0,8150
    POT_CHECK32 dword ptr [pdOptTestSnapshot+PD_S_TARGET],PD_TARGET_VERIFIED,8158
    POT_CHECK32 dword ptr [pdOptTestSnapshot+PD_S_FIELDS+PD_F_IDENTITY*16],PD_STATUS_VALID,8159
    cmp dword ptr [pdOptTestSnapshot+PD_S_FIELDS+PD_F_COMMAND*16],PD_STATUS_VALID
    jne potLiveSkip
    call GetCommandLineW
    mov rdx,rax
    lea rcx,pdOptTestSnapshot+PD_S_COMMAND
    call lstrcmpW
    POT_CHECK32 eax,0,8151
    call GetCommandLineW
    mov rcx,rax
    call lstrlenW
    POT_CHECK32 eax,dword ptr [pdOptTestSnapshot+PD_S_FIELDS+PD_F_COMMAND*16+8],8160
    mov pdOptTestLive,1
    inc pdOptTestAssertions
    jmp potLiveDone
potLiveSkip:
    ; Invalid data/query is a regression, not missing provider coverage.
    mov eax,dword ptr [pdOptTestSnapshot+PD_S_FIELDS+PD_F_COMMAND*16+4]
    mov pdOptTestStage,8161
    test eax,eax
    jz potReport
    cmp eax,13
    je potReport
    cmp eax,122
    je potReport
    cmp eax,87
    je potReport
    cmp eax,80041017h ; WBEM_E_INVALID_QUERY
    je potReport
    cmp eax,80041008h ; WBEM_E_INVALID_PARAMETER
    je potReport
    cmp eax,80070057h ; E_INVALIDARG
    je potReport
    lea rcx,pdOptTestOutput
    lea rdx,pdOptTestLiveFormat
    mov r8d,dword ptr [pdOptTestSnapshot+PD_S_FIELDS+PD_F_COMMAND*16]
    mov r9d,dword ptr [pdOptTestSnapshot+PD_S_FIELDS+PD_F_COMMAND*16+4]
    mov eax,dword ptr [pdOptTestSnapshot+PD_S_FIELDS+PD_F_COMMAND*16+12]
    mov [rsp+20h],rax
    call wsprintfA
    mov ebx,eax
    mov ecx,-11
    call GetStdHandle
    mov rcx,rax
    lea rdx,pdOptTestOutput
    mov r8d,ebx
    lea r9,pdOptTestWritten
    mov qword ptr [rsp+20h],0
    call WriteFile
potLiveDone:
    cmp dword ptr [pdOptTestSnapshot+PD_S_FIELDS+PD_F_ACCOUNT*16],PD_STATUS_VALID
    jne potAccountUnavailable
    lea rcx,pdOptTestUser
    lea rdx,pdOptTestUserLength
    call GetUserNameW
    POT_CHECK32 eax,1,8155
    lea rbx,pdOptTestSnapshot+PD_S_ACCOUNT
    mov rsi,rbx
potAccountNext:
    movzx eax,word ptr [rsi]
    test eax,eax
    jz potAccountCompare
    add rsi,2
    cmp eax,92
    jne potAccountNext
    mov rbx,rsi
    jmp potAccountNext
potAccountCompare:
    mov rcx,rbx
    lea rdx,pdOptTestUser
    call lstrcmpiW
    POT_CHECK32 eax,0,8156
    lea rcx,pdOptTestSnapshot+PD_S_ACCOUNT
    call lstrlenW
    POT_CHECK32 eax,dword ptr [pdOptTestSnapshot+PD_S_FIELDS+PD_F_ACCOUNT*16+8],8157
    mov pdOptTestLiveAccount,1
    jmp potAccountDone
potAccountUnavailable:
    mov eax,dword ptr [pdOptTestSnapshot+PD_S_FIELDS+PD_F_ACCOUNT*16+4]
    mov pdOptTestStage,8162
    test eax,eax
    jz potReport
    cmp eax,13
    je potReport
    lea rcx,pdOptTestOutput
    lea rdx,pdOptTestAccountFormat
    mov r8d,dword ptr [pdOptTestSnapshot+PD_S_FIELDS+PD_F_ACCOUNT*16]
    mov r9d,dword ptr [pdOptTestSnapshot+PD_S_FIELDS+PD_F_ACCOUNT*16+4]
    mov eax,dword ptr [pdOptTestSnapshot+PD_S_FIELDS+PD_F_ACCOUNT*16+12]
    mov [rsp+20h],rax
    call wsprintfA
    mov ebx,eax
    mov ecx,-11
    call GetStdHandle
    mov rcx,rax
    lea rdx,pdOptTestOutput
    mov r8d,ebx
    lea r9,pdOptTestWritten
    mov qword ptr [rsp+20h],0
    call WriteFile
potAccountDone:
    mov rax,pdOptTestGuardValue
    mov pdOptTestStage,8152
    cmp pdOptTestGuard,rax
    jne potReport
    POT_CHECK32 pdOptTestBad,0,8153
    POT_CHECK32 pdOptTestRefs,0,8154
  ENDIF
  mov pdOptTestStage,0
potReport:
  IFDEF PD_OPTIONAL_ENGINE_READY
    call PdOptTestRestore
  ENDIF
  lea rcx,pdOptTestOutput
  lea rdx,pdOptTestFormat
  mov r8d,pdOptTestStage
  mov r9d,pdOptTestAssertions
  mov eax,pdOptTestLive
  mov [rsp+20h],rax
  mov eax,pdOptTestLiveAccount
  mov [rsp+28h],rax
  call wsprintfA
  mov [rsp+30h],eax
  mov ecx,-11
  call GetStdHandle
  mov rcx,rax
  lea rdx,pdOptTestOutput
  mov r8d,[rsp+30h]
  lea r9,pdOptTestWritten
  mov qword ptr [rsp+20h],0
  call WriteFile
  mov ecx,pdOptTestStage
  call ExitProcess
PdOptionalCollectorTest ENDP

PdOptTestCollect PROC
  push rsi
  push rdi
  sub rsp,38h
  lea rsi,pdOptTestRequest
  lea rdi,pdOptTestRequestBefore
  mov ecx,PD_REQUEST_SIZE/8
  rep movsq
  lea rcx,pdOptTestRequest
  lea rdx,pdOptTestSnapshot
  mov r8d,PD_SNAPSHOT_SIZE
  call PdCollectOptional
  mov [rsp+30h],eax
  lea rsi,pdOptTestRequest
  lea rdi,pdOptTestRequestBefore
  mov ecx,PD_REQUEST_SIZE
  repe cmpsb
  je potcUnchanged
  mov pdOptTestBad,1
potcUnchanged:
  mov eax,[rsp+30h]
  add rsp,38h
  pop rdi
  pop rsi
  ret
PdOptTestCollect ENDP
PdOptTestReset PROC
  mov pdOptTestTick,1000
  mov qword ptr [pdOptTestRequest+PD_R_DEADLINE],4000
  mov rax,pdOptTestTimes
  mov qword ptr [pdOptTestRequest+PD_R_BIRTH],rax
  mov rax,pdOptTestGuardValue
  mov pdOptTestGuard,rax
  mov pdOptTestOpenCalls,0
  mov pdOptTestFactoryCalls,0
  mov pdOptTestNextMs,0
  ret
PdOptTestReset ENDP
PdOptTestInstall PROC
  lea rax,PdOptTestOpen
  mov pdOptionalOpenProcess,rax
  lea rax,PdOptTestLookup
  mov pdOptionalLookupAccount,rax
  lea rax,PdOptTestClock
  mov pdOptionalClock,rax
  lea rax,PdOptTestFactory
  mov pdOptionalCreateInstance,rax
  lea rax,PdOptTestProxy
  mov pdOptionalProxyBlanket,rax
  ret
PdOptTestInstall ENDP
PdOptTestRestore PROC
  lea rax,OpenProcess
  mov pdOptionalOpenProcess,rax
  lea rax,LookupAccountSidW
  mov pdOptionalLookupAccount,rax
  lea rax,GetTickCount64
  mov pdOptionalClock,rax
  lea rax,CoCreateInstance
  mov pdOptionalCreateInstance,rax
  lea rax,CoSetProxyBlanket
  mov pdOptionalProxyBlanket,rax
  ret
PdOptTestRestore ENDP
PdOptTestClock PROC
  mov rax,pdOptTestTick
  ret
PdOptTestClock ENDP
PdOptTestFormatBirth PROC
  sub rsp,58h
  lea rcx,pdOptTestTimes
  lea rdx,pdOptTestSystemTime
  call FileTimeToSystemTime
  lea rcx,pdOptTestCreation
  lea rdx,pdOptTestDateFormat
  movzx r8d,word ptr [pdOptTestSystemTime]
  movzx r9d,word ptr [pdOptTestSystemTime+2]
  movzx eax,word ptr [pdOptTestSystemTime+6]
  mov [rsp+20h],rax
  movzx eax,word ptr [pdOptTestSystemTime+8]
  mov [rsp+28h],rax
  movzx eax,word ptr [pdOptTestSystemTime+10]
  mov [rsp+30h],rax
  movzx eax,word ptr [pdOptTestSystemTime+12]
  mov [rsp+38h],rax
  mov rax,pdOptTestTimes
  xor edx,edx
  mov r10d,10000000
  div r10
  mov rax,rdx
  xor edx,edx
  mov r10d,10
  div r10
  mov [rsp+40h],rax
  lea rdx,pdOptTestDateFormat
  call wsprintfW
  lea rcx,pdOptTestWrongCreation
  lea rdx,pdOptTestCreation
  call lstrcpyW
  mov word ptr [pdOptTestWrongCreation],'3'
  lea rcx,pdOptTestExpectedQuery
  lea rdx,pdOptTestQuery
  mov r8d,dword ptr [pdOptTestRequest+PD_R_PID]
  call wsprintfW
  add rsp,58h
  ret
PdOptTestFormatBirth ENDP

PdOptTestOpen PROC
  sub rsp,28h
  inc pdOptTestOpenCalls
  cmp pdOptTestMode,21
  je potoDenied
  cmp pdOptTestMode,20
  jne potoReal
  test ecx,100000h
  jnz potoDenied
potoReal:
  call OpenProcess
  jmp potoDone
potoDenied:
  mov ecx,5
  call SetLastError
  xor eax,eax
potoDone:
  add rsp,28h
  ret
PdOptTestOpen ENDP

PdOptTestLookup PROC
  push rbx
  push rsi
  push rdi
  sub rsp,40h
  mov rbx,r8
  mov rsi,r9
  mov rdi,[rsp+80h]
  test rcx,rcx
  jnz potlBad
  test rdx,rdx
  jz potlBad
  cmp dword ptr [rsi],PD_ACCOUNT_CAPACITY
  jne potlBad
  mov rax,[rsp+88h]
  cmp dword ptr [rax],PD_ACCOUNT_CAPACITY
  jne potlBad
  cmp pdOptTestMode,18
  je potlNotMapped
  cmp pdOptTestMode,19
  je potlLong
  cmp pdOptTestMode,32
  je potlDenied
  mov rcx,rbx
  lea rdx,pdOptTestAccount
  call lstrcpyW
  mov rcx,rdi
  lea rdx,pdOptTestDomain
  call lstrcpyW
  cmp pdOptTestMode,15
  jne potlSuccess
  mov pdOptTestTick,4001
potlSuccess:
  mov eax,1
  jmp potlDone
potlBad:
  mov pdOptTestBad,1
potlDenied:
  mov ecx,5
  jmp potlFail
potlNotMapped:
  mov ecx,1332
  jmp potlFail
potlLong:
  mov dword ptr [rsi],2048
  mov ecx,122
potlFail:
  call SetLastError
  xor eax,eax
potlDone:
  add rsp,40h
  pop rdi
  pop rsi
  pop rbx
  ret
PdOptTestLookup ENDP

PdOptTestFactory PROC
  inc pdOptTestFactoryCalls
  test rdx,rdx
  jnz potfBad
  cmp r8d,1
  jne potfBad
  mov rax,11D01D3A4590F811h
  cmp [rcx],rax
  jne potfBad
  mov rax,242E4B00AA001F89h
  cmp [rcx+8],rax
  jne potfBad
  mov rax,11CF737FDC12A687h
  cmp [r9],rax
  jne potfBad
  mov rax,242E4B00AA004D88h
  cmp [r9+8],rax
  jne potfBad
  mov r10,[rsp+28h]
  cmp pdOptTestMode,7
  je potfFail
  cmp pdOptTestMode,33
  je potfNull
  lea rax,pdOptTestLocator
  mov [r10],rax
  inc pdOptTestRefs
  xor eax,eax
  ret
potfNull:
  mov qword ptr [r10],0
  xor eax,eax
  ret
potfBad:
  mov pdOptTestBad,1
potfFail:
  mov eax,80004005h
  ret
PdOptTestFactory ENDP
PdOptTestProxy PROC
  cmp edx,10
  jne potpBad
  test r8d,r8d
  jnz potpBad
  test r9,r9
  jnz potpBad
  cmp qword ptr [rsp+28h],3
  jne potpBad
  cmp qword ptr [rsp+30h],3
  jne potpBad
  cmp qword ptr [rsp+38h],0
  jne potpBad
  cmp qword ptr [rsp+40h],0
  jne potpBad
  cmp pdOptTestMode,9
  je potpFail
  xor eax,eax
  ret
potpBad:
  mov pdOptTestBad,1
potpFail:
  mov eax,80004002h
  ret
PdOptTestProxy ENDP
PdOptTestConnect PROC
  push rbx
  sub rsp,30h
  mov rbx,[rsp+80h]
  test r8,r8
  jnz potconBad
  test r9,r9
  jnz potconBad
  cmp qword ptr [rsp+60h],0
  jne potconBad
  cmp qword ptr [rsp+68h],80h
  jne potconBad
  cmp qword ptr [rsp+70h],0
  jne potconBad
  cmp qword ptr [rsp+78h],0
  jne potconBad
  mov rcx,rdx
  lea rdx,pdOptTestNamespace
  call lstrcmpW
  test eax,eax
  jnz potconBad
  cmp pdOptTestMode,8
  je potconFail
  cmp pdOptTestMode,34
  je potconNull
  lea rax,pdOptTestService
  mov [rbx],rax
  inc pdOptTestRefs
  cmp pdOptTestMode,16
  jne potconRemaining
  mov pdOptTestTick,4001
potconRemaining:
  cmp pdOptTestMode,22
  jne potconSuccess
  mov pdOptTestTick,3900
potconSuccess:
  xor eax,eax
  jmp potconDone
potconNull:
  mov qword ptr [rbx],0
  jmp potconSuccess
potconBad:
  mov pdOptTestBad,1
potconFail:
  mov eax,80070005h
potconDone:
  add rsp,30h
  pop rbx
  ret
PdOptTestConnect ENDP
PdOptTestExec PROC
  push rbx
  sub rsp,30h
  mov rbx,[rsp+68h]
  mov [rsp+28h],r8
  cmp r9d,30h
  jne poteBad
  cmp qword ptr [rsp+60h],0
  jne poteBad
  mov rcx,rdx
  lea rdx,pdOptTestWql
  call lstrcmpW
  test eax,eax
  jnz poteBad
  mov rcx,[rsp+28h]
  lea rdx,pdOptTestExpectedQuery
  call lstrcmpW
  test eax,eax
  jnz poteBad
  cmp pdOptTestMode,10
  je poteFail
  lea rax,pdOptTestEnum
  mov [rbx],rax
  inc pdOptTestRefs
  xor eax,eax
  jmp poteDone
poteBad:
  mov pdOptTestBad,1
poteFail:
  mov eax,80041010h
poteDone:
  add rsp,30h
  pop rbx
  ret
PdOptTestExec ENDP
PdOptTestNext PROC
  mov pdOptTestNextMs,edx
  cmp edx,1
  jb potnBad
  cmp edx,3000
  ja potnBad
  cmp r8d,1
  jne potnBad
  mov r10,[rsp+28h]
  cmp pdOptTestMode,12
  je potnEmptyTimeout
  cmp pdOptTestMode,31
  je potnEmptyEnd
  lea rax,pdOptTestObject
  mov [r9],rax
  mov dword ptr [r10],1
  inc pdOptTestRefs
  cmp pdOptTestMode,11
  je potnFail
  cmp pdOptTestMode,17
  jne potnReturn
  mov pdOptTestTick,4001
potnReturn:
  cmp pdOptTestMode,13
  je potnTimeout
  xor eax,eax
  ret
potnEmptyTimeout:
  mov qword ptr [r9],0
  mov dword ptr [r10],0
potnTimeout:
  mov eax,40004h
  ret
potnEmptyEnd:
  mov qword ptr [r9],0
  mov dword ptr [r10],0
  mov eax,1
  ret
potnBad:
  mov pdOptTestBad,1
potnFail:
  mov eax,80070005h
  ret
PdOptTestNext ENDP
PdOptTestGet PROC
  push rbx
  push rsi
  push rdi
  sub rsp,40h
  mov rbx,r9
  mov rsi,rdx
  mov rdi,[rsp+80h]
  test r8d,r8d
  jnz potgBad
  cmp qword ptr [rsp+88h],0
  jne potgBad
  mov rcx,rsi
  lea rdx,pdOptTestPidName
  call lstrcmpW
  test eax,eax
  jnz potgBirth
  cmp pdOptTestMode,14
  je potgDenied
  mov dword ptr [rdi],19
  cmp pdOptTestMode,23
  jne potgPidType
  mov dword ptr [rdi],8
potgPidType:
  mov word ptr [rbx],3
  cmp pdOptTestMode,29
  jne potgPidValue
  mov word ptr [rbx],19
potgPidValue:
  mov eax,dword ptr [pdOptTestRequest+PD_R_PID]
  cmp pdOptTestMode,1
  jne potgPidStore
  inc eax
potgPidStore:
  mov [rbx+8],eax
  xor eax,eax
  jmp potgDone
potgBirth:
  mov rcx,rsi
  lea rdx,pdOptTestBirthName
  call lstrcmpW
  test eax,eax
  jnz potgCommand
  cmp pdOptTestMode,27
  je potgDenied
  mov dword ptr [rdi],101
  lea rcx,pdOptTestCreation
  cmp pdOptTestMode,2
  jne potgWildcard
  lea rcx,pdOptTestWrongCreation
potgWildcard:
  cmp pdOptTestMode,25
  jne potgAlloc
  lea rcx,pdOptTestWildcard
  jmp potgAlloc
potgCommand:
  mov rcx,rsi
  lea rdx,pdOptTestCommandName
  call lstrcmpW
  test eax,eax
  jnz potgBad
  mov dword ptr [rdi],8
  cmp pdOptTestMode,24
  jne potgCommandType
  mov dword ptr [rdi],19
potgCommandType:
  cmp pdOptTestMode,3
  jne potgEmptyNull
  mov word ptr [rbx],1
  xor eax,eax
  jmp potgDone
potgEmptyNull:
  cmp pdOptTestMode,30
  jne potgLength
  mov word ptr [rbx],8
  mov qword ptr [rbx+8],0
  xor eax,eax
  jmp potgDone
potgLength:
  lea rcx,pdOptTestLarge
  mov edx,32767
  cmp pdOptTestMode,5
  je potgAllocLen
  dec edx
  cmp pdOptTestMode,26
  je potgAllocLen
  lea rcx,pdOptTestEmbedded
  mov edx,3
  cmp pdOptTestMode,6
  je potgAllocLen
  lea rcx,pdOptTestCommand
  cmp pdOptTestMode,4
  jne potgAlloc
  lea rcx,pdOptTestEmpty
potgAlloc:
  call SysAllocString
  jmp potgString
potgAllocLen:
  call SysAllocStringLen
potgString:
  mov word ptr [rbx],8
  mov [rbx+8],rax
  test rax,rax
  jz potgMemory
  cmp pdOptTestMode,28
  je potgFail
  xor eax,eax
  jmp potgDone
potgBad:
  mov pdOptTestBad,1
potgFail:
  mov eax,80004005h
  jmp potgDone
potgDenied:
  mov eax,80070005h
  jmp potgDone
potgMemory:
  mov eax,8007000Eh
potgDone:
  add rsp,40h
  pop rdi
  pop rsi
  pop rbx
  ret
PdOptTestGet ENDP
PdOptTestRelease PROC
  dec pdOptTestRefs
  mov eax,pdOptTestRefs
  ret
PdOptTestRelease ENDP
PdOptTestUnexpected PROC
  mov pdOptTestBad,1
  mov eax,80004001h
  ret
PdOptTestUnexpected ENDP
