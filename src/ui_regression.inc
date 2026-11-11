; Hidden-window regression. Uses the real controls, main WndProc and navigation.
; This entry never calls LoadSettings or starts the production collector.
; The test collector delays each snapshot by 600ms, exercising stale publication.
; No process mutations, system-wide trim, UI automation or user settings writes.
extern SendMessageTimeoutW:proc, GetStdHandle:proc, wsprintfA:proc
.data
align 8
; uiRegressionMode is declared by test_runtime_support.inc for both builds.
uiRegressionFailure dd 0
uiRegressionAck dd 0
uiRegressionObservedMode dd 0
uiRegressionObservedDesired dd 0
uiRegressionObservedBusy dd 0
uiRegressionObservedColumns dd 0
uiRegressionObservedVisible dd 0
uiRegressionNavCount dd 0
uiRegressionProbeCount dd 0
uiRegressionSnapshotCount dd 0
uiRegressionAccepted dd 0
uiRegressionAcceptedBase dd 0
uiRegressionSettledCount dd 0
uiRegressionNativeRows dd 0
uiRegressionMaxNavMs dq 0
uiRegressionMaxNullMs dq 0
uiRegressionProbeHandle dq 0
uiRegressionNullResult dq 0
uiRegressionBytes dd 0
uiRegressionModes dd 2,5,4,3,6,0
uiRegressionColumns dd 7,5,3,4,7,7
uiRegressionClass dw 'T','m','x','H','i','d','d','e','n','R','e','g','r','e','s','s','i','o','n',0
uiRegressionResultFmt db 'UI regression: exit=%u, navigation=%u, WM_NULL=%u, maxNav=%I64u ms, maxNull=%I64u ms, delayedSnapshots=%u, themeAssertions=%u, controlAssertions=%u, acceptedDuringSettle=%u, nativeRows=%u',13,10,0
uiRegressionFixtureName dw 'f','i','x','t','u','r','e','-','%','u','.','e','x','e',0
.data?
uiRegressionOutput db 256 dup(?)
.code

; A posted checkpoint follows each posted WM_COMMAND in FIFO order. This
; observes completed UI work, rather than merely seeing an early field write.
UiRegressionWndProc PROC
  cmp edx,8050h
  je urwCheckpoint
  jmp SimpleWndProc
urwCheckpoint:
  sub rsp,28h
  mov [rsp+20h],r8d
  mov eax,frontMode
  mov uiRegressionObservedMode,eax
  mov eax,desiredMode
  mov uiRegressionObservedDesired,eax
  mov eax,actionBusy
  mov uiRegressionObservedBusy,eax
  mov rcx,gMain
  call IsWindowVisible
  mov uiRegressionObservedVisible,eax
  mov rcx,gList
  mov edx,101Fh ; LVM_GETHEADER
  xor r8d,r8d
  xor r9d,r9d
  call SendMessageW
  mov rcx,rax
  mov edx,1200h ; HDM_GETITEMCOUNT
  xor r8d,r8d
  xor r9d,r9d
  call SendMessageW
  mov uiRegressionObservedColumns,eax
  mov eax,[rsp+20h]
  mov uiRegressionAck,eax
  xor eax,eax
  add rsp,28h
  ret
UiRegressionWndProc ENDP

UiRegressionCollector PROC
  sub rsp,28h
urcLoop:
  cmp collectorStop,0
  jne urcDone
  mov rax,qword ptr desiredMode
  mov qword ptr backMode,rax
  mov ecx,600
  call Sleep
  cmp collectorStop,0
  jne urcDone
  mov backCount,0
  cmp backMode,0
  jne urcMetrics
  call UiRegressionFillRows
urcMetrics:
  mov dword ptr [chartBack],-1
  mov dword ptr [chartBack+4],-1
  mov dword ptr [chartBack+8],-1
  mov dword ptr [chartBack+12],-1
  mov dword ptr [chartBack+16],-1
  mov rcx,gMain
  mov edx,8001h
  xor r8d,r8d
  xor r9d,r9d
  call PostMessageW
  test eax,eax
  jz urcDone
  inc uiRegressionSnapshotCount
