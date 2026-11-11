; Deterministic owned worker tests. The gated collector substitutes only native
; reading; production queue, synchronization, allocation and notification run.
extern GetStdHandle:proc, WriteFile:proc, wsprintfA:proc
extern GetTickCount64:proc, Sleep:proc, GetCurrentProcess:proc
extern DuplicateHandle:proc, ResetEvent:proc, CreateWindowExW:proc, DestroyWindow:proc
.data
pdwtStage dd 0
pdwtAssertions dd 0
pdwtBytes dd 0
pdwtCalls dd 0
pdwtReleaseCount dd 0
pdwtFailAllocation dd 0
pdwtBackendError dd 0
pdwtWindow dq 0
pdwtDuplicate dq 0
pdwtFormat db 'Process details worker: exit=%u assertions=%u calls=%u threads=%u blocks=%u',13,10,0
align 2 ; Combined module layout must not place a WCHAR API string at odd address.
pdwtClass dw 'S','T','A','T','I','C',0
.data?
pdwtOutput db 192 dup(?)
align 8
pdwtRequest db PD_REQUEST_SIZE dup(?)
pdwtFailureCopy db PD_REQUEST_SIZE dup(?)
.code
PDWT_ASSERT_ZERO MACRO
  test eax,eax
  jnz pdwtFail
  inc pdwtAssertions
ENDM
PDWT_SUBMIT MACRO
  lea rcx,pdwtRequest
  call PdWorkerSubmit
  PDWT_ASSERT_ZERO
ENDM
PDWT_WAIT MACRO counterName,expectedCount
  lea rcx,counterName
  mov edx,expectedCount
  call PdwtWaitCount
  PDWT_ASSERT_ZERO
ENDM

PdwtWaitCount PROC
  push rbx
  push rsi
  push rdi
  sub rsp,20h
  mov rbx,rcx
  mov esi,edx
  call GetTickCount64
  lea rdi,[rax+3000]
pdwtwcNext:
  cmp dword ptr [rbx],esi
  jae pdwtwcPass
  call GetTickCount64
  cmp rax,rdi
  jae pdwtwcTimeout
  mov ecx,1
  call Sleep
  jmp pdwtwcNext
pdwtwcTimeout:
  mov eax,1460
  jmp pdwtwcDone
pdwtwcPass:
  xor eax,eax
pdwtwcDone:
  add rsp,20h
  pop rdi
  pop rsi
  pop rbx
  ret
PdwtWaitCount ENDP

PdwtCollector PROC
  push rbx
  push rsi
  push rdi
  sub rsp,20h
  mov rbx,rcx
  mov rdi,rdx
  mov eax,1
  lock xadd pdwtCalls,eax
  lea esi,[rax+1]
pdwtcGate:
  cmp pdwtReleaseCount,esi
  jae pdwtcRead
  mov ecx,1
  call Sleep
  jmp pdwtcGate
pdwtcRead:
  mov eax,pdwtBackendError
  test eax,eax
  jnz pdwtcDone
  ; The test snapshot echoes only immutable identity and ordering values.
  ; The service must copy request values before returning from Submit.
  mov rsi,rbx
  mov ecx,PD_REQUEST_SIZE/8
  rep movsq
  sub rdi,PD_REQUEST_SIZE
  mov dword ptr [rdi+PD_S_SIZE],PD_SNAPSHOT_SIZE
  mov dword ptr [rdi+PD_S_TARGET],PD_TARGET_VERIFIED
  xor eax,eax
pdwtcDone:
  add rsp,20h
  pop rdi
  pop rsi
  pop rbx
  ret
PdwtCollector ENDP

PdwtAllocate PROC
  cmp pdwtFailAllocation,0
  je HeapAlloc
  xor eax,eax
  ret
PdwtAllocate ENDP

