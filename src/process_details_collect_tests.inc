; Every real target below is the disposable child created by this entry.
extern VirtualAlloc:proc, VirtualFree:proc, GetProcessHandleCount:proc
extern ConvertSidToStringSidW:proc, GetExitCodeProcess:proc
.data
align 8
pdTestStage dd 7100
pdTestAssertions dd 0
pdTestWritten dd 0
pdTestReady dq 0
pdTestStop dq 0
pdTestSid dq 0
pdTestToken dq 0
pdTestLength dd 0
pdTestMode dd 0
pdTestOpenCalls dd 0
pdTestMemoryCalls dd 0
pdTestRefHandles dd 0
pdTestOwnHandles dd 0
pdTestUnexpectedAccess dd 0
pdTestFormat db 'Process details native: exit=%u assertions=%u',13,10,0
align 2
pdTestReadyFmt dw 'L','o','c','a','l','\','P','d','R','e','a','d','y','-','%','u',0
pdTestStopFmt dw 'L','o','c','a','l','\','P','d','S','t','o','p','-','%','u',0
pdTestExeFmt dw '%','s','\','p','r','o','c','e','s','s','-','d','e','t','a','i','l','s','-','f','i','x','t','u','r','e','.','e','x','e',0
pdTestCmdFmt dw '"','%','s','"',' ','%','s',' ','%','s',0
pdFixtureFile dw 'o','w','n','e','d','-','i','o','.','b','i','n',0
.data?
align 8
pdTestRequest db PD_REQUEST_SIZE dup(?)
pdTestSnapshot db PD_SNAPSHOT_SIZE dup(?)
pdTestGuard dq ?
pdTestSi db 104 dup(?)
pdTestPi db 24 dup(?)
pdTestReadyName dw 128 dup(?)
pdTestStopName dw 128 dup(?)
pdTestCwd dw 32768 dup(?)
pdTestExe dw 32768 dup(?)
pdTestCommand dw 32768 dup(?)
pdTestRefTimes dq 8 dup(?)
pdTestRefMemory db 80 dup(?)
pdTestRefMemoryAfter db 80 dup(?)
pdTestRefIo dq 6 dup(?)
pdTestRefIoAfter dq 6 dup(?)
pdTestRefToken db 512 dup(?)
pdTestOutput db 128 dup(?)
pdFixtureBytes db 16384 dup(?)
.code
PDT_CHECK32 MACRO value,expected,stage
  mov pdTestStage,stage
  mov r10d,value
  cmp r10d,expected
  jne pdtFinish
  inc pdTestAssertions
ENDM
PDT_CHECK64 MACRO value,expected,stage
  mov pdTestStage,stage
  mov r10,value
  cmp r10,expected
  jne pdtFinish
  inc pdTestAssertions
ENDM

