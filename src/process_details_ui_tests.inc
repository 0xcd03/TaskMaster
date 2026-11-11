; Native hidden-window tests: real resources/controls, worker publication and
; acceptance; target reading alone is replaced by an owned deterministic source.
extern GetWindowLongPtrW:proc, GetWindowRect:proc, MapWindowPoints:proc
extern GetTextColor:proc, GetBkColor:proc, GetDC:proc, ReleaseDC:proc
extern GetObjectW:proc, IsWindow:proc, IsDialogMessageW:proc, SetWindowPos:proc
.data
align 8
pdutStage dd 0
pdutAssertions dd 0
pdutWritten dd 0
pdutCalls dd 0
pdutHold dd 1
pdutBackendError dd 0
pdutSavedGlobalDpi dd 0
pdutSavedTheme dd 0
pdutCookie dq 0
pdutOldSnapshot dq 0
pdutOldWindow dq 0
pdutCollect dq 0
pdutName dw 'O','w','n','e','d',' ','F','i','x','t','u','r','e',0
pdutPath dw 'C',':','\','O','w','n','e','d',' ','F','i','x','t','u','r','e','\',442h,435h,441h,442h,'.','e','x','e',0
pdutSid dw 'S','-','1','-','5','-','2','1','-','1','2','3',0
pdutYearFormat dw '%','0','4','u','-',0
pdutInjected dd 0
pdutFormat db 'Process details UI: exit=%u assertions=%u nativeReads=%u blocks=%u',13,10,0
pdutDebugFormat db 'UI fixture path=[%S] expected=[%S]',13,10,0
.data?
align 8
pdutText dw 8192 dup(?)
pdutOutput db 192 dup(?)
pdutRect db 16 dup(?)
pdutClient db 16 dup(?)
pdutFont db 104 dup(?)
pdutMessage db 48 dup(?)
pdutYear dw 16 dup(?)
pdutFailureExpected dw 128 dup(?)
pdutOldStatus dw 1024 dup(?)
.code
PDUT_CHECK_TEXT MACRO controlId,expectedText
  mov rcx,pdWindow
  mov edx,controlId
  lea r8,pdutText
  mov r9d,8192
  call GetDlgItemTextW
  lea rcx,pdutText
  lea rdx,expectedText
  call lstrcmpW
  test eax,eax
  jnz pdutFail
  inc pdutAssertions
ENDM

PdutInitHook PROC
  ret
PdutInitHook ENDP

; Fill only this owned test snapshot. No native/user process is queried.
PdutFillSnapshot PROC
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
  mov dword ptr [rbx+PD_S_FIELDS+PD_F_IDENTITY*16],PD_STATUS_VALID
  mov rax,133444736000000000
  mov [rbx+PD_S_OBSERVED_BIRTH],rax
  mov dword ptr [rbx+PD_S_FIELDS+PD_F_PATH*16],PD_STATUS_VALID
  mov dword ptr [rbx+PD_S_FIELDS+PD_F_SID*16],PD_STATUS_VALID
  mov dword ptr [rbx+PD_S_FIELDS+PD_F_HANDLES*16],PD_STATUS_VALID
  mov qword ptr [rbx+PD_S_HANDLES],123
  lea rcx,[rbx+PD_S_PATH]
  lea rdx,pdutPath
  call lstrcpyW
  lea rcx,[rbx+PD_S_SID]
  lea rdx,pdutSid
  call lstrcpyW
  xor eax,eax
  add rsp,20h
  pop rdi
  pop rsi
  pop rbx
  ret
PdutFillSnapshot ENDP

PdutCollector PROC
  push rbx
  push rsi
  sub rsp,28h
  mov rbx,rcx
  mov rsi,rdx
  lock inc pdutCalls
pdutcWait:
  cmp pdutHold,0
  je pdutcRead
  mov ecx,1
  call Sleep
  jmp pdutcWait
pdutcRead:
  mov eax,pdutBackendError
  test eax,eax
  jnz pdutcDone
  mov rcx,rbx
  mov rdx,rsi
  call PdutFillSnapshot
pdutcDone:
  add rsp,28h
  pop rsi
  pop rbx
  ret
PdutCollector ENDP

PdutNoAllocate PROC
  xor eax,eax
  ret
PdutNoAllocate ENDP