urcAck:
  mov rcx,ackEvent
  mov edx,100
  call WaitForSingleObject
  cmp collectorStop,0
  jne urcDone
  test eax,eax
  jne urcAck
  jmp urcLoop
urcDone:
  xor eax,eax
  add rsp,28h
  ret
UiRegressionCollector ENDP

; 5000 fixture records with the production 4096-byte row layout. Only memory
; model calls here; native owner-data ListView receives its count once per pass.
UiRegressionFillRows PROC
  push rbx
  push rsi
  push rdi
  sub rsp,20h
  mov rdi,backBuffer
  xor esi,esi
urfrNext:
  mov rcx,rdi
  call ClearRow
  mov rcx,rdi
  lea rdx,uiRegressionFixtureName
  mov r8d,esi
  call wsprintfW
  lea ecx,[rsi+10000]
  mov [rdi+3584],ecx
  lea rdx,[rdi+512]
  call StoreDecimal
  lea eax,[rsi+1]
  mov [rdi+3600],rax
  test esi,esi
  jz urfrRoot
  lea eax,[rsi-1]
  xor edx,edx
  mov ecx,7
  div ecx
  add eax,10000
  mov [rdi+3588],eax
urfrRoot:
  mov ecx,[rdi+3588]
  lea rdx,[rdi+1024]
  call StoreDecimal
  mov ecx,2
  lea rdx,[rdi+1536]
  call StoreDecimal
  mov dword ptr [rdi+3656],1
  mov dword ptr [rdi+3672],1
  mov dword ptr [rdi+3896],1
  mov eax,processDefaultIcon
  mov [rdi+3880],eax
  mov eax,esi
  and eax,127
  inc eax
  shl rax,20
  mov [rdi+3624],rax
  mov [rdi+3632],rax
  mov rcx,rax
  shr rcx,20
  lea rdx,[rdi+2048]
  call StoreDecimal
  mov rcx,[rdi+3632]
  lea rdx,[rdi+2560]
  mov r8d,1048576
  call StoreScaledMetric
  mov edx,esi
  and edx,7
  mov rcx,rdi
  call FormatProcessCpu
  mov ecx,esi
  add ecx,uiRegressionSnapshotCount
  shl rcx,10
  mov [rdi+3888],rcx
  lea rdx,[rdi+3912]
  mov r8d,1024
  call StoreScaledMetric
  inc esi
  add rdi,4096
  cmp esi,5000
  jb urfrNext
  mov backCount,5000
  add rsp,20h
  pop rdi
  pop rsi
  pop rbx
  ret
UiRegressionFillRows ENDP

UiRegressionProbe PROC
  push rbx
  push rsi
  push rdi
  push r12
  sub rsp,48h
  xor esi,esi
  xor ebx,ebx
urpTick:
  ; First twelve transitions are idle; the next twelve must work while busy.
  xor edi,edi
  cmp esi,12
  jb urpBusySet
  mov edi,1
urpBusySet:
  mov actionBusy,edi
  lea rax,uiRegressionModes
  mov r12d,[rax+rbx*4]
  call GetTickCount64
  mov [rsp+38h],rax
  mov rcx,gMain
  mov edx,111h ; WM_COMMAND, same route as a real sidebar command
  lea r8d,[r12d+2100]
  xor r9d,r9d
  call PostMessageW
  test eax,eax
  jz urpPostFail
  mov rcx,gMain
  mov edx,8050h
  lea r8d,[rsi+1]
  xor r9d,r9d
  call PostMessageW
  test eax,eax
  jz urpPostFail
urpNavWait:
  call GetTickCount64
  sub rax,[rsp+38h]
  cmp rax,150
  jae urpNavTimeout
  lea edx,[rsi+1]
  cmp uiRegressionAck,edx
  je urpNavObserved
  mov ecx,1
  call Sleep
  jmp urpNavWait
urpNavObserved:
  cmp rax,uiRegressionMaxNavMs
  jbe urpNavMaxDone
  mov uiRegressionMaxNavMs,rax