ProcessDetailsNativeTest PROC
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
  jz pdtFinish
  xor ecx,ecx
  mov edx,1
  xor r8d,r8d
  lea r9,pdTestStopName
  call CreateEventW
  mov pdTestStop,rax
  test rax,rax
  jz pdtFinish
  mov ecx,32768
  lea rdx,pdTestCwd
  call GetCurrentDirectoryW
  test eax,eax
  jz pdtFinish
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
  jz pdtFinish
  mov rcx,pdTestReady
  mov edx,5000
  call WaitForSingleObject
  PDT_CHECK32 eax,0,7101
  mov rcx,qword ptr [pdTestPi]
  lea rdx,pdTestRefTimes
  lea r8,pdTestRefTimes+8
  lea r9,pdTestRefTimes+16
  lea rax,pdTestRefTimes+24
  mov [rsp+20h],rax
  call GetProcessTimes
  PDT_CHECK32 eax,1,7102
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
  lea rcx,pdTestRequest
  lea rdx,pdTestSnapshot
  mov r8d,PD_SNAPSHOT_SIZE
  call PdCollectNative
  PDT_CHECK32 eax,0,7103
  PDT_CHECK32 dword ptr [pdTestSnapshot+PD_S_TARGET],PD_TARGET_VERIFIED,7104
  PDT_CHECK32 dword ptr [pdTestSnapshot+PD_S_SIZE],PD_SNAPSHOT_SIZE,7105
  PDT_CHECK64 qword ptr [pdTestSnapshot+PD_S_COOKIE],111,7106
  PDT_CHECK64 qword ptr [pdTestSnapshot+PD_S_GENERATION],222,7107
  PDT_CHECK64 qword ptr [pdTestSnapshot+PD_S_SEQUENCE],333,7108
  PDT_CHECK64 qword ptr [pdTestSnapshot+PD_S_EPOCH],444,7109
  mov rax,[pdTestRefTimes]
  mov rdx,qword ptr [pdTestSnapshot+PD_S_OBSERVED_BIRTH]
  PDT_CHECK64 rdx,rax,7110
  PDT_CHECK32 dword ptr [pdTestSnapshot+PD_S_FIELDS+PD_F_IDENTITY*16],PD_STATUS_VALID,7111
  PDT_CHECK32 dword ptr [pdTestSnapshot+PD_S_FIELDS+PD_F_LIVENESS*16],PD_STATUS_VALID,7112
  PDT_CHECK32 dword ptr [pdTestSnapshot+PD_S_FIELDS+PD_F_PATH*16],PD_STATUS_VALID,7113
  lea rcx,pdTestExe
  lea rdx,pdTestSnapshot+PD_S_PATH
  call lstrcmpW
  PDT_CHECK32 eax,0,7114
  lea rcx,pdTestExe
  call lstrlenW
  PDT_CHECK32 eax,dword ptr [pdTestSnapshot+PD_S_FIELDS+PD_F_PATH*16+8],7115
  PDT_CHECK32 dword ptr [pdTestSnapshot+PD_S_FIELDS+PD_F_ACCOUNT*16],PD_STATUS_NOT_REQUESTED,7116
  PDT_CHECK32 dword ptr [pdTestSnapshot+PD_S_FIELDS+PD_F_COMMAND*16],PD_STATUS_NOT_REQUESTED,7117
  PDT_CHECK32 dword ptr [pdTestSnapshot+PD_S_FIELDS+PD_F_CPU_RATE*16],PD_STATUS_NOT_REQUESTED,7118
  PDT_CHECK32 dword ptr [pdTestSnapshot+PD_S_FIELDS+PD_F_IO_RATE*16],PD_STATUS_NOT_REQUESTED,7119
  ; Independently query the inherited current token, not the collector's token.
  call GetCurrentProcess
  mov rcx,rax
  mov edx,8
  lea r8,pdTestToken
  call OpenProcessToken
  PDT_CHECK32 eax,1,7120
  mov rcx,pdTestToken
  mov edx,1
  lea r8,pdTestRefToken
  mov r9d,512
  lea rax,pdTestLength
  mov [rsp+20h],rax
  call GetTokenInformation
  PDT_CHECK32 eax,1,7121
  mov rcx,qword ptr [pdTestRefToken]
  lea rdx,pdTestSid
  call ConvertSidToStringSidW
  PDT_CHECK32 eax,1,7122
  mov rcx,pdTestSid
  lea rdx,pdTestSnapshot+PD_S_SID
  call lstrcmpW
  PDT_CHECK32 eax,0,7123
  PDT_CHECK32 dword ptr [pdTestSnapshot+PD_S_FIELDS+PD_F_SID*16],PD_STATUS_VALID,7124
  mov rcx,pdTestSid
  call LocalFree
  mov pdTestSid,0
  mov rcx,pdTestToken
  call CloseHandle
  mov pdTestToken,0
  mov rcx,qword ptr [pdTestPi]
  lea rdx,pdTestRefHandles
  call GetProcessHandleCount
  PDT_CHECK32 eax,1,7125
  mov eax,pdTestRefHandles
  mov rdx,qword ptr [pdTestSnapshot+PD_S_HANDLES]
  PDT_CHECK64 rdx,rax,7126
  PDT_CHECK32 dword ptr [pdTestSnapshot+PD_S_FIELDS+PD_F_HANDLES*16],PD_STATUS_VALID,7127
  ; Collect another sample bracketed by independent memory/I/O readings.
  call PdTestReferenceBefore
  PDT_CHECK32 eax,1,7128
  call PdTestCollect
  PDT_CHECK32 eax,0,7129
  call PdTestReferenceAfter
  PDT_CHECK32 eax,1,7130
  PDT_CHECK32 dword ptr [pdTestSnapshot+PD_S_FIELDS+PD_F_MEMORY*16],PD_STATUS_VALID,7131
  mov rax,qword ptr [pdTestSnapshot+PD_S_PRIVATE]
  cmp rax,16777216
  mov pdTestStage,7132
  jb pdtFinish
  mov rdx,qword ptr [pdTestRefMemory+72]
  PDT_CHECK64 rax,rdx,7133
  mov rax,qword ptr [pdTestSnapshot+PD_S_WORKING_SET]
  mov rcx,qword ptr [pdTestRefMemory+16]
  mov rdx,qword ptr [pdTestRefMemoryAfter+16]
  call PdTestBetween
  PDT_CHECK32 eax,1,7134
  PDT_CHECK32 dword ptr [pdTestSnapshot+PD_S_FIELDS+PD_F_IO*16],PD_STATUS_VALID,7135
  xor edi,edi
  lea r8,pdTestSnapshot+PD_S_READ_OPS
  lea r9,pdTestRefIo
  lea r11,pdTestRefIoAfter
