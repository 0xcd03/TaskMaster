extern OpenSCManagerW:proc, EnumServicesStatusExW:proc, CloseServiceHandle:proc
extern WTSEnumerateSessionsW:proc, WTSQuerySessionInformationW:proc, WTSFreeMemory:proc
extern GlobalMemoryStatusEx:proc, GetPerformanceInfo:proc
.data
align 8
viewMenu dq 0
panelMenuText dw 'V','i','e','w',0
servicesText dw 'S','e','r','v','i','c','e','s',0
usersText dw 'U','s','e','r','s',0
perfText dw 'P','e','r','f','o','r','m','a','n','c','e',0
.data?
align 8
serviceBuffer db 262144 dup(?)
.data
serviceBytes dd 0
serviceCount dd 0
serviceResume dd 0
serviceEnumPages dd 0
serviceEnumError dd 0
serviceEnumTruncated dd 0
wtsBuffer dq 0
wtsCount dd 0
wtsName dq 0
wtsBytes dd 0
percentText dw '%',0
.data?
align 8
memStatus db 64 dup(?)
.data
.data?
align 8
perfInfo db 104 dup(?)
.data
.code
BuildPanelMenu PROC
  sub rsp,28h
  call CreatePopupMenu
  mov viewMenu,rax
  mov rcx,mainMenu
  mov edx,10h
  mov r8,rax
  lea r9,panelMenuText
  call AppendMenuW
  mov rcx,viewMenu
  xor edx,edx
  mov r8d,2100
  lea r9,szProcess
  call AppendMenuW
  mov rcx,viewMenu
  xor edx,edx
  mov r8d,2102
  lea r9,servicesText
  call AppendMenuW
  mov rcx,viewMenu
  xor edx,edx
  mov r8d,2103
  lea r9,usersText
  call AppendMenuW
  mov rcx,viewMenu
  xor edx,edx
  mov r8d,2104
  lea r9,perfText
  call AppendMenuW
  add rsp,28h
  ret
BuildPanelMenu ENDP

PanelColumns PROC
  sub rsp,28h
  cmp frontMode,5
  je pcStartup
  cmp frontMode,6
  je pcHistory
  cmp frontMode,2
  je pcServices
  cmp frontMode,3
  je pcUsers
  xor ecx,ecx
  lea rdx,metricText
  mov r8d,400
  call GuiColumn
  mov ecx,1
  lea rdx,valueText
  mov r8d,200
  call GuiColumn
  mov ecx,2
  lea rdx,unitText
  mov r8d,200
  call GuiColumn
  jmp pcDone
pcUsers:
  xor ecx,ecx
  lea rdx,userText
  mov r8d,300
  call GuiColumn
  mov ecx,1
  lea rdx,sessionText
  mov r8d,140
  call GuiColumn
  mov ecx,2
  lea rdx,stationText
  mov r8d,220
  call GuiColumn
  mov ecx,3
  lea rdx,stateText
  mov r8d,150
  call GuiColumn
  jmp pcDone
pcServices:
  xor ecx,ecx
  lea rdx,serviceNameText
  mov r8d,240
  call GuiColumn
  mov ecx,1
  lea rdx,displayText
  mov r8d,320
  call GuiColumn
  mov ecx,2
  lea rdx,szPid
  mov r8d,90
  call GuiColumn
  mov ecx,3
  lea rdx,stateText
  mov r8d,90
  call GuiColumn
  mov ecx,4
  lea rdx,typeText
  mov r8d,90
  call GuiColumn
  mov ecx,5
  lea rdx,acceptedText
  mov r8d,150
  call GuiColumn
  mov ecx,6
  lea rdx,flagsText
  mov r8d,90
  call GuiColumn
  jmp pcDone
pcStartup:
  xor ecx,ecx
  lea rdx,startupText
  mov r8d,240
  call GuiColumn
  mov ecx,1
  lea rdx,commandText
  mov r8d,400
  call GuiColumn
  mov ecx,2
  lea rdx,sourceText
  mov r8d,200
  call GuiColumn
  mov ecx,3
  lea rdx,stateText
  mov r8d,250
  call GuiColumn
  mov ecx,4
  lea rdx,startupRightsCaption
  mov r8d,180
  call GuiColumn
  jmp pcDone
pcHistory:
  xor ecx,ecx
  lea rdx,historyText
  mov r8d,240
  call GuiColumn
  mov ecx,1
  lea rdx,szPid
  mov r8d,90
  call GuiColumn
  mov ecx,2
  lea rdx,cpuTimeText
  mov r8d,130
  call GuiColumn
  mov ecx,3
  lea rdx,peakText
  mov r8d,180
  call GuiColumn
  mov ecx,4
  lea rdx,observedText
  mov r8d,130
  call GuiColumn
  mov ecx,5
  lea rdx,historyReadCaption
  mov r8d,180
  call GuiColumn
  mov ecx,6
  lea rdx,historyWriteCaption
  mov r8d,180
  call GuiColumn
pcDone:
  add rsp,28h
  ret
PanelColumns ENDP