; ECX expected transport error. Verify the actual status includes that code.
PdutCheckFailureStatus PROC
  push rbx
  sub rsp,20h
  mov ebx,ecx
  mov edx,52086
  call PdUiLoadText
  lea rcx,pdutFailureExpected
  mov rdx,rax
  mov r8d,ebx
  call wsprintfW
  mov rcx,pdWindow
  mov edx,4101
  lea r8,pdutText
  mov r9d,8192
  call GetDlgItemTextW
  lea rcx,pdutText
  lea rdx,pdutFailureExpected
  call StrStrIW
  test rax,rax
  setz al
  movzx eax,al
  add rsp,20h
  pop rbx
  ret
PdutCheckFailureStatus ENDP

; Create an owned snapshot and return its pointer. Caller changes selected keys
; before publishing; real acceptance and ownership code handle every outcome.
PdutMakeSnapshot PROC
  push rbx
  sub rsp,20h
  call GetProcessHeap
  mov rcx,rax
  mov edx,8
  mov r8d,PD_SNAPSHOT_SIZE
  call HeapAlloc
  mov rbx,rax
  test rax,rax
  jz pdutmsDone
  lock inc pdWorkerBlocks
  lea rcx,pdUiRequest
  mov rdx,rax
  call PdutFillSnapshot
pdutmsDone:
  mov rax,rbx
  add rsp,20h
  pop rbx
  ret
PdutMakeSnapshot ENDP

; RCX snapshot. Test publication substitutes only worker result-slot delivery.
PdutPublish PROC
  push rbx
  push rsi
  sub rsp,28h
  mov rbx,rcx
  lea rcx,pdWorkerLock
  call AcquireSRWLockExclusive
  mov rsi,pdWorkerResult
  mov pdWorkerResult,rbx
  lea rcx,pdWorkerLock
  call ReleaseSRWLockExclusive
  mov rcx,rsi
  call PdWorkerFree
  call PdUiAccept
  add rsp,28h
  pop rsi
  pop rbx
  ret
PdutPublish ENDP

; Gate publication-boundary assertions only after the real source is quiescent.
PdutWaitIdle PROC
  push rbx
  push rsi
  sub rsp,28h
  call GetTickCount64
  lea rbx,[rax+3000]
pdutwiNext:
  lea rcx,pdWorkerLock
  call AcquireSRWLockExclusive
  mov esi,pdWorkerActive
  or esi,pdWorkerPendingReady
  lea rcx,pdWorkerLock
  call ReleaseSRWLockExclusive
  test esi,esi
  jz pdutwiPass
  call GetTickCount64
  cmp rax,rbx
  jae pdutwiFail
  mov ecx,1
  call Sleep
  jmp pdutwiNext
pdutwiFail:
  mov eax,1460
  jmp pdutwiDone
pdutwiPass:
  xor eax,eax
pdutwiDone:
  add rsp,28h
  pop rsi
  pop rbx
  ret
PdutWaitIdle ENDP

PdUiTest PROC
  push rbx
  push rsi
  push rdi
  push r12
  sub rsp,78h
  mov pdutStage,7200
  xor ecx,ecx
  call GetModuleHandleW
  mov gInst,rax
  mov eax,uiDpi
  mov pdutSavedGlobalDpi,eax
  mov eax,themeOption
  mov pdutSavedTheme,eax
  mov rax,pdWorkerCollector
  mov pdutCollect,rax
  lea rax,PdutCollector
  mov pdWorkerCollector,rax
  mov pdOptionalWorkerCollector,rax ; this fixture never queries external WMI
  lea rax,PdutInitHook
  mov pdUiInitHook,rax
  mov appLanguage,0
pdutLanguage:
  mov pdutStage,7201
  mov ecx,4242
  mov rdx,133444736000000000
  lea r8,pdutName
  call PdUiShowTarget
  mov rbx,pdWindow
  test rbx,rbx
  jz pdutFail
  mov pdutOldWindow,rbx
  mov rax,qword ptr [pdUiRequest+PD_R_COOKIE]
  mov pdutCookie,rax
  inc pdutAssertions
  mov rcx,rbx
  call IsWindowVisible
  test eax,eax
  jnz pdutFail
  inc pdutAssertions
  ; Same target activates the same pinned window even after navigation changes.
  mov frontMode,2
  mov ecx,4242
  mov rdx,133444736000000000
  lea r8,pdutName
  call PdUiShowTarget
  cmp pdWindow,rbx
  jne pdutFail
  mov rax,pdutCookie
  cmp rax,qword ptr [pdUiRequest+PD_R_COOKIE]
  jne pdutFail
  inc pdutAssertions
  mov frontMode,0

  mov pdutStage,7202
  mov esi,4102
