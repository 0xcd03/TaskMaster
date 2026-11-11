; Isolated optional UI contract tests: real windows/controls/acceptance, no WMI.
extern SendMessageTimeoutW:proc
.data
align 8
pdoutStage dd 0
pdoutAssertions dd 0
pdoutCalls dd 0
pdoutHold dd 1
pdoutWritten dd 0
pdoutNow dq 10000
pdoutNative dq 0
pdoutEpoch dq 0
pdoutOld dq 0
pdoutFormat db 'Optional UI: exit=%u assertions=%u optionalCalls=%u',13,10,0
align 2
pdoutAccount dw 'D','O','M','A','I','N','\','u','s','e','r',0
pdoutCommand dw '"','C',':','\',442h,435h,441h,442h,' ','a','p','p','.','e','x','e','"',' ','"','a',' ','b','"',0
pdoutHrNeedle dw '8','0','0','4','1','0','0','3',0
.data?
align 8
pdoutText dw 32768 dup(?)
pdoutOutput db 192 dup(?)
pdoutMessageResult dq ?
pdoutMessage db 48 dup(?)
.code
PDOUT_ASSERT MACRO value,expected
  cmp value,expected
  jne pdoutFail
  inc pdoutAssertions
ENDM
PDOUT_TEXT MACRO controlId,labelName
  mov rcx,pdWindow
  mov edx,controlId
  lea r8,pdoutText
  mov r9d,32768
  call GetDlgItemTextW
  lea rcx,pdoutText
  lea rdx,labelName
  call lstrcmpW
  test eax,eax
  jnz pdoutFail
  inc pdoutAssertions
ENDM
PdoutClock PROC
  mov rax,pdoutNow
  ret
PdoutClock ENDP
; First UI check sees deadline-1; completion sees the exact deadline.
PdoutStepClock PROC
  mov rax,pdoutNow
  mov r10,qword ptr [pdUiOptionalRequest+PD_R_DEADLINE]
  mov pdoutNow,r10
  ret
PdoutStepClock ENDP
; Keep transport's independent clock just before expiry to isolate UI recheck.
PdoutTransportClock PROC
  mov rax,qword ptr [pdUiOptionalRequest+PD_R_DEADLINE]
  dec rax
  ret
PdoutTransportClock ENDP

; Fill optional fields only. A valid empty command is represented by length0.
PdoutFill PROC
  push rbx
  push rsi
  push rdi
  sub rsp,20h
  mov rbx,rdx
  mov rsi,rcx
  mov rdi,rdx
  mov ecx,PD_REQUEST_SIZE/8
  rep movsq
  mov dword ptr [rbx+PD_S_SIZE],PD_SNAPSHOT_SIZE
  mov dword ptr [rbx+PD_S_TARGET],PD_TARGET_VERIFIED
  mov rax,[rbx+PD_S_BIRTH]
  mov [rbx+PD_S_OBSERVED_BIRTH],rax
  mov dword ptr [rbx+PD_S_FIELDS+PD_F_IDENTITY*16],PD_STATUS_VALID
  mov dword ptr [rbx+PD_S_FIELDS+PD_F_ACCOUNT*16],PD_STATUS_VALID
  mov dword ptr [rbx+PD_S_FIELDS+PD_F_COMMAND*16],PD_STATUS_VALID
  lea rcx,[rbx+PD_S_ACCOUNT]
  lea rdx,pdoutAccount
  call lstrcpyW
  lea rcx,pdoutAccount
  call lstrlenW
  mov [rbx+PD_S_FIELDS+PD_F_ACCOUNT*16+8],eax
  lea rcx,[rbx+PD_S_COMMAND]
  lea rdx,pdoutCommand
  call lstrcpyW
  lea rcx,pdoutCommand
  call lstrlenW
  mov [rbx+PD_S_FIELDS+PD_F_COMMAND*16+8],eax
  xor eax,eax
  add rsp,20h
  pop rdi
  pop rsi
  pop rbx
  ret
PdoutFill ENDP

PdoutCollector PROC
  push rbx
  push rsi
  sub rsp,28h
  mov rbx,rcx
  mov rsi,rdx
  lock inc pdoutCalls
