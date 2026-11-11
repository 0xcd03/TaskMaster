; Hidden real common controls and real bounded metadata workers. Collector seam
; accepts only this fixture process's independently observed PID/birth.
extern CallWindowProcW:proc, ResetEvent:proc
.data
align 8
pmutStage dd 8801
pmutAssertions dd 0
pmutReady dd 0
pmutTimers dd 0
pmutPid dd 0
align 8
pmutBirth dq 0
pmutEpoch dq 0
pmutStart dq 0
pmutGate dq 0
pmutEntered dq 0
pmutOriginal dq 0
pmutCalls dd 3 dup(0)
pmutGateAll dd 0
pmutForeignCalls dd 0
pmutControls dd 8,1
align 2
pmutStatic dw 'S','T','A','T','I','C',0
pmutList dw 'S','y','s','L','i','s','t','V','i','e','w','3','2',0
pmutFormat db 'Metadata UI: exit=%u assertions=%u ready=%u timers=%u calls=%u/%u/%u threads=%u/%u/%u',13,10,0
.data?
align 8
pmutRows db 12288 dup(?)
pmutTimes dq 4 dup(?)
pmutMessage db 48 dup(?)
pmutItem db 88 dup(?)
pmutOutput db 256 dup(?)
pmutWritten dd ?
pmutColumn db 56 dup(?)
pmutCaption dw 256 dup(?)
pmutFairRequest db PD_REQUEST_SIZE dup(?)
.code
PMUT_EQ MACRO actual,expected
 cmp actual,expected
 jne pmutFail
 inc pmutAssertions
ENDM

PmutWindowProc PROC
  cmp edx,PM_READY_MESSAGE
  je pmutWindowReady
  cmp edx,113h
  jne pmutWindowForward
  cmp r8d,PM_TIMER_ID
  jne pmutWindowForward
  inc pmutTimers
  jmp pmutWindowPump
pmutWindowReady:
  inc pmutReady
pmutWindowPump:
  sub rsp,28h
  call PmUiPump
  xor eax,eax
  add rsp,28h
  ret
pmutWindowForward:
  sub rsp,38h
  mov [rsp+20h],r9
  mov r9,r8
  mov r8d,edx
  mov rdx,rcx
  mov rcx,pmutOriginal
  call CallWindowProcW
  add rsp,38h
  ret
PmutWindowProc ENDP

; Drain the actual HWND message queue; no direct deadline/cache mutation.
PmutDispatch PROC
  sub rsp,38h
pmutDispatchNext:
  lea rcx,pmutMessage
  xor edx,edx
  xor r8d,r8d
  xor r9d,r9d
  mov qword ptr [rsp+20h],1
  call PeekMessageW
  test eax,eax
  jz pmutDispatchDone
  lea rcx,pmutMessage
  call DispatchMessageW
  jmp pmutDispatchNext
pmutDispatchDone:
  mov ecx,5
  call Sleep
  add rsp,38h
  ret
PmutDispatch ENDP

PmutCollector PROC
  push rbx
  push rsi
  push rdi
  sub rsp,20h
  mov rsi,rcx
  mov rbx,rdx
  call PmValidateRequest
  test eax,eax
  jnz pmutCollectDone
  mov eax,13
  mov ecx,pmutPid
  cmp [rsi+PD_R_PID],ecx
  jne pmutCollectForeign
  mov rcx,pmutBirth
  cmp [rsi+PD_R_BIRTH],rcx
  jne pmutCollectForeign
  mov ecx,[rsi+PD_R_LANE]
  lea rdx,pmutCalls
  lock inc dword ptr [rdx+rcx*4-4]
  cmp ecx,PM_LANE_COMMAND
  je pmutCollectGate
  cmp pmutGateAll,0
  je pmutCollectBuild
pmutCollectGate:
  mov rcx,pmutEntered
  call SetEvent
  mov rcx,pmutGate
  mov edx,10000
  call WaitForSingleObject
  test eax,eax
  jz pmutCollectBuild
  mov eax,1460
  jmp pmutCollectDone