PdWorkerTest PROC
  push rbx
  push rsi
  push rdi
  push r12
  sub rsp,68h
  mov pdwtStage,7100
  call PdWorkerEnsure
  PDWT_ASSERT_ZERO
  call PdWorkerEnsure
  PDWT_ASSERT_ZERO
  cmp pdWorkerThreadsCreated,1
  jne pdwtFail
  inc pdwtAssertions
  lea rax,PdwtCollector
  mov pdWorkerCollector,rax
  lea rax,PdwtAllocate
  mov pdWorkerAllocate,rax
  xor ecx,ecx
  lea rdx,pdwtClass
  xor r8d,r8d
  xor r9d,r9d
  mov qword ptr [rsp+20h],0
  mov qword ptr [rsp+28h],0
  mov qword ptr [rsp+30h],0
  mov qword ptr [rsp+38h],0
  mov qword ptr [rsp+40h],-3 ; HWND_MESSAGE, never visible
  mov qword ptr [rsp+48h],0
  mov qword ptr [rsp+50h],0
  mov qword ptr [rsp+58h],0
  call CreateWindowExW
  mov pdwtWindow,rax
  test rax,rax
  jz pdwtFail
  inc pdwtAssertions
  mov dword ptr [pdwtRequest+PD_R_SIZE],PD_REQUEST_SIZE
  mov dword ptr [pdwtRequest+PD_R_VERSION],PD_VERSION
  mov dword ptr [pdwtRequest+PD_R_LANE],1
  mov qword ptr [pdwtRequest+PD_R_COOKIE],1
  mov qword ptr [pdwtRequest+PD_R_GENERATION],1
  mov qword ptr [pdwtRequest+PD_R_SEQUENCE],1
  mov dword ptr [pdwtRequest+PD_R_PID],42
  mov qword ptr [pdwtRequest+PD_R_BIRTH],123456
  mov qword ptr [pdwtRequest+PD_R_NOTIFY],rax

  mov pdwtStage,7101
  mov dword ptr [pdwtRequest+PD_R_SIZE],95
  lea rcx,pdwtRequest
  call PdWorkerSubmit
  cmp eax,87
  jne pdwtFail
  inc pdwtAssertions
  mov dword ptr [pdwtRequest+PD_R_SIZE],PD_REQUEST_SIZE
  mov dword ptr [pdwtRequest+PD_R_LANE],2
  lea rcx,pdwtRequest
  call PdWorkerSubmit
  cmp eax,50
  jne pdwtFail
  inc pdwtAssertions
  mov dword ptr [pdwtRequest+PD_R_LANE],1
  mov qword ptr [pdwtRequest+PD_R_CANCEL],1
  lea rcx,pdwtRequest
  call PdWorkerSubmit
  cmp eax,87
  jne pdwtFail
  inc pdwtAssertions
  mov qword ptr [pdwtRequest+PD_R_CANCEL],0

  mov pdwtStage,7102
  PDWT_SUBMIT
  PDWT_WAIT pdwtCalls,1
  mov qword ptr [pdwtRequest+PD_R_SEQUENCE],2
  PDWT_SUBMIT
  mov qword ptr [pdwtRequest+PD_R_SEQUENCE],3
  PDWT_SUBMIT
  ; Mutate caller memory: neither the active nor pending copy may change.
  mov qword ptr [pdwtRequest+PD_R_SEQUENCE],777
  mov pdwtReleaseCount,1
  PDWT_WAIT pdWorkerCompleted,1
  PDWT_WAIT pdwtCalls,2
  call PdWorkerTake
  mov rbx,rax
  test rax,rax
  jz pdwtFail
  cmp qword ptr [rax+PD_S_SEQUENCE],1
  jne pdwtFreeFail
  inc pdwtAssertions
  mov rcx,rbx
  call PdWorkerFree
  mov pdwtReleaseCount,2
  PDWT_WAIT pdWorkerCompleted,2
  call PdWorkerTake
  mov rbx,rax
  test rax,rax
  jz pdwtFail
  cmp qword ptr [rax+PD_S_SEQUENCE],3
  jne pdwtFreeFail
  inc pdwtAssertions
  mov rcx,rbx
  call PdWorkerFree
  cmp pdwtCalls,2 ; request2 was overwritten, not queued
  jne pdwtFail
  inc pdwtAssertions

  mov pdwtStage,7103
  mov pdwtReleaseCount,-1
  mov qword ptr [pdwtRequest+PD_R_SEQUENCE],4
  PDWT_SUBMIT
  PDWT_WAIT pdWorkerCompleted,3
  mov qword ptr [pdwtRequest+PD_R_SEQUENCE],5
  PDWT_SUBMIT
  PDWT_WAIT pdWorkerCompleted,4
  call PdWorkerTake
  mov rbx,rax
  test rax,rax
  jz pdwtFail
  cmp qword ptr [rax+PD_S_SEQUENCE],5
  jne pdwtFreeFail
  inc pdwtAssertions
  mov rcx,rbx
  call PdWorkerFree
  cmp pdWorkerBlocks,0
  jne pdwtFail
  inc pdwtAssertions

  mov pdwtStage,7104
  mov qword ptr [pdwtRequest+PD_R_NOTIFY],1 ; invalid HWND => PostMessage failure
  mov qword ptr [pdwtRequest+PD_R_SEQUENCE],6
  PDWT_SUBMIT
  PDWT_WAIT pdWorkerCompleted,5
  call PdWorkerTake
  test rax,rax
  jnz pdwtFreeUnexpected
  cmp pdWorkerBlocks,0
  jne pdwtFail
  inc pdwtAssertions
  mov rax,pdwtWindow
  mov qword ptr [pdwtRequest+PD_R_NOTIFY],rax

  mov pdwtStage,7105
  mov pdwtReleaseCount,5
  mov qword ptr [pdwtRequest+PD_R_SEQUENCE],7
  PDWT_SUBMIT
  PDWT_WAIT pdwtCalls,6
  mov ecx,1
  call PdWorkerCancel
  mov pdwtReleaseCount,6
  PDWT_WAIT pdWorkerCompleted,6
  call PdWorkerTake
  test rax,rax
  jnz pdwtFreeUnexpected
  cmp pdWorkerBlocks,0
  jne pdwtFail
  inc pdwtAssertions

  mov pdwtStage,7106
  mov qword ptr [pdwtRequest+PD_R_COOKIE],2
  mov qword ptr [pdwtRequest+PD_R_SEQUENCE],1
  PDWT_SUBMIT
  PDWT_WAIT pdwtCalls,7
  mov esi,3
