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
  xor ecx,ecx
  xor edx,edx
  lea r8,CollectorMain
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
  je asCurrent
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
  call AddChartSample
  call BuildRowMap
  mov rcx,chartWnd
  xor edx,edx
  xor r8d,r8d
  call InvalidateRect
  mov rcx,gList
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
  call InitTaskScheduler
  call InitHistoryStore
  call InitTelemetry
cmLoop:
  cmp collectorStop,0
  jne cmExit
  call RunQueuedAction
  call CollectTelemetry
  call CollectRows
  call HistoryCheckpoint
  mov eax,desiredMode
  mov backMode,eax
  test eax,eax
  jnz cmThreads
  jmp cmPublish
cmThreads:
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

CollectRows PROC
  push rbx
  push rsi
  push rdi
  push r12
  push r13
  sub rsp,30h
  call CollectApplicationRoots
  lea rcx,historyNow
  call GetSystemTimeAsFileTime
  lea rcx,sysIdle
  lea rdx,sysKernel
  lea r8,sysUser
  call GetSystemTimes
  mov rax,sysKernel
  add rax,sysUser
  mov rcx,rax
  sub rax,sysPrev
  mov sysPrev,rcx
  mov sysDelta,rax
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
  cmp backCount,8192
  jae crClose
  mov edi,backCount
  shl rdi,12
  add rdi,backBuffer
  mov rcx,rdi
  call ClearRow
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
  mov ecx,410h
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
  jz crTimes
  mov dword ptr [rdi+3656],1
  mov rcx,qword ptr [memoryBuffer+72]
  mov [rdi+3624],rcx
  shr rcx,20
  lea rdx,[rdi+2048]
  call StoreDecimal
  mov rcx,qword ptr [memoryBuffer+16]
  mov [rdi+3632],rcx
  shr rcx,20
  lea rdx,[rdi+2560]
  call StoreDecimal
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
  mov rcx,rdi
  call RecordHistory
  cmp dword ptr [rdi+3672],0
  je crHandleClose
  mov rax,[rdi+3664]
  mov ecx,100
  mul rcx
  cmp sysDelta,0
  je crHandleClose
  div sysDelta
  mov rcx,rax
  lea rdx,[rdi+3072]
  call StoreDecimal
crHandleClose:
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