pdtIoValues:
  mov rax,[r8+rdi*8]
  mov rcx,[r9+rdi*8]
  mov rdx,[r11+rdi*8]
  call PdTestBetween
  PDT_CHECK32 eax,1,7136
  inc edi
  cmp edi,6
  jb pdtIoValues
  mov pdTestStage,7137
  cmp qword ptr [pdTestSnapshot+PD_S_READ_OPS],1
  jb pdtFinish
  cmp qword ptr [pdTestSnapshot+PD_S_WRITE_OPS],1
  jb pdtFinish
  cmp qword ptr [pdTestSnapshot+PD_S_READ_BYTES],16384
  jb pdtFinish
  cmp qword ptr [pdTestSnapshot+PD_S_WRITE_BYTES],16384
  jb pdtFinish
  inc pdTestAssertions
  PDT_CHECK32 dword ptr [pdTestSnapshot+PD_S_FIELDS+PD_F_CPU_TIMES*16],PD_STATUS_VALID,7138
  mov rax,qword ptr [pdTestSnapshot+PD_S_KERNEL]
  mov rcx,[pdTestRefTimes+16]
  mov rdx,[pdTestRefTimes+48]
  call PdTestBetween
  PDT_CHECK32 eax,1,7139
  mov rax,qword ptr [pdTestSnapshot+PD_S_USER]
  mov rcx,[pdTestRefTimes+24]
  mov rdx,[pdTestRefTimes+56]
  call PdTestBetween
  PDT_CHECK32 eax,1,7140
  mov pdTestStage,7141
  cmp qword ptr [pdTestSnapshot+PD_S_TICK],0
  je pdtFinish
  cmp qword ptr [pdTestSnapshot+PD_S_FREQUENCY],0
  je pdtFinish
  cmp dword ptr [pdTestSnapshot+PD_S_CAPACITY],0
  je pdtFinish
  inc pdTestAssertions
  ; Invalid requests cannot write even a partial header into caller storage.
  mov rax,1122334455667788h
  mov qword ptr [pdTestSnapshot],rax
  mov pdTestGuard,rax
  lea rcx,pdTestRequest
  lea rdx,pdTestSnapshot
  mov r8d,PD_SNAPSHOT_SIZE-1
  call PdCollectNative
  PDT_CHECK32 eax,122,7142
  mov rax,1122334455667788h
  mov rdx,qword ptr [pdTestSnapshot]
  PDT_CHECK64 rdx,rax,7143
  mov dword ptr [pdTestRequest+PD_R_VERSION],2
  call PdTestCollect
  PDT_CHECK32 eax,87,7144
  mov rax,1122334455667788h
  mov rdx,qword ptr [pdTestSnapshot]
  PDT_CHECK64 rdx,rax,7145
  mov dword ptr [pdTestRequest+PD_R_VERSION],1
  mov dword ptr [pdTestRequest+PD_R_LANE],2
  call PdTestCollect
  PDT_CHECK32 eax,50,7146
  mov rax,1122334455667788h
  mov rdx,qword ptr [pdTestSnapshot]
  PDT_CHECK64 rdx,rax,7147
  mov dword ptr [pdTestRequest+PD_R_LANE],1
  mov qword ptr [pdTestRequest+PD_R_CONTEXT],1
  call PdTestCollect
  PDT_CHECK32 eax,87,7148
  mov qword ptr [pdTestRequest+PD_R_CONTEXT],0
  lea rax,PdTestOpenProcess
  mov pdNativeOpenProcess,rax
  lea rax,PdTestMemoryInfo
  mov pdNativeMemoryInfo,rax
  mov pdTestOpenCalls,0
  inc qword ptr [pdTestRequest+PD_R_BIRTH]
  call PdTestCollect
  PDT_CHECK32 eax,0,7149
  PDT_CHECK32 dword ptr [pdTestSnapshot+PD_S_TARGET],PD_TARGET_MISMATCH,7150
  PDT_CHECK32 dword ptr [pdTestSnapshot+PD_S_FIELDS+PD_F_IDENTITY*16],PD_STATUS_UNAVAILABLE,7151
  PDT_CHECK32 dword ptr [pdTestSnapshot+PD_S_FIELDS+PD_F_PATH*16],PD_STATUS_UNAVAILABLE,7152
  PDT_CHECK64 qword ptr [pdTestSnapshot+PD_S_PRIVATE],0,7153
  PDT_CHECK32 pdTestOpenCalls,1,7154
  mov qword ptr [pdTestRequest+PD_R_BIRTH],0
  mov pdTestOpenCalls,0
  call PdTestCollect
  PDT_CHECK32 eax,0,7155
  PDT_CHECK32 dword ptr [pdTestSnapshot+PD_S_TARGET],PD_TARGET_MISMATCH,7156
  PDT_CHECK32 dword ptr [pdTestSnapshot+PD_S_ERROR],87,7157
  PDT_CHECK32 pdTestOpenCalls,0,7158
  mov rax,[pdTestRefTimes]
  mov qword ptr [pdTestRequest+PD_R_BIRTH],rax
  mov pdTestMode,1 ; deterministic access boundary refusal, real validation
  call PdTestCollect
  PDT_CHECK32 eax,0,7159
  PDT_CHECK32 dword ptr [pdTestSnapshot+PD_S_TARGET],PD_TARGET_DENIED,7160
  PDT_CHECK32 dword ptr [pdTestSnapshot+PD_S_ERROR],5,7161
  PDT_CHECK32 dword ptr [pdTestSnapshot+PD_S_FIELDS+PD_F_MEMORY*16+4],5,7162
  PDT_CHECK32 pdTestOpenCalls,2,7163
  mov pdTestMode,2 ; only SYNCHRONIZE denied: limited data remains available
  call PdTestCollect
  PDT_CHECK32 eax,0,7164
  PDT_CHECK32 dword ptr [pdTestSnapshot+PD_S_TARGET],PD_TARGET_UNKNOWN,7165
  PDT_CHECK32 dword ptr [pdTestSnapshot+PD_S_FIELDS+PD_F_IDENTITY*16],PD_STATUS_VALID,7166
  PDT_CHECK32 dword ptr [pdTestSnapshot+PD_S_FIELDS+PD_F_LIVENESS*16],PD_STATUS_UNAVAILABLE,7167
  PDT_CHECK32 dword ptr [pdTestSnapshot+PD_S_FIELDS+PD_F_LIVENESS*16+4],50,7168
  PDT_CHECK32 dword ptr [pdTestSnapshot+PD_S_FIELDS+PD_F_PATH*16],PD_STATUS_VALID,7169
  mov pdTestMode,3 ; fallback is another real process: birth guard must refuse it
  mov pdTestMemoryCalls,0
  call PdTestCollect
  PDT_CHECK32 eax,0,7170
  PDT_CHECK32 dword ptr [pdTestSnapshot+PD_S_TARGET],PD_TARGET_VERIFIED,7171
  PDT_CHECK32 dword ptr [pdTestSnapshot+PD_S_FIELDS+PD_F_MEMORY*16],PD_STATUS_UNAVAILABLE,7172
  PDT_CHECK32 dword ptr [pdTestSnapshot+PD_S_FIELDS+PD_F_MEMORY*16+4],13,7173
  PDT_CHECK32 pdTestMemoryCalls,1,7174
  PDT_CHECK32 dword ptr [pdTestSnapshot+PD_S_FIELDS+PD_F_IO*16],PD_STATUS_VALID,7175
  mov pdTestMode,4 ; valid stronger-memory handle is verified and used
  mov pdTestMemoryCalls,0
  call PdTestCollect
  PDT_CHECK32 eax,0,7176
  PDT_CHECK32 dword ptr [pdTestSnapshot+PD_S_FIELDS+PD_F_MEMORY*16],PD_STATUS_VALID,7177
  PDT_CHECK32 pdTestMemoryCalls,2,7178
  mov pdTestMode,5 ; one field's error cannot erase unrelated valid fields
  call PdTestCollect
  PDT_CHECK32 eax,0,7179
  PDT_CHECK32 dword ptr [pdTestSnapshot+PD_S_FIELDS+PD_F_MEMORY*16],PD_STATUS_UNAVAILABLE,7180
  PDT_CHECK32 dword ptr [pdTestSnapshot+PD_S_FIELDS+PD_F_MEMORY*16+4],299,7181
  PDT_CHECK32 dword ptr [pdTestSnapshot+PD_S_FIELDS+PD_F_SID*16],PD_STATUS_VALID,7182
  mov pdTestMode,0
  call GetCurrentProcess
  mov rcx,rax
  lea rdx,pdTestOwnHandles
  call GetProcessHandleCount
  PDT_CHECK32 eax,1,7200
  mov edi,50