pmutCollectBuild:
  mov rdi,rbx
  mov ecx,PD_REQUEST_SIZE/8
  rep movsq
  mov dword ptr [rbx+PD_S_SIZE],PD_SNAPSHOT_SIZE
  mov dword ptr [rbx+PD_S_TARGET],PD_TARGET_VERIFIED
  mov rax,pmutBirth
  mov [rbx+PD_S_OBSERVED_BIRTH],rax
  mov qword ptr [rbx+PD_S_CONTEXT],0
  mov qword ptr [rbx+PD_S_CAPACITY],0
  mov eax,[rbx+PD_S_LANE]
  lea rdx,[rbx+PD_S_FIELDS+PD_F_PATH*PD_FIELD_SIZE]
  lea r8,[rbx+PD_S_PATH]
  mov ecx,'P'
  cmp eax,1
  jne pmutCollectAccount
  mov dword ptr [rbx+PD_S_CAPACITY],64
  jmp pmutCollectSource
pmutCollectAccount:
  lea rdx,[rbx+PD_S_FIELDS+PD_F_ACCOUNT*PD_FIELD_SIZE]
  lea r8,[rbx+PD_S_ACCOUNT]
  mov ecx,'A'
  cmp eax,2
  je pmutCollectSource
  lea rdx,[rbx+PD_S_FIELDS+PD_F_COMMAND*PD_FIELD_SIZE]
  lea r8,[rbx+PD_S_COMMAND]
  mov ecx,'C'
pmutCollectSource:
  mov dword ptr [rdx+PD_FIELD_STATUS],PD_STATUS_VALID
  mov dword ptr [rdx+PD_FIELD_LENGTH],1
  mov [r8],cx
  mov word ptr [r8+2],0
  xor eax,eax
  jmp pmutCollectDone
pmutCollectForeign:
  lock inc pmutForeignCalls
  mov eax,13
pmutCollectDone:
  add rsp,20h
  pop rdi
  pop rsi
  pop rbx
  ret
PmutCollector ENDP

PmutGet PROC
  mov ecx,pmutPid
  mov rdx,pmutBirth
  jmp PmCacheGet
PmutGet ENDP

PmutAllColumns PROC
  xor eax,eax
  lea rdx,pcVisibleIds
  lea r8,pcLayout
pmutAllIds:
  mov [rdx+rax*4],eax
  mov [r8+rax*4+PC_L_IDS],eax
  inc eax
  cmp eax,PC_COLUMN_COUNT
  jb pmutAllIds
  mov pcVisibleCount,PC_COLUMN_COUNT
  mov dword ptr [pcLayout+PC_L_COUNT],PC_COLUMN_COUNT
  ret
PmutAllColumns ENDP

; Cache-only synthetic identities never reach PmWorkerSubmit or a provider API.
PmutSeed PROC
  push rbx
  push rsi
  sub rsp,28h
  mov esi,r8d
  call PmCacheEnsure
  mov rbx,rax
  call GetProcessHeap
  mov rcx,rax
  mov edx,8
  mov r8d,4
  call HeapAlloc
  test rax,rax
  jz pmutSeedFailed
  mov [rax],si
  mov [rbx+PM_E_FIELDS+PM_FIELD_SIZE+PM_F_POINTER],rax
  mov dword ptr [rbx+PM_E_FIELDS+PM_FIELD_SIZE+PM_F_BYTES],4
  mov dword ptr [rbx+PM_E_FIELDS+PM_FIELD_SIZE+PM_F_STATUS],PM_STATUS_VALID
  mov rax,pmNow
  add rax,PM_TTL_MS
  mov [rbx+PM_E_FIELDS+PM_FIELD_SIZE+PM_F_DUE],rax
  add pmBytes,4
  mov rax,rbx
  add rsp,28h
  pop rsi
  pop rbx
  ret
pmutSeedFailed:
  mov ecx,8899
  call ExitProcess
PmutSeed ENDP

