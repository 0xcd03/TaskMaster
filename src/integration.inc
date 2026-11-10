; Session mutex + reversible Task Manager integration. No service/driver/runtime.
extern CreateMutexExW:proc, ReleaseMutex:proc, FindWindowW:proc
extern RegisterWindowMessageW:proc, ChangeWindowMessageFilterEx:proc
extern SetForegroundWindow:proc, ShowWindowAsync:proc
extern AllowSetForegroundWindow:proc, GetWindowThreadProcessId:proc
extern PathFindFileNameW:proc, lstrcmpiW:proc
extern ConvertStringSecurityDescriptorToSecurityDescriptorW:proc, LocalFree:proc
extern GetModuleFileNameW:proc, lstrcmpW:proc, PostMessageW:proc
.data
align 8
singleInstance dd 1
integrationMessage dd 0
integrationOwnsMutex dd 0
integrationReplacement dd 0
integrationMutex dq 0
integrationRoot dq -2147483646 ; HKLM (tests substitute HKCU)
integrationKeyPath dq integrationIfeoPath
integrationLastError dd 0
integrationTestMode dd 0
integrationDescriptor dq 0
integrationSecurityAttributes dd 24,0
dq 0
dd 0,0
integrationMutexName dw 76,111,99,97,108,92,84,97,115,107,77,97,115,116,101,114,46,83,105,110,103,108,101,73,110,115,116,97,110,99,101,46,50,55,67,54,56,67,50,54,46,118,49,0
integrationMessageName dw 84,97,115,107,77,97,115,116,101,114,46,83,105,110,103,108,101,73,110,115,116,97,110,99,101,46,50,55,67,54,56,67,50,54,46,118,49,0
integrationSingleKey dw 83,105,110,103,108,101,73,110,115,116,97,110,99,101,0
integrationSecurity dw 68,58,40,65,59,59,48,120,48,48,49,48,48,48,48,49,59,59,59,65,85,41,40,65,59,59,71,65,59,59,59,83
dw 89,41,40,65,59,59,71,65,59,59,59,66,65,41,83,58,40,77,76,59,59,78,87,59,59,59,77,69,41,0
integrationIfeoPath dw 83,79,70,84,87,65,82,69,92,77,105,99,114,111,115,111,102,116,92,87,105,110,100,111,119,115,32,78,84,92,67,117
dw 114,114,101,110,116,86,101,114,115,105,111,110,92,73,109,97,103,101,32,70,105,108,101,32,69,120,101,99,117,116,105,111
dw 110,32,79,112,116,105,111,110,115,92,116,97,115,107,109,103,114,46,101,120,101,0
integrationTaskmgrName dw 116,97,115,107,109,103,114,46,101,120,101,0
integrationDebuggerName dw 68,101,98,117,103,103,101,114,0
integrationOwnerName dw 84,97,115,107,77,97,115,116,101,114,46,68,101,98,117,103,103,101,114,0
integrationCommandFormat dw 34,37,115,34,32,45,45,116,97,115,107,109,103,114,0
integrationConfirmEn dw 82,101,112,108,97,99,101,32,87,105,110,100,111,119,115,32,84,97,115,107,32,77,97,110,97,103,101,114,32,119,105,116
dw 104,32,116,104,105,115,32,99,111,112,121,32,111,102,32,84,97,115,107,77,97,115,116,101,114,63,32,84,97,115,107,32
dw 77,97,110,97,103,101,114,32,99,111,109,109,97,110,100,115,44,32,105,110,99,108,117,100,105,110,103,32,116,104,101,32
dw 115,101,99,117,114,105,116,121,32,115,99,114,101,101,110,32,40,67,116,114,108,43,65,108,116,43,68,101,108,41,44,32
dw 119,105,108,108,32,111,112,101,110,32,116,104,105,115,32,69,88,69,46,32,65,100,109,105,110,105,115,116,114,97,116,111
dw 114,32,114,105,103,104,116,115,32,97,114,101,32,114,101,113,117,105,114,101,100,46,32,75,101,101,112,32,116,104,101,32
dw 69,88,69,32,97,116,32,116,104,105,115,32,112,97,116,104,59,32,116,117,114,110,32,116,104,105,115,32,111,112,116,105
dw 111,110,32,111,102,102,32,98,101,102,111,114,101,32,109,111,118,105,110,103,32,111,114,32,100,101,108,101,116,105,110,103
dw 32,105,116,46,0
integrationConfirmRu dw 1047,1072,1084,1077,1085,1080,1090,1100,32,1044,1080,1089,1087,1077,1090,1095,1077,1088,32,1079,1072,1076,1072,1095,32,87,105,110,100,111,119,115
dw 32,1101,1090,1086,1081,32,1082,1086,1087,1080,1077,1081,32,84,97,115,107,77,97,115,116,101,114,63,32,1042,1099,1079,1086,1074,32,1076
dw 1080,1089,1087,1077,1090,1095,1077,1088,1072,44,32,1074,1082,1083,1102,1095,1072,1103,32,1101,1082,1088,1072,1085,32,67,116,114,108,43,65,108
dw 116,43,68,101,108,44,32,1073,1091,1076,1077,1090,32,1086,1090,1082,1088,1099,1074,1072,1090,1100,32,1101,1090,1086,1090,32,69,88,69,46
dw 32,1053,1091,1078,1085,1099,32,1087,1088,1072,1074,1072,32,1072,1076,1084,1080,1085,1080,1089,1090,1088,1072,1090,1086,1088,1072,46,32,1057,1086,1093
dw 1088,1072,1085,1080,1090,1077,32,69,88,69,32,1087,1086,32,1101,1090,1086,1084,1091,32,1087,1091,1090,1080,59,32,1086,1090,1082,1083,1102,1095
dw 1080,1090,1077,32,1079,1072,1084,1077,1085,1091,32,1087,1077,1088,1077,1076,32,1087,1077,1088,1077,1084,1077,1097,1077,1085,1080,1077,1084,32,1080,1083
dw 1080,32,1091,1076,1072,1083,1077,1085,1080,1077,1084,32,1092,1072,1081,1083,1072,46,0
integrationEnabledEn dw 84,97,115,107,77,97,115,116,101,114,32,110,111,119,32,114,101,112,108,97,99,101,115,32,87,105,110,100,111,119,115,32
dw 84,97,115,107,32,77,97,110,97,103,101,114,46,32,84,117,114,110,32,111,102,102,32,116,104,101,32,99,104,101,99,107
dw 101,100,32,111,112,116,105,111,110,32,98,101,102,111,114,101,32,109,111,118,105,110,103,32,111,114,32,100,101,108,101,116
dw 105,110,103,32,116,104,105,115,32,69,88,69,46,0
integrationEnabledRu dw 84,97,115,107,77,97,115,116,101,114,32,1079,1072,1084,1077,1085,1103,1077,1090,32,1044,1080,1089,1087,1077,1090,1095,1077,1088,32,1079,1072
dw 1076,1072,1095,32,87,105,110,100,111,119,115,46,32,1054,1090,1082,1083,1102,1095,1080,1090,1077,32,1086,1090,1084,1077,1095,1077,1085,1085,1091
dw 1102,32,1086,1087,1094,1080,1102,32,1087,1077,1088,1077,1076,32,1087,1077,1088,1077,1084,1077,1097,1077,1085,1080,1077,1084,32,1080,1083,1080,32,1091
dw 1076,1072,1083,1077,1085,1080,1077,1084,32,69,88,69,46,0
integrationDisabledEn dw 87,105,110,100,111,119,115,32,84,97,115,107,32,77,97,110,97,103,101,114,32,104,97,115,32,98,101,101,110,32,114,101,115,116,111,114,101,100,46,0
integrationDisabledRu dw 1064,1090,1072,1090,1085,1099,1081,32,1044,1080,1089,1087,1077,1090,1095,1077,1088,32,1079,1072,1076,1072,1095,32,87,105,110,100,111,119,115,32
dw 1074,1086,1089,1089,1090,1072,1085,1086,1074,1083,1077,1085,46,0
integrationFailureEn dw 67,111,117,108,100,32,110,111,116,32,99,104,97,110,103,101,32,116,104,101,32,115,101,116,116,105,110,103,46,32,87,105
dw 110,100,111,119,115,32,101,114,114,111,114,58,32,37,117,46,32,65,100,109,105,110,105,115,116,114,97,116,111,114,32,114
dw 105,103,104,116,115,32,97,114,101,32,114,101,113,117,105,114,101,100,32,102,111,114,32,84,97,115,107,32,77,97,110,97
dw 103,101,114,32,114,101,112,108,97,99,101,109,101,110,116,46,32,65,110,32,101,120,105,115,116,105,110,103,32,114,101,112
dw 108,97,99,101,109,101,110,116,32,98,121,32,97,110,111,116,104,101,114,32,97,112,112,108,105,99,97,116,105,111,110,32
dw 105,115,32,110,101,118,101,114,32,111,118,101,114,119,114,105,116,116,101,110,46,0
integrationFailureRu dw 1053,1077,32,1091,1076,1072,1083,1086,1089,1100,32,1080,1079,1084,1077,1085,1080,1090,1100,32,1085,1072,1089,1090,1088,1086,1081,1082,1091,46,32,1054
dw 1096,1080,1073,1082,1072,32,87,105,110,100,111,119,115,58,32,37,117,46,32,1044,1083,1103,32,1079,1072,1084,1077,1085,1099,32,1076,1080
dw 1089,1087,1077,1090,1095,1077,1088,1072,32,1085,1091,1078,1085,1099,32,1087,1088,1072,1074,1072,32,1072,1076,1084,1080,1085,1080,1089,1090,1088,1072,1090
dw 1086,1088,1072,46,32,1047,1072,1084,1077,1085,1072,44,32,1085,1072,1089,1090,1088,1086,1077,1085,1085,1072,1103,32,1076,1088,1091,1075,1080,1084,32
dw 1087,1088,1080,1083,1086,1078,1077,1085,1080,1077,1084,44,32,1085,1077,32,1087,1077,1088,1077,1079,1072,1087,1080,1089,1099,1074,1072,1077,1090,1089,1103
dw 46,0
integrationStartFailureEn dw 84,97,115,107,77,97,115,116,101,114,32,99,111,117,108,100,32,110,111,116,32,99,104,101,99,107,32,116,104,101,32,115
dw 105,110,103,108,101,45,105,110,115,116,97,110,99,101,32,108,111,99,107,46,32,65,110,111,116,104,101,114,32,99,111,112
dw 121,32,109,97,121,32,97,108,114,101,97,100,121,32,98,101,32,115,116,97,114,116,105,110,103,46,32,84,104,105,115,32
dw 108,97,117,110,99,104,32,119,105,108,108,32,99,108,111,115,101,46,0
integrationStartFailureRu dw 1053,1077,32,1091,1076,1072,1083,1086,1089,1100,32,1087,1088,1086,1074,1077,1088,1080,1090,1100,32,1077,1076,1080,1085,1089,1090,1074,1077,1085,1085,1091
dw 1102,32,1082,1086,1087,1080,1102,32,84,97,115,107,77,97,115,116,101,114,46,32,1042,1086,1079,1084,1086,1078,1085,1086,44,32,1076,1088
dw 1091,1075,1072,1103,32,1082,1086,1087,1080,1103,32,1091,1078,1077,32,1079,1072,1087,1091,1089,1082,1072,1077,1090,1089,1103,46,32,1069,1090,1086,1090
dw 32,1079,1072,1087,1091,1089,1082,32,1073,1091,1076,1077,1090,32,1079,1072,1082,1088,1099,1090,46,0
.data?
align 8
integrationExe dw 32768 dup(?)
integrationCommand dw 32784 dup(?)
integrationActual dw 32784 dup(?)
integrationOwner dw 32784 dup(?)
integrationReport dw 512 dup(?)
.code

