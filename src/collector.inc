; Immutable double buffered process snapshots. Only UI thread calls ListView.
extern HeapAlloc:proc, GetProcessHeap:proc, Sleep:proc
extern PostMessageW:proc, CreateEventW:proc, SetEvent:proc, WaitForSingleObject:proc
extern lstrcpyW:proc, GetProcessTimes:proc, GetSystemTimes:proc
extern GetTickCount64:proc, InvalidateRect:proc
.data
align 8
frontBuffer dq 0
backBuffer dq 0
frontCount dd 0
backCount dd 0
movingWindow dd 0
snapshotPending dd 0
collectorStop dd 0
collectorHandle dq 0
ackEvent dq 0
refreshEvent dq 0
timeCreate dq 0
timeExit dq 0
timeKernel dq 0
timeUser dq 0
sysIdle dq 0
sysKernel dq 0
sysUser dq 0
sysPrev dq 0
sysDelta dq 0
sysSampleTotal dq 0
historyObservationTick dq 0
historyOnlyPass dd 0
telemetryReady dd 0
.code
StartCollector PROC
  sub rsp,38h
  call GetProcessHeap
  mov rcx,rax
  mov edx,8
  mov r8d,33554432
  call HeapAlloc
  mov frontBuffer,rax
  call GetProcessHeap
  mov rcx,rax
  mov edx,8
  mov r8d,33554432
  call HeapAlloc
  mov backBuffer,rax
  test rax,rax
  jz collectorInitFail
  cmp frontBuffer,0
  je collectorInitFail
  xor ecx,ecx
  xor edx,edx
  xor r8d,r8d
  xor r9d,r9d
  call CreateEventW
  mov ackEvent,rax
  xor ecx,ecx
  xor edx,edx
  xor r8d,r8d
  xor r9d,r9d
  call CreateEventW
  mov refreshEvent,rax
  cmp uiRegressionMode,0
  jne scSkipIconWorker
  call StartIconWorker
scSkipIconWorker:
  xor ecx,ecx
  xor edx,edx
  lea r8,CollectorMain
IFDEF TM_TESTS
  cmp uiRegressionMode,0
  je scProductionEntry
  lea r8,UiRegressionCollector
scProductionEntry:
ENDIF
  xor r9d,r9d
  mov qword ptr [rsp+20h],0
  mov qword ptr [rsp+28h],0
  call CreateThread
  mov collectorHandle,rax
collectorInitFail:
  add rsp,38h
  ret
StartCollector ENDP

ApplySnapshot PROC
  sub rsp,28h
  mov eax,backMode
  cmp eax,desiredMode
  jne asReject
  mov eax,backGeneration
  cmp eax,desiredGeneration
  je asCurrent
asReject:
  mov snapshotPending,0
  mov rcx,ackEvent
  call SetEvent
  jmp asDone
asCurrent:
  call RememberSelection
  mov eax,backMode
  cmp eax,frontMode
  je asSameMode
  mov frontMode,eax
  call SetModeColumns
  call LayoutContent
asSameMode:
  mov rax,frontBuffer
  mov rdx,backBuffer
  mov frontBuffer,rdx
  mov backBuffer,rax
  mov eax,backCount
  mov frontCount,eax
  mov snapshotPending,0
  call ApplyTelemetryChart
  call PmUiFrame
  call GpUiDemand
  call GpUiPump
  call BuildRowMap
  call PmUiPump
IFDEF TM_TESTS
  cmp uiRegressionMode,0
  je asNotRegression
  inc uiRegressionAccepted
asNotRegression:
ENDIF
  mov rcx,chartWnd
  xor edx,edx
  xor r8d,r8d
  call InvalidateRect
  mov rcx,ackEvent
  call SetEvent
asDone:
  add rsp,28h
  ret
ApplySnapshot ENDP

CollectorMain PROC
  sub rsp,28h
  ; Shell icon lookup uses COM too, but connecting to Task Scheduler is lazy.
  xor ecx,ecx
  xor edx,edx
  call CoInitializeEx
  test eax,eax
  js cmHistoryInit
  mov startupCom,1