pdtRepeated:
  call PdTestCollect
  PDT_CHECK32 eax,0,7201
  dec edi
  jnz pdtRepeated
  call GetCurrentProcess
  mov rcx,rax
  lea rdx,pdTestRefHandles
  call GetProcessHandleCount
  PDT_CHECK32 eax,1,7202
  mov eax,pdTestOwnHandles
  PDT_CHECK32 pdTestRefHandles,eax,7203
  PDT_CHECK32 pdTestUnexpectedAccess,0,7204
  mov eax,dword ptr [pdTestRequest+PD_R_PID]
  mov [rsp+50h],eax
  mov dword ptr [pdTestRequest+PD_R_PID],0
  call PdTestCollect
  PDT_CHECK32 eax,0,7183
  PDT_CHECK32 dword ptr [pdTestSnapshot+PD_S_TARGET],PD_TARGET_UNKNOWN,7184
  PDT_CHECK32 dword ptr [pdTestSnapshot+PD_S_ERROR],87,7185
  mov eax,[rsp+50h]
  mov dword ptr [pdTestRequest+PD_R_PID],eax
  mov rax,pdTestReady
  mov qword ptr [pdTestRequest+PD_R_CANCEL],rax
  mov pdTestOpenCalls,0
  call PdTestCollect
  PDT_CHECK32 eax,0,7186
  PDT_CHECK32 dword ptr [pdTestSnapshot+PD_S_TARGET],PD_TARGET_CANCELLED,7187
  PDT_CHECK32 pdTestOpenCalls,0,7188
  mov qword ptr [pdTestRequest+PD_R_CANCEL],0
  mov rcx,pdTestStop
  call SetEvent
  mov rcx,qword ptr [pdTestPi]
  mov edx,5000
  call WaitForSingleObject
  PDT_CHECK32 eax,0,7189
  mov rcx,qword ptr [pdTestPi]
  lea rdx,pdTestLength
  call GetExitCodeProcess
  PDT_CHECK32 eax,1,7205
  PDT_CHECK32 pdTestLength,259,7206
  call PdTestCollect
  PDT_CHECK32 eax,0,7190
  PDT_CHECK32 dword ptr [pdTestSnapshot+PD_S_TARGET],PD_TARGET_EXITED,7191
  mov pdTestMode,2 ; actual child exited259 remains ambiguous without wait right
  call PdTestCollect
  PDT_CHECK32 eax,0,7192
  PDT_CHECK32 dword ptr [pdTestSnapshot+PD_S_TARGET],PD_TARGET_UNKNOWN,7193
  PDT_CHECK32 dword ptr [pdTestSnapshot+PD_S_FIELDS+PD_F_IDENTITY*16],PD_STATUS_VALID,7194
  mov rax,1122334455667788h
  mov rdx,pdTestGuard
  PDT_CHECK64 rdx,rax,7195
  mov pdTestStage,0
