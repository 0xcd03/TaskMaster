; Three independent bounded mailboxes. All provider work runs outside locks.
.data
align 8
pmWorkerContexts db PM_LANES*PM_WORKER_CONTEXT_SIZE dup(0)
pmWorkerCollectorQ dq PmCollectField
pmWorkerAllocateQ dq HeapAlloc
pmWorkerClockQ dq GetTickCount64
.code
PmWorkerContext PROC
  xor eax,eax
  dec ecx
  cmp ecx,PM_LANES
  jae pmwcDone
  shl ecx,8
  lea rax,pmWorkerContexts
  add rax,rcx
pmwcDone:
  ret
PmWorkerContext ENDP

PmWorkerSubmit PROC
  push rbx
  push rsi
  push rdi
  push r12
  sub rsp,38h
  mov rsi,rcx
  call PmValidateRequest
  test eax,eax
  jnz pmwsDone
  mov ecx,[rsi+PD_R_LANE]
  call PmWorkerContext
  mov rbx,rax
  mov rcx,rbx
  call AcquireSRWLockExclusive
  mov r12d,1115
  cmp dword ptr [rbx+PM_W_STOP],0
  jne pmwsUnlock
  mov r12d,170
  cmp dword ptr [rbx+PM_W_STATE],0
  jne pmwsUnlock
  cmp qword ptr [rbx+PM_W_THREAD],0
  jne pmwsCopy
  xor ecx,ecx
  xor edx,edx
  xor r8d,r8d
  xor r9d,r9d
  call CreateEventW
  mov [rbx+PM_W_EVENT],rax
  test rax,rax
  jz pmwsError
  xor ecx,ecx
  xor edx,edx
  lea r8,PmWorkerMain
  mov r9,rbx
  mov qword ptr [rsp+20h],0
  mov qword ptr [rsp+28h],0
  call CreateThread
  mov [rbx+PM_W_THREAD],rax
  test rax,rax
  jnz pmwsCreated
  call GetLastError
  mov r12d,eax
  mov rcx,[rbx+PM_W_EVENT]
  call CloseHandle
  mov qword ptr [rbx+PM_W_EVENT],0
  jmp pmwsUnlock
pmwsCreated:
  inc dword ptr [rbx+PM_W_THREADS]
pmwsCopy:
  mov eax,[rsi+PD_R_LANE]
  mov [rbx+PM_W_LANE],eax
  lea rdi,[rbx+PM_W_REQUEST]
  mov ecx,PD_REQUEST_SIZE/8
  rep movsq
  mov dword ptr [rbx+PM_W_STATE],1
  mov dword ptr [rbx+PM_W_ERROR],0
  mov rcx,[rbx+PM_W_EVENT]
  call SetEvent
  test eax,eax
  jz pmwsSignalError
  xor r12d,r12d
  jmp pmwsUnlock
pmwsSignalError:
  mov dword ptr [rbx+PM_W_STATE],0
pmwsError:
  call GetLastError
  mov r12d,eax
  test r12d,r12d
  jnz pmwsUnlock
  mov r12d,8
pmwsUnlock:
  mov rcx,rbx
  call ReleaseSRWLockExclusive
  mov eax,r12d
pmwsDone:
  add rsp,38h
  pop r12
  pop rdi
  pop rsi
  pop rbx
  ret
PmWorkerSubmit ENDP

PmWorkerBusy PROC
  push rbx
  sub rsp,30h
  call PmWorkerContext
  mov rbx,rax
  mov eax,1
  test rbx,rbx
  jz pmwbDone
  mov rcx,rbx
  call AcquireSRWLockExclusive
  mov eax,[rbx+PM_W_STATE]
  or eax,[rbx+PM_W_STOP]
  test eax,eax
  setnz al
  movzx eax,al
  mov [rsp+20h],eax
  mov rcx,rbx
  call ReleaseSRWLockExclusive
  mov eax,[rsp+20h]
pmwbDone:
  add rsp,30h
  pop rbx
  ret
PmWorkerBusy ENDP

PmWorkerTake PROC
  push rbx
  push rsi
  push rdi
  push r12
  push r13
  sub rsp,20h
  xor r12d,r12d
  xor r13d,r13d
  test rdx,rdx
  jz pmwtDone
  test r8,r8
  jz pmwtDone
  mov rdi,rdx
  mov rsi,r8
  call PmWorkerContext
  mov rbx,rax
  test rbx,rbx
  jz pmwtDone
  mov rcx,rbx
  call AcquireSRWLockExclusive
  cmp dword ptr [rbx+PM_W_STATE],2
  jne pmwtUnlock
  mov eax,[rbx+PM_W_ERROR]
  mov [rsi],eax
  lea rsi,[rbx+PM_W_REQUEST]
  mov ecx,PD_REQUEST_SIZE/8
  rep movsq
  mov r12,[rbx+PM_W_RESULT]
  mov qword ptr [rbx+PM_W_RESULT],0
  mov dword ptr [rbx+PM_W_ERROR],0
  mov dword ptr [rbx+PM_W_STATE],0
  mov r13d,1
pmwtUnlock:
  mov rcx,rbx
  call ReleaseSRWLockExclusive