PMUiTest PROC
  sub rsp,88h
  xor ecx,ecx
  call GetModuleHandleW
  mov gInst,rax
  lea rcx,pmutControls
  call InitCommonControlsEx
  call LoadUiStrings
  xor ecx,ecx
  lea rdx,pmutStatic
  xor r8d,r8d
  xor r9d,r9d
  mov qword ptr [rsp+20h],0
  mov qword ptr [rsp+28h],0
  mov qword ptr [rsp+30h],800
  mov qword ptr [rsp+38h],500
  mov qword ptr [rsp+40h],0
  mov qword ptr [rsp+48h],0
  mov rax,gInst
  mov [rsp+50h],rax
  mov qword ptr [rsp+58h],0
  call CreateWindowExW
  mov gMain,rax
  test rax,rax
  jz pmutFail
  mov rcx,rax
  mov edx,-4
  lea r8,PmutWindowProc
  call SetWindowLongPtrW
  mov pmutOriginal,rax
  mov rax,gMain
  mov [rsp+40h],rax
  xor ecx,ecx
  lea rdx,pmutList
  xor r8d,r8d
  mov r9d,40001001h
  call CreateWindowExW
  mov gList,rax
  test rax,rax
  jz pmutFail
  mov rcx,rax
  mov edx,101Fh
  xor r8d,r8d
  xor r9d,r9d
  call SendMessageW
  mov processHeader,rax
  call PcEnsureLayout
  call PmutAllColumns
  mov uiRegressionMode,1
  mov frontMode,0
  mov pcAppliedMode,-1
  mov uiDpi,96
  call SetModeColumns
  mov rcx,processHeader
  mov edx,1200h
  xor r8d,r8d
  xor r9d,r9d
  call SendMessageW
  PMUT_EQ eax,PC_COLUMN_COUNT
  ; Query the actual native header in both resource languages.
  xor ebx,ebx
pmutLanguage:
  mov appLanguage,ebx
  mov pcApplying,1
  call SetModeColumns
  mov esi,12
pmutCaptionLoop:
  mov dword ptr [pmutColumn],5
  lea rax,pmutCaption
  mov qword ptr [pmutColumn+16],rax
  mov dword ptr [pmutColumn+24],256
  mov rcx,gList
  mov edx,105Fh
  mov r8d,esi
  lea r9,pmutColumn
  call SendMessageW
  PMUT_EQ eax,1
  cmp word ptr [pmutCaption],0
  je pmutFail
  inc pmutAssertions
  mov eax,dword ptr [pmutColumn+4]
  and eax,3
  xor edx,edx
  cmp esi,15
  setae dl
  PMUT_EQ eax,edx
  inc esi
  cmp esi,16
  jb pmutCaptionLoop
  inc ebx
  cmp ebx,2
  jb pmutLanguage
  mov appLanguage,0
  call PmUiDemand
  PMUT_EQ pmUiMask,0
  mov pmUiTestEnable,1
  call PmUiDemand
  PMUT_EQ pmUiMask,7
  PMUT_EQ pmUiTimer,1
  mov pmutStage,8802
  call GetCurrentProcessId
  mov pmutPid,eax
  mov dword ptr [pmutRows+3584],eax
  mov dword ptr [pmutRows+8192+3584],eax
  call GetCurrentProcess
  mov rcx,rax
  lea rdx,pmutTimes
  lea r8,pmutTimes+8
  lea r9,pmutTimes+16
  lea rax,pmutTimes+24
  mov [rsp+20h],rax
  call GetProcessTimes
  test eax,eax
  jz pmutFail
  mov rax,qword ptr pmutTimes
  mov pmutBirth,rax
  mov qword ptr [pmutRows+3600],rax
  mov qword ptr [pmutRows+4096+3600],rax
  lea rax,pmutRows
  mov frontBuffer,rax
  mov frontCount,3
  mov visibleCount,1
  mov qword ptr [rowMap],rax
  call PmUiFrame
  PMUT_EQ pmEntryCount,1
  mov rax,pmDemandEpoch
  mov pmutEpoch,rax
  call PmUiFrame
  mov rax,pmutEpoch
  PMUT_EQ pmDemandEpoch,rax
  mov frontCount,1
  mov sortColumn,PC_ACCOUNT
  mov rcx,gList
  mov edx,102Fh
  mov r8d,1
  xor r9d,r9d
  call SendMessageW
  ; Every second scheduler admission must serve the fair cursor even while
  ; the selected viewport identity remains continuously due.
  call PmCacheReset
  call PmUiDemand
  mov ecx,100000001
  mov edx,111
  call PmCacheEnsure
  mov ecx,100000002
  mov edx,111
  call PmCacheEnsure
  mov ecx,100000003
  mov edx,111
  call PmCacheEnsure
  mov dword ptr [pmutRows+3584],100000001
  mov qword ptr [pmutRows+3600],111
  mov dword ptr [pmUiFairTurns],0
  mov ecx,1
  lea rdx,pmutFairRequest
  call PmUiPrepare
  PMUT_EQ eax,0
  PMUT_EQ dword ptr [pmutFairRequest+PD_R_PID],100000001
  mov ecx,100000001
  mov edx,111
  call PmCacheFind
  mov dword ptr [rax+PM_E_FIELDS+PM_F_STATUS],PM_STATUS_MISSING
  mov qword ptr [rax+PM_E_FIELDS+PM_F_DUE],0
  mov dword ptr [pmCursors],1
  mov ecx,1
  lea rdx,pmutFairRequest
  call PmUiPrepare
  PMUT_EQ eax,0
  PMUT_EQ dword ptr [pmutFairRequest+PD_R_PID],100000002
  PMUT_EQ dword ptr [pmWorkerContexts+PM_W_THREADS],0
  call PmCacheReset
  mov eax,pmutPid
  mov dword ptr [pmutRows+3584],eax
  mov rax,pmutBirth
  mov qword ptr [pmutRows+3600],rax
  call PmUiFrame
  mov rax,pmDemandEpoch
  mov pmutEpoch,rax
  mov dword ptr [pmutItem+12],3
  mov dword ptr [pmutItem+16],3
  mov rcx,gList
  mov edx,102Bh
  xor r8d,r8d
  lea r9,pmutItem
  call SendMessageW
  xor ecx,ecx
  mov edx,1
  xor r8d,r8d
  xor r9d,r9d
  call CreateEventW
  mov pmutGate,rax
  xor ecx,ecx
  mov edx,1
  xor r8d,r8d
  xor r9d,r9d
  call CreateEventW
  mov pmutEntered,rax
  lea rax,PmutCollector
  mov pmWorkerCollectorQ,rax
  call PmUiPump
  call GetTickCount64
  mov pmutStart,rax