urpNavMaxDone:
  cmp uiRegressionObservedMode,r12d
  jne urpNavModeFail
  cmp uiRegressionObservedDesired,r12d
  jne urpNavModeFail
  cmp uiRegressionObservedBusy,edi
  jne urpBusyFail
  cmp uiRegressionObservedVisible,0
  jne urpVisibleFail
  lea rax,uiRegressionColumns
  mov eax,[rax+rbx*4]
  cmp uiRegressionObservedColumns,eax
  jne urpColumnsFail
  inc uiRegressionNavCount
  call GetTickCount64
  mov [rsp+38h],rax
  mov rcx,gMain
  xor edx,edx ; WM_NULL, synchronous cross-thread responsiveness probe
  xor r8d,r8d
  xor r9d,r9d
  mov qword ptr [rsp+20h],2 ; SMTO_ABORTIFHUNG
  mov qword ptr [rsp+28h],250
  lea rax,uiRegressionNullResult
  mov [rsp+30h],rax
  call SendMessageTimeoutW
  test rax,rax
  jz urpNullTimeout
  call GetTickCount64
  sub rax,[rsp+38h]
  cmp rax,250
  jae urpNullTimeout
  cmp rax,uiRegressionMaxNullMs
  jbe urpNullMaxDone
  mov uiRegressionMaxNullMs,rax
urpNullMaxDone:
  inc uiRegressionProbeCount
  mov ecx,100 ; spans multiple real sidebar timer ticks and delayed snapshots
  call Sleep
  ; A delayed old snapshot must not switch the presented section back again.
  cmp frontMode,r12d
  jne urpNavModeFail
  cmp desiredMode,r12d
  jne urpNavModeFail
  cmp actionBusy,edi
  jne urpBusyFail
  inc ebx
  cmp ebx,6
  jb urpNext
  xor ebx,ebx
urpNext:
  inc esi
  cmp esi,24
  jb urpTick
  cmp uiRegressionSnapshotCount,2
  jb urpCollectorFail
  ; Stop navigating on the Process tab, so real ApplySnapshot/BuildRowMap
  ; and LVM_SETITEMCOUNTEX accept multiple 5000-row samples during probes.
  mov actionBusy,0
  mov eax,uiRegressionAccepted
  mov uiRegressionAcceptedBase,eax
  mov esi,300
urpSettle:
  call GetTickCount64
  mov [rsp+38h],rax
  mov rcx,gMain
  xor edx,edx
  xor r8d,r8d
  xor r9d,r9d
  mov qword ptr [rsp+20h],2
  mov qword ptr [rsp+28h],250
  lea rax,uiRegressionNullResult
  mov [rsp+30h],rax
  call SendMessageTimeoutW
  test rax,rax
  jz urpNullTimeout
  call GetTickCount64
  sub rax,[rsp+38h]
  cmp rax,250
  jae urpNullTimeout
  cmp rax,uiRegressionMaxNullMs
  jbe urpSettleMax
  mov uiRegressionMaxNullMs,rax
urpSettleMax:
  inc uiRegressionProbeCount
  cmp frontMode,0
  jne urpNavModeFail
  cmp desiredMode,0
  jne urpNavModeFail
  mov ecx,10
  call Sleep
  dec esi
  jnz urpSettle
  mov eax,uiRegressionAccepted
  sub eax,uiRegressionAcceptedBase
  mov uiRegressionSettledCount,eax
  cmp eax,3
  jb urpAcceptedFail
  mov rcx,gList
  mov edx,1004h ; LVM_GETITEMCOUNT from the real native control
  xor r8d,r8d
  xor r9d,r9d
  mov qword ptr [rsp+20h],2
  mov qword ptr [rsp+28h],250
  lea rax,uiRegressionNullResult
  mov [rsp+30h],rax
  call SendMessageTimeoutW
  test rax,rax
  jz urpNullTimeout
  mov eax,dword ptr [uiRegressionNullResult]
  mov uiRegressionNativeRows,eax
  cmp eax,5000
  jne urpRowsFail
  xor eax,eax
  jmp urpFinish
urpPostFail:
  mov eax,111
  jmp urpFinish
urpNavTimeout:
  mov eax,112
  jmp urpFinish
urpNavModeFail:
  mov eax,113
  jmp urpFinish