pdutReadonly:
  mov rcx,rbx
  mov edx,esi
  call GetDlgItem
  mov rcx,rax
  mov edx,-16
  call GetWindowLongPtrW
  test eax,800h ; ES_READONLY is enforced by the actual control
  jz pdutFail
  inc pdutAssertions
  inc esi
  cmp esi,4104
  jbe pdutReadonly
  ; Validate real localized button text rather than only language state.
  mov rcx,pdWindow
  mov edx,2
  lea r8,pdutText
  mov r9d,8192
  call GetDlgItemTextW
  test eax,eax
  jz pdutFail
  cmp appLanguage,0
  jne pdutRussian
  cmp word ptr [pdutText],'C'
  jne pdutFail
  jmp pdutLabelOk
pdutRussian:
  cmp word ptr [pdutText],417h ; Cyrillic capital Zhe: Закрыть
  jne pdutFail
pdutLabelOk:
  inc pdutAssertions
  ; The default button is the native Refresh control; a real BM_CLICK asks for
  ; another sample without retargeting the pinned process.
  mov r12,qword ptr [pdUiRequest+PD_R_SEQUENCE]
  mov rdi,qword ptr [pdUiRequest+PD_R_GENERATION]
  mov rcx,pdWindow
  mov edx,1
  call GetDlgItem
  mov rcx,rax
  mov edx,0F5h
  xor r8d,r8d
  xor r9d,r9d
  call SendMessageW
  inc r12
  cmp r12,qword ptr [pdUiRequest+PD_R_SEQUENCE]
  jne pdutFail
  cmp rdi,qword ptr [pdUiRequest+PD_R_GENERATION]
  jne pdutFail
  inc pdutAssertions

  mov pdutStage,7203
  ; The first EN window additionally consumes a real worker-published result.
  cmp appLanguage,0
  jne pdutFixture
  lea rcx,pdutCalls
  mov edx,1
  call PdwtWaitCount
  test eax,eax
  jnz pdutFail
  mov pdutHold,0
  lea rcx,pdWorkerCompleted
  mov edx,1
  call PdwtWaitCount
  test eax,eax
  jnz pdutFail
  call PdUiAccept
  cmp pdUiLastSequence,1
  jb pdutFail
  inc pdutAssertions
pdutFixture:
  mov pdutHold,0
  call PdutWaitIdle
  test eax,eax
  jnz pdutFail
  call PdUiAccept
  call PdutMakeSnapshot
  test rax,rax
  jz pdutFail
  mov qword ptr [rax+PD_S_SEQUENCE],10
  mov rcx,rax
  call PdutPublish
  cmp pdUiLastSequence,10
  jne pdutFail
  inc pdutAssertions
  PDUT_CHECK_TEXT 4102,pdutPath
  PDUT_CHECK_TEXT 4104,pdutSid
  ; A typed character cannot change a native read-only field. Selection and
  ; WM_GETTEXT remain available for the ordinary copy workflow.
  mov rcx,pdWindow
  mov edx,4102
  call GetDlgItem
  mov rbx,rax
  mov rcx,rax
  mov edx,0B1h ; EM_SETSEL
  xor r8d,r8d
  mov r9,-1
  call SendMessageW
  mov rcx,rbx
  mov edx,102h ; WM_CHAR
  mov r8d,'X'
  xor r9d,r9d
  call SendMessageW
  PDUT_CHECK_TEXT 4102,pdutPath
  mov rax,pdUiSnapshot
  mov pdutOldSnapshot,rax

  mov pdutStage,7204
  xor esi,esi
pdutReject:
  call PdutMakeSnapshot
  test rax,rax
  jz pdutFail
  mov rbx,rax
  mov qword ptr [rax+PD_S_SEQUENCE],11
  cmp esi,0
  jne pdutBadGeneration
  inc qword ptr [rax+PD_S_COOKIE]
  jmp pdutBadPublish
pdutBadGeneration:
  cmp esi,1
  jne pdutBadPid
  inc qword ptr [rax+PD_S_GENERATION]
  jmp pdutBadPublish
pdutBadPid:
  cmp esi,2
  jne pdutBadBirth
  inc dword ptr [rax+PD_S_PID]
  jmp pdutBadPublish