cmHistoryInit:
  call InitHistoryStore
  call EnableObservationPrivilege
cmLoop:
  cmp collectorStop,0
  jne cmExit
  call RunQueuedAction
  ; Capture mode and navigation generation before any potentially expensive
  ; enumeration. A UI switch invalidates this pass without waiting for it.
  mov rax,qword ptr desiredMode
  mov qword ptr collectMode,rax
  mov eax,inspectedPid
  mov collectInspectedPid,eax
  mov rax,inspectedBirth
  mov collectInspectedBirth,rax
  mov historyOnlyPass,0
  mov eax,collectMode
  test eax,eax
  je cmTelemetry
  cmp eax,4
  jnz cmAfterTelemetry
cmTelemetry:
  cmp telemetryReady,0
  jne cmCollectTelemetry
  call InitTelemetry
  mov telemetryReady,1
cmCollectTelemetry:
  call CollectTelemetry
cmAfterTelemetry:
  call CollectorSnapshotCurrent
  test eax,eax
  jz cmLoop
  mov eax,collectMode
  test eax,eax
  jnz cmThreads
  call CollectRows
  call CollectorSnapshotCurrent
  test eax,eax
  jz cmLoop
  call GetTickCount64
  mov historyObservationTick,rax
  jmp cmPublish
cmThreads:
  mov eax,collectMode
  cmp eax,1
  jne cmOther
  call CollectThreads
  jmp cmPublish
cmOther:
  cmp eax,2
  jne cmNotServices
  call CollectServices
  jmp cmPublish
cmNotServices:
  cmp eax,3
  jne cmNotUsers
  call CollectUsers
  jmp cmPublish
cmNotUsers:
  cmp eax,5
  jne cmNotStartup
  call InitTaskScheduler
  call CollectStartupV2
  jmp cmPublish
cmNotStartup:
  cmp eax,6
  jne cmPerformance
  call CollectHistory
  jmp cmPublish
cmPerformance:
  call CollectPerformance
cmPublish:
  call CollectorSnapshotCurrent
  test eax,eax
  jz cmLoop
  mov rax,qword ptr collectMode
  mov qword ptr backMode,rax
  mov rcx,gMain
  mov edx,8001h
  xor r8d,r8d
  xor r9d,r9d
  call PostMessageW
  test eax,eax
  jz cmExit
cmAck:
  mov rcx,ackEvent
  mov edx,100
  call WaitForSingleObject
  cmp collectorStop,0
  jne cmExit
  test eax,eax
  jne cmAck
  call CollectorSnapshotCurrent
  test eax,eax
  jz cmLoop
  ; The requested panel is already visible before maintenance work begins.
  ; Outside Processes, take interruptible history observations at most once
  ; per five seconds, without window-root or shell-icon discovery.
  call HistoryCheckpoint
  cmp collectMode,0
  je cmPrepareInterval
  call GetTickCount64
  sub rax,historyObservationTick
  cmp rax,5000
  jb cmPrepareInterval
  mov historyOnlyPass,1
  call CollectRows
  mov historyOnlyPass,0
  call CollectorSnapshotCurrent
  test eax,eax
  jz cmLoop
  call GetTickCount64
  mov historyObservationTick,rax
  call HistoryCheckpoint
cmPrepareInterval:
  mov rcx,gMain
  call IsIconic
  mov edx,refreshInterval
  test eax,eax
  jz cmInterval
  mov edx,5000
cmInterval:
  mov rcx,refreshEvent
  call WaitForSingleObject
  jmp cmLoop
cmExit:
  call CloseTelemetry
  call SaveHistoryStore
  call ReleaseSuspensions
  mov rcx,taskFolder
  call ReleaseCom
  mov rcx,taskService
  call ReleaseCom
  cmp startupCom,0
  je cmNoCom
  call CoUninitialize
cmNoCom:
  xor eax,eax
  add rsp,28h
  ret
CollectorMain ENDP