pdoutcGate:
  cmp pdoutHold,0
  je pdoutcRead
  mov ecx,1
  call Sleep
  jmp pdoutcGate
pdoutcRead:
  mov rcx,rbx
  mov rdx,rsi
  call PdoutFill
  add rsp,28h
  pop rsi
  pop rbx
  ret
PdoutCollector ENDP

PdoutMake PROC
  push rbx
  sub rsp,20h
  call GetProcessHeap
  mov rcx,rax
  mov edx,8
  mov r8d,PD_SNAPSHOT_SIZE
  call HeapAlloc
  mov rbx,rax
  test rax,rax
  jz pdoutmDone
  lock inc pdOptionalWorkerBlocks
  lea rcx,pdUiOptionalRequest
  mov rdx,rax
  call PdoutFill
pdoutmDone:
  mov rax,rbx
  add rsp,20h
  pop rbx
  ret
PdoutMake ENDP

PdoutPublish PROC
  push rbx
  push rsi
  push rdi
  sub rsp,30h
  mov rbx,rcx
  lea rcx,pdOptionalWorkerLock
  call AcquireSRWLockExclusive
  mov rsi,pdOptionalWorkerResult
  mov pdOptionalWorkerResult,rbx
  mov [rsp+20h],rsi
  lea rsi,pdUiOptionalRequest
  lea rdi,pdOptionalWorkerResultRequest
  mov ecx,PD_REQUEST_SIZE/8
  rep movsq
  mov rsi,[rsp+20h]
  lea rcx,pdOptionalWorkerLock
  call ReleaseSRWLockExclusive
  mov rcx,rsi
  call PdOptionalWorkerFree
  call PdUiAcceptOptional
  add rsp,30h
  pop rdi
  pop rsi
  pop rbx
  ret
PdoutPublish ENDP

PdOptionalUiTest PROC
  push rbx
  push rsi
  push rdi
  push r12
  sub rsp,68h
  mov pdoutStage,7600
  xor ecx,ecx
  call GetModuleHandleW
  mov gInst,rax
  lea rax,PdutCollector
  mov pdWorkerCollector,rax
  mov pdutHold,1
  lea rax,PdoutCollector
  mov pdOptionalWorkerCollector,rax
  lea rax,PdoutClock
  mov pdUiOptionalClock,rax
  mov pdOptionalWorkerClock,rax
  lea rax,PdutInitHook
  mov pdUiInitHook,rax
  mov appLanguage,0
pdoutLanguage:
  mov ecx,4242
  mov rdx,133444736000000000
  lea r8,pdutName
  call PdUiShowTarget
  cmp pdWindow,0
  je pdoutFail
  PDOUT_ASSERT pdUiOptionalPending,1
  PDOUT_ASSERT dword ptr [pdUiOptionalRequest+PD_R_LANE],2
  PDOUT_ASSERT qword ptr [pdUiOptionalRequest+PD_R_SEQUENCE],0
  mov rax,pdoutNow
  add rax,3000
  PDOUT_ASSERT qword ptr [pdUiOptionalRequest+PD_R_DEADLINE],rax
  mov rax,qword ptr [pdUiOptionalRequest+PD_R_EPOCH]
  test rax,rax
  jz pdoutFail
  mov pdoutEpoch,rax
  mov esi,4106