pdutBadBirth:
  cmp esi,3
  jne pdutBadSequence
  inc qword ptr [rax+PD_S_BIRTH]
  jmp pdutBadPublish
pdutBadSequence:
  cmp esi,4
  jne pdutBadSize
  mov qword ptr [rax+PD_S_SEQUENCE],10 ; duplicate or older sample
  jmp pdutBadPublish
pdutBadSize:
  cmp esi,5
  jne pdutBadVersion
  dec dword ptr [rax+PD_S_SIZE]
  jmp pdutBadPublish
pdutBadVersion:
  cmp esi,6
  jne pdutBadLane
  inc dword ptr [rax+PD_S_VERSION]
  jmp pdutBadPublish
pdutBadLane:
  mov dword ptr [rax+PD_S_LANE],2
pdutBadPublish:
  mov rcx,rbx
  call PdutPublish
  cmp pdUiLastSequence,10
  jne pdutFail
  mov rax,pdutOldSnapshot
  cmp rax,pdUiSnapshot
  jne pdutFail
  inc pdutAssertions
  inc esi
  cmp esi,8
  jb pdutReject

  mov pdutStage,7210
  mov qword ptr [pdUiRequest+PD_R_SEQUENCE],10
  lea rax,PdutNoAllocate
  mov pdWorkerAllocate,rax
  call PdUiSubmit
  call PdutWaitIdle
  test eax,eax
  jnz pdutFail
  call PdUiAccept
  cmp pdUiLastSequence,11
  jne pdutFail
  cmp pdUiLastTarget,PD_TARGET_UNKNOWN
  jne pdutFail
  cmp pdUiBaselineValid,0
  jne pdutFail
  mov rax,pdutOldSnapshot
  cmp rax,pdUiSnapshot
  jne pdutFail
  mov ecx,8
  call PdutCheckFailureStatus
  test eax,eax
  jnz pdutFail
  inc pdutAssertions
  lea rax,HeapAlloc
  mov pdWorkerAllocate,rax
  mov pdutBackendError,5
  call PdUiSubmit
  call PdutWaitIdle
  test eax,eax
  jnz pdutFail
  call PdUiAccept
  cmp pdUiLastSequence,12
  jne pdutFail
  mov ecx,5
  call PdutCheckFailureStatus
  test eax,eax
  jnz pdutFail
  cmp pdUiBaselineValid,0
  jne pdutFail
  inc pdutAssertions
  mov pdutBackendError,0
  ; A delayed error from another opening cannot replace the visible status.
  mov rcx,pdWindow
  mov edx,4101
  lea r8,pdutOldStatus
  mov r9d,1024
  call GetDlgItemTextW
  lea rcx,pdWorkerLock
  call AcquireSRWLockExclusive
  lea rsi,pdUiRequest
  lea rdi,pdWorkerFailureRequest
  mov ecx,PD_REQUEST_SIZE/8
  rep movsq
  inc qword ptr [pdWorkerFailureRequest+PD_R_COOKIE]
  mov qword ptr [pdWorkerFailureRequest+PD_R_SEQUENCE],13
  mov pdWorkerFailure,123
  lea rcx,pdWorkerLock
  call ReleaseSRWLockExclusive
  call PdUiAccept
  cmp pdUiLastSequence,12
  jne pdutFail
  PDUT_CHECK_TEXT 4101,pdutOldStatus

  mov pdutStage,7209
  call PdutMakeSnapshot
  test rax,rax
  jz pdutFail
  mov qword ptr [rax+PD_S_SEQUENCE],13
  mov dword ptr [rax+PD_S_TARGET],PD_TARGET_DENIED
  mov dword ptr [rax+PD_S_ERROR],5
  mov dword ptr [rax+PD_S_FIELDS+PD_F_IDENTITY*16],PD_STATUS_UNAVAILABLE
  mov rcx,rax
  call PdutPublish
  cmp pdUiLastSequence,13
  jne pdutFail
  cmp pdUiLastTarget,PD_TARGET_DENIED
  jne pdutFail
  cmp pdUiBaselineValid,0
  jne pdutFail
  cmp pdUiStopped,0
  jne pdutFail
  mov rax,pdutOldSnapshot
  cmp rax,pdUiSnapshot
  jne pdutFail
  cmp dword ptr [rax+PD_S_FIELDS+PD_F_CPU_RATE*16],PD_STATUS_UNAVAILABLE
  jne pdutFail
  cmp dword ptr [rax+PD_S_FIELDS+PD_F_IO_RATE*16],PD_STATUS_UNAVAILABLE
  jne pdutFail
  inc pdutAssertions
  PDUT_CHECK_TEXT 4102,pdutPath
  ; Retained values must be explicitly dated, rather than appearing current.
  mov rcx,pdWindow
  mov edx,4101
  lea r8,pdutText
  mov r9d,8192
  call GetDlgItemTextW
  lea rcx,pdutYear
  lea rdx,pdutYearFormat
  movzx r8d,word ptr [pdUiGoodTime]
  call wsprintfW
  lea rcx,pdutText
  lea rdx,pdutYear
  call StrStrIW
  test rax,rax
  jz pdutFail
  inc pdutAssertions
  call PdutMakeSnapshot
  test rax,rax
  jz pdutFail
  mov qword ptr [rax+PD_S_SEQUENCE],14
  mov dword ptr [rax+PD_S_FIELDS+PD_F_CPU_TIMES*16],PD_STATUS_VALID
  mov rcx,rax
  call PdutPublish
  cmp pdUiLastTarget,PD_TARGET_VERIFIED
  jne pdutFail
  cmp pdUiBaselineValid,1
  jne pdutFail
  mov rax,pdUiSnapshot
  cmp dword ptr [rax+PD_S_FIELDS+PD_F_CPU_RATE*16],PD_STATUS_VALID
  je pdutFail
  inc pdutAssertions
  mov pdutOldSnapshot,rax

  mov pdutStage,7205
  mov esi,96