ReadSingleInstance PROC
  sub rsp,28h
  lea rcx,configSection
  lea rdx,integrationSingleKey
  mov r8d,1
  lea r9,configPath
  call GetPrivateProfileIntW
  and eax,1
  mov singleInstance,eax
  add rsp,28h
  ret
ReadSingleInstance ENDP

; Return 1 only when launch may proceed. Keep mutex for entire process lifetime.
InitIntegration PROC
  push rbx
  sub rsp,30h
  call ReadSingleInstance
  lea rcx,integrationMessageName
  call RegisterWindowMessageW
  mov integrationMessage,eax
  test eax,eax
  jz iiError
  lea rcx,integrationSecurity
  mov edx,1
  lea r8,integrationDescriptor
  xor r9d,r9d
  call ConvertStringSecurityDescriptorToSecurityDescriptorW
  test eax,eax
  jz iiError
  mov rax,integrationDescriptor
  mov qword ptr [integrationSecurityAttributes+8],rax
  lea rcx,integrationSecurityAttributes
  lea rdx,integrationMutexName
  xor r8d,r8d
  mov r9d,100001h ; SYNCHRONIZE | MUTEX_MODIFY_STATE, not ALL_ACCESS
  call CreateMutexExW
  mov integrationMutex,rax
  call GetLastError
  mov ebx,eax
  mov rcx,integrationDescriptor
  call LocalFree
  mov integrationDescriptor,0
  cmp integrationMutex,0
  je iiError
  mov rcx,integrationMutex
  xor edx,edx
  call WaitForSingleObject
  test eax,eax
  jz iiOwner
  cmp eax,80h ; abandoned owner
  je iiOwner
  cmp eax,102h
  jne iiError
  cmp singleInstance,0
  je iiReady
  ; The first process may still be creating its window. Wait at most one
  ; second; never open a second window while it holds the mutex.
  mov ebx,20
 iiWaitWindow:
  lea rcx,szClass
  xor edx,edx
  call FindWindowW
  test rax,rax
  jnz iiActivate
  mov ecx,50
  call Sleep
  dec ebx
  jnz iiWaitWindow
 iiActivate:
  call BroadcastIntegration
  xor eax,eax
  jmp iiDone
 iiOwner:
  mov integrationOwnsMutex,1
 iiReady:
  call InitReplacementCommand
  call QueryTaskManagerReplacement
  mov eax,1
  jmp iiDone
 iiError:
  xor ecx,ecx
  lea rdx,integrationStartFailureEn
  cmp appLanguage,0
  je iiErrorText
  lea rdx,integrationStartFailureRu
 iiErrorText:
  lea r8,szTitle
  mov r9d,30h
  call MessageBoxW
  xor eax,eax
 iiDone:
  add rsp,30h
  pop rbx
  ret