pmutWaitIndependent:
  call PmutDispatch
  mov r8d,1
  call PmutGet
  cmp edx,PM_STATUS_VALID
  jne pmutWaitIndependentTime
  mov r8d,2
  call PmutGet
  cmp edx,PM_STATUS_VALID
  jne pmutWaitIndependentTime
  mov rcx,pmutEntered
  xor edx,edx
  call WaitForSingleObject
  test eax,eax
  jz pmutIndependentReady
pmutWaitIndependentTime:
  call GetTickCount64
  sub rax,pmutStart
  cmp rax,5000
  jb pmutWaitIndependent
  jmp pmutFail
pmutIndependentReady:
  mov pmutStage,8803
  PMUT_EQ pmutCalls,1
  PMUT_EQ dword ptr [pmutCalls+4],1
  PMUT_EQ dword ptr [pmutCalls+8],1
  mov r8d,3
  call PmutGet
  PMUT_EQ edx,PM_STATUS_LOADING
  mov rcx,gList
  mov edx,100Ch
  mov r8,-1
  mov r9d,2
  call SendMessageW
  PMUT_EQ eax,0
  mov rax,qword ptr [rowMap]
  lea rdx,pmutRows
  PMUT_EQ rax,rdx
  ; Hold all three real worker requests before a cache-only TTL/resort phase.
  mov pmutGateAll,1
  mov ecx,pmutPid
  mov rdx,pmutBirth
  call PmCacheFind
  mov qword ptr [rax+PM_E_FIELDS+PM_F_DUE],0
  mov qword ptr [rax+PM_E_FIELDS+PM_FIELD_SIZE+PM_F_DUE],0
  call PmUiPump
  call GetTickCount64
  mov pmutStart,rax
pmutWaitAllHeld:
  call PmutDispatch
  cmp pmutCalls,2
  jne pmutWaitAllTime
  cmp dword ptr [pmutCalls+4],2
  je pmutAllHeld
pmutWaitAllTime:
  call GetTickCount64
  sub rax,pmutStart
  cmp rax,1000
  jb pmutWaitAllHeld
  jmp pmutFail