pmwtDone:
  mov rax,r12
  mov edx,r13d
  add rsp,20h
  pop r13
  pop r12
  pop rdi
  pop rsi
  pop rbx
  ret
PmWorkerTake ENDP

PmWorkerFree PROC
  push rbx
  sub rsp,20h
  mov rbx,rcx
  test rcx,rcx
  jz pmwfDone
  call GetProcessHeap
  mov rcx,rax
  xor edx,edx
  mov r8,rbx
  call HeapFree
pmwfDone:
  add rsp,20h
  pop rbx
  ret
PmWorkerFree ENDP

PmWorkerStop PROC
  push rbx
  push rsi
  push rdi
  sub rsp,20h
  lea rbx,pmWorkerContexts
  mov edi,PM_LANES
pmwstNext:
  mov rcx,rbx
  call AcquireSRWLockExclusive
  mov dword ptr [rbx+PM_W_STOP],1
  mov rsi,[rbx+PM_W_RESULT]
  mov qword ptr [rbx+PM_W_RESULT],0
  mov dword ptr [rbx+PM_W_ERROR],0
  cmp dword ptr [rbx+PM_W_STATE],2
  jne pmwstSignal
  mov dword ptr [rbx+PM_W_STATE],0
pmwstSignal:
  mov rcx,[rbx+PM_W_EVENT]
  test rcx,rcx
  jz pmwstUnlock
  call SetEvent
pmwstUnlock:
  mov rcx,rbx
  call ReleaseSRWLockExclusive
  mov rcx,rsi
  call PmWorkerFree
  add rbx,PM_WORKER_CONTEXT_SIZE
  dec edi
  jnz pmwstNext
  add rsp,20h
  pop rdi
  pop rsi
  pop rbx
  ret
PmWorkerStop ENDP

PmWorkerMain PROC
  push rbx
  push rsi
  push rdi
  push r12
  push r13
  sub rsp,80h
  mov rbx,rcx
pmwmWait:
  mov rcx,[rbx+PM_W_EVENT]
  mov edx,-1
  call WaitForSingleObject
  mov rcx,rbx
  call AcquireSRWLockExclusive
  cmp dword ptr [rbx+PM_W_STOP],0
  jne pmwmExit
  cmp dword ptr [rbx+PM_W_STATE],1
  jne pmwmIdle
  lea rsi,[rbx+PM_W_REQUEST]
  lea rdi,[rsp+20h]
  mov ecx,PD_REQUEST_SIZE/8
  rep movsq
  mov rcx,rbx
  call ReleaseSRWLockExclusive
  xor r12d,r12d
  mov r13d,1460
  call qword ptr pmWorkerClockQ
  cmp rax,[rsp+20h+PD_R_DEADLINE]
  jae pmwmPublish
  mov r13d,8
  call GetProcessHeap
  mov rcx,rax
  mov edx,8
  mov r8d,PD_SNAPSHOT_SIZE
  call qword ptr pmWorkerAllocateQ
  mov r12,rax
  test rax,rax
  jz pmwmPublish
  lea rcx,[rsp+20h]
  mov rdx,r12
  mov r8d,PD_SNAPSHOT_SIZE
  call qword ptr pmWorkerCollectorQ
  mov r13d,eax
pmwmPublish:
  ; Check acceptance after every outcome, including allocation/backend failure.
  mov rcx,rbx
  call AcquireSRWLockExclusive
  call qword ptr pmWorkerClockQ
  cmp rax,[rsp+20h+PD_R_DEADLINE]
  jb pmwmChecked
  mov r13d,1460
pmwmChecked:
  cmp dword ptr [rbx+PM_W_STOP],0
  jne pmwmDrop
  test r13d,r13d
  jz pmwmStore
  mov rcx,r12
  call PmWorkerFree
  xor r12d,r12d
pmwmStore:
  mov [rbx+PM_W_RESULT],r12
  mov [rbx+PM_W_ERROR],r13d
  mov dword ptr [rbx+PM_W_STATE],2
  inc dword ptr [rbx+PM_W_COMPLETED]
  ; Retain the mailbox if posting fails: the UI timer can still drain it.
  mov rcx,[rsp+20h+PD_R_NOTIFY]
  mov edx,PM_READY_MESSAGE
  mov r8d,[rbx+PM_W_LANE]
  xor r9d,r9d
  call PostMessageW
  test eax,eax
  jz pmwmIdle
  inc dword ptr [rbx+PM_W_PUBLISHED]
pmwmIdle:
  mov rcx,rbx
  call ReleaseSRWLockExclusive
  jmp pmwmWait
pmwmDrop:
  mov rcx,r12
  call PmWorkerFree
pmwmExit:
  mov rsi,[rbx+PM_W_EVENT]
  mov rdi,[rbx+PM_W_THREAD]
  mov qword ptr [rbx+PM_W_EVENT],0
  mov qword ptr [rbx+PM_W_THREAD],0
  mov dword ptr [rbx+PM_W_STATE],0
  mov rcx,rbx
  call ReleaseSRWLockExclusive
  mov rcx,rsi
  call CloseHandle
  mov rcx,rdi
  call CloseHandle
  xor eax,eax
  add rsp,80h
  pop r13
  pop r12
  pop rdi
  pop rsi
  pop rbx
  ret
PmWorkerMain ENDP