InitIntegration ENDP

RegisterIntegrationWindow PROC
  sub rsp,28h
  mov rcx,gMain
  mov edx,integrationMessage
  mov r8d,1 ; MSGFLT_ALLOW only this custom, revalidated notification
  xor r9d,r9d
  call ChangeWindowMessageFilterEx
  add rsp,28h
  ret
RegisterIntegrationWindow ENDP

BroadcastIntegration PROC
  sub rsp,28h
  lea rcx,szClass
  xor edx,edx
  call FindWindowW
  test rax,rax
  jz biPost
  mov rcx,rax
  lea rdx,[rsp+20h]
  call GetWindowThreadProcessId
  mov ecx,[rsp+20h]
  call AllowSetForegroundWindow
biPost:
  mov ecx,0FFFFh ; HWND_BROADCAST
  mov edx,integrationMessage
  xor r8d,r8d
  xor r9d,r9d
  call PostMessageW
  add rsp,28h
  ret
BroadcastIntegration ENDP

HandleIntegrationMessage PROC
  sub rsp,28h
  call ReadSingleInstance
  cmp singleInstance,0
  je himDone
  cmp integrationOwnsMutex,0
  jne himActivate
  mov rcx,integrationMutex
  xor edx,edx
  call WaitForSingleObject
  test eax,eax
  jz himOwn
  cmp eax,80h
  je himOwn
  ; A secondary window closes gracefully after a real saved setting change.
  mov rcx,gMain
  mov edx,10h
  xor r8d,r8d
  xor r9d,r9d
  call PostMessageW
  jmp himDone
 himOwn:
  mov integrationOwnsMutex,1
 himActivate:
  mov rcx,gMain
  call IsIconic
  test eax,eax
  jz himForeground
  mov rcx,gMain
  mov edx,9 ; SW_RESTORE
  call ShowWindowAsync
 himForeground:
  mov rcx,gMain
  call SetForegroundWindow
 himDone:
  xor eax,eax
  add rsp,28h
  ret
