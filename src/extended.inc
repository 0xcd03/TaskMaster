extern OpenServiceW:proc, StartServiceW:proc, ControlService:proc
extern ChangeServiceConfigW:proc, GetProcessAffinityMask:proc, SetProcessAffinityMask:proc
extern ShellExecuteExW:proc, CreateFileW:proc, MiniDumpWriteDump:proc
extern CreateJobObjectW:proc, SetInformationJobObject:proc, AssignProcessToJobObject:proc
.data
align 8
.data?
align 8
actionService dw 256 dup(?)
.data
.data?
align 8
serviceStatus db 36 dup(?)
.data
pathCapacity dd 32768
.data?
align 8
actionPath dw 32768 dup(?)
.data
.data?
align 8
dumpPath dw 1024 dup(?)
.data
dumpFmt dw '%','s','\','p','r','o','c','e','s','s','-','%','u','-','%','I','6','4','u','.','d','m','p',0
selectFmt dw '/','s','e','l','e','c','t',',','"','%','s','"',0
.data?
align 8
selectArgs dw 32790 dup(?)
.data
explorerText dw 'e','x','p','l','o','r','e','r','.','e','x','e',0
openVerb dw 'o','p','e','n',0
propertiesVerb dw 'p','r','o','p','e','r','t','i','e','s',0
.data?
align 8
shellInfo db 112 dup(?)
.data
processAffinity dq 0
systemAffinity dq 0
cpuJobInfo dd 0,0
.data?
align 8
jobHandles dq 128 dup(?)
.data
.data?
align 8
jobPids dd 128 dup(?)
.data
.data?
align 8
jobBirths dq 128 dup(?)
.data
.code
RunServiceAction PROC
  push rbx
  push rsi
  sub rsp,78h
  xor ecx,ecx
  xor edx,edx
  mov r8d,1
  call OpenSCManagerW
  mov rsi,rax
  test rax,rax
  jz rsaError
  mov rcx,rax
  lea rdx,actionService
  mov r8d,36h
  call OpenServiceW
  mov rbx,rax
  test rax,rax
  jz rsaScmError
  cmp actionCode,2200
  je rsaStart
  cmp actionCode,2201
  je rsaStop
  mov rcx,rbx
  mov edx,-1
  mov r8d,2
  cmp actionCode,2202
  je rsaConfigure
  mov r8d,4
rsaConfigure:
  mov r9d,-1
  mov qword ptr [rsp+20h],0
  mov qword ptr [rsp+28h],0
  mov qword ptr [rsp+30h],0
  mov qword ptr [rsp+38h],0
  mov qword ptr [rsp+40h],0
  mov qword ptr [rsp+48h],0
  mov qword ptr [rsp+50h],0
  call ChangeServiceConfigW
  jmp rsaResult
rsaStart:
  mov rcx,rbx
  xor edx,edx
  xor r8d,r8d
  call StartServiceW
  jmp rsaResult
rsaStop:
  mov rcx,rbx
  mov edx,1
  lea r8,serviceStatus
  call ControlService
rsaResult:
  test eax,eax
  jz rsaServiceError
  inc actionSucceeded
  jmp rsaClose
rsaServiceError:
  call GetLastError
  mov actionResult,eax
  inc actionFailed
rsaClose:
  mov rcx,rbx
  call CloseServiceHandle
  mov rcx,rsi
  call CloseServiceHandle
  jmp rsaDone
rsaScmError:
  call GetLastError
  mov actionResult,eax
  inc actionFailed
  mov rcx,rsi
  call CloseServiceHandle
  jmp rsaDone
rsaError:
  call GetLastError
  mov actionResult,eax
  inc actionFailed
rsaDone:
  add rsp,78h
  pop rsi
  pop rbx
  ret
RunServiceAction ENDP

RunExtendedAction PROC
  push rbx
  push rsi
  push rdi
  sub rsp,60h
  mov ecx,1511h
  xor edx,edx
  mov r8d,actionPid
  call OpenProcess
  mov rbx,rax
  test rax,rax
  jz reaError
  mov rcx,rax
  mov edx,actionPid
  call GuardProcess
  test eax,eax
  jz reaBlocked
  mov rcx,rbx
  lea rdx,checkCreate
  lea r8,checkExit
  lea r9,checkKernel
  lea rax,checkUser
  mov [rsp+20h],rax
  call GetProcessTimes
  test eax,eax
  jz reaCloseError
  mov rax,checkCreate
  cmp rax,actionBirth
  jne reaBlocked
  cmp actionCode,2015
  jae reaLimits
  mov pathCapacity,32768
  mov rcx,rbx
  xor edx,edx
  lea r8,actionPath
  lea r9,pathCapacity
  call QueryFullProcessImageNameW
  test eax,eax
  jz reaCloseError
  cmp actionCode,2014
  je reaDump
  mov dword ptr [shellInfo],112
  mov dword ptr [shellInfo+4],40Ch
  mov rax,gMain
  mov qword ptr [shellInfo+8],rax
  lea rax,actionPath
  mov qword ptr [shellInfo+24],rax
  mov qword ptr [shellInfo+32],0
  mov dword ptr [shellInfo+48],1
  lea rax,propertiesVerb
  mov qword ptr [shellInfo+16],rax
  cmp actionCode,2013
  je reaShell
  lea rcx,selectArgs
  lea rdx,selectFmt
  lea r8,actionPath
  call wsprintfW
  lea rax,explorerText
  mov qword ptr [shellInfo+24],rax
  lea rax,selectArgs
  mov qword ptr [shellInfo+32],rax
  lea rax,openVerb
  mov qword ptr [shellInfo+16],rax
