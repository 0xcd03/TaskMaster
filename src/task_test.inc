extern GetEnvironmentVariableW:proc
.data
taskFixtureEnv dw 'T','M','X','_','T','E','S','T','_','T','A','S','K',0
.code
TaskMutationTest PROC
  push rbx
  sub rsp,40h
  lea rcx,taskFixtureEnv
  lea rdx,startupActionPath
  mov r8d,512
  call GetEnvironmentVariableW
  test eax,eax
  jz tmtFail
  cmp eax,512
  jae tmtFail
  call InitTaskScheduler
  cmp taskFolder,0
  je tmtFail
  mov actionCode,2400
  call ChangeStartupTask
  test eax,eax
  js tmtFail
  call TestTaskEnabled
  test eax,eax
  jne tmtFail
  mov actionCode,2401
  call ChangeStartupTask
  test eax,eax
  js tmtFail
  call TestTaskEnabled
  cmp eax,-1
  jne tmtFail
  xor ecx,ecx
  call ExitProcess
tmtFail:
  mov ecx,41
  call ExitProcess
TaskMutationTest ENDP

TestTaskEnabled PROC
  push rbx
  sub rsp,40h
  lea rcx,startupActionPath
  call SysAllocString
  mov rbx,rax
  mov rcx,taskFolder
  mov rdx,rbx
  lea r8,[rsp+30h]
  mov rax,[rcx]
  call qword ptr [rax+104]
  mov [rsp+38h],eax
  mov rcx,rbx
  call SysFreeString
  mov eax,[rsp+38h]
  test eax,eax
  js tteDone
  mov rbx,[rsp+30h]
  mov rcx,rbx
  lea rdx,[rsp+38h]
  mov rax,[rcx]
  call qword ptr [rax+80]
  test eax,eax
  js tteRelease
  movsx eax,word ptr [rsp+38h]
tteRelease:
  mov [rsp+3Ch],eax
  mov rcx,rbx
  call ReleaseCom
  mov eax,[rsp+3Ch]
tteDone:
  add rsp,40h
  pop rbx
  ret
TestTaskEnabled ENDP