HandleIntegrationMessage ENDP

InitReplacementCommand PROC
  sub rsp,28h
  xor ecx,ecx
  lea rdx,integrationExe
  mov r8d,32768
  call GetModuleFileNameW
  test eax,eax
  jz ircError
  cmp eax,32768
  jae ircError
  ; A renamed taskmgr.exe would recursively trigger its own IFEO redirect.
  lea rcx,integrationExe
  call PathFindFileNameW
  mov rcx,rax
  lea rdx,integrationTaskmgrName
  call lstrcmpiW
  test eax,eax
  jz ircError
  ; Assemble explicitly: wsprintfW has a 1024-char limit.
  lea rcx,integrationCommand
  mov word ptr [rcx],34
  add rcx,2
  lea rdx,integrationExe
  call lstrcpyW
  lea rcx,integrationCommand
  call lstrlenW
  lea rdx,integrationCommand
  lea rdx,[rdx+rax*2]
  mov word ptr [rdx],34
  mov word ptr [rdx+2],32
  mov word ptr [rdx+4],45
  mov word ptr [rdx+6],45
  mov word ptr [rdx+8],116
  mov word ptr [rdx+10],97
  mov word ptr [rdx+12],115
  mov word ptr [rdx+14],107
  mov word ptr [rdx+16],109
  mov word ptr [rdx+18],103
  mov word ptr [rdx+20],114
  mov word ptr [rdx+22],0
  xor eax,eax
  jmp ircDone
 ircError:
  mov word ptr integrationCommand,0
  mov eax,206
 ircDone:
  add rsp,28h
  ret