pdoutReadonly:
  mov rcx,pdWindow
  mov edx,esi
  call GetDlgItem
  test rax,rax
  jz pdoutFail
  mov rcx,rax
  mov edx,-16
  call GetWindowLongPtrW
  test eax,800h
  jz pdoutFail
  inc pdoutAssertions
  inc esi
  cmp esi,4108
  jb pdoutReadonly

  ; Native samples are accepted while the optional backend is blocked.
  mov pdoutStage,7601
  call PdutMakeSnapshot
  test rax,rax
  jz pdoutFail
  mov qword ptr [rax+PD_S_SEQUENCE],100
  mov rcx,rax
  call PdutPublish
  mov rax,pdUiSnapshot
  mov pdoutNative,rax
  mov pdUiBaselineValid,1
  mov rax,pdUiLastSequence
  mov r12,rax
  add pdoutNow,2500
  call PdUiSubmit
  call PdUiSubmit
  mov rax,pdoutEpoch
  PDOUT_ASSERT qword ptr [pdUiOptionalRequest+PD_R_EPOCH],rax
  PDOUT_ASSERT pdUiLastSequence,r12
  PDOUT_ASSERT pdUiBaselineValid,1
  mov rcx,pdWindow
  xor edx,edx ; WM_NULL must complete while worker remains gated
  xor r8d,r8d
  xor r9d,r9d
  mov qword ptr [rsp+20h],2
  mov qword ptr [rsp+28h],100
  lea rax,pdoutMessageResult
  mov [rsp+30h],rax
  call SendMessageTimeoutW
  test rax,rax
  jz pdoutFail
  inc pdoutAssertions
  cmp appLanguage,0
  jne pdoutInjectFirst
  lea rcx,pdoutCalls
  mov edx,1
  call PdwtWaitCount
  test eax,eax
  jnz pdoutFail
  mov pdoutHold,0
  lea rcx,pdOptionalWorkerCompleted
  mov edx,1
  call PdwtWaitCount
  test eax,eax
  jnz pdoutFail
  call PdUiAcceptOptional
  mov pdoutHold,1
  jmp pdoutFirstAccepted
pdoutInjectFirst:
  call PdoutMake
  test rax,rax
  jz pdoutFail
  mov rcx,rax
  call PdoutPublish
pdoutFirstAccepted:
  cmp pdUiOptionalSnapshot,0
  je pdoutFail
  PDOUT_ASSERT pdUiOptionalPending,0
  mov rax,pdoutNative
  PDOUT_ASSERT pdUiSnapshot,rax
  PDOUT_ASSERT pdUiBaselineValid,1
  PDOUT_ASSERT pdUiLastSequence,r12
  PDOUT_TEXT 4106,pdoutAccount
  PDOUT_TEXT 4107,pdoutCommand
  ; A later native render cannot erase accepted optional fields.
  call PdutMakeSnapshot
  test rax,rax
  jz pdoutFail
  mov qword ptr [rax+PD_S_SEQUENCE],101
  mov rcx,rax
  call PdutPublish
  PDOUT_TEXT 4106,pdoutAccount
  PDOUT_TEXT 4107,pdoutCommand
  mov rax,pdUiSnapshot
  mov pdoutNative,rax
  mov pdUiBaselineValid,1

  mov pdoutStage,7602
  ; Duplicate completion is rejected and freed.
  mov rax,pdUiOptionalSnapshot
  mov pdoutOld,rax
  call PdoutMake
  test rax,rax
  jz pdoutFail
  mov rcx,rax
  call PdoutPublish
  mov rax,pdoutOld
  PDOUT_ASSERT pdUiOptionalSnapshot,rax
  ; Explicit refresh invalidates optional only, preserving native baseline.
  call PdUiRefreshOptional
  PDOUT_ASSERT pdUiOptionalSnapshot,0
  PDOUT_ASSERT pdUiOptionalPending,1
  PDOUT_ASSERT pdUiBaselineValid,1
  mov rax,pdoutNative
  PDOUT_ASSERT pdUiSnapshot,rax
  PDOUT_ASSERT pdUiLastSequence,101
  mov rax,pdoutEpoch
  inc rax
  PDOUT_ASSERT qword ptr [pdUiOptionalRequest+PD_R_EPOCH],rax
  mov pdoutEpoch,rax
  ; Wrong cookie/gen/PID/birth/epoch/lane/observed birth are never applied.
  xor esi,esi
pdoutReject:
  call PdoutMake
  test rax,rax
  jz pdoutFail
  cmp esi,0
  jne pdoutBadGen
  inc qword ptr [rax+PD_S_COOKIE]
  jmp pdoutBadPublish
pdoutBadGen:
  cmp esi,1
  jne pdoutBadPid
  inc qword ptr [rax+PD_S_GENERATION]
  jmp pdoutBadPublish
pdoutBadPid:
  cmp esi,2
  jne pdoutBadBirth
  inc dword ptr [rax+PD_S_PID]
  jmp pdoutBadPublish
pdoutBadBirth:
  cmp esi,3
  jne pdoutBadEpoch
  inc qword ptr [rax+PD_S_BIRTH]
  jmp pdoutBadPublish