pmutAllHeld:
  PMUT_EQ dword ptr [pmWorkerContexts+PM_W_STATE],1
  PMUT_EQ dword ptr [pmWorkerContexts+256+PM_W_STATE],1
  PMUT_EQ dword ptr [pmWorkerContexts+512+PM_W_STATE],1
  call PmCacheReset
  call PmUiDemand
  mov ecx,pmutPid
  mov rdx,pmutBirth
  mov r8d,'B'
  call PmutSeed
  mov ecx,100000001
  mov edx,111
  mov r8d,'A'
  call PmutSeed
  mov [rsp+70h],rax
  mov ecx,100000002
  mov edx,111
  mov r8d,'C'
  call PmutSeed
  mov dword ptr [pmutRows+4096+3584],100000001
  mov qword ptr [pmutRows+4096+3600],111
  mov dword ptr [pmutRows+8192+3584],100000002
  mov qword ptr [pmutRows+8192+3600],111
  mov frontCount,3
  mov sortColumn,PC_ACCOUNT
  mov sortDescending,0
  call BuildRowMap
  mov rax,qword ptr [rowMap]
  lea rdx,pmutRows+4096
  PMUT_EQ rax,rdx
  mov rcx,gList
  mov edx,102Bh
  mov r8,-1
  mov dword ptr [pmutItem+12],0
  lea r9,pmutItem
  call SendMessageW
  mov rcx,gList
  mov edx,102Bh
  mov r8d,1
  mov dword ptr [pmutItem+12],3
  lea r9,pmutItem
  call SendMessageW
  ; This is an owned cache expiry boundary; the timer uses its real clock.
  ; All worker lanes are held on actual PID/birth, so no synthetic job starts.
  mov rax,[rsp+70h]
  mov qword ptr [rax+PM_E_FIELDS+PM_FIELD_SIZE+PM_F_DUE],0
  mov eax,pmutTimers
  mov [rsp+68h],eax
  call GetTickCount64
  mov pmutStart,rax
pmutWaitTtl:
  call PmutDispatch
  mov eax,pmutTimers
  cmp eax,[rsp+68h]
  ja pmutTtlDone
  call GetTickCount64
  sub rax,pmutStart
  cmp rax,1000
  jb pmutWaitTtl
  jmp pmutFail
pmutTtlDone:
  mov rax,qword ptr [rowMap]
  lea rdx,pmutRows
  PMUT_EQ rax,rdx
  mov rcx,gList
  mov edx,100Ch
  mov r8,-1
  mov r9d,2
  call SendMessageW
  PMUT_EQ eax,0
  PMUT_EQ pmutCalls,2
  PMUT_EQ dword ptr [pmutCalls+4],2
  PMUT_EQ dword ptr [pmutCalls+8],1
  call PmCacheReset
  mov frontCount,1
  mov dword ptr [pmutRows+4096+3584],0
  mov qword ptr [pmutRows+8192+3600],0
  call PmUiFrame
  call BuildRowMap
  mov pmutGateAll,0
  ; Hide account+command: only path remains demanded. No provider replacement.
  mov dword ptr [pcLayout+PC_L_COUNT],3
  mov dword ptr [pcLayout+PC_L_SORT],PC_NAME
  mov dword ptr [pcLayout+PC_L_IDS],PC_NAME
  mov dword ptr [pcLayout+PC_L_IDS+4],PC_PID
  mov dword ptr [pcLayout+PC_L_IDS+8],PC_IMAGE_PATH
  mov eax,3
  lea rdx,pcLayout
pmutHideIds:
  mov dword ptr [rdx+rax*4+PC_L_IDS],-1
  inc eax
  cmp eax,PC_LAYOUT_CAPACITY
  jb pmutHideIds
  mov pcApplying,1
  call SetModeColumns
  PMUT_EQ pmUiMask,1
  mov rax,pmutEpoch
  cmp pmDemandEpoch,rax
  jbe pmutFail
  inc pmutAssertions
  mov rcx,pmutGate
  call SetEvent
  call GetTickCount64
  mov pmutStart,rax
pmutWaitStale:
  call PmutDispatch
  mov ecx,3
  call PmWorkerBusy
  test eax,eax
  jz pmutStaleDone
  call GetTickCount64
  sub rax,pmutStart
  cmp rax,5000
  jb pmutWaitStale
  jmp pmutFail