urpBusyFail:
  mov eax,114
  jmp urpFinish
urpNullTimeout:
  mov eax,115
  jmp urpFinish
urpVisibleFail:
  mov eax,116
  jmp urpFinish
urpColumnsFail:
  mov eax,117
  jmp urpFinish
urpCollectorFail:
  mov eax,118
  jmp urpFinish
urpAcceptedFail:
  mov eax,121
  jmp urpFinish
urpRowsFail:
  mov eax,122
urpFinish:
  mov uiRegressionFailure,eax
  mov actionBusy,0
  mov rcx,gMain
  mov edx,10h ; WM_CLOSE affects only this test's hidden window
  xor r8d,r8d
  xor r9d,r9d
  call PostMessageW
  xor eax,eax
  add rsp,48h
  pop r12
  pop rdi
  pop rsi
  pop rbx
  ret
UiRegressionProbe ENDP

UiRegressionTest PROC
  sub rsp,68h
  mov uiRegressionMode,1
  mov configReady,0
  mov appLanguage,0
  mov themeOption,0
  mov refreshInterval,1000
  lea rcx,commonControls
  call InitCommonControlsEx
  xor ecx,ecx
  call GetModuleHandleW
  mov gInst,rax
  call LoadUiStrings
  cmp word ptr [szProcess],0
  je urtResourceFail
  call GetDpiForSystem
  mov uiDpi,eax
  call RecalcUiScale
  mov gClass.wcSize,SIZEOF WNDCLASSEXW
  mov gClass.wcStyle,3
  lea rax,UiRegressionWndProc
  mov gClass.wcProc,rax
  mov rax,gInst
  mov gClass.wcInst,rax
  mov gClass.wcBrush,COLOR_WINDOW+1
  lea rax,uiRegressionClass
  mov gClass.wcName,rax
  lea rcx,gClass
  call RegisterClassExW
  test ax,ax
  jz urtClassFail
  xor ecx,ecx
  lea rdx,uiRegressionClass
  lea r8,szTitle
  mov r9d,WS_OVERLAPPEDWINDOW ; deliberately omits WS_VISIBLE
  mov qword ptr [rsp+20h],0
  mov qword ptr [rsp+28h],0
  mov qword ptr [rsp+30h],1320
  mov qword ptr [rsp+38h],760
  mov qword ptr [rsp+40h],0
  mov qword ptr [rsp+48h],0
  mov rax,gInst
  mov [rsp+50h],rax
  mov qword ptr [rsp+58h],0
  call CreateWindowExW
  mov gMain,rax
  test rax,rax
  jz urtWindowFail
  cmp gList,0
  je urtControlFail
  cmp searchWnd,0
  je urtControlFail
  cmp shellWnd,0
  je urtControlFail
  cmp chartWnd,0
  je urtControlFail
  cmp memoryButton,0
  je urtControlFail
  cmp collectorHandle,0
  je urtControlFail
  cmp ackEvent,0
  je urtControlFail
  lea rdx,navButtons
  xor ecx,ecx
urtNavControl:
  cmp qword ptr [rdx+rcx*8],0
  je urtControlFail
  inc ecx
  cmp ecx,6
  jb urtNavControl
  call UiControlsRegression
  test eax,eax
  jz urtControlsPassed
  mov uiRegressionFailure,eax
  jmp urtDestroy
urtControlsPassed:
  call NewTaskDialogRoutingTests
  test eax,eax
  jz urtNewTaskRoutePassed
  mov uiRegressionFailure,eax
  jmp urtDestroy
urtNewTaskRoutePassed:
IFDEF PC_TESTS
  call PcUiRoutingTests
  test eax,eax
  jz urtProcessColumnsRoutePassed
  mov uiRegressionFailure,eax
  jmp urtDestroy
urtProcessColumnsRoutePassed:
ENDIF
IFDEF PD_TESTS
  call PdUiRoutingTests
  test eax,eax
  jz urtProcessDetailsRoutePassed
  mov uiRegressionFailure,eax
  jmp urtDestroy
urtProcessDetailsRoutePassed:
ENDIF
  call MemoryAnimationTests
  test eax,eax
  jz urtAnimationPassed
  mov uiRegressionFailure,eax
  jmp urtDestroy
