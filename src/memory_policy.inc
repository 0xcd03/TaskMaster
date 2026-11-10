; UI-owned trigger state; collector performs the bounded actual trim.
; No standby-list purge, compression toggle, process killing or undocumented API.
extern OpenProcessToken:proc, LookupPrivilegeValueW:proc, AdjustTokenPrivileges:proc
extern GetCurrentProcess:proc, GetForegroundWindow:proc, GetWindowThreadProcessId:proc
extern GetTokenInformation:proc
.data
align 8
memoryPolicyAutoEnabled dd 0
memoryPolicyThreshold dd 85
memoryPolicyArmed dd 1
memoryPolicyPrivilege dd 0
memoryPolicyForegroundPid dd 0
align 8
memoryPolicyHighSince dq 0
memoryPolicyLastRun dq 0
observationHintEn dw 82,117,110,32,97,115,32,97,100,109,105,110,105,115,116,114,97,116,111,114,32,116,111,32,115,101,101,32,115,101,114,118
dw 105,99,101,32,109,101,116,114,105,99,115,46,32,80,114,111,116,101,99,116,101,100,32,100,97,116,97,32,99,97,110,32
dw 114,101,109,97,105,110,32,117,110,97,118,97,105,108,97,98,108,101,46,0
observationHintRu dw 1047,1072,1087,1091,1089,1090,1080,1090,1077,32,1086,1090,32,1072,1076,1084,1080,1085,1080,1089,1090,1088,1072,1090,1086,1088,1072,32,1076,1083,1103,32
dw 1076,1072,1085,1085,1099,1093,32,1089,1083,1091,1078,1073,46,32,1047,1072,1097,1080,1097,1105,1085,1085,1099,1077,32,1076,1072,1085,1085,1099,1077,32
dw 1084,1086,1075,1091,1090,32,1086,1089,1090,1072,1074,1072,1090,1100,1089,1103,32,1085,1077,1076,1086,1089,1090,1091,1087,1085,1099,46,0
memoryPolicyPrivilegeName dw 'S','e','D','e','b','u','g','P','r','i','v','i','l','e','g','e',0
.data?
align 8
memoryPolicyStatus db 64 dup(?)
.code
; Enables only a privilege already present in this process token. A standard
; user receives ERROR_NOT_ALL_ASSIGNED; no elevation or bypass is attempted.
EnableObservationPrivilege PROC
  sub rsp,68h
  call GetCurrentProcess
  mov rcx,rax
  mov edx,28h ; TOKEN_QUERY | TOKEN_ADJUST_PRIVILEGES
  lea r8,[rsp+30h]
  call OpenProcessToken
  test eax,eax
  jz eopDone
  xor ecx,ecx
  lea rdx,memoryPolicyPrivilegeName
  lea r8,[rsp+44h]
  call LookupPrivilegeValueW
  test eax,eax
  jz eopClose
  mov dword ptr [rsp+40h],1
  mov dword ptr [rsp+4Ch],2 ; SE_PRIVILEGE_ENABLED
  xor ecx,ecx
  call SetLastError
  mov rcx,[rsp+30h]
  xor edx,edx
  lea r8,[rsp+40h]
  xor r9d,r9d
  mov qword ptr [rsp+20h],0
  mov qword ptr [rsp+28h],0
  call AdjustTokenPrivileges
  test eax,eax
  jz eopClose
  call GetLastError
  test eax,eax
  jnz eopClose
  mov memoryPolicyPrivilege,1
eopClose:
  mov rcx,[rsp+30h]
  call CloseHandle
eopDone:
  add rsp,68h
  ret
EnableObservationPrivilege ENDP

MemoryPolicyReset PROC
  mov memoryPolicyHighSince,0
  mov memoryPolicyArmed,1
  ; Keep last run: changing an option must not bypass the five-minute cooldown.
  ret
MemoryPolicyReset ENDP