InitReplacementCommand ENDP

; RCX key, RDX value name, R8 destination (65568 bytes). Return Win32 status.
ReadIntegrationString PROC
  sub rsp,38h
  mov qword ptr [rsp+20h],r8
  mov dword ptr [rsp+30h],65568
  lea rax,[rsp+30h]
  mov [rsp+28h],rax
  xor r8d,r8d
  lea r9,[rsp+34h]
  call RegQueryValueExW
  test eax,eax
  jnz risDone
  cmp dword ptr [rsp+34h],1 ; REG_SZ only, reject type confusion
  jne risInvalid
  mov eax,[rsp+30h]
  test eax,1
  jnz risInvalid
  cmp eax,2
  jb risInvalid
  cmp eax,65568
  ja risInvalid
  mov rdx,[rsp+20h]
  cmp word ptr [rdx+rax-2],0
  jne risInvalid
  xor eax,eax
  jmp risDone
 risInvalid:
  mov eax,13
 risDone:
  add rsp,38h
  ret
ReadIntegrationString ENDP

; Checkbox reflects actual registry state; an ownership marker must match the
; complete Debugger command. Other debugger values are never taken over.
QueryTaskManagerReplacement PROC
  push rbx
  sub rsp,30h
  mov integrationReplacement,0
  mov rcx,integrationRoot
  mov rdx,integrationKeyPath
  xor r8d,r8d
  mov r9d,101h ; QUERY_VALUE | KEY_WOW64_64KEY
  lea rax,[rsp+28h]
  mov [rsp+20h],rax
  call RegOpenKeyExW
  test eax,eax
  jnz qtmDone
  mov rbx,[rsp+28h]
  mov rcx,rbx
  lea rdx,integrationDebuggerName
  lea r8,integrationActual
  call ReadIntegrationString
  test eax,eax
  jnz qtmClose
  mov rcx,rbx
  lea rdx,integrationOwnerName
  lea r8,integrationOwner
  call ReadIntegrationString
  test eax,eax
  jnz qtmClose
  lea rcx,integrationActual
  lea rdx,integrationOwner
  call lstrcmpW
  test eax,eax
  jnz qtmClose
  cmp word ptr integrationActual,0
  je qtmClose
  mov integrationReplacement,1
 qtmClose:
  mov rcx,rbx
  call RegCloseKey
 qtmDone:
  mov eax,integrationReplacement
  add rsp,30h
  pop rbx
  ret