pdoutBadEpoch:
  cmp esi,4
  jne pdoutBadLane
  dec qword ptr [rax+PD_S_EPOCH]
  jmp pdoutBadPublish
pdoutBadLane:
  cmp esi,5
  jne pdoutBadObserved
  mov dword ptr [rax+PD_S_LANE],1
  jmp pdoutBadPublish
pdoutBadObserved:
  inc qword ptr [rax+PD_S_OBSERVED_BIRTH]
pdoutBadPublish:
  mov rcx,rax
  call PdoutPublish
  PDOUT_ASSERT pdUiOptionalSnapshot,0
  PDOUT_ASSERT pdUiOptionalPending,1
  inc esi
  cmp esi,7
  jb pdoutReject

  mov pdoutStage,7603
  ; Deadline is absolute; hidden window and later native submits do not extend it.
  mov rax,qword ptr [pdUiOptionalRequest+PD_R_DEADLINE]
  mov pdoutNow,rax
  mov rcx,pdWindow
  mov edx,113h
  mov r8d,81h
  xor r9d,r9d
  call SendMessageW ; real hidden-window timer must check deadline before visibility
  PDOUT_ASSERT pdUiOptionalPending,0
  PDOUT_ASSERT pdUiOptionalStatus,PD_STATUS_TIMEOUT
  PDOUT_ASSERT pdUiBaselineValid,1
  call PdoutMake
  test rax,rax
  jz pdoutFail
  mov rcx,rax
  call PdoutPublish
  PDOUT_ASSERT pdUiOptionalSnapshot,0
  PDOUT_ASSERT pdUiOptionalStatus,PD_STATUS_TIMEOUT

  mov pdoutStage,7609
  call PdUiRefreshOptional
  ; A declared length at capacity must never become a truncated VALID string.
  call PdoutMake
  test rax,rax
  jz pdoutFail
  mov dword ptr [rax+PD_S_FIELDS+PD_F_COMMAND*16+8],PD_COMMAND_CAPACITY
  mov rcx,rax
  call PdoutPublish
  PDOUT_ASSERT pdUiOptionalSnapshot,0
  PDOUT_ASSERT pdUiOptionalPending,1
  ; Length disagrees with NUL position: reject, not silently repair metadata.
  call PdoutMake
  test rax,rax
  jz pdoutFail
  mov dword ptr [rax+PD_S_FIELDS+PD_F_ACCOUNT*16+8],0
  mov rcx,rax
  call PdoutPublish
  PDOUT_ASSERT pdUiOptionalSnapshot,0
  PDOUT_ASSERT pdUiOptionalPending,1

  mov pdoutStage,7604
  call PdUiRefreshOptional
  call PdoutMake
  test rax,rax
  jz pdoutFail
  mov rbx,rax
  ; Maximum valid values are read back from actual EDIT controls without truncation.
  lea rdi,[rbx+PD_S_ACCOUNT]
  mov ecx,1023
  mov ax,416h
  rep stosw
  mov word ptr [rdi],0
  mov dword ptr [rbx+PD_S_FIELDS+PD_F_ACCOUNT*16+8],1023
  lea rdi,[rbx+PD_S_COMMAND]
  mov ecx,32766
  mov ax,42Fh
  rep stosw
  mov word ptr [rdi],0
  mov dword ptr [rbx+PD_S_FIELDS+PD_F_COMMAND*16+8],32766
  mov rcx,rbx
  call PdoutPublish
  mov rcx,pdWindow
  mov edx,4106
  lea r8,pdoutText
  mov r9d,32768
  call GetDlgItemTextW
  PDOUT_ASSERT eax,1023
  PDOUT_ASSERT word ptr [pdoutText+1022*2],416h
  mov rcx,pdWindow
  mov edx,4107
  lea r8,pdoutText
  mov r9d,32768
  call GetDlgItemTextW
  PDOUT_ASSERT eax,32766
  PDOUT_ASSERT word ptr [pdoutText+32765*2],42Fh

  mov pdoutStage,7605
  call PdUiRefreshOptional
  call PdoutMake
  test rax,rax
  jz pdoutFail
  mov word ptr [rax+PD_S_COMMAND],0
  mov dword ptr [rax+PD_S_FIELDS+PD_F_COMMAND*16+8],0
  mov rcx,rax
  call PdoutPublish
  mov rcx,pdWindow
  mov edx,4107
  lea r8,pdoutText
  mov r9d,32768
  call GetDlgItemTextW
  PDOUT_ASSERT eax,0
  mov rax,pdUiOptionalSnapshot
  PDOUT_ASSERT dword ptr [rax+PD_S_FIELDS+PD_F_COMMAND*16],PD_STATUS_VALID

  mov pdoutStage,7606
  call PdUiRefreshOptional
  call PdoutMake
  test rax,rax
  jz pdoutFail
  mov dword ptr [rax+PD_S_FIELDS+PD_F_COMMAND*16],PD_STATUS_UNAVAILABLE
  mov dword ptr [rax+PD_S_FIELDS+PD_F_COMMAND*16+8],0
  mov word ptr [rax+PD_S_COMMAND],0
  mov dword ptr [rax+PD_S_FIELDS+PD_F_COMMAND*16+4],80041003h
  mov dword ptr [rax+PD_S_FIELDS+PD_F_COMMAND*16+12],1
  mov rcx,rax
  call PdoutPublish
  PDOUT_TEXT 4106,pdoutAccount
  mov rcx,pdWindow
  mov edx,4107
  lea r8,pdoutText
  mov r9d,32768
  call GetDlgItemTextW
  test eax,eax
  jz pdoutFail
  lea rcx,pdoutText
  lea rdx,pdoutHrNeedle
  call StrStrIW
  test rax,rax
  jz pdoutFail
  inc pdoutAssertions
  mov rax,pdoutNative
  PDOUT_ASSERT pdUiSnapshot,rax
  PDOUT_ASSERT pdUiBaselineValid,1

  mov pdoutStage,7608
  call PdUiRefreshOptional
  ; Fixed transport failures carry an epoch, without allocating a snapshot.
  lea rcx,pdOptionalWorkerLock
  call AcquireSRWLockExclusive
  lea rsi,pdUiOptionalRequest
  lea rdi,pdOptionalWorkerFailureRequest
  mov ecx,PD_REQUEST_SIZE/8
  rep movsq
  dec qword ptr [pdOptionalWorkerFailureRequest+PD_R_EPOCH]
  mov pdOptionalWorkerFailure,8
  lea rcx,pdOptionalWorkerLock
  call ReleaseSRWLockExclusive
  call PdUiAcceptOptional
  PDOUT_ASSERT pdUiOptionalPending,1
  lea rcx,pdOptionalWorkerLock
  call AcquireSRWLockExclusive
  lea rsi,pdUiOptionalRequest
  lea rdi,pdOptionalWorkerFailureRequest
  mov ecx,PD_REQUEST_SIZE/8
  rep movsq
  mov pdOptionalWorkerFailure,8
  lea rcx,pdOptionalWorkerLock
  call ReleaseSRWLockExclusive
  call PdUiAcceptOptional
  PDOUT_ASSERT pdUiOptionalPending,0
  PDOUT_ASSERT pdUiOptionalStatus,PD_STATUS_UNAVAILABLE
  PDOUT_ASSERT pdUiOptionalError,8
  PDOUT_ASSERT pdUiBaselineValid,1
  mov rax,pdoutNative
  PDOUT_ASSERT pdUiSnapshot,rax

  mov pdoutStage,7612
  call PdUiRefreshOptional
  lea rcx,pdOptionalWorkerLock
  call AcquireSRWLockExclusive
  lea rsi,pdUiOptionalRequest
  lea rdi,pdOptionalWorkerFailureRequest
  mov ecx,PD_REQUEST_SIZE/8
  rep movsq
  mov pdOptionalWorkerFailure,8
  lea rcx,pdOptionalWorkerLock
  call ReleaseSRWLockExclusive
  mov rax,qword ptr [pdUiOptionalRequest+PD_R_DEADLINE]
  dec rax
  mov pdoutNow,rax
  lea rax,PdoutStepClock
  mov pdUiOptionalClock,rax
  lea rax,PdoutTransportClock
  mov pdOptionalWorkerClock,rax
  call PdUiAcceptOptional
  PDOUT_ASSERT pdUiOptionalPending,0
  PDOUT_ASSERT pdUiOptionalStatus,PD_STATUS_TIMEOUT
  PDOUT_ASSERT pdUiOptionalError,1460
  PDOUT_ASSERT pdUiBaselineValid,1
  mov rax,pdoutNative
  PDOUT_ASSERT pdUiSnapshot,rax
  lea rax,PdoutClock
  mov pdUiOptionalClock,rax
  mov pdOptionalWorkerClock,rax

  mov pdoutStage,7613
  call PdUiRefreshOptional
  call PdoutMake
  test rax,rax
  jz pdoutFail
  mov rbx,rax
  mov dword ptr [rbx+PD_S_TARGET],PD_TARGET_MISMATCH
  mov dword ptr [rbx+PD_S_ERROR],13
  mov rax,qword ptr [pdUiOptionalRequest+PD_R_DEADLINE]
  dec rax
  mov pdoutNow,rax
  lea rax,PdoutStepClock
  mov pdUiOptionalClock,rax
  lea rax,PdoutTransportClock
  mov pdOptionalWorkerClock,rax
  mov rcx,rbx
  call PdoutPublish
  PDOUT_ASSERT pdUiOptionalPending,0
  PDOUT_ASSERT pdUiOptionalStatus,PD_STATUS_TIMEOUT
  PDOUT_ASSERT pdUiOptionalError,1460
  PDOUT_ASSERT pdUiBaselineValid,1
  mov rax,pdoutNative
  PDOUT_ASSERT pdUiSnapshot,rax
  lea rax,PdoutClock
  mov pdUiOptionalClock,rax
  mov pdOptionalWorkerClock,rax

  mov pdoutStage,7607
  ; Confirmed exit cancels optional and cannot resurrect it via a late response.
  call PdUiRefreshOptional
  call PdoutMake
  test rax,rax
  jz pdoutFail
  mov r12,rax
  call PdutMakeSnapshot
  test rax,rax
  jz pdoutLateFreeFail
  mov qword ptr [rax+PD_S_SEQUENCE],102
  mov dword ptr [rax+PD_S_TARGET],PD_TARGET_EXITED
  mov rcx,rax
  call PdutPublish
  PDOUT_ASSERT pdUiOptionalPending,0
  PDOUT_ASSERT pdUiOptionalSnapshot,0
  mov rcx,r12
  call PdoutPublish
  xor r12d,r12d
  PDOUT_ASSERT pdUiOptionalSnapshot,0
  mov rcx,pdWindow
  call DestroyWindow
  inc appLanguage
  cmp appLanguage,2
  jb pdoutLanguage
  ; Real elapsed delay, with actual message pumping and accepted native samples.
  mov pdoutStage,7611
  mov pdoutHold,0
  mov pdutHold,0
  call PdutWaitIdle
  test eax,eax
  jnz pdoutFail
  call GetTickCount64
  lea rbx,[rax+3000]
