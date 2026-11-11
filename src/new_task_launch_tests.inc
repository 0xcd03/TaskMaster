; Only the owned fixture executable is launched. The elevated OS boundary is
; replaced in this test entry only; request validation remains production code.
extern GetCurrentDirectoryW:proc, GetCommandLineW:proc, CommandLineToArgvW:proc
extern GetEnvironmentVariableW:proc, SetEnvironmentVariableW:proc
extern GetHandleInformation:proc, SetLastError:proc, ReadFile:proc
extern OpenEventW:proc, GetProcAddress:proc
.data
newTaskTestRequestFile dw 'n','e','w','-','t','a','s','k','-','r','e','q','u','e','s','t','.','b','i','n',0
newTaskTestReportFile dw 'n','e','w','-','t','a','s','k','-','r','e','s','u','l','t','.','b','i','n',0
newTaskTestHandleEnv dw 'T','A','S','K','M','A','S','T','E','R','_','F','I','X','T','U','R','E','_','H','A','N','D','L','E',0
newTaskTestHandleFormat dw '%','I','6','4','u',0
newTaskTestEventEnv dw 'T','A','S','K','M','A','S','T','E','R','_','F','I','X','T','U','R','E','_','E','V','E','N','T',0
newTaskTestEventFormat dw 'L','o','c','a','l','\','T','a','s','k','M','a','s','t','e','r','N','e','w','T','a','s','k','-','%','u',0
newTaskTestKernelBase dw 'k','e','r','n','e','l','b','a','s','e','.','d','l','l',0
newTaskTestCompareName db 'CompareObjectHandles',0
newTaskTestCompare dq 0
newTaskTestSecurity dd 24,0
dq 0
dd 1,0
newTaskTestHandle dq 0
newTaskTestFakeHandle dq 0
newTaskTestFakeCalls dd 0
newTaskTestBoundaryInvalid dd 0
newTaskTestComMode dd 0
newTaskTestRead dd 0
newTaskTestStage dd 0
newTaskTestActual dd 120
newTaskTestFlags dd 0
newTaskTestFixtureArgv dq 0
newTaskTestFormat db 'New task launch: stage=%u expected=%u actual=%u elevationCalls=%u',13,10,0
.data?
newTaskTestInput db 262144 dup(?)
newTaskTestRequest dq 4 dup(?)
newTaskTestHandleText dw 32 dup(?)
newTaskTestEventName dw 128 dup(?)
newTaskTestOutput db 192 dup(?)
; Header magic/argc/token/handle; cwd/raw command; eight bounded argv strings.
newTaskFixtureReport db 139280 dup(?)
.code
NewTaskLaunchTest PROC
  push rbx
  push rsi
  sub rsp,58h
  mov newTaskTestStage,5000
  lea rcx,newTaskTestRequestFile
  mov edx,80000000h
  mov r8d,1
  xor r9d,r9d
  mov qword ptr [rsp+20h],3
  mov qword ptr [rsp+28h],80h
  mov qword ptr [rsp+30h],0
  call CreateFileW
  cmp rax,-1
  je ntltReport
  mov rbx,rax
  mov rcx,rax
  lea rdx,newTaskTestInput
  mov r8d,262144
  lea r9,newTaskTestRead
  mov qword ptr [rsp+20h],0
  call ReadFile
  mov esi,eax
  mov rcx,rbx
  call CloseHandle
  test esi,esi
  jz ntltReport
  cmp newTaskTestRead,32
  jb ntltReport
  lea rax,newTaskTestInput+32
  mov newTaskTestRequest,rax
  mov ecx,dword ptr [newTaskTestInput+12]
  cmp ecx,32767
  ja ntltReport
  lea rax,[rax+rcx*2+2]
  mov [newTaskTestRequest+8],rax
  mov ecx,dword ptr [newTaskTestInput+16]
  cmp ecx,32767
  ja ntltReport
  lea rax,[rax+rcx*2+2]
  mov [newTaskTestRequest+16],rax
  mov ecx,dword ptr [newTaskTestInput+20]
  cmp ecx,-1
  jne ntltDirectory
  mov qword ptr [newTaskTestRequest+16],0
  jmp ntltEndInput