QueryTaskManagerReplacement ENDP

; ECX 1 enable, 0 restore. Tests redirect integrationRoot/keyPath to HKCU.
; Never delete an IFEO key or unrelated values. Keep no stale backup that could
; resurrect another application's newer replacement.
SetTaskManagerReplacement PROC
  push rbx
  push rsi
  sub rsp,68h
  mov esi,ecx
  xor ebx,ebx
  mov rcx,integrationRoot
  mov rdx,integrationKeyPath
  xor r8d,r8d
  mov r9d,103h ; QUERY_VALUE | SET_VALUE | KEY_WOW64_64KEY
  lea rax,[rsp+50h]
  mov [rsp+20h],rax
  call RegOpenKeyExW
  test eax,eax
  jz strOpened
  cmp eax,2
  jne strDone
  test esi,esi
  jnz strCreate
  xor eax,eax
  jmp strDone
 strCreate:
  mov rcx,integrationRoot
  mov rdx,integrationKeyPath
  xor r8d,r8d
  xor r9d,r9d
  mov qword ptr [rsp+20h],0
  mov qword ptr [rsp+28h],103h
  mov qword ptr [rsp+30h],0
  lea rax,[rsp+50h]
  mov [rsp+38h],rax
  mov qword ptr [rsp+40h],0
  ; ninth parameter disposition pointer (NULL)
  call RegCreateKeyExW
  test eax,eax
  jnz strDone
 strOpened:
  mov rbx,[rsp+50h]
  mov rcx,rbx
  lea rdx,integrationDebuggerName
  lea r8,integrationActual
  call ReadIntegrationString
  cmp eax,2
  je strMissing
  test eax,eax
  jnz strClose
  cmp word ptr integrationActual,0
  je strMissing
  mov rcx,rbx
  lea rdx,integrationOwnerName
  lea r8,integrationOwner
  call ReadIntegrationString
  test eax,eax
  jnz strConflict
  lea rcx,integrationActual
  lea rdx,integrationOwner
  call lstrcmpW
  test eax,eax
  jnz strConflict
  test esi,esi
  jnz strSuccess ; already ours, retain existing installed path
  mov rcx,rbx
  lea rdx,integrationDebuggerName
  call RegDeleteValueW
  test eax,eax
  jnz strClose
  mov rcx,rbx
  lea rdx,integrationOwnerName
  call RegDeleteValueW
  jmp strSuccess
 strMissing:
  test esi,esi
  jz strSuccess
  cmp word ptr integrationCommand,0
  je strBadPath
  ; Marker first: failure cannot leave an unowned redirect behind.
  mov rcx,rbx
  lea rdx,integrationOwnerName
  call WriteIntegrationCommand
  test eax,eax
  jnz strClose
  mov rcx,rbx
  lea rdx,integrationDebuggerName
  call WriteIntegrationCommand
  test eax,eax
  jz strSuccess
  mov [rsp+58h],eax
  mov rcx,rbx
  lea rdx,integrationOwnerName
  call RegDeleteValueW
  mov eax,[rsp+58h]
  jmp strClose
 strConflict:
  mov eax,183 ; ERROR_ALREADY_EXISTS: foreign replacement
  jmp strClose
 strBadPath:
  mov eax,206
  jmp strClose
 strSuccess:
  xor eax,eax
 strClose:
  mov [rsp+58h],eax
  mov rcx,rbx
  call RegCloseKey
  mov eax,[rsp+58h]
 strDone:
  mov integrationLastError,eax
  add rsp,68h
  pop rsi
  pop rbx
  ret