pdoutRealDrain:
  lea rcx,pdOptionalWorkerLock
  call AcquireSRWLockExclusive
  mov esi,pdOptionalWorkerActive
  or esi,pdOptionalWorkerPendingReady
  lea rcx,pdOptionalWorkerLock
  call ReleaseSRWLockExclusive
  test esi,esi
  jz pdoutRealStart
  call GetTickCount64
  cmp rax,rbx
  jae pdoutFail
  mov ecx,1
  call Sleep
  jmp pdoutRealDrain
pdoutRealStart:
  lea rax,GetTickCount64
  mov pdUiOptionalClock,rax
  mov pdOptionalWorkerClock,rax
  mov pdoutHold,1
  mov pdutHold,1
  mov eax,pdOptionalWorkerCompleted
  inc eax
  mov [rsp+40h],eax
  mov appLanguage,0
  mov ecx,4242
  mov rdx,133444736000000000
  lea r8,pdutName
  call PdUiShowTarget
  cmp pdWindow,0
  je pdoutFail
  PDOUT_ASSERT pdUiOptionalPending,1
  call GetTickCount64
  mov rdi,rax
  mov rsi,rax
  mov r12d,1000
pdoutRealLoop:
  ; Pump the normal modeless routing while worker remains genuinely blocked.