pdtFinish:
  lea rax,OpenProcess
  mov pdNativeOpenProcess,rax
  lea rax,GetProcessMemoryInfo
  mov pdNativeMemoryInfo,rax
  mov rcx,pdTestSid
  test rcx,rcx
  jz pdtTokenClose
  call LocalFree
pdtTokenClose:
  mov rcx,pdTestToken
  test rcx,rcx
  jz pdtSignalStop
  call CloseHandle
pdtSignalStop:
  mov rcx,pdTestStop
  test rcx,rcx
  jz pdtCloseChild
  call SetEvent
pdtCloseChild:
  mov rcx,qword ptr [pdTestPi]
  test rcx,rcx
  jz pdtEvents
  mov edx,5000
  call WaitForSingleObject
  mov rcx,qword ptr [pdTestPi]
  call CloseHandle
  mov rcx,qword ptr [pdTestPi+8]
  call CloseHandle
pdtEvents:
  mov rcx,pdTestReady
  test rcx,rcx
  jz pdtStopClose
  call CloseHandle
pdtStopClose:
  mov rcx,pdTestStop
  test rcx,rcx
  jz pdtReport
  call CloseHandle
pdtReport:
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
ProcessDetailsNativeTest ENDP

ProcessDetailsFixtureEntry PROC
  push rbx
  push rsi
  push rdi
  sub rsp,40h
  call GetCommandLineW
  mov rcx,rax
  lea rdx,pdTestLength
  call CommandLineToArgvW
  mov rsi,rax
  test rax,rax
  jz pdfFail
  cmp pdTestLength,3
  jne pdfFail
  mov ecx,2
  xor edx,edx
  mov r8,[rsi+8]
  call OpenEventW
  mov pdTestReady,rax
  test rax,rax
  jz pdfFail
  mov ecx,100000h
  xor edx,edx
  mov r8,[rsi+16]
  call OpenEventW
  mov pdTestStop,rax
  test rax,rax
  jz pdfFail
  mov rcx,rsi
  call LocalFree
  xor ecx,ecx
  mov edx,16777216
  mov r8d,3000h
  mov r9d,4
  call VirtualAlloc
  mov rdi,rax
  test rax,rax
  jz pdfFail
  xor ecx,ecx