ntltDirectory:
  cmp ecx,32767
  ja ntltReport
  lea rax,[rax+rcx*2+2]
ntltEndInput:
  lea rdx,newTaskTestInput
  sub rax,rdx
  cmp eax,newTaskTestRead
  ja ntltReport
  mov eax,dword ptr [newTaskTestInput+4]
  mov dword ptr [newTaskTestRequest+24],eax
  mov eax,dword ptr [newTaskTestInput+8]
  mov dword ptr [newTaskTestRequest+28],eax
  ; Inheritable event deliberately exists in parent; fixture must not receive it.
  call GetCurrentProcessId
  mov r8d,eax
  lea rcx,newTaskTestEventName
  lea rdx,newTaskTestEventFormat
  call wsprintfW
  lea rcx,newTaskTestEventEnv
  lea rdx,newTaskTestEventName
  call SetEnvironmentVariableW
  lea rcx,newTaskTestSecurity
  xor edx,edx
  xor r8d,r8d
  lea r9,newTaskTestEventName
  call CreateEventW
  mov newTaskTestHandle,rax
  test rax,rax
  jz ntltReport
  lea rcx,newTaskTestHandleText
  lea rdx,newTaskTestHandleFormat
  mov r8,rax
  call wsprintfW
  lea rcx,newTaskTestHandleEnv
  lea rdx,newTaskTestHandleText
  call SetEnvironmentVariableW
  test eax,eax
  jz ntltClose
  cmp dword ptr [newTaskTestInput+4],1
  jne ntltLaunch
  cmp dword ptr [newTaskTestInput+24],1223
  jne ntltComSta
  xor ecx,ecx
  xor edx,edx
  call CoInitializeEx
  mov newTaskTestComMode,2 ; existing MTA must survive RPC_E_CHANGED_MODE
  jmp ntltLaunch
ntltComSta:
  cmp dword ptr [newTaskTestInput+24],0
  jne ntltLaunch
  xor ecx,ecx
  mov edx,2
  call CoInitializeEx
  mov newTaskTestComMode,1 ; existing STA tests the S_FALSE balancing path
ntltLaunch:
  ; This pointer assignment is enabled after the production boundary exists.
  IFDEF NEW_TASK_ENGINE_READY
    lea rax,NewTaskTestElevation
    mov newTaskShellExecute,rax
  ENDIF
  lea rcx,newTaskTestRequest
  call LaunchNewTask
  mov newTaskTestActual,eax
  mov newTaskTestStage,5005
  cmp newTaskTestBoundaryInvalid,0
  jne ntltClose
  mov newTaskTestStage,5001
  cmp eax,dword ptr [newTaskTestInput]
  jne ntltClose
  mov newTaskTestStage,5002
  mov eax,newTaskTestFakeCalls
  cmp eax,dword ptr [newTaskTestInput+28]
  jne ntltClose
  cmp dword ptr [newTaskTestInput+4],0
  jne ntltComCheck
  cmp newTaskTestActual,0
  jne ntltComCheck
  mov rcx,newTaskTestHandle
  mov edx,5000
  call WaitForSingleObject
  mov newTaskTestStage,5006
  test eax,eax
  jnz ntltClose
ntltComCheck:
  cmp dword ptr [newTaskTestInput+4],1
  jne ntltFakeHandleCheck
  cmp newTaskTestComMode,2
  jne ntltComStaCheck
  xor ecx,ecx
  xor edx,edx
  call CoInitializeEx
  mov newTaskTestStage,5004
  cmp eax,1 ; the caller's MTA still exists
  jne ntltClose
  call CoUninitialize
  jmp ntltReleasePreinit
ntltComStaCheck:
  cmp newTaskTestComMode,1
  jne ntltReleasePreinit
  xor ecx,ecx
  mov edx,2
  call CoInitializeEx
  mov newTaskTestStage,5004
  cmp eax,1 ; the caller's STA still exists
  jne ntltClose
  call CoUninitialize