pdoutRealPump:
  lea rcx,pdoutMessage
  xor edx,edx
  xor r8d,r8d
  xor r9d,r9d
  mov qword ptr [rsp+20h],1
  call PeekMessageW
  test eax,eax
  jz pdoutRealProbe
  lea rcx,pdoutMessage
  call PdUiPreTranslate
  test eax,eax
  jnz pdoutRealPump
  lea rcx,pdoutMessage
  call TranslateMessage
  lea rcx,pdoutMessage
  call DispatchMessageW
  jmp pdoutRealPump
pdoutRealProbe:
  mov rcx,pdWindow
  xor edx,edx
  xor r8d,r8d
  xor r9d,r9d
  mov qword ptr [rsp+20h],2
  mov qword ptr [rsp+28h],100
  lea rax,pdoutMessageResult
  mov [rsp+30h],rax
  call SendMessageTimeoutW
  test rax,rax
  jz pdoutFail
  call GetTickCount64
  cmp rax,rsi
  jb pdoutRealTime
  lea rsi,[rax+500]
  call PdutMakeSnapshot
  test rax,rax
  jz pdoutFail
  mov [rax+PD_S_SEQUENCE],r12
  mov rcx,rax
  call PdutPublish
  PDOUT_ASSERT pdUiLastSequence,r12
  PDOUT_ASSERT pdUiOptionalPending,1
  inc r12