; ECX load percent, RDX monotonic milliseconds. Pure decision, no OS mutations.
; Requires ten continuous seconds >= threshold, rearm <= threshold-5,
; and at least five minutes since any global optimization.
MemoryPolicyEvaluate PROC
  xor eax,eax
  cmp memoryPolicyAutoEnabled,0
  je mpeReset
  cmp memoryPolicyThreshold,50
  jb mpeReset
  cmp memoryPolicyThreshold,95
  ja mpeReset
  cmp ecx,100
  ja mpeReset
  mov r8d,memoryPolicyThreshold
  sub r8d,5
  cmp ecx,r8d
  ja mpeHigh
  mov memoryPolicyArmed,1
mpeHigh:
  cmp ecx,memoryPolicyThreshold
  jb mpeReset
  cmp memoryPolicyArmed,0
  je mpeReset
  mov r8,memoryPolicyLastRun
  test r8,r8
  jz mpeDuration
  mov r9,rdx
  sub r9,r8
  cmp r9,300000
  jb mpeReset
mpeDuration:
  mov r8,memoryPolicyHighSince
  test r8,r8
  jnz mpeElapsed
  mov memoryPolicyHighSince,rdx
  ret
mpeElapsed:
  mov r9,rdx
  sub r9,r8
  cmp r9,10000
  jb mpeDone
  mov memoryPolicyLastRun,rdx
  mov memoryPolicyHighSince,0
  mov memoryPolicyArmed,0
  mov eax,1
  ret
mpeReset:
  mov memoryPolicyHighSince,0
mpeDone:
  ret
MemoryPolicyEvaluate ENDP

; Call from the main/UI timer. QueueAction uses its existing single-action gate.
MemoryPolicyTick PROC
  sub rsp,28h
  cmp uiRegressionMode,0
  jne mptDone
  ; Remember the last external foreground window before the user clicks Boost.
  call GetForegroundWindow
  test rax,rax
  jz mptCheckEnabled
  mov dword ptr [rsp+20h],0
  mov rcx,rax
  lea rdx,[rsp+20h]
  call GetWindowThreadProcessId
  call GetCurrentProcessId
  cmp eax,dword ptr [rsp+20h]
  je mptCheckEnabled
  mov eax,dword ptr [rsp+20h]
  mov memoryPolicyForegroundPid,eax
mptCheckEnabled:
  cmp memoryPolicyAutoEnabled,0
  je mptDone
  cmp actionBusy,0
  jne mptBusy
  mov dword ptr [memoryPolicyStatus],64
  lea rcx,memoryPolicyStatus
  call GlobalMemoryStatusEx
  test eax,eax
  jz mptBusy
  call GetTickCount64
  mov rdx,rax
  mov ecx,dword ptr [memoryPolicyStatus+4]
  call MemoryPolicyEvaluate
  test eax,eax
  jz mptDone
  mov ecx,2005
  call QueueAction
  jmp mptDone
mptBusy:
  mov memoryPolicyHighSince,0
mptDone:
  add rsp,28h
  ret
MemoryPolicyTick ENDP

; Return EAX=1 only for a positively identified elevated token.
IsObservationElevated PROC
  sub rsp,48h
  call GetCurrentProcess
  mov rcx,rax
  mov edx,8
  lea r8,[rsp+30h]
  call OpenProcessToken
  test eax,eax
  jz ioeNo
  mov dword ptr [rsp+38h],0
  mov rcx,[rsp+30h]
  mov edx,20 ; TokenElevation
  lea r8,[rsp+38h]
  mov r9d,4
  lea rax,[rsp+3Ch]
  mov [rsp+20h],rax
  call GetTokenInformation
  mov [rsp+40h],eax
  mov rcx,[rsp+30h]
  call CloseHandle
  cmp dword ptr [rsp+40h],0
  je ioeNo
  mov eax,[rsp+38h]
  jmp ioeDone
ioeNo:
  xor eax,eax
ioeDone:
  add rsp,48h
  ret
IsObservationElevated ENDP