; Shared cancellation point for collector enumerations. Isolated model tests
; do not have a collector handle and are not driven by navigation requests.
CollectorSnapshotCurrent PROC
  cmp collectorStop,0
  jne cscCancelled
  cmp collectorHandle,0
  je cscCurrent
  mov rax,qword ptr desiredMode
  cmp rax,qword ptr collectMode
  jne cscCancelled
  cmp actionPending,0
  jne cscCancelled
cscCurrent:
  mov eax,1
  ret
cscCancelled:
  xor eax,eax
  ret
CollectorSnapshotCurrent ENDP

; RCX integer, RDX destination. DecimalBuffer belongs solely to worker now.
StoreDecimal PROC
  push rbx
  sub rsp,20h
  mov rbx,rdx
  call GuiDecimal
  mov rcx,rbx
  mov rdx,rax
  call lstrcpyW
  add rsp,20h
  pop rbx
  ret
StoreDecimal ENDP

; RCX process row. RecordHistory supplies CPU and system deltas for the same
; PID+birth interval. Invalid/overflowed/impossible samples remain unavailable.
CalculateProcessCpu PROC
  mov r10,rcx
  cmp dword ptr [r10+3672],0
  je cpcDone
  mov r11,[r10+4000]
  test r11,r11
  jz cpcInvalid
  mov rax,[r10+3664]
  cmp rax,r11
  ja cpcInvalid
  mov ecx,1000
  mul rcx
  cmp rdx,r11
  jae cpcInvalid
  div r11
  cmp rax,1000
  ja cpcInvalid
  mov edx,eax
  mov rcx,r10
  jmp FormatProcessCpu
cpcInvalid:
  mov dword ptr [r10+3672],0
  mov dword ptr [r10+3900],0
cpcDone:
  ret
CalculateProcessCpu ENDP

CollectRows PROC
  push rbx
  push rsi
  push rdi
  push r12
  push r13
  sub rsp,30h
  cmp historyOnlyPass,0
  jne crTimesStart
  call CollectApplicationRoots
  call BuildProcessPauseMap
crTimesStart:
  lea rcx,historyNow
  call GetSystemTimeAsFileTime
  lea rcx,sysIdle
  lea rdx,sysKernel
  lea r8,sysUser
  call GetSystemTimes
  mov sysSampleTotal,0
  mov sysDelta,0
  test eax,eax
  jz crSystemTimesReady
  mov rax,sysKernel
  add rax,sysUser
  jc crSystemTimesReady
  mov sysSampleTotal,rax
  mov rcx,rax
  sub rax,sysPrev
  mov sysPrev,rcx
  mov sysDelta,rax
crSystemTimesReady:
  mov backCount,0
  mov ecx,2
  xor edx,edx
  call CreateToolhelp32Snapshot
  mov rbx,rax
  cmp rax,-1
  je crExit
  mov dword ptr [processBuffer],568
  mov rcx,rbx
  lea rdx,processBuffer
  call Process32FirstW