pdwtReopen:
  mov qword ptr [pdwtRequest+PD_R_COOKIE],rsi
  PDWT_SUBMIT
  mov rcx,rsi
  call PdWorkerCancel
  cmp pdWorkerThreadsCreated,1
  jne pdwtFail
  cmp pdWorkerBlocks,1
  jne pdwtFail
  cmp pdWorkerPendingReady,0
  jne pdwtFail
  inc pdwtAssertions
  inc esi
  cmp esi,103
  jb pdwtReopen
  mov qword ptr [pdwtRequest+PD_R_COOKIE],103
  mov qword ptr [pdwtRequest+PD_R_SEQUENCE],99
  PDWT_SUBMIT
  mov pdwtReleaseCount,-1
  PDWT_WAIT pdWorkerCompleted,8
  call PdWorkerTake
  mov rbx,rax
  test rax,rax
  jz pdwtFail
  cmp qword ptr [rax+PD_S_COOKIE],103
  jne pdwtFreeFail
  cmp qword ptr [rax+PD_S_SEQUENCE],99
  jne pdwtFreeFail
  inc pdwtAssertions
  mov rcx,rbx
  call PdWorkerFree

  mov pdwtStage,7107
  mov pdwtReleaseCount,8
  mov qword ptr [pdwtRequest+PD_R_GENERATION],2
  mov qword ptr [pdwtRequest+PD_R_SEQUENCE],1
  PDWT_SUBMIT
  PDWT_WAIT pdwtCalls,9
  mov qword ptr [pdwtRequest+PD_R_GENERATION],3
  mov qword ptr [pdwtRequest+PD_R_SEQUENCE],2
  PDWT_SUBMIT
  mov pdwtReleaseCount,9
  PDWT_WAIT pdWorkerCompleted,9
  PDWT_WAIT pdwtCalls,10
  call PdWorkerTake
  test rax,rax
  jnz pdwtFreeUnexpected
  inc pdwtAssertions
  mov pdwtReleaseCount,10
  PDWT_WAIT pdWorkerCompleted,10
  call PdWorkerTake
  mov rbx,rax
  test rax,rax
  jz pdwtFail
  cmp qword ptr [rax+PD_S_GENERATION],3
  jne pdwtFreeFail
  inc pdwtAssertions
  mov rcx,rbx
  call PdWorkerFree

  mov pdwtStage,7109
  mov pdwtReleaseCount,-1
  mov pdwtFailAllocation,1
  mov qword ptr [pdwtRequest+PD_R_SEQUENCE],3
  PDWT_SUBMIT
  PDWT_WAIT pdWorkerCompleted,11
  cmp pdwtCalls,10 ; failed allocation never invokes the reader
  jne pdwtFail
  call PdWorkerTake
  test rax,rax
  jnz pdwtFreeUnexpected
  lea rcx,pdwtFailureCopy
  call PdWorkerTakeFailure
  cmp eax,8
  jne pdwtFail
  cmp qword ptr [pdwtFailureCopy+PD_R_SEQUENCE],3
  jne pdwtFail
  cmp qword ptr [pdwtFailureCopy+PD_R_COOKIE],103
  jne pdwtFail
  inc pdwtAssertions
  lea rcx,pdwtFailureCopy
  call PdWorkerTakeFailure
  PDWT_ASSERT_ZERO
  cmp pdWorkerBlocks,0
  jne pdwtFail
  inc pdwtAssertions

  mov pdwtStage,7110
  mov pdwtFailAllocation,0
  mov pdwtBackendError,5
  mov qword ptr [pdwtRequest+PD_R_SEQUENCE],4
  PDWT_SUBMIT
  PDWT_WAIT pdWorkerCompleted,12
  lea rcx,pdwtFailureCopy
  call PdWorkerTakeFailure
  cmp eax,5
  jne pdwtFail
  cmp qword ptr [pdwtFailureCopy+PD_R_SEQUENCE],4
  jne pdwtFail
  inc pdwtAssertions
  cmp pdWorkerBlocks,0
  jne pdwtFail
  inc pdwtAssertions

  mov pdwtStage,7111
  mov pdwtFailAllocation,1
  mov qword ptr [pdwtRequest+PD_R_SEQUENCE],5
  PDWT_SUBMIT
  PDWT_WAIT pdWorkerCompleted,13
  mov ecx,103
  call PdWorkerCancel
  lea rcx,pdwtFailureCopy
  call PdWorkerTakeFailure
  PDWT_ASSERT_ZERO
  mov qword ptr [pdwtRequest+PD_R_SEQUENCE],6
  PDWT_SUBMIT
  PDWT_WAIT pdWorkerCompleted,14
  cmp pdWorkerFailure,8
  jne pdwtFail
  inc pdwtAssertions
  mov pdwtFailAllocation,0
  mov pdwtBackendError,0
  mov qword ptr [pdwtRequest+PD_R_SEQUENCE],7
  PDWT_SUBMIT
  PDWT_WAIT pdWorkerCompleted,15
  lea rcx,pdwtFailureCopy
  call PdWorkerTakeFailure
  PDWT_ASSERT_ZERO ; successful publication supersedes the previous error
  call PdWorkerTake
  mov rbx,rax
  test rax,rax
  jz pdwtFail
  cmp qword ptr [rax+PD_S_SEQUENCE],7
  jne pdwtFreeFail
  inc pdwtAssertions
  mov rcx,rbx
  call PdWorkerFree

  mov pdwtStage,7108
  mov pdwtReleaseCount,12
  mov qword ptr [pdwtRequest+PD_R_COOKIE],104
  mov qword ptr [pdwtRequest+PD_R_GENERATION],1
  mov qword ptr [pdwtRequest+PD_R_SEQUENCE],1
  PDWT_SUBMIT
  PDWT_WAIT pdwtCalls,13
  call GetCurrentProcess
  mov r12,rax
  mov rcx,rax
  mov rdx,pdWorkerThread
  mov r8,rax
  lea r9,pdwtDuplicate
  mov qword ptr [rsp+20h],0
  mov qword ptr [rsp+28h],0
  mov qword ptr [rsp+30h],2
  call DuplicateHandle
  test eax,eax
  jz pdwtFail
  inc pdwtAssertions
  call GetTickCount64
  mov r12,rax
  call PdWorkerStop
  call GetTickCount64
  sub rax,r12
  cmp rax,1000 ; an active gated read must not block Stop
  jae pdwtFail
  inc pdwtAssertions
  call PdWorkerEnsure
  cmp eax,1115
  jne pdwtFail
  inc pdwtAssertions
  mov pdwtReleaseCount,-1
  mov rcx,pdwtDuplicate
  mov edx,3000
  call WaitForSingleObject
  PDWT_ASSERT_ZERO
  cmp pdWorkerBlocks,0
  jne pdwtFail
  cmp pdWorkerThread,0
  jne pdwtFail
  cmp pdWorkerEvent,0
  jne pdwtFail
  cmp pdWorkerThreadsCreated,1
  jne pdwtFail
  inc pdwtAssertions
  mov pdwtStage,0
  jmp pdwtReport