pdfTouch:
  mov byte ptr [rdi+rcx],5Ah
  add ecx,4096
  cmp ecx,16777216
  jb pdfTouch
  lea rcx,pdFixtureFile
  mov edx,0C0000000h
  mov r8d,1
  xor r9d,r9d
  mov qword ptr [rsp+20h],2
  mov qword ptr [rsp+28h],80h
  mov qword ptr [rsp+30h],0
  call CreateFileW
  mov rbx,rax
  cmp rax,-1
  je pdfFail
  mov rcx,rax
  lea rdx,pdFixtureBytes
  mov r8d,16384
  lea r9,pdTestWritten
  mov qword ptr [rsp+20h],0
  call WriteFile
  test eax,eax
  jz pdfFail
  mov rcx,rbx
  xor edx,edx
  xor r8d,r8d
  xor r9d,r9d
  call SetFilePointerEx
  mov rcx,rbx
  lea rdx,pdFixtureBytes
  mov r8d,16384
  lea r9,pdTestWritten
  mov qword ptr [rsp+20h],0
  call ReadFile
  test eax,eax
  jz pdfFail
  mov rcx,pdTestReady
  call SetEvent
  mov rcx,pdTestStop
  mov edx,15000
  call WaitForSingleObject
  mov rcx,rbx
  call CloseHandle
  mov rcx,rdi
  xor edx,edx
  mov r8d,8000h
  call VirtualFree
  mov ecx,259 ; permitted exit code must never imply alive without a wait handle
  call ExitProcess