SetTaskManagerReplacement ENDP

WriteIntegrationCommand PROC
  sub rsp,48h
  mov [rsp+30h],rcx
  mov [rsp+38h],rdx
  lea rcx,integrationCommand
  call lstrlenW
  lea eax,[eax*2+2]
  mov [rsp+28h],rax
  lea rax,integrationCommand
  mov [rsp+20h],rax
  mov rcx,[rsp+30h]
  mov rdx,[rsp+38h]
  xor r8d,r8d
  mov r9d,1
  call RegSetValueExW
  add rsp,48h
  ret
WriteIntegrationCommand ENDP

IntegrationCommand PROC
  push rbx
  sub rsp,30h
  cmp ecx,2330
  jne icReplacement
  call ReadSingleInstance
  xor singleInstance,1
  lea rcx,configSection
  lea rdx,integrationSingleKey
  lea r8,zeroText
  cmp singleInstance,0
  je icSaveSingle
  lea r8,oneText
 icSaveSingle:
  lea r9,configPath
  call WritePrivateProfileStringW
  test eax,eax
  jnz icNotifySingle
  call GetLastError
  mov integrationLastError,eax
  call ReadSingleInstance
  jmp icError
 icNotifySingle:
  call BroadcastIntegration
  jmp icDone
 icReplacement:
  cmp ecx,2331
  jne icDone
  call QueryTaskManagerReplacement
  mov ebx,eax
  xor ebx,1
  test ebx,ebx
  jz icChange
  mov rcx,gMain
  lea rdx,integrationConfirmEn
  cmp appLanguage,0
  je icConfirm
  lea rdx,integrationConfirmRu
 icConfirm:
  lea r8,szTitle
  mov r9d,124h ; YESNO | ICONQUESTION | DEFBUTTON2
  call MessageBoxW
  cmp eax,6
  jne icDone
 icChange:
  mov ecx,ebx
  call SetTaskManagerReplacement
  test eax,eax
  jnz icError
  lea rdx,integrationDisabledEn
  cmp appLanguage,0
  je icDisabledText
  lea rdx,integrationDisabledRu
 icDisabledText:
  test ebx,ebx
  jz icNotice
  lea rdx,integrationEnabledEn
  cmp appLanguage,0
  je icNotice
  lea rdx,integrationEnabledRu
 icNotice:
  mov rcx,gMain
  lea r8,szTitle
  mov r9d,40h
  call MessageBoxW
  jmp icDone
 icError:
  lea rcx,integrationReport
  lea rdx,integrationFailureEn
  cmp appLanguage,0
  je icFormatError
  lea rdx,integrationFailureRu
 icFormatError:
  mov r8d,integrationLastError
  call wsprintfW
  mov rcx,gMain
  lea rdx,integrationReport
  lea r8,szTitle
  mov r9d,30h
  call MessageBoxW
 icDone:
  call RefreshIntegrationChecks
  add rsp,30h
  pop rbx
  ret
IntegrationCommand ENDP

RefreshIntegrationChecks PROC
  push rbx
  sub rsp,20h
  call QueryTaskManagerReplacement
  mov rcx,mainMenu
  mov edx,4
  call GetSubMenu
  mov rbx,rax
  test rax,rax
  jz ricDone
  mov rcx,rbx
  mov edx,2330
  xor r8d,r8d
  cmp singleInstance,0
  je ricSingle
  mov r8d,8
 ricSingle:
  call CheckMenuItem
  mov rcx,rbx
  mov edx,2331
  xor r8d,r8d
  cmp integrationReplacement,0
  je ricReplacement
  mov r8d,8
 ricReplacement:
  call CheckMenuItem
 ricDone:
  add rsp,20h
  pop rbx
  ret
RefreshIntegrationChecks ENDP
