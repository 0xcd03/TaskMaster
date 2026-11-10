; Isolated registry and named-mutex tests. Never writes production IFEO.
.data
integrationTestStage dd 0
integrationTestCreated dd 0
integrationTestKey dq 0
integrationTestFirstMutex dq 0
integrationTestSecondMutex dq 0
integrationTestThread dq 0
integrationTestWaitResult dd -1
integrationTestBase dw 'S','o','f','t','w','a','r','e','\','T','a','s','k','M','a','s','t','e','r','-','I','n','t','e','g','r','a','t','i','o','n','-','T','e','s','t','-','%','u','-','%','I','6','4','u',0
integrationTestMutexFmt dw 'L','o','c','a','l','\','T','a','s','k','M','a','s','t','e','r','-','M','u','t','e','x','T','e','s','t','-','%','u','-','%','I','6','4','u',0
integrationTestForeign dw '"','C',':','\','f','o','r','e','i','g','n','.','e','x','e','"',0
integrationTestSibling dw 'P','r','e','s','e','r','v','e','M','e',0
integrationTestValue dd 2703
.data?
integrationTestPath dw 256 dup(?)
integrationTestMutexName dw 256 dup(?)
.code
IntegrationTest PROC
  sub rsp,68h
  mov integrationTestMode,1
  mov integrationTestStage,1701
  call GetCurrentProcessId
  mov [rsp+50h],eax
  call GetTickCount64
  mov [rsp+58h],rax
  lea rcx,integrationTestPath
  lea rdx,integrationTestBase
  mov r8d,[rsp+50h]
  mov r9,[rsp+58h]
  call wsprintfW
  lea rcx,integrationTestMutexName
  lea rdx,integrationTestMutexFmt
  mov r8d,[rsp+50h]
  mov r9,[rsp+58h]
  call wsprintfW
  mov integrationRoot,-2147483647 ; HKCU only
  lea rax,integrationTestPath
  mov integrationKeyPath,rax
  mov rcx,integrationRoot
  mov rdx,integrationKeyPath
  xor r8d,r8d
  xor r9d,r9d
  mov qword ptr [rsp+20h],0
  mov qword ptr [rsp+28h],103h
  mov qword ptr [rsp+30h],0
  lea rax,integrationTestKey
  mov [rsp+38h],rax
  lea rax,[rsp+48h]
  mov [rsp+40h],rax
  call RegCreateKeyExW
  test eax,eax
  jnz itFinish
  cmp dword ptr [rsp+48h],1 ; never delete a pre-existing key
  jne itFinish
  mov integrationTestCreated,1
  call InitReplacementCommand
  test eax,eax
  jnz itFinish
  cmp word ptr integrationCommand,34
  jne itFinish
  mov integrationTestStage,1702
  call QueryTaskManagerReplacement
  test eax,eax
  jnz itFinish
  mov ecx,1
  call SetTaskManagerReplacement
  test eax,eax
  jnz itFinish
  call QueryTaskManagerReplacement
  cmp eax,1
  jne itFinish
  mov integrationTestStage,1703
  ; An unrelated value must survive restoration.
  mov rcx,integrationTestKey
  lea rdx,integrationTestSibling
  xor r8d,r8d
  mov r9d,4
  lea rax,integrationTestValue
  mov [rsp+20h],rax
  mov qword ptr [rsp+28h],4
  call RegSetValueExW
  test eax,eax
  jnz itFinish
  xor ecx,ecx
  call SetTaskManagerReplacement
  test eax,eax
  jnz itFinish
  call QueryTaskManagerReplacement
  test eax,eax
  jnz itFinish
  mov rcx,integrationTestKey
  lea rdx,integrationTestSibling
  xor r8d,r8d
  xor r9d,r9d
  lea rax,[rsp+50h]
  mov [rsp+20h],rax
  mov dword ptr [rsp+58h],4
  lea rax,[rsp+58h]
  mov [rsp+28h],rax
  call RegQueryValueExW
  test eax,eax
  jnz itFinish
  cmp dword ptr [rsp+50h],2703
  jne itFinish
  mov integrationTestStage,1704
  ; A foreign Debugger value must survive enable AND disable attempts.
  lea rcx,integrationTestForeign
  call lstrlenW
  lea eax,[eax*2+2]
  mov [rsp+28h],rax
  lea rax,integrationTestForeign
  mov [rsp+20h],rax
  mov rcx,integrationTestKey
  lea rdx,integrationDebuggerName
  xor r8d,r8d
  mov r9d,1
  call RegSetValueExW
  test eax,eax
  jnz itFinish
  mov ecx,1
  call SetTaskManagerReplacement
  cmp eax,183
  jne itFinish
  xor ecx,ecx
  call SetTaskManagerReplacement
  cmp eax,183
  jne itFinish
  mov rcx,integrationTestKey
  lea rdx,integrationDebuggerName
  lea r8,integrationActual
  call ReadIntegrationString
  test eax,eax
  jnz itFinish
  lea rcx,integrationActual
  lea rdx,integrationTestForeign
  call lstrcmpW
  test eax,eax
  jnz itFinish
  mov integrationTestStage,1705
  call QueryTaskManagerReplacement
  test eax,eax
  jnz itFinish
  mov rcx,integrationTestKey
  lea rdx,integrationDebuggerName
  call RegDeleteValueW
  mov ecx,1
  call SetTaskManagerReplacement
  test eax,eax
  jnz itFinish
  ; Marker mismatch must not delete a changed debugger command.
  mov rcx,integrationTestKey
  lea rdx,integrationOwnerName
  call RegDeleteValueW
  xor ecx,ecx
  call SetTaskManagerReplacement
  cmp eax,183
  jne itFinish
  mov integrationTestStage,1706
  lea rcx,integrationSecurity
  mov edx,1
  lea r8,integrationDescriptor
  xor r9d,r9d
  call ConvertStringSecurityDescriptorToSecurityDescriptorW
  test eax,eax
  jz itFinish
  mov rax,integrationDescriptor
  mov qword ptr [integrationSecurityAttributes+8],rax
  lea rcx,integrationSecurityAttributes
  lea rdx,integrationTestMutexName
  mov r8d,1 ; CREATE_MUTEX_INITIAL_OWNER
  mov r9d,100001h
  call CreateMutexExW
  mov integrationTestFirstMutex,rax
  test rax,rax
  jz itFinish
  lea rcx,integrationSecurityAttributes
  lea rdx,integrationTestMutexName
  xor r8d,r8d
  mov r9d,100001h
  call CreateMutexExW
  mov integrationTestSecondMutex,rax
  test rax,rax
  jz itFinish
  call GetLastError
  cmp eax,183
  jne itFinish
  xor ecx,ecx
  xor edx,edx
  lea r8,IntegrationMutexProbe
  xor r9d,r9d
  mov qword ptr [rsp+20h],0
  mov qword ptr [rsp+28h],0
  call CreateThread
  mov integrationTestThread,rax
  test rax,rax
  jz itFinish
  mov rcx,rax
  mov edx,2000
  call WaitForSingleObject
  test eax,eax
  jnz itFinish
  cmp integrationTestWaitResult,102h
  jne itFinish
  mov integrationTestStage,0