pdutDpi:
  mov rcx,pdWindow
  lea rdx,pdutRect
  call GetWindowRect
  mov eax,esi
  imul eax,680
  xor edx,edx
  mov ecx,96
  div ecx
  add eax,dword ptr [pdutRect]
  mov dword ptr [pdutRect+8],eax
  mov eax,esi
  imul eax,750
  xor edx,edx
  mov ecx,96
  div ecx
  add eax,dword ptr [pdutRect+4]
  mov dword ptr [pdutRect+12],eax
  mov rcx,pdWindow
  mov edx,2E0h ; real WM_DPICHANGED path with a suggested rectangle
  mov r8d,esi
  shl r8d,16
  or r8d,esi
  lea r9,pdutRect
  call SendMessageW
  cmp pdUiDpi,esi
  jne pdutFail
  inc pdutAssertions
  mov rcx,pdWindow
  lea rdx,pdutClient
  call GetClientRect
  xor edi,edi
pdutControlRect:
  mov rcx,pdWindow
  lea rax,pdUiControlIds
  mov edx,[rax+rdi*4]
  call GetDlgItem
  mov rbx,rax
  mov rcx,rax
  lea rdx,pdutRect
  call GetWindowRect
  xor ecx,ecx
  mov rdx,pdWindow
  lea r8,pdutRect
  mov r9d,2
  call MapWindowPoints
  cmp dword ptr [pdutRect],0
  jl pdutFail
  cmp dword ptr [pdutRect+4],0
  jl pdutFail
  mov eax,dword ptr [pdutRect+8]
  cmp eax,dword ptr [pdutClient+8]
  jg pdutFail
  mov eax,dword ptr [pdutRect+12]
  cmp eax,dword ptr [pdutClient+12]
  jg pdutFail
  inc pdutAssertions
  inc edi
  cmp edi,LENGTHOF pdUiControlIds
  jb pdutControlRect
  mov eax,pdutSavedGlobalDpi
  cmp eax,uiDpi
  jne pdutFail
  inc pdutAssertions
  mov rcx,pdWindowFont
  mov edx,104
  lea r8,pdutFont
  call GetObjectW
  test eax,eax
  jz pdutFail
  mov eax,esi
  imul eax,16
  xor edx,edx
  mov ecx,96
  div ecx
  neg eax
  cmp eax,dword ptr [pdutFont]
  jne pdutFail
  inc pdutAssertions
  add esi,48
  cmp esi,192
  jbe pdutDpi

  mov pdutStage,7206
  mov themeOption,0
pdutTheme:
  call PdUiApplyTheme
  mov rcx,pdWindow
  call GetDC
  mov rbx,rax
  mov rcx,pdWindow
  mov edx,138h
  mov r8,rbx
  xor r9d,r9d
  call SendMessageW
  test rax,rax
  jz pdutFail
  mov rcx,rbx
  call GetTextColor
  cmp eax,shellTextColor
  jne pdutFail
  mov rcx,rbx
  call GetBkColor
  mov ecx,0FFFFFFh
  cmp themeOption,0
  je pdutExpectedColor
  mov ecx,202020h