crNext:
  test eax,eax
  jz crClose
  call CollectorSnapshotCurrent
  test eax,eax
  jz crClose
  cmp backCount,8192
  jae crClose
  mov edi,backCount
  shl rdi,12
  add rdi,backBuffer
  mov rcx,rdi
  call ClearRow
  mov eax,processDefaultIcon
  mov [rdi+3880],eax
  mov dword ptr [rdi+3884],2
  mov rcx,rdi
  lea rdx,processBuffer+44
  mov r8d,256
  call lstrcpynW
  mov ecx,dword ptr [processBuffer+8]
  mov [rdi+3584],ecx
  call IsApplicationRoot
  mov [rdi+3676],eax
  mov ecx,dword ptr [processBuffer+8]
  lea rdx,[rdi+512]
  call StoreDecimal
  mov ecx,dword ptr [processBuffer+32]
  mov [rdi+3588],ecx
  lea rdx,[rdi+1024]
  call StoreDecimal
  mov ecx,dword ptr [processBuffer+28]
  lea rdx,[rdi+1536]
  call StoreDecimal
  lea rcx,[rdi+2048]
  lea rdx,unavailable
  call lstrcpyW
  lea rcx,[rdi+2560]
  lea rdx,unavailable
  call lstrcpyW
  lea rcx,[rdi+3072]
  lea rdx,unavailable
  call lstrcpyW
  mov qword ptr [rdi+3600],0
  ; Do not require VM_READ merely to observe CPU, I/O and creation time.
  ; Reduced-rights queries work for many services that deny a full query handle.
  mov ecx,1000h ; PROCESS_QUERY_LIMITED_INFORMATION
  xor edx,edx
  mov r8d,dword ptr [processBuffer+8]
  call OpenProcess
  mov rsi,rax
  test rax,rax
  jz crRowDone
  mov rcx,rsi
  lea rdx,memoryBuffer
  mov r8d,80
  mov dword ptr [memoryBuffer],80
  call GetProcessMemoryInfo
  test eax,eax
  jnz crMemoryAvailable
  ; Legacy memory API access can require QUERY_INFORMATION | VM_READ.
  ; Keep the limited handle even when this optional stronger query is denied.
  mov ecx,410h
  xor edx,edx
  mov r8d,dword ptr [rdi+3584]
  call OpenProcess
  mov r12,rax
  test rax,rax
  jz crTimes
  mov rcx,r12
  lea rdx,memoryBuffer
  mov r8d,80
  call GetProcessMemoryInfo
  mov r13d,eax
  mov rcx,r12
  call CloseHandle
  test r13d,r13d
  jz crTimes
crMemoryAvailable:
  mov dword ptr [rdi+3656],1
  mov rcx,qword ptr [memoryBuffer+72]
  mov [rdi+3624],rcx
  shr rcx,20
  lea rdx,[rdi+2048]
  call StoreDecimal
  mov rcx,qword ptr [memoryBuffer+16]
  mov [rdi+3632],rcx
  lea rdx,[rdi+2560]
  mov r8d,1048576
  call StoreScaledMetric
crTimes:
  mov rcx,rsi
  lea rdx,processIo
  call GetProcessIoCounters
  test eax,eax
  jz crGetTimes
  mov dword ptr [rdi+3660],1
  mov rax,[processIo+24]
  mov [rdi+3640],rax
  mov rax,[processIo+32]
  mov [rdi+3648],rax
crGetTimes:
  mov rcx,rsi
  lea rdx,timeCreate
  lea r8,timeExit
  lea r9,timeKernel
  lea rax,timeUser
  mov [rsp+20h],rax
  call GetProcessTimes
  test eax,eax
  jz crHandleClose
  mov rax,timeCreate
  mov [rdi+3600],rax
  mov rax,timeKernel
  add rax,timeUser
  mov [rdi+3608],rax
  ; The held limited handle has just supplied this row's creation identity.
  ; A failed handle-count sample remains explicitly unavailable.
  mov rcx,rdi
  mov rdx,rsi
  call PmRecordHandles
  mov rcx,rdi
  call RecordHistory
  mov rcx,rdi
  call CalculateProcessCpu
crHandleClose:
  cmp historyOnlyPass,0
  jne crProcessClose
  call CollectorSnapshotCurrent
  test eax,eax
  jz crProcessClose
  mov rcx,rdi
  mov rdx,rsi
  call CollectProcessPresentation
crProcessClose:
  mov rcx,rsi
  call CloseHandle
crRowDone:
  inc backCount
  mov rcx,rbx
  lea rdx,processBuffer
  call Process32NextW
  jmp crNext
crClose:
  mov rcx,rbx
  call CloseHandle
crExit:
  add rsp,30h
  pop r13
  pop r12
  pop rdi
  pop rsi
  pop rbx
  ret
CollectRows ENDP

ClearRow PROC
  push rdi
  mov rdi,rcx
  xor eax,eax
  mov ecx,512
  rep stosq
  pop rdi
  ret
ClearRow ENDP


extern GetProcessIoCounters:proc
.data?
align 8
processIo dq 6 dup(?)
extern IsIconic:proc