ntltReleasePreinit:
  cmp newTaskTestComMode,0
  je ntltProbeClean
  call CoUninitialize
ntltProbeClean:
  xor ecx,ecx
  xor edx,edx
  call CoInitializeEx
  mov newTaskTestStage,5004
  test eax,eax ; all engine-owned STA refs were released
  jnz ntltClose
  call CoUninitialize
ntltFakeHandleCheck:
  mov rcx,newTaskTestFakeHandle
  test rcx,rcx
  jz ntltPassed
  lea rdx,newTaskTestFlags
  call GetHandleInformation
  mov newTaskTestStage,5003
  test eax,eax
  jnz ntltClose
ntltPassed:
  mov newTaskTestStage,0
ntltClose:
  mov rcx,newTaskTestHandle
  call CloseHandle
ntltReport:
  lea rcx,newTaskTestOutput
  lea rdx,newTaskTestFormat
  mov r8d,newTaskTestStage
  mov r9d,dword ptr [newTaskTestInput]
  mov eax,newTaskTestActual
  mov [rsp+20h],rax
  mov eax,newTaskTestFakeCalls
  mov [rsp+28h],rax
  call wsprintfA
  mov [rsp+40h],eax
  mov ecx,-11
  call GetStdHandle
  mov rcx,rax
  lea rdx,newTaskTestOutput
  mov r8d,[rsp+40h]
  lea r9,newTaskTestRead
  mov qword ptr [rsp+20h],0
  call WriteFile
  mov ecx,newTaskTestStage
  call ExitProcess
NewTaskLaunchTest ENDP

NewTaskTestElevation PROC
  push rbx
  sub rsp,20h
  mov rbx,rcx
  inc newTaskTestFakeCalls
  cmp dword ptr [rbx],112
  jne ntteInvalid
  mov eax,[rbx+4]
  and eax,540h
  cmp eax,540h
  jne ntteInvalid
  cmp dword ptr [rbx+48],1
  jne ntteInvalid
  mov rcx,[rbx+16]
  test rcx,rcx
  jz ntteInvalid
  lea rdx,newTaskRunAs
  call lstrcmpW
  test eax,eax
  jnz ntteInvalid
  mov rcx,[rbx+24]
  test rcx,rcx
  jz ntteInvalid
  mov rdx,newTaskTestRequest
  call lstrcmpW
  test eax,eax
  jnz ntteInvalid
  mov rcx,[rbx+32]
  test rcx,rcx
  jz ntteInvalid
  mov rdx,[newTaskTestRequest+8]
  call lstrcmpW
  test eax,eax
  jnz ntteInvalid
  mov rcx,[rbx+40]
  mov rdx,[newTaskTestRequest+16]
  cmp rcx,rdx
  je ntteFieldsValid
  test rcx,rcx
  jz ntteInvalid
  test rdx,rdx
  jz ntteInvalid
  call lstrcmpW
  test eax,eax
  jnz ntteInvalid
ntteFieldsValid:
  mov ecx,dword ptr [newTaskTestInput+24]
  test ecx,ecx
  jz ntteSuccess
  call SetLastError
  xor eax,eax
  jmp ntteDone
ntteInvalid:
  mov newTaskTestBoundaryInvalid,1 ; sticky, independent of expected API error
  mov ecx,87
  call SetLastError
  xor eax,eax
  jmp ntteDone
ntteSuccess:
  xor ecx,ecx
  xor edx,edx
  xor r8d,r8d
  xor r9d,r9d
  call CreateEventW
  mov newTaskTestFakeHandle,rax
  mov [rbx+104],rax
  test rax,rax
  setne al
  movzx eax,al
ntteDone:
  add rsp,20h
  pop rbx
  ret
NewTaskTestElevation ENDP