CollectServices PROC
  mov ecx,262144
  mov edx,4096
  jmp CollectServicesWithLimits
CollectServices ENDP

; ECX page bytes (1..262144), EDX row capacity (1..4096). The production entry
; above always uses full limits. EAX/serviceEnumError reports incomplete reads;
; usable records from ERROR_MORE_DATA pages are retained before resuming.
CollectServicesWithLimits PROC
  push rbx
  push rsi
  push rdi
  push r12
  push r13
  push r14
  push r15
  sub rsp,60h
  mov r12d,ecx
  mov r13d,edx
  mov backCount,0
  mov serviceEnumPages,0
  mov serviceEnumError,0
  mov serviceEnumTruncated,0
  mov serviceResume,0
  mov serviceCount,0
  mov serviceBytes,0
  xor ebx,ebx
  test r12d,r12d
  jz csInvalid
  cmp r12d,SIZEOF serviceBuffer
  ja csInvalid
  test r13d,r13d
  jz csInvalid
  cmp r13d,4096
  ja csInvalid
  call CollectorSnapshotCurrent
  test eax,eax
  jz csCancelled
  xor ecx,ecx
  xor edx,edx
  mov r8d,4
  call OpenSCManagerW
  mov rbx,rax
  test rax,rax
  jnz csPage
  call GetLastError
  mov serviceEnumError,eax
  jmp csDone
csPage:
  call CollectorSnapshotCurrent
  test eax,eax
  jz csCancelled
  mov eax,serviceResume
  mov [rsp+50h],eax
  mov serviceCount,0
  xor r15d,r15d ; this page is final unless SCM says ERROR_MORE_DATA
  mov rcx,rbx
  xor edx,edx
  mov r8d,30h
  mov r9d,3
  lea rax,serviceBuffer
  mov [rsp+20h],rax
  mov [rsp+28h],r12
  lea rax,serviceBytes
  mov [rsp+30h],rax
  lea rax,serviceCount
  mov [rsp+38h],rax
  lea rax,serviceResume
  mov [rsp+40h],rax
  mov qword ptr [rsp+48h],0
  inc serviceEnumPages
  call EnumServicesStatusExW
  test eax,eax
  jnz csPageReady
  call GetLastError
  cmp eax,234 ; ERROR_MORE_DATA still returns serviceCount valid records
  jne csEnumFailed
  mov r15d,1
csPageReady:
  lea rsi,serviceBuffer
  xor r14d,r14d
csNext:
  call CollectorSnapshotCurrent
  test eax,eax
  jz csCancelled
  cmp r14d,serviceCount
  jae csPageDone
  mov eax,backCount
  cmp eax,r13d
  jae csCapacity
  mov edi,eax
  shl rdi,12
  add rdi,backBuffer
  mov rcx,rdi
  call ClearRow
  mov rcx,rdi
  mov rdx,[rsi]
  mov r8d,256
  call lstrcpynW
  lea rcx,[rdi+512]
  mov rdx,[rsi+8]
  mov r8d,256
  call lstrcpynW
  mov ecx,[rsi+44]
  lea rdx,[rdi+1024]
  call StoreDecimal
  mov eax,[rsi+20]
  dec eax
  cmp eax,7
  jae csUnknownState
  lea rdx,serviceStates
  mov rdx,[rdx+rax*8]
  lea rcx,[rdi+1536]
  call lstrcpyW
  jmp csStateDone
csUnknownState:
  mov ecx,[rsi+20]
  lea rdx,[rdi+1536]
  call StoreDecimal
csStateDone:
  mov ecx,[rsi+16]
  lea rdx,[rdi+2048]
  call StoreDecimal
  mov ecx,[rsi+24]
  lea rdx,[rdi+2560]
  call StoreDecimal
  mov ecx,[rsi+48]
  lea rdx,[rdi+3072]
  call StoreDecimal
  inc backCount
  inc r14d
  add rsi,56
  jmp csNext
csPageDone:
  test r15d,r15d
  jz csClose
  ; A too-small buffer or a nonadvancing resume handle must not spin forever.
  cmp serviceCount,0
  je csNoProgress
  mov eax,serviceResume
  test eax,eax
  jz csNoProgress
  cmp eax,[rsp+50h]
  je csNoProgress
  mov eax,backCount
  cmp eax,r13d
  jae csCapacity
  jmp csPage
csCapacity:
  mov serviceEnumTruncated,1
csNoProgress:
  mov eax,234 ; incomplete snapshot: ERROR_MORE_DATA
csEnumFailed:
  mov serviceEnumError,eax
  jmp csClose
csInvalid:
  mov serviceEnumError,87 ; ERROR_INVALID_PARAMETER
  jmp csDone
csCancelled:
  mov serviceEnumError,1223 ; ERROR_CANCELLED
csClose:
  mov rcx,rbx
  test rcx,rcx
  jz csDone
  call CloseServiceHandle
csDone:
  mov eax,serviceEnumError
  add rsp,60h
  pop r15
  pop r14
  pop r13
  pop r12
  pop rdi
  pop rsi
  pop rbx
  ret
CollectServicesWithLimits ENDP