pdwtFreeUnexpected:
  mov rbx,rax
pdwtFreeFail:
  mov rcx,rbx
  call PdWorkerFree
pdwtFail:
  mov pdwtReleaseCount,-1
pdwtReport:
  call PdWorkerStop
  mov rcx,pdwtDuplicate
  test rcx,rcx
  jz pdwtDestroy
  call CloseHandle
pdwtDestroy:
  mov rcx,pdwtWindow
  test rcx,rcx
  jz pdwtWrite
  call DestroyWindow
pdwtWrite:
  lea rcx,pdwtOutput
  lea rdx,pdwtFormat
  mov r8d,pdwtStage
  mov r9d,pdwtAssertions
  mov eax,pdwtCalls
  mov [rsp+20h],rax
  mov eax,pdWorkerThreadsCreated
  mov [rsp+28h],rax
  mov eax,pdWorkerBlocks
  mov [rsp+30h],rax
  call wsprintfA
  mov [rsp+60h],eax
  mov ecx,-11
  call GetStdHandle
  mov rcx,rax
  lea rdx,pdwtOutput
  mov r8d,[rsp+60h]
  lea r9,pdwtBytes
  mov qword ptr [rsp+20h],0
  call WriteFile
  mov ecx,pdwtStage
  call ExitProcess
PdWorkerTest ENDP