itFinish:
  mov rcx,integrationTestThread
  test rcx,rcx
  jz itNoThread
  call CloseHandle
itNoThread:
  mov rcx,integrationTestFirstMutex
  test rcx,rcx
  jz itNoFirstMutex
  call ReleaseMutex
  mov rcx,integrationTestFirstMutex
  call CloseHandle
itNoFirstMutex:
  mov rcx,integrationTestSecondMutex
  test rcx,rcx
  jz itNoSecondMutex
  call CloseHandle
itNoSecondMutex:
  mov rcx,integrationDescriptor
  test rcx,rcx
  jz itNoDescriptor
  call LocalFree
itNoDescriptor:
  mov rcx,integrationTestKey
  test rcx,rcx
  jz itNoKey
  call RegCloseKey
itNoKey:
  cmp integrationTestCreated,0
  je itExit
  mov rcx,-2147483647
  lea rdx,integrationTestPath
  call RegDeleteTreeW
itExit:
  mov ecx,integrationTestStage
  call ExitProcess
IntegrationTest ENDP

IntegrationMutexProbe PROC
  sub rsp,28h
  mov rcx,integrationTestSecondMutex
  xor edx,edx
  call WaitForSingleObject
  mov integrationTestWaitResult,eax
  cmp eax,0
  je impRelease
  cmp eax,80h
  jne impDone
impRelease:
  mov rcx,integrationTestSecondMutex
  call ReleaseMutex
impDone:
  xor eax,eax
  add rsp,28h
  ret
IntegrationMutexProbe ENDP