NewTaskFixtureEntry PROC
  push rbx
  push rsi
  push rdi
  sub rsp,40h
  mov dword ptr [newTaskFixtureReport],31544B4Eh
  mov ecx,32768
  lea rdx,newTaskFixtureReport+16
  call GetCurrentDirectoryW
  test eax,eax
  jz ntfeFail
  cmp eax,32768
  jae ntfeFail
  call GetCommandLineW
  mov rbx,rax
  lea rcx,newTaskFixtureReport+65552
  mov rdx,rax
  mov r8d,32768
  call lstrcpynW
  mov rcx,rbx
  lea rdx,newTaskFixtureReport+4
  call CommandLineToArgvW
  mov newTaskTestFixtureArgv,rax
  test rax,rax
  jz ntfeFail
  mov rbx,rax
  xor esi,esi
  lea rdi,newTaskFixtureReport+131088
ntfeArgument:
  cmp esi,dword ptr [newTaskFixtureReport+4]
  jae ntfeArgumentsDone
  cmp esi,8
  jae ntfeArgumentsDone
  mov rcx,rdi
  mov rdx,[rbx+rsi*8]
  mov r8d,512
  call lstrcpynW
  inc esi
  add rdi,1024
  jmp ntfeArgument
ntfeArgumentsDone:
  mov rcx,newTaskTestFixtureArgv
  call LocalFree
  call IsObservationElevated
  mov dword ptr [newTaskFixtureReport+8],eax
  lea rcx,newTaskTestHandleEnv
  lea rdx,newTaskTestHandleText
  mov r8d,32
  call GetEnvironmentVariableW
  test eax,eax
  jz ntfeFail
  lea rcx,newTaskTestHandleText
  xor edx,edx
  lea r8,newTaskTestHandle
  call StrToInt64ExW
  test eax,eax
  jz ntfeFail
  lea rcx,newTaskTestKernelBase
  call GetModuleHandleW
  test rax,rax
  jz ntfeFail
  mov rcx,rax
  lea rdx,newTaskTestCompareName
  call GetProcAddress
  test rax,rax
  jz ntfeFail
  mov newTaskTestCompare,rax
  lea rcx,newTaskTestEventEnv
  lea rdx,newTaskTestEventName
  mov r8d,128
  call GetEnvironmentVariableW
  test eax,eax
  jz ntfeFail
  mov rcx,newTaskTestHandle
  lea rdx,newTaskTestFlags
  call GetHandleInformation
  test eax,eax
  jz ntfeInheritanceDone ; do not allocate a new handle into this empty number
  mov ecx,100000h ; SYNCHRONIZE; compare actual object identity, not handle number
  xor edx,edx
  lea r8,newTaskTestEventName
  call OpenEventW
  test rax,rax
  jz ntfeFail
  mov rbx,rax
  mov rcx,newTaskTestHandle
  mov rdx,rbx
  call qword ptr newTaskTestCompare
  mov dword ptr [newTaskFixtureReport+12],eax
  mov rcx,rbx
  call CloseHandle
ntfeInheritanceDone:
  lea rcx,newTaskTestReportFile
  mov edx,40000000h
  mov r8d,1
  xor r9d,r9d
  mov qword ptr [rsp+20h],2
  mov qword ptr [rsp+28h],80h
  mov qword ptr [rsp+30h],0
  call CreateFileW
  cmp rax,-1
  je ntfeFail
  mov rbx,rax
  mov rcx,rax
  lea rdx,newTaskFixtureReport
  mov r8d,139280
  lea r9,newTaskTestRead
  mov qword ptr [rsp+20h],0
  call WriteFile
  mov esi,eax
  mov rcx,rbx
  call CloseHandle
  test esi,esi
  jz ntfeFail
  cmp newTaskTestRead,139280
  jne ntfeFail
  mov ecx,2 ; signal only after the complete report is closed
  xor edx,edx
  lea r8,newTaskTestEventName
  call OpenEventW
  test rax,rax
  jz ntfeFail
  mov rbx,rax
  mov rcx,rax
  call SetEvent
  mov rcx,rbx
  call CloseHandle
  xor ecx,ecx
  call ExitProcess
ntfeFail:
  mov ecx,6001
  call ExitProcess
NewTaskFixtureEntry ENDP