urtAnimationPassed:
  call UiTreeControlRegression
  test eax,eax
  jz urtTreePassed
  mov uiRegressionFailure,eax
  jmp urtDestroy
urtTreePassed:
  call MemoryPolicyTests
  test eax,eax
  jz urtPolicyPassed
  mov uiRegressionFailure,eax
  jmp urtDestroy
urtPolicyPassed:
  call UiReportLayoutTests
  test eax,eax
  jz urtReportPassed
  mov uiRegressionFailure,eax
  jmp urtDestroy
urtReportPassed:
  call UiThemeRegression
  test eax,eax
  jz urtThemePassed
  mov uiRegressionFailure,eax
  jmp urtDestroy
urtThemePassed:
  xor ecx,ecx
  xor edx,edx
  lea r8,UiRegressionProbe
  xor r9d,r9d
  mov qword ptr [rsp+20h],0
  mov qword ptr [rsp+28h],0
  call CreateThread
  mov uiRegressionProbeHandle,rax
  test rax,rax
  jz urtProbeFail
urtLoop:
  lea rcx,messageBuffer
  xor edx,edx
  xor r8d,r8d
  xor r9d,r9d
  call GetMessageW
  test eax,eax
  js urtMessageFail
  jz urtShutdown
  lea rcx,messageBuffer
  call TranslateMessage
  lea rcx,messageBuffer
  call DispatchMessageW
  jmp urtLoop
urtMessageFail:
  mov uiRegressionFailure,120
  jmp urtShutdown
urtResourceFail:
  mov uiRegressionFailure,101
  jmp urtShutdown
urtClassFail:
  mov uiRegressionFailure,102
  jmp urtShutdown
urtWindowFail:
  mov uiRegressionFailure,103
  jmp urtShutdown
urtControlFail:
  mov uiRegressionFailure,104
  jmp urtDestroy
urtProbeFail:
  mov uiRegressionFailure,105
urtDestroy:
  mov rcx,gMain
  call DestroyWindow
urtShutdown:
  mov collectorStop,1
  mov rcx,ackEvent
  call SetEvent
  mov rcx,collectorHandle
  test rcx,rcx
  jz urtProbeJoin
  mov edx,2000
  call WaitForSingleObject
  test eax,eax
  jz urtCollectorClose
  mov uiRegressionFailure,119
urtCollectorClose:
  mov rcx,collectorHandle
  call CloseHandle
urtProbeJoin:
  mov rcx,uiRegressionProbeHandle
  test rcx,rcx
  jz urtReport
  mov edx,1000
  call WaitForSingleObject
  mov rcx,uiRegressionProbeHandle
  call CloseHandle
urtReport:
  lea rcx,uiRegressionOutput
  lea rdx,uiRegressionResultFmt
  mov r8d,uiRegressionFailure
  mov r9d,uiRegressionNavCount
  mov eax,uiRegressionProbeCount
  mov [rsp+20h],rax
  mov rax,uiRegressionMaxNavMs
  mov [rsp+28h],rax
  mov rax,uiRegressionMaxNullMs
  mov [rsp+30h],rax
  mov eax,uiRegressionSnapshotCount
  mov [rsp+38h],rax
  mov eax,uiThemeAssertions
  mov [rsp+40h],rax
  mov eax,uiControlsAssertions
  mov [rsp+48h],rax
  mov eax,uiRegressionSettledCount
  mov [rsp+50h],rax
  mov eax,uiRegressionNativeRows
  mov [rsp+58h],rax
  call wsprintfA
  mov [rsp+60h],eax
  mov ecx,-11
  call GetStdHandle
  mov rcx,rax
  lea rdx,uiRegressionOutput
  mov r8d,[rsp+60h]
  lea r9,uiRegressionBytes
  mov qword ptr [rsp+20h],0
  call WriteFile
  mov ecx,uiRegressionFailure
  call ExitProcess
UiRegressionTest ENDP
include ui_theme_regression.inc
include ui_controls_regression.inc

include report_layout_tests.inc

include memory_animation_tests.inc