pdoutRealTime:
  call GetTickCount64
  sub rax,rdi
  cmp rax,2500
  jae pdoutRealRelease
  mov ecx,10
  call Sleep
  jmp pdoutRealLoop
pdoutRealRelease:
  cmp r12,1005
  jb pdoutFail
  mov pdoutHold,0
  lea rcx,pdOptionalWorkerCompleted
  mov edx,[rsp+40h]
  call PdwtWaitCount
  test eax,eax
  jnz pdoutFail
  call PdUiAcceptOptional
  cmp pdUiOptionalSnapshot,0
  je pdoutFail
  PDOUT_ASSERT pdUiOptionalPending,0
  dec r12
  PDOUT_ASSERT pdUiLastSequence,r12
  PDOUT_TEXT 4106,pdoutAccount
  PDOUT_TEXT 4107,pdoutCommand
  mov pdoutStage,0
  jmp pdoutCleanup
pdoutLateFreeFail:
  mov rcx,r12
  call PdOptionalWorkerFree
pdoutFail:
pdoutCleanup:
  mov rcx,pdWindow
  test rcx,rcx
  jz pdoutStop
  call DestroyWindow
pdoutStop:
  mov pdoutHold,0
  mov pdutHold,0
  call PdOptionalWorkerStop
  call PdWorkerStop
  call GetTickCount64
  lea rbx,[rax+3000]
pdoutDrain:
  cmp pdWorkerThread,0
  jne pdoutDrainWait
  cmp pdOptionalWorkerThread,0
  jne pdoutDrainWait
  cmp pdoutStage,0
  jne pdoutReport
  cmp pdWorkerBlocks,0
  jne pdoutCleanupFailure
  cmp pdOptionalWorkerBlocks,0
  jne pdoutCleanupFailure
  add pdoutAssertions,2
  jmp pdoutReport
pdoutDrainWait:
  call GetTickCount64
  cmp rax,rbx
  jae pdoutCleanupFailure
  mov ecx,1
  call Sleep
  jmp pdoutDrain
pdoutCleanupFailure:
  cmp pdoutStage,0
  jne pdoutReport
  mov pdoutStage,7610
pdoutReport:
  lea rcx,pdoutOutput
  lea rdx,pdoutFormat
  mov r8d,pdoutStage
  mov r9d,pdoutAssertions
  mov eax,pdoutCalls
  mov [rsp+20h],rax
  call wsprintfA
  mov [rsp+58h],eax
  mov ecx,-11
  call GetStdHandle
  mov rcx,rax
  lea rdx,pdoutOutput
  mov r8d,[rsp+58h]
  lea r9,pdoutWritten
  mov qword ptr [rsp+20h],0
  call WriteFile
  mov ecx,pdoutStage
  call ExitProcess
PdOptionalUiTest ENDP