pmutStaleDone:
  mov r8d,3
  call PmutGet
  PMUT_EQ edx,PM_STATUS_MISSING
  mov pmutStage,8804
  mov rax,pmDemandEpoch
  mov pmutEpoch,rax
  mov frontMode,2
  call PmUiDemand
  PMUT_EQ pmUiMask,0
  PMUT_EQ pmUiTimer,0
  PMUT_EQ pmUiFrameValid,0
  mov rax,pmutEpoch
  cmp pmDemandEpoch,rax
  jbe pmutFail
  inc pmutAssertions
  mov frontMode,0
  call PmutAllColumns
  mov pcApplying,1
  call SetModeColumns
  mov rcx,pmutGate
  call ResetEvent
  mov rcx,pmutEntered
  call ResetEvent
  call PmUiFrame
  call PmUiPump
  call GetTickCount64
  mov pmutStart,rax
pmutWaitTimeout:
  call PmutDispatch
  mov r8d,3
  call PmutGet
  cmp edx,PM_STATUS_TIMEOUT
  je pmutTimedOut
  call GetTickCount64
  sub rax,pmutStart
  cmp rax,5000
  jb pmutWaitTimeout
  jmp pmutFail
pmutTimedOut:
  PMUT_EQ r8d,1460
  PMUT_EQ dword ptr [pmutCalls+8],2
  cmp pmutTimers,0
  je pmutFail
  inc pmutAssertions
  PMUT_EQ dword ptr [pmWorkerContexts+PM_W_THREADS],1
  PMUT_EQ dword ptr [pmWorkerContexts+256+PM_W_THREADS],1
  PMUT_EQ dword ptr [pmWorkerContexts+512+PM_W_THREADS],1
  PMUT_EQ dword ptr [pmWorkerContexts+512+PM_W_STATE],1
  ; Frame pruning invalidates the sole admitted identity while command is blocked.
  mov frontCount,0
  call PmUiFrame
  PMUT_EQ pmEntryCount,0
  mov pmutStage,8805
  call GetTickCount64
  mov pmutStart,rax
  call PmUiStop
  call GetTickCount64
  sub rax,pmutStart
  cmp rax,250
  jae pmutFail
  inc pmutAssertions
  PMUT_EQ pmUiTimer,0
  PMUT_EQ pmEntryCount,0
  PMUT_EQ pdWorkerThreadsCreated,0
  mov rcx,pmutGate
  call SetEvent
  call GetTickCount64
  mov pmutStart,rax
pmutWaitStopped:
  call PmutDispatch
  mov rax,qword ptr [pmWorkerContexts+PM_W_THREAD]
  or rax,qword ptr [pmWorkerContexts+256+PM_W_THREAD]
  or rax,qword ptr [pmWorkerContexts+512+PM_W_THREAD]
  jz pmutStopped
  call GetTickCount64
  sub rax,pmutStart
  cmp rax,5000
  jb pmutWaitStopped
  jmp pmutFail
pmutStopped:
  PMUT_EQ pmEntryCount,0
  PMUT_EQ pmBytes,0
  PMUT_EQ pdWorkerThreadsCreated,0
  PMUT_EQ pmutForeignCalls,0
  mov pmutStage,0
pmutFail:
  ; Always unblock a fixture provider before final process cleanup.
  mov rcx,pmutGate
  call SetEvent
  call PmUiStop
  lea rcx,pmutOutput
  lea rdx,pmutFormat
  mov r8d,pmutStage
  mov r9d,pmutAssertions
  mov eax,pmutReady
  mov [rsp+20h],rax
  mov eax,pmutTimers
  mov [rsp+28h],rax
  mov eax,pmutCalls
  mov [rsp+30h],rax
  mov eax,dword ptr [pmutCalls+4]
  mov [rsp+38h],rax
  mov eax,dword ptr [pmutCalls+8]
  mov [rsp+40h],rax
  mov eax,dword ptr [pmWorkerContexts+PM_W_THREADS]
  mov [rsp+48h],rax
  mov eax,dword ptr [pmWorkerContexts+256+PM_W_THREADS]
  mov [rsp+50h],rax
  mov eax,dword ptr [pmWorkerContexts+512+PM_W_THREADS]
  mov [rsp+58h],rax
  call wsprintfA
  mov [rsp+78h],eax
  mov ecx,-11
  call GetStdHandle
  mov rcx,rax
  lea rdx,pmutOutput
  mov r8d,[rsp+78h]
  lea r9,pmutWritten
  mov qword ptr [rsp+20h],0
  call WriteFile
  mov ecx,pmutStage
  call ExitProcess
PMUiTest ENDP