pdfFail:
  mov ecx,7199
  call ExitProcess
ProcessDetailsFixtureEntry ENDP

PdTestCollect PROC
  lea rcx,pdTestRequest
  lea rdx,pdTestSnapshot
  mov r8d,PD_SNAPSHOT_SIZE
  jmp PdCollectNative
PdTestCollect ENDP

; RAX sample, RCX before, RDX after; tolerate ordinary working-set trimming.
PdTestBetween PROC
  cmp rcx,rdx
  jbe pdtbBounds
  xchg rcx,rdx
pdtbBounds:
  cmp rax,rcx
  jb pdtbNo
  cmp rax,rdx
  ja pdtbNo
  mov eax,1
  ret
pdtbNo:
  xor eax,eax
  ret
PdTestBetween ENDP

PdTestReferenceBefore PROC
  lea rdx,pdTestRefMemory
  lea r8,pdTestRefIo
  lea r9,pdTestRefTimes
  jmp PdTestReference
PdTestReferenceBefore ENDP
PdTestReferenceAfter PROC
  lea rdx,pdTestRefMemoryAfter
  lea r8,pdTestRefIoAfter
  lea r9,pdTestRefTimes+32
  jmp PdTestReference
PdTestReferenceAfter ENDP
PdTestReference PROC
  push rbx
  push rsi
  push rdi
  sub rsp,30h
  mov rbx,rdx
  mov rsi,r8
  mov rdi,r9
  mov dword ptr [rbx],80
  mov rcx,qword ptr [pdTestPi]
  mov r8d,80
  call GetProcessMemoryInfo
  test eax,eax
  jz pdtrReturn
  mov rcx,qword ptr [pdTestPi]
  mov rdx,rsi
  call GetProcessIoCounters
  test eax,eax
  jz pdtrReturn
  mov rcx,qword ptr [pdTestPi]
  mov rdx,rdi
  lea r8,[rdi+8]
  lea r9,[rdi+16]
  lea rax,[rdi+24]
  mov [rsp+20h],rax
  call GetProcessTimes
pdtrReturn:
  add rsp,30h
  pop rdi
  pop rsi
  pop rbx
  ret
PdTestReference ENDP

PdTestOpenProcess PROC
  sub rsp,38h
  inc pdTestOpenCalls
  test edx,edx
  jnz pdtopUnexpected
  cmp r8d,dword ptr [pdTestRequest+PD_R_PID]
  jne pdtopUnexpected
  cmp ecx,101000h
  je pdtopMaskValid
  cmp ecx,1000h
  je pdtopMaskValid
  cmp ecx,410h
  je pdtopMaskValid
pdtopUnexpected:
  mov pdTestUnexpectedAccess,1
pdtopMaskValid:
  cmp pdTestMode,1
  je pdtopDenied
  cmp pdTestMode,2
  jne pdtopMemory
  test ecx,100000h
  jnz pdtopDenied
pdtopMemory:
  cmp pdTestMode,3
  jne pdtopReal
  cmp ecx,410h
  jne pdtopReal
  call GetCurrentProcessId
  mov r8d,eax
  mov ecx,410h
  xor edx,edx
pdtopReal:
  call OpenProcess
  jmp pdtopReturn
pdtopDenied:
  mov ecx,5
  call SetLastError
  xor eax,eax
pdtopReturn:
  add rsp,38h
  ret
PdTestOpenProcess ENDP

PdTestMemoryInfo PROC
  sub rsp,28h
  inc pdTestMemoryCalls
  cmp pdTestMode,5
  je pdtmiUnavailable
  cmp pdTestMode,3
  je pdtmiFirst
  cmp pdTestMode,4
  jne pdtmiReal
pdtmiFirst:
  cmp pdTestMemoryCalls,1
  je pdtmiUnavailable
pdtmiReal:
  call GetProcessMemoryInfo
  jmp pdtmiReturn
pdtmiUnavailable:
  mov ecx,299
  call SetLastError
  xor eax,eax
pdtmiReturn:
  add rsp,28h
  ret
PdTestMemoryInfo ENDP