reaShell:
  lea rcx,shellInfo
  call ShellExecuteExW
  jmp reaResult
reaDump:
  lea rcx,dumpPath
  lea rdx,dumpFmt
  lea r8,storageDir
  mov r9d,actionPid
  mov rax,actionBirth
  mov [rsp+20h],rax
  call wsprintfW
  lea rcx,dumpPath
  mov edx,40000000h
  xor r8d,r8d
  xor r9d,r9d
  mov qword ptr [rsp+20h],1
  mov qword ptr [rsp+28h],80h
  mov qword ptr [rsp+30h],0
  call CreateFileW
  mov rsi,rax
  cmp rax,-1
  je reaCloseError
  mov rcx,rbx
  mov edx,actionPid
  mov r8,rsi
  xor r9d,r9d
  mov qword ptr [rsp+20h],0
  mov qword ptr [rsp+28h],0
  mov qword ptr [rsp+30h],0
  call MiniDumpWriteDump
  mov edi,eax
  call GetLastError
  mov actionResult,eax
  mov rcx,rsi
  call CloseHandle
  mov eax,edi
  jmp reaResult
reaLimits:
  cmp actionCode,2017
  jb reaCpuLimit
  mov rcx,rbx
  lea rdx,processAffinity
  lea r8,systemAffinity
  call GetProcessAffinityMask
  test eax,eax
  jz reaCloseError
  mov rdx,systemAffinity
  cmp actionCode,2018
  je reaAffinity
  mov rdx,processAffinity
  mov rax,rdx
  neg rax
  and rdx,rax
reaAffinity:
  mov rcx,rbx
  call SetProcessAffinityMask
  jmp reaResult
reaCpuLimit:
  mov rcx,rbx
  call ApplyCpuJob
  jmp reaResult
reaResult:
  test eax,eax
  jz reaCloseError
  inc actionSucceeded
  jmp reaClose
reaBlocked:
  mov actionResult,5
  inc actionFailed
  jmp reaClose
reaCloseError:
  call GetLastError
  mov actionResult,eax
  inc actionFailed
reaClose:
  mov rcx,rbx
  call CloseHandle
  jmp reaDone
reaError:
  call GetLastError
  mov actionResult,eax
  inc actionFailed
reaDone:
  add rsp,60h
  pop rdi
  pop rsi
  pop rbx
  ret
RunExtendedAction ENDP

ApplyCpuJob PROC
  push rbx
  push rsi
  push rdi
  sub rsp,20h
  mov rbx,rcx
  lea rdi,jobHandles
  xor esi,esi
acjFind:
  cmp esi,128
  jae acjFail
  cmp qword ptr [rdi+rsi*8],0
  je acjNext
  lea rdx,jobPids
  mov eax,[rdx+rsi*4]
  cmp eax,actionPid
  jne acjNext
  lea rdx,jobBirths
  mov rax,[rdx+rsi*8]
  cmp rax,actionBirth
  je acjConfigure
acjNext:
  inc esi
  jmp acjFind
acjFail:
  cmp actionCode,2016
  je acjFalse
  xor esi,esi
acjSlot:
  cmp esi,128
  jae acjFalse
  cmp qword ptr [rdi+rsi*8],0
  je acjCreate
  inc esi
  jmp acjSlot
acjCreate:
  xor ecx,ecx
  xor edx,edx
  call CreateJobObjectW
  test rax,rax
  jz acjFalse
  mov [rdi+rsi*8],rax
  mov rcx,rax
  mov rdx,rbx
  call AssignProcessToJobObject
  test eax,eax
  jz acjCloseFail
  lea rdx,jobPids
  mov eax,actionPid
  mov [rdx+rsi*4],eax
  lea rdx,jobBirths
  mov rax,actionBirth
  mov [rdx+rsi*8],rax
acjConfigure:
  mov cpuJobInfo,5
  mov dword ptr [cpuJobInfo+4],2000
  cmp actionCode,2015
  je acjSet
  mov cpuJobInfo,0
acjSet:
  mov rcx,[rdi+rsi*8]
  mov edx,15
  lea r8,cpuJobInfo
  mov r9d,8
  call SetInformationJobObject
  jmp acjDone
acjCloseFail:
  mov rcx,[rdi+rsi*8]
  call CloseHandle
  mov qword ptr [rdi+rsi*8],0
acjFalse:
  xor eax,eax
acjDone:
  add rsp,20h
  pop rdi
  pop rsi
  pop rbx
  ret
ApplyCpuJob ENDP