pdutExpectedColor:
  cmp eax,ecx
  jne pdutFail
  inc pdutAssertions
  mov rcx,pdWindow
  mov rdx,rbx
  call ReleaseDC
  inc themeOption
  cmp themeOption,2
  jb pdutTheme

  mov pdutStage,7207
  ; Mismatch stops timer-driven submits; visible data stays available.
  call PdutMakeSnapshot
  test rax,rax
  jz pdutFail
  mov qword ptr [rax+PD_S_SEQUENCE],15
  mov dword ptr [rax+PD_S_TARGET],PD_TARGET_MISMATCH
  mov dword ptr [rax+PD_S_FIELDS+PD_F_IDENTITY*16],PD_STATUS_UNAVAILABLE
  mov rcx,rax
  call PdutPublish
  cmp pdUiStopped,1
  jne pdutFail
  mov rbx,qword ptr [pdUiRequest+PD_R_SEQUENCE]
  call PdUiSubmit
  cmp rbx,qword ptr [pdUiRequest+PD_R_SEQUENCE]
  jne pdutFail
  inc pdutAssertions
  PDUT_CHECK_TEXT 4102,pdutPath

  ; Mismatch is terminal for that identity. Explicitly open a different owned
  ; target before testing process exit rather than clearing the stopped flag.
  mov ecx,4244
  mov rdx,133444736000000000
  lea r8,pdutName
  call PdUiShowTarget
  cmp pdUiStopped,0
  jne pdutFail
  call PdutWaitIdle
  test eax,eax
  jnz pdutFail
  call PdUiAccept
  mov rax,pdUiSnapshot
  test rax,rax
  jz pdutFail
  mov pdutOldSnapshot,rax
  inc pdutAssertions
  call PdutMakeSnapshot
  test rax,rax
  jz pdutFail
  mov qword ptr [rax+PD_S_SEQUENCE],16
  mov dword ptr [rax+PD_S_TARGET],PD_TARGET_EXITED
  mov rcx,rax
  call PdutPublish
  cmp pdUiLastTarget,PD_TARGET_EXITED
  jne pdutFail
  cmp pdUiStopped,1
  jne pdutFail
  cmp pdUiBaselineValid,0
  jne pdutFail
  mov rax,pdutOldSnapshot
  cmp rax,pdUiSnapshot
  jne pdutFail
  inc pdutAssertions
  PDUT_CHECK_TEXT 4102,pdutPath

  ; A native request queued before exit was observed may finish afterwards.
  ; Its unavailable result/error must not replace the confirmed terminal state.
  mov pdutStage,7211
  mov rcx,pdWindow
  mov edx,4101
  lea r8,pdutOldStatus
  mov r9d,1024
  call GetDlgItemTextW
  call PdutMakeSnapshot
  test rax,rax
  jz pdutFail
  mov qword ptr [rax+PD_S_SEQUENCE],17
  mov dword ptr [rax+PD_S_TARGET],PD_TARGET_UNKNOWN
  mov dword ptr [rax+PD_S_ERROR],87
  mov dword ptr [rax+PD_S_FIELDS+PD_F_IDENTITY*16],PD_STATUS_UNAVAILABLE
  mov rcx,rax
  call PdutPublish
  cmp pdUiLastTarget,PD_TARGET_EXITED
  jne pdutFail
  cmp pdUiLastSequence,16
  jne pdutFail
  cmp pdUiStopped,1
  jne pdutFail
  cmp pdUiBaselineValid,0
  jne pdutFail
  mov rax,pdutOldSnapshot
  cmp rax,pdUiSnapshot
  jne pdutFail
  inc pdutAssertions
  PDUT_CHECK_TEXT 4101,pdutOldStatus
  lea rcx,pdWorkerLock
  call AcquireSRWLockExclusive
  lea rsi,pdUiRequest
  lea rdi,pdWorkerFailureRequest
  mov ecx,PD_REQUEST_SIZE/8
  rep movsq
  mov qword ptr [pdWorkerFailureRequest+PD_R_SEQUENCE],18
  mov pdWorkerFailure,8
  lea rcx,pdWorkerLock
  call ReleaseSRWLockExclusive
  call PdUiAccept
  cmp pdUiLastTarget,PD_TARGET_EXITED
  jne pdutFail
  cmp pdUiLastSequence,16
  jne pdutFail
  cmp pdUiStopped,1
  jne pdutFail
  cmp pdUiBaselineValid,0
  jne pdutFail
  mov rax,pdutOldSnapshot
  cmp rax,pdUiSnapshot
  jne pdutFail
  cmp pdWorkerFailure,0
  jne pdutFail
  inc pdutAssertions
  PDUT_CHECK_TEXT 4101,pdutOldStatus

  mov pdutStage,7208
  mov rbx,pdWindow
  mov ecx,4243
  mov rdx,133444736000000001
  lea r8,pdutName
  call PdUiShowTarget
  cmp pdWindow,0
  je pdutFail
  mov rax,pdutCookie
  cmp rax,qword ptr [pdUiRequest+PD_R_COOKIE]
  je pdutFail
  cmp dword ptr [pdUiRequest+PD_R_PID],4243
  jne pdutFail
  cmp pdUiStopped,0
  jne pdutFail
  inc pdutAssertions
  ; Feed Escape through the same native dialog manager used by the main loop.
  mov rcx,pdWindow
  mov edx,4102
  call GetDlgItem
  mov qword ptr [pdutMessage],rax
  mov dword ptr [pdutMessage+8],100h
  mov qword ptr [pdutMessage+16],1Bh
  mov qword ptr [pdutMessage+24],0
  mov rbx,qword ptr [pdutMessage]
  mov qword ptr [pdutMessage],0 ; an unrelated window cannot close details
  lea rcx,pdutMessage
  call PdUiPreTranslate
  test eax,eax
  jnz pdutFail
  cmp pdWindow,0
  je pdutFail
  inc pdutAssertions
  mov qword ptr [pdutMessage],rbx
  lea rcx,pdutMessage
  call PdUiPreTranslate
  test eax,eax
  jz pdutFail
  inc pdutAssertions
  cmp pdWindow,0
  jne pdutFail
  cmp pdUiSnapshot,0
  jne pdutFail
  cmp pdWindowFont,0
  jne pdutFail
  inc pdutAssertions
  call PdutWaitIdle
  test eax,eax
  jnz pdutFail
  call PdutMakeSnapshot
  test rax,rax
  jz pdutFail
  mov qword ptr [rax+PD_S_SEQUENCE],20
  mov rcx,rax
  call PdutPublish
  cmp pdUiSnapshot,0
  jne pdutFail
  cmp pdWorkerBlocks,0
  jne pdutFail
  inc pdutAssertions
  inc appLanguage
  cmp appLanguage,2
  jb pdutLanguage

  mov pdutStage,0
  jmp pdutCleanup