CollectUsers PROC
  push rbx
  push rsi
  push rdi
  sub rsp,30h
  mov backCount,0
  xor ecx,ecx
  xor edx,edx
  mov r8d,1
  lea r9,wtsBuffer
  lea rax,wtsCount
  mov [rsp+20h],rax
  call WTSEnumerateSessionsW
  test eax,eax
  jz cuDone
  mov rsi,wtsBuffer
  mov ebx,wtsCount
cuNext:
  call CollectorSnapshotCurrent
  test eax,eax
  jz cuFree
  test ebx,ebx
  jz cuFree
  cmp backCount,4096
  jae cuFree
  mov edi,backCount
  shl rdi,12
  add rdi,backBuffer
  mov rcx,rdi
  call ClearRow
  mov ecx,[rsi]
  lea rdx,[rdi+512]
  call StoreDecimal
  lea rcx,[rdi+1024]
  mov rdx,[rsi+8]
  mov r8d,256
  call lstrcpynW
  mov eax,[rsi+16]
  cmp eax,10
  jae cuUnknownState
  lea rdx,sessionStates
  mov rdx,[rdx+rax*8]
  lea rcx,[rdi+1536]
  call lstrcpyW
  jmp cuStateDone
cuUnknownState:
  mov ecx,[rsi+16]
  lea rdx,[rdi+1536]
  call StoreDecimal
cuStateDone:
  mov rcx,rdi
  lea rdx,unavailable
  call lstrcpyW
  xor ecx,ecx
  mov edx,[rsi]
  mov r8d,5
  lea r9,wtsName
  lea rax,wtsBytes
  mov [rsp+20h],rax
  call WTSQuerySessionInformationW
  test eax,eax
  jz cuRow
  mov rcx,rdi
  mov rdx,wtsName
  mov r8d,256
  call lstrcpynW
  mov rcx,wtsName
  call WTSFreeMemory
cuRow:
  inc backCount
  add rsi,24
  dec ebx
  jmp cuNext
cuFree:
  mov rcx,wtsBuffer
  call WTSFreeMemory
cuDone:
  add rsp,30h
  pop rdi
  pop rsi
  pop rbx
  ret
CollectUsers ENDP

; RCX label, RDX value, R8 units. Adds a stable model row, never HWND calls.
MetricRow PROC
  push rbx
  push rsi
  push rdi
  sub rsp,20h
  mov rbx,rcx
  mov rsi,rdx
  mov rdi,r8
  mov eax,backCount
  shl rax,12
  add rax,backBuffer
  mov rcx,rax
  mov rdx,rbx
  mov rbx,rax
  call lstrcpyW
  mov rcx,rsi
  lea rdx,[rbx+512]
  call StoreDecimal
  lea rcx,[rbx+1024]
  mov rdx,rdi
  call lstrcpyW
  inc backCount
  add rsp,20h
  pop rdi
  pop rsi
  pop rbx
  ret
MetricRow ENDP

CollectPerformance PROC
  sub rsp,28h
  mov backCount,0
  cmp cpuValid,0
  je cpGpu
  lea rcx,cpuText
  mov edx,cpuValue
  lea r8,percentText
  call MetricRow
cpGpu:
  call GpCollectPerformance
cpRam:
  mov dword ptr [memStatus],64
  lea rcx,memStatus
  call GlobalMemoryStatusEx
  test eax,eax
  jz cpDone
  lea rcx,totalText
  mov rdx,qword ptr [memStatus+8]
  shr rdx,20
  lea r8,mbText
  call MetricRow
  lea rcx,availableText
  mov rdx,qword ptr [memStatus+16]
  shr rdx,20
  lea r8,mbText
  call MetricRow
  lea rcx,loadText
  mov edx,dword ptr [memStatus+4]
  lea r8,percentText
  call MetricRow
  lea rcx,perfInfo
  mov edx,104
  mov dword ptr [perfInfo],104
  call GetPerformanceInfo
  test eax,eax
  jz cpDone
  mov rax,qword ptr [perfInfo+8]
  mul qword ptr [perfInfo+80]
  mov rdx,rax
  shr rdx,20
  lea rcx,commitText
  lea r8,mbText
  call MetricRow
  mov rax,qword ptr [perfInfo+16]
  mul qword ptr [perfInfo+80]
  mov rdx,rax
  shr rdx,20
  lea rcx,limitText
  lea r8,mbText
  call MetricRow
cpDone:
  lea rcx,receiveText
  mov rdx,netIn
  shr rdx,20
  lea r8,mbText
  call MetricRow
  lea rcx,sendText
  mov rdx,netOut
  shr rdx,20
  lea r8,mbText
  call MetricRow
  add rsp,28h
  ret
CollectPerformance ENDP


.data
serviceStates dq serviceStopped,serviceStartPending,serviceStopPending,serviceRunning
dq serviceContinuePending,servicePausePending,servicePaused
sessionStates dq sessionActive,sessionConnected,sessionConnectQuery,sessionShadow,sessionDisconnected
dq sessionIdle,sessionListen,sessionReset,sessionDown,sessionInit