pdutFail:
  mov pdutHold,0
  mov rcx,pdWindow
  mov edx,4102
  lea r8,pdutText
  mov r9d,8192
  call GetDlgItemTextW
  lea rcx,pdutOutput
  lea rdx,pdutDebugFormat
  lea r8,pdutText
  lea r9,pdutPath
  call wsprintfA
  mov [rsp+60h],eax
  mov ecx,-11
  call GetStdHandle
  mov rcx,rax
  lea rdx,pdutOutput
  mov r8d,[rsp+60h]
  lea r9,pdutWritten
  mov qword ptr [rsp+20h],0
  call WriteFile
pdutCleanup:
  mov rcx,pdWindow
  test rcx,rcx
  jz pdutStop
  call DestroyWindow
pdutStop:
  mov pdutHold,0
  call PdWorkerStop
  call PdOptionalWorkerStop
  mov pdUiInitHook,0
  mov eax,pdutSavedTheme
  mov themeOption,eax
  lea rcx,pdutOutput
  lea rdx,pdutFormat
  mov r8d,pdutStage
  mov r9d,pdutAssertions
  mov eax,pdutCalls
  mov [rsp+20h],rax
  mov eax,pdWorkerBlocks
  mov [rsp+28h],rax
  call wsprintfA
  mov [rsp+60h],eax
  mov ecx,-11
  call GetStdHandle
  mov rcx,rax
  lea rdx,pdutOutput
  mov r8d,[rsp+60h]
  lea r9,pdutWritten
  mov qword ptr [rsp+20h],0
  call WriteFile
  mov ecx,pdutStage
  call ExitProcess
PdUiTest ENDP
