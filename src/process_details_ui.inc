; UI owns the window, fonts and accepted snapshot. Workers only receive values.
extern CreateDialogParamW:proc, FileTimeToSystemTime:proc
extern SystemTimeToTzSpecificLocalTime:proc, IsIconic:proc
extern GetDynamicTimeZoneInformation:proc, SystemTimeToTzSpecificLocalTimeEx:proc
extern EnableMenuItem:proc
extern IsChild:proc, GetLocalTime:proc
.data
align 8
pdWindow dq 0
pdWindowFont dq 0
pdCookieCounter dq 0
pdUiLastSequence dq 0
pdUiSnapshot dq 0
pdUiDpi dd 96
pdUiStopped dd 0
pdUiBaselineValid dd 0
pdUiLastTarget dd 0
pdUiInitHook dq 0 ; isolated hidden-native tests only; zero in the application
pdUiControlIds dd 4100,4101,4110,4102,4111,4103,4112,4104,4113,4106,4114,4107,4105,1,2
; control, base96 x,y,width,height. Counter area grows with the window.
pdUiPositions dd 4100,12,10,600,22,4101,12,34,600,64
dd 4110,12,102,600,18,4102,12,122,600,50
dd 4111,12,180,600,18,4103,12,200,600,188
dd 4112,12,396,600,18,4104,12,416,600,24
dd 4113,12,448,600,18,4106,12,468,600,24
dd 4114,12,500,600,18,4107,12,520,600,70
dd 4105,12,598,600,32,1,420,638,90,28,2,522,638,90,28
.data?
align 8
pdUiRequest db PD_REQUEST_SIZE dup(?)
pdUiName dw 256 dup(?)
pdUiFormat dw 512 dup(?)
pdUiText dw 1024 dup(?)
pdUiLabel dw 512 dup(?)
pdUiLine dw 1024 dup(?)
pdUiMetrics dw 8192 dup(?)
pdUiUtc db 16 dup(?)
pdUiLocal db 16 dup(?)
pdUiGoodTime db 16 dup(?)
pdUiTimeZone db 432 dup(?)
pdUiFailureText dw 1024 dup(?)
pdUiErrorRequest db PD_REQUEST_SIZE dup(?)
pdUiErrorHeader db 64 dup(?)
.code
; Multiline read-only EDIT controls can consume Escape in IsDialogMessage.
; Only keyboard input belonging to this modeless window may close it.
PdUiPreTranslate PROC
  push rbx
  sub rsp,20h
  mov rbx,rcx
  cmp pdWindow,0
  je pduptNo
  cmp dword ptr [rbx+8],100h
  jne pduptDialog
  cmp qword ptr [rbx+16],27
  jne pduptDialog
  mov rdx,[rbx]
  mov rcx,pdWindow
  cmp rcx,rdx
  je pduptClose
  call IsChild
  test eax,eax
  jz pduptDialog
pduptClose:
  mov rcx,pdWindow
  call DestroyWindow
  mov eax,1
  jmp pduptDone
pduptDialog:
  mov rcx,pdWindow
  mov rdx,rbx
  call IsDialogMessageW
  jmp pduptDone
pduptNo:
  xor eax,eax
pduptDone:
  add rsp,20h
  pop rbx
  ret
PdUiPreTranslate ENDP

; EDX even EN resource id -> RAX private text. Applied language, not next-restart preference.
PdUiLoadText PROC
  sub rsp,28h
  add edx,appLanguage
  mov rcx,gInst
  lea r8,pdUiText
  mov r9d,1024
  call LoadStringW
  lea rax,pdUiText
  add rsp,28h
  ret
PdUiLoadText ENDP

PdUiUpdateMenu PROC
  sub rsp,28h
  mov r8d,1
  cmp frontMode,0
  jne pduumSet
  mov rcx,gList
  mov edx,LVM_GETNEXTITEM
  mov r8,-1
  mov r9d,LVNI_SELECTED
  call SendMessageW
  mov r8d,1
  cmp eax,visibleCount
  jae pduumSet
  xor r8d,r8d
pduumSet:
  mov rcx,mainMenu
  mov edx,2501
  call EnableMenuItem
  add rsp,28h
  ret
PdUiUpdateMenu ENDP

; Receives selected row identity while still owned by UI. No row pointer escapes.
ShowSelectedProcessDetails PROC
  sub rsp,28h
  cmp frontMode,0
  jne sspdDone
  mov rcx,gList
  mov edx,LVM_GETNEXTITEM
  mov r8,-1
  mov r9d,LVNI_SELECTED
  call SendMessageW
  cmp eax,visibleCount
  jae sspdDone
  lea rdx,rowMap
  mov r8,[rdx+rax*8]
  mov ecx,[r8+3584]
  mov rdx,[r8+3600]
  call PdUiShowTarget
sspdDone:
  add rsp,28h
  ret
ShowSelectedProcessDetails ENDP

; ECX PID, RDX exact creation FILETIME, R8 name. Single pinned modeless window.
PdUiShowTarget PROC
  push rbx
  push rsi
  push rdi
  sub rsp,40h
  mov ebx,ecx
  mov rsi,rdx
  mov rdi,r8
  cmp pdWindow,0
  je pdustNew
  cmp ebx,dword ptr [pdUiRequest+PD_R_PID]
  jne pdustChange
  cmp rsi,qword ptr [pdUiRequest+PD_R_BIRTH]
  je pdustActivate
pdustChange:
  mov rcx,pdWindow
  call DestroyWindow
pdustNew:
  call PdWorkerEnsure
  test eax,eax
  jnz pdustError
  lea rcx,pdUiName
  mov rdx,rdi
  mov r8d,256
  call lstrcpynW
  lea rdi,pdUiRequest
  xor eax,eax
  mov ecx,PD_REQUEST_SIZE/8
  rep stosq
  mov dword ptr [pdUiRequest+PD_R_SIZE],PD_REQUEST_SIZE
  mov dword ptr [pdUiRequest+PD_R_VERSION],PD_VERSION
  mov dword ptr [pdUiRequest+PD_R_LANE],1
  inc pdCookieCounter
  mov rax,pdCookieCounter
  mov qword ptr [pdUiRequest+PD_R_COOKIE],rax
  mov qword ptr [pdUiRequest+PD_R_GENERATION],rax
  mov dword ptr [pdUiRequest+PD_R_PID],ebx
  mov qword ptr [pdUiRequest+PD_R_BIRTH],rsi
  mov pdUiStopped,0
  mov pdUiBaselineValid,0
  mov pdUiLastTarget,0
  mov pdUiLastSequence,0
  mov rcx,gInst
  mov edx,305
  add edx,appLanguage
  mov r8,gMain
  lea r9,PdUiDialogProc
  mov qword ptr [rsp+20h],0
  call CreateDialogParamW
  test rax,rax
  jnz pdustActivate
  call GetLastError
  jmp pdustError
pdustActivate:
  cmp pdUiInitHook,0 ; hidden tests keep their own window invisible
  jne pdustDone
  mov rcx,pdWindow
  mov edx,9 ; SW_RESTORE also restores a previously minimized details window
  call ShowWindow
  mov rcx,pdWindow
  call SetForegroundWindow
  jmp pdustDone
pdustError:
  mov [rsp+30h],eax
  mov edx,52080
  call PdUiLoadText
  lea rcx,pdUiLine
  mov rdx,rax
  mov r8d,[rsp+30h]
  call wsprintfW
  mov rcx,gMain
  lea rdx,pdUiLine
  lea r8,szTitle
  mov r9d,10h
  call MessageBoxW
pdustDone:
  add rsp,40h
  pop rdi
  pop rsi
  pop rbx
  ret
PdUiShowTarget ENDP

PdUiSubmit PROC
  sub rsp,28h
  cmp pdUiStopped,0
  jne pdusDone
  inc qword ptr [pdUiRequest+PD_R_SEQUENCE]
  lea rcx,pdUiRequest
  call PdWorkerSubmit
  test eax,eax
  jz pdusDone
  mov edx,eax
  call PdUiShowError
pdusDone:
  add rsp,28h
  ret
PdUiSubmit ENDP

; EDX Win32. Never clears valid fields or uses the shared action result panel.
PdUiShowError PROC
  sub rsp,38h
  mov [rsp+28h],edx
  mov edx,52040
  call PdUiLoadText
  lea rcx,pdUiLine
  mov rdx,rax
  mov r8d,[rsp+28h]
  call wsprintfW
  mov rcx,pdWindow
  mov edx,4101
  lea r8,pdUiLine
  call SetDlgItemTextW
  add rsp,38h
  ret
PdUiShowError ENDP

; Append a bounded line to the metrics edit.
PdUiAppendLine PROC
  push rbx
  sub rsp,20h
  lea rcx,pdUiMetrics
  call lstrlenW
  mov ebx,eax
  lea rcx,pdUiLine
  call lstrlenW
  add eax,ebx
  cmp eax,8192
  jae pdualDone
  lea rcx,pdUiMetrics
  lea rdx,pdUiLine
  call lstrcatW
pdualDone:
  add rsp,20h
  pop rbx
  ret
PdUiAppendLine ENDP

; RCX snapshot, EDX field index, R8D label resource, R9 raw value.
PdUiMetric PROC
  push rbx
  push rsi
  sub rsp,38h
  mov rbx,rcx
  shl edx,4
  lea rsi,[rbx+rdx+PD_S_FIELDS]
  mov [rsp+28h],r9
  mov edx,r8d
  call PdUiLoadText
  lea rcx,pdUiLabel
  mov rdx,rax
  mov r8d,512
  call lstrcpynW
  mov edx,52042
  cmp dword ptr [rsi],PD_STATUS_VALID
  je pdumFormat
  mov edx,52044
  mov eax,[rsi+4]
  mov [rsp+28h],rax
pdumFormat:
  call PdUiLoadText
  lea rcx,pdUiLine
  mov rdx,rax
  lea r8,pdUiLabel
  mov r9,[rsp+28h]
  call wsprintfW
  call PdUiAppendLine
  add rsp,38h
  pop rsi
  pop rbx
  ret
PdUiMetric ENDP

; RCX SYSTEMTIME, EDX even format resource id.
PdUiTimeLine PROC
  push rbx
  sub rsp,40h
  mov rbx,rcx
  call PdUiLoadText
  mov rdx,rax
  lea rcx,pdUiLine
  movzx r8d,word ptr [rbx]
  movzx r9d,word ptr [rbx+2]
  movzx eax,word ptr [rbx+6]
  mov [rsp+20h],rax
  movzx eax,word ptr [rbx+8]
  mov [rsp+28h],rax
  movzx eax,word ptr [rbx+10]
  mov [rsp+30h],rax
  movzx eax,word ptr [rbx+12]
  mov [rsp+38h],rax
  call wsprintfW
  call PdUiAppendLine
  add rsp,40h
  pop rbx
  ret
PdUiTimeLine ENDP

; Render the accepted immutable snapshot. No external calls to target processes.
PdUiRender PROC
  push rbx
  sub rsp,30h
  mov rbx,pdUiSnapshot
  test rbx,rbx
  jz pdurDone
  mov eax,[rbx+PD_S_TARGET]
  cmp eax,PD_TARGET_CANCELLED
  jbe pdurStatus
  mov eax,PD_TARGET_UNKNOWN
pdurStatus:
  lea edx,[rax*2+52020]
  call PdUiLoadText
  mov rcx,pdWindow
  mov edx,4101
  mov r8,rax
  call SetDlgItemTextW
  ; The first failed identity check must not expose fields of a reused PID.
  cmp dword ptr [rbx+PD_S_FIELDS+PD_F_IDENTITY*16],PD_STATUS_VALID
  jne pdurDone
  cmp dword ptr [rbx+PD_S_TARGET],PD_TARGET_EXITED
  je pdurDone
  mov rcx,rbx
  mov edx,PD_F_PATH
  mov r8d,4102
  mov r9d,PD_S_PATH
  call PdUiStringField
  mov rcx,rbx
  mov edx,PD_F_SID
  mov r8d,4104
  mov r9d,PD_S_SID
  call PdUiStringField
  mov word ptr [pdUiMetrics],0
  lea rcx,[rbx+PD_S_OBSERVED_BIRTH]
  lea rdx,pdUiUtc
  call FileTimeToSystemTime
  test eax,eax
  jz pdurNumbers
  lea rcx,pdUiUtc
  mov edx,52046
  call PdUiTimeLine
  lea rcx,pdUiTimeZone
  call GetDynamicTimeZoneInformation
  cmp eax,-1
  je pdurNumbers
  lea rcx,pdUiTimeZone
  lea rdx,pdUiUtc
  lea r8,pdUiLocal
  call SystemTimeToTzSpecificLocalTimeEx
  test eax,eax
  jz pdurNumbers
  lea rcx,pdUiLocal
  mov edx,52048
  call PdUiTimeLine
  mov edx,52088
  call PdUiLoadText
  lea rcx,pdUiLine
  mov rdx,rax
  lea r8,pdUiTimeZone+172 ; Windows time-zone key, independent of DST name
  call wsprintfW
  call PdUiAppendLine
pdurNumbers:
  mov rcx,rbx
  mov edx,PD_F_HANDLES
  mov r8d,52050
  mov r9,[rbx+PD_S_HANDLES]
  call PdUiMetric
  mov rcx,rbx
  mov edx,PD_F_MEMORY
  mov r8d,52052
  mov r9,[rbx+PD_S_PRIVATE]
  call PdUiMetric
  mov rcx,rbx
  mov edx,PD_F_MEMORY
  mov r8d,52054
  mov r9,[rbx+PD_S_WORKING_SET]
  call PdUiMetric
  mov rcx,rbx
  mov edx,PD_F_CPU_TIMES
  mov r8d,52056
  mov r9,[rbx+PD_S_KERNEL]
  call PdUiMetric
  mov rcx,rbx
  mov edx,PD_F_CPU_TIMES
  mov r8d,52058
  mov r9,[rbx+PD_S_USER]
  call PdUiMetric
  mov rcx,rbx
  call PdUiCpuLine
  mov rcx,rbx
  mov edx,PD_F_IO
  mov r8d,52060
  mov r9,[rbx+PD_S_READ_OPS]
  call PdUiMetric
  mov rcx,rbx
  mov edx,PD_F_IO
  mov r8d,52062
  mov r9,[rbx+PD_S_WRITE_OPS]
  call PdUiMetric
  mov rcx,rbx
  mov edx,PD_F_IO
  mov r8d,52064
  mov r9,[rbx+PD_S_OTHER_OPS]
  call PdUiMetric
  mov rcx,rbx
  mov edx,PD_F_IO
  mov r8d,52066
  mov r9,[rbx+PD_S_READ_BYTES]
  call PdUiMetric
  mov rcx,rbx
  mov edx,PD_F_IO
  mov r8d,52068
  mov r9,[rbx+PD_S_WRITE_BYTES]
  call PdUiMetric
  mov rcx,rbx
  mov edx,PD_F_IO
  mov r8d,52070
  mov r9,[rbx+PD_S_OTHER_BYTES]
  call PdUiMetric
  mov rcx,rbx
  mov edx,PD_F_IO_RATE
  mov r8d,52074
  mov r9,[rbx+PD_S_READ_BPS]
  call PdUiMetric
  mov rcx,rbx
  mov edx,PD_F_IO_RATE
  mov r8d,52076
  mov r9,[rbx+PD_S_WRITE_BPS]
  call PdUiMetric
  mov rcx,rbx
  mov edx,PD_F_IO_RATE
  mov r8d,52078
  mov r9,[rbx+PD_S_OTHER_BPS]
  call PdUiMetric
  mov rcx,pdWindow
  mov edx,4103
  lea r8,pdUiMetrics
  call SetDlgItemTextW
pdurDone:
  add rsp,30h
  pop rbx
  ret
PdUiRender ENDP

PdUiCpuLine PROC
  push rbx
  sub rsp,30h
  mov rbx,rcx
  cmp dword ptr [rbx+PD_S_FIELDS+PD_F_CPU_RATE*16],PD_STATUS_VALID
  jne pduclUnknown
  mov edx,52072
  call PdUiLoadText
  lea rcx,pdUiLabel
  mov rdx,rax
  mov r8d,512
  call lstrcpynW
  mov edx,52082
  call PdUiLoadText
  mov rdx,rax
  mov eax,[rbx+PD_S_CPU_BASIS_POINTS]
  xor edx,edx
  mov ecx,100
  div ecx
  mov [rsp+20h],rdx
  mov r9d,eax
  lea rcx,pdUiLine
  lea rdx,pdUiText
  lea r8,pdUiLabel
  call wsprintfW
  call PdUiAppendLine
  jmp pduclDone
pduclUnknown:
  mov rcx,rbx
  mov edx,PD_F_CPU_RATE
  mov r8d,52072
  xor r9d,r9d
  call PdUiMetric
pduclDone:
  add rsp,30h
  pop rbx
  ret
PdUiCpuLine ENDP

; RCX snapshot, EDX field index, R8D control id, R9D string offset.
PdUiStringField PROC
  push rbx
  sub rsp,30h
  mov ebx,r8d
  mov eax,edx
  shl eax,4
  cmp dword ptr [rcx+rax+PD_S_FIELDS],PD_STATUS_VALID
  jne pdusfError
  lea r8,[rcx+r9]
  jmp pdusfSet
pdusfError:
  mov eax,[rcx+rax+PD_S_FIELDS+4]
  mov [rsp+28h],eax
  mov edx,52040
  call PdUiLoadText
  lea rcx,pdUiLine
  mov rdx,rax
  mov r8d,[rsp+28h]
  call wsprintfW
  lea r8,pdUiLine
pdusfSet:
  mov rcx,pdWindow
  mov edx,ebx
  call SetDlgItemTextW
  add rsp,30h
  pop rbx
  ret
PdUiStringField ENDP

; Notification carries no owning pointer; validate copied identity before apply.
PdUiAccept PROC
  push rbx
  sub rsp,20h
  call PdWorkerTake
  mov rbx,rax
  test rax,rax
  jz pduaReadFailure
  cmp pdWindow,0
  je pduaFree
  cmp pdUiStopped,0
  jne pduaFree
  cmp dword ptr [rbx+PD_S_SIZE],PD_SNAPSHOT_SIZE
  jne pduaFree
  cmp dword ptr [rbx+PD_S_VERSION],PD_VERSION
  jne pduaFree
  cmp dword ptr [rbx+PD_S_LANE],1
  jne pduaFree
  mov rax,[rbx+PD_S_COOKIE]
  cmp rax,qword ptr [pdUiRequest+PD_R_COOKIE]
  jne pduaFree
  mov rax,[rbx+PD_S_GENERATION]
  cmp rax,qword ptr [pdUiRequest+PD_R_GENERATION]
  jne pduaFree
  mov eax,[rbx+PD_S_PID]
  cmp eax,dword ptr [pdUiRequest+PD_R_PID]
  jne pduaFree
  mov rax,[rbx+PD_S_BIRTH]
  cmp rax,qword ptr [pdUiRequest+PD_R_BIRTH]
  jne pduaFree
  mov rax,[rbx+PD_S_SEQUENCE]
  cmp rax,pdUiLastSequence
  jbe pduaFree
  mov pdUiLastSequence,rax
  mov eax,[rbx+PD_S_TARGET]
  cmp eax,PD_TARGET_EXITED
  je pduaStop
  cmp eax,PD_TARGET_MISMATCH
  jne pduaReplace
pduaStop:
  mov pdUiStopped,1
  mov rcx,qword ptr [pdUiRequest+PD_R_COOKIE]
  call PdWorkerCancel
  call PdUiCancelOptional
pduaReplace:
  mov eax,[rbx+PD_S_TARGET]
  mov pdUiLastTarget,eax
  cmp eax,PD_TARGET_EXITED
  je pduaFailure
  cmp dword ptr [rbx+PD_S_FIELDS+PD_F_IDENTITY*16],PD_STATUS_VALID
  jne pduaFailure
  ; Rates are computed only from accepted samples, independent of requested seq.
  mov rcx,pdUiSnapshot
  cmp pdUiBaselineValid,0
  jne pduaBaseline
  xor ecx,ecx
pduaBaseline:
  mov rdx,rbx
  call PdDeriveRates
  mov rcx,pdUiSnapshot
  test rcx,rcx
  jz pduaStore
  call PdWorkerFree
pduaStore:
  mov pdUiSnapshot,rbx
  mov eax,[rbx+PD_S_TARGET]
  xor ecx,ecx
  cmp eax,PD_TARGET_VERIFIED
  sete cl
  mov pdUiBaselineValid,ecx
  lea rcx,pdUiGoodTime
  call GetLocalTime
  call PdUiRender
  jmp pduaDone
pduaFailure:
  mov pdUiBaselineValid,0
  mov rcx,rbx
  call PdUiRenderFailure
  jmp pduaFree
pduaFree:
  mov rcx,rbx
  call PdWorkerFree
  jmp pduaDone
pduaReadFailure:
  lea rcx,pdUiErrorRequest
  call PdWorkerTakeFailure
  test eax,eax
  jz pduaDone
  mov dword ptr [pdUiErrorHeader+PD_S_ERROR],eax
  cmp pdWindow,0
  je pduaDone
  cmp pdUiStopped,0
  jne pduaDone
  mov rax,qword ptr [pdUiErrorRequest+PD_R_COOKIE]
  cmp rax,qword ptr [pdUiRequest+PD_R_COOKIE]
  jne pduaDone
  mov rax,qword ptr [pdUiErrorRequest+PD_R_GENERATION]
  cmp rax,qword ptr [pdUiRequest+PD_R_GENERATION]
  jne pduaDone
  mov eax,dword ptr [pdUiErrorRequest+PD_R_PID]
  cmp eax,dword ptr [pdUiRequest+PD_R_PID]
  jne pduaDone
  mov rax,qword ptr [pdUiErrorRequest+PD_R_BIRTH]
  cmp rax,qword ptr [pdUiRequest+PD_R_BIRTH]
  jne pduaDone
  mov rax,qword ptr [pdUiErrorRequest+PD_R_SEQUENCE]
  cmp rax,pdUiLastSequence
  jbe pduaDone
  mov pdUiLastSequence,rax
  mov pdUiBaselineValid,0
  mov pdUiLastTarget,PD_TARGET_UNKNOWN
  mov dword ptr [pdUiErrorHeader+PD_S_TARGET],PD_TARGET_UNKNOWN
  lea rcx,pdUiErrorHeader
  call PdUiRenderFailure
pduaDone:
  add rsp,20h
  pop rbx
  ret
PdUiAccept ENDP

; Preserve clearly dated last values. A failed read never leaves a live rate.
PdUiRenderFailure PROC
  push rbx
  sub rsp,50h
  mov rbx,rcx
  mov rax,pdUiSnapshot
  test rax,rax
  jz pdurfStatus
  mov dword ptr [rax+PD_S_FIELDS+PD_F_CPU_RATE*16],PD_STATUS_UNAVAILABLE
  mov dword ptr [rax+PD_S_FIELDS+PD_F_CPU_RATE*16+4],21
  mov dword ptr [rax+PD_S_FIELDS+PD_F_IO_RATE*16],PD_STATUS_UNAVAILABLE
  mov dword ptr [rax+PD_S_FIELDS+PD_F_IO_RATE*16+4],21
  call PdUiRender
pdurfStatus:
  mov eax,[rbx+PD_S_TARGET]
  cmp eax,6
  jbe pdurfKnown
  mov eax,5
pdurfKnown:
  lea edx,[rax*2+52020]
  call PdUiLoadText
  lea rcx,pdUiFailureText
  mov rdx,rax
  mov r8d,1024
  call lstrcpynW
  mov edx,[rbx+PD_S_ERROR]
  test edx,edx
  jz pdurfDate
  mov [rsp+40h],edx
  mov edx,52086
  call PdUiLoadText
  lea rcx,pdUiLine
  mov rdx,rax
  mov r8d,[rsp+40h]
  call wsprintfW
  lea rcx,pdUiFailureText
  lea rdx,pdUiLine
  call lstrcatW
pdurfDate:
  cmp pdUiSnapshot,0
  je pdurfSet
  mov edx,52084
  call PdUiLoadText
  lea rcx,pdUiLine
  mov rdx,rax
  movzx r8d,word ptr [pdUiGoodTime]
  movzx r9d,word ptr [pdUiGoodTime+2]
  movzx eax,word ptr [pdUiGoodTime+6]
  mov [rsp+20h],rax
  movzx eax,word ptr [pdUiGoodTime+8]
  mov [rsp+28h],rax
  movzx eax,word ptr [pdUiGoodTime+10]
  mov [rsp+30h],rax
  movzx eax,word ptr [pdUiGoodTime+12]
  mov [rsp+38h],rax
  call wsprintfW
  lea rcx,pdUiFailureText
  lea rdx,pdUiLine
  call lstrcatW
pdurfSet:
  mov rcx,pdWindow
  mov edx,4101
  lea r8,pdUiFailureText
  call SetDlgItemTextW
  add rsp,50h
  pop rbx
  ret
PdUiRenderFailure ENDP

PdUiDialogProc PROC
  push rbx
  sub rsp,30h
  mov rbx,rcx
  cmp edx,110h
  je pduInit
  cmp edx,PD_READY_MESSAGE
  je pduReady
  cmp edx,PD_OPTIONAL_READY_MESSAGE
  je pduOptionalReady
  cmp edx,PD_THEME_MESSAGE
  je pduTheme
  cmp edx,111h
  je pduCommand
  cmp edx,113h
  je pduTimer
  cmp edx,5
  je pduSize
  cmp edx,2E0h
  je pduDpi
  cmp edx,24h
  je pduMin
  cmp edx,10h
  je pduClose
  cmp edx,2
  je pduDestroy
  cmp edx,133h
  je pduColor
  cmp edx,138h
  je pduColor
  cmp edx,136h
  je pduColor
  xor eax,eax
  jmp pduDone
pduInit:
  mov pdWindow,rbx
  mov qword ptr [pdUiRequest+PD_R_NOTIFY],rbx
  mov edx,52010
  call PdUiLoadText
  lea rcx,pdUiLine
  mov rdx,rax
  lea r8,pdUiName
  mov r9d,dword ptr [pdUiRequest+PD_R_PID]
  call wsprintfW
  mov rcx,rbx
  mov edx,4100
  lea r8,pdUiLine
  call SetDlgItemTextW
  mov rcx,rbx
  call GetDpiForWindow
  mov pdUiDpi,eax
  call PdUiFontAndLayout
  call PdUiApplyTheme
  mov rcx,rbx
  mov edx,4106
  call GetDlgItem
  mov rcx,rax
  mov edx,0C5h ; EM_SETLIMITTEXT, capacities exclude the terminating NUL
  mov r8d,PD_ACCOUNT_CAPACITY-1
  xor r9d,r9d
  call SendMessageW
  mov rcx,rbx
  mov edx,4107
  call GetDlgItem
  mov rcx,rax
  mov edx,0C5h
  mov r8d,PD_COMMAND_CAPACITY-1
  xor r9d,r9d
  call SendMessageW
  mov rcx,rbx
  mov edx,81h
  mov r8d,1000
  xor r9d,r9d
  call SetTimer
  call PdUiSubmit
  call PdUiRefreshOptional
  mov rax,pdUiInitHook
  test rax,rax
  jz pduHandled
  mov rcx,rbx
  call rax
  jmp pduHandled
pduReady:
  call PdUiAccept
  jmp pduHandled
pduOptionalReady:
  call PdUiAcceptOptional
  jmp pduHandled
pduTheme:
  call PdUiApplyTheme
  jmp pduHandled
pduCommand:
  and r8d,0FFFFh
  cmp r8d,2
  je pduClose
  cmp r8d,1
  jne pduIgnored
  call PdUiSubmit
  call PdUiRefreshOptional
  jmp pduHandled
pduTimer:
  cmp r8d,81h
  jne pduIgnored
  call PdUiCheckOptionalDeadline
  mov rcx,rbx
  call IsIconic
  test eax,eax
  jnz pduHandled
  mov rcx,rbx
  call IsWindowVisible
  test eax,eax
  jz pduHandled
  call PdUiSubmit
  jmp pduHandled
pduSize:
  mov [rsp+28h],r8d
  call PdUiLayout
  cmp dword ptr [rsp+28h],1
  je pduHandled
  mov rcx,rbx
  call IsWindowVisible
  test eax,eax
  jz pduHandled
  call PdUiSubmit
  jmp pduHandled
pduDpi:
  and r8d,0FFFFh
  mov pdUiDpi,r8d
  mov [rsp+28h],r9
  call PdUiFontAndLayout
  mov rax,[rsp+28h]
  mov rcx,rbx
  mov rdx,rax
  call PdUiSuggestedRect
  jmp pduHandled
pduMin:
  mov eax,pdUiDpi
  imul eax,640
  xor edx,edx
  mov ecx,96
  div ecx
  mov [r9+24],eax
  mov eax,pdUiDpi
  imul eax,730
  xor edx,edx
  div ecx
  mov [r9+28],eax
  jmp pduHandled
pduColor:
  mov [rsp+28h],r8
  mov rcx,r8
  mov edx,shellTextColor
  call SetTextColor
  mov rcx,[rsp+28h]
  mov edx,0FFFFFFh
  cmp themeOption,0
  je pduBk
  mov edx,202020h
pduBk:
  call SetBkColor
  mov rax,shellCardBrush
  test rax,rax
  jnz pduDone
  mov ecx,15
  call GetSysColorBrush
  jmp pduDone
pduClose:
  mov rcx,rbx
  call DestroyWindow
  jmp pduHandled
pduDestroy:
  mov rcx,rbx
  mov edx,81h
  call KillTimer
  mov rcx,qword ptr [pdUiRequest+PD_R_COOKIE]
  call PdWorkerCancel
  call PdUiCancelOptional
  mov pdWindow,0
  mov rcx,pdUiSnapshot
  mov pdUiSnapshot,0
  test rcx,rcx
  jz pduFontFree
  call PdWorkerFree
pduFontFree:
  mov rcx,pdWindowFont
  mov pdWindowFont,0
  test rcx,rcx
  jz pduHandled
  call DeleteObject
pduHandled:
  mov eax,1
  jmp pduDone
pduIgnored:
  xor eax,eax
pduDone:
  add rsp,30h
  pop rbx
  ret
PdUiDialogProc ENDP

PdUiSuggestedRect PROC
  sub rsp,38h
  mov rax,rdx
  xor edx,edx
  mov r8d,[rax]
  mov r9d,[rax+4]
  mov r10d,[rax+8]
  sub r10d,r8d
  mov [rsp+20h],r10
  mov r10d,[rax+12]
  sub r10d,r9d
  mov [rsp+28h],r10
  mov qword ptr [rsp+30h],14h
  call SetWindowPos
  add rsp,38h
  ret
PdUiSuggestedRect ENDP

PdUiApplyTheme PROC
  sub rsp,28h
  cmp pdWindow,0
  je pduatDone
  mov rcx,pdWindow
  mov edx,20
  lea r8,themeOption
  mov r9d,4
  call DwmSetWindowAttribute
  mov rcx,pdWindow
  xor edx,edx
  xor r8d,r8d
  mov r9d,85h
  call RedrawWindow
pduatDone:
  add rsp,28h
  ret
PdUiApplyTheme ENDP

PdUiFontAndLayout PROC
  push rbx
  push rsi
  sub rsp,78h
  mov rbx,pdWindowFont
  mov eax,pdUiDpi
  imul eax,16
  xor edx,edx
  mov ecx,96
  div ecx
  neg eax
  mov ecx,eax
  xor edx,edx
  xor r8d,r8d
  xor r9d,r9d
  mov qword ptr [rsp+20h],400
  mov qword ptr [rsp+28h],0
  mov qword ptr [rsp+30h],0
  mov qword ptr [rsp+38h],0
  mov qword ptr [rsp+40h],1
  mov qword ptr [rsp+48h],0
  mov qword ptr [rsp+50h],0
  mov qword ptr [rsp+58h],5
  mov qword ptr [rsp+60h],0
  lea rax,fontFace
  mov [rsp+68h],rax
  call CreateFontW
  test rax,rax
  jz pduflLayout
  mov pdWindowFont,rax
  xor esi,esi
pduflNext:
  mov rcx,pdWindow
  lea rax,pdUiControlIds
  mov edx,[rax+rsi*4]
  call GetDlgItem
  mov rcx,rax
  mov edx,30h
  mov r8,pdWindowFont
  mov r9d,1
  call SendMessageW
  inc esi
  cmp esi,LENGTHOF pdUiControlIds
  jb pduflNext
  test rbx,rbx
  jz pduflLayout
  mov rcx,rbx
  call DeleteObject
pduflLayout:
  call PdUiLayout
  add rsp,78h
  pop rsi
  pop rbx
  ret
PdUiFontAndLayout ENDP

PdUiLayout PROC
  push rbx
  push rsi
  push rdi
  sub rsp,60h
  cmp pdWindow,0
  je pdulDone
  mov rcx,pdWindow
  lea rdx,[rsp+40h]
  call GetClientRect
  mov eax,pdUiDpi
  cmp eax,48
  jb pdulDone
  mov edi,eax
  imul eax,678
  xor edx,edx
  mov ecx,96
  div ecx
  mov ecx,[rsp+4Ch]
  sub ecx,eax
  jns pdulGrow
  xor ecx,ecx
pdulGrow:
  mov [rsp+50h],ecx
  lea rsi,pdUiPositions
  mov ebx,LENGTHOF pdUiControlIds
pdulNext:
  mov rcx,pdWindow
  mov edx,[rsi]
  call GetDlgItem
  mov [rsp+38h],rax
  xor r10d,r10d
pdulScale:
  mov eax,[rsi+r10*4+4]
  imul eax,edi
  cdq
  mov ecx,96
  idiv ecx
  mov [rsp+20h+r10*4],eax
  inc r10d
  cmp r10d,4
  jb pdulScale
  ; Stretch all wide fields to the right client edge.
  cmp dword ptr [rsi],2
  je pdulButton
  cmp dword ptr [rsi],1
  je pdulButton
  mov eax,[rsp+48h]
  mov ecx,[rsp+20h]
  sub eax,ecx
  sub eax,ecx
  mov [rsp+28h],eax
pdulButton:
  cmp dword ptr [rsi],4103
  jne pdulBottom
  mov eax,[rsp+50h]
  add [rsp+2Ch],eax
pdulBottom:
  cmp dword ptr [rsi],4112
  je pdulAnchor
  cmp dword ptr [rsi],4104
  je pdulAnchor
  cmp dword ptr [rsi],4105
  je pdulAnchor
  cmp dword ptr [rsi],4113
  je pdulAnchor
  cmp dword ptr [rsi],4106
  je pdulAnchor
  cmp dword ptr [rsi],4114
  je pdulAnchor
  cmp dword ptr [rsi],4107
  je pdulAnchor
  cmp dword ptr [rsi],1
  je pdulAnchor
  cmp dword ptr [rsi],2
  jne pdulMove
pdulAnchor:
  mov eax,[rsp+50h]
  add [rsp+24h],eax
  cmp dword ptr [rsi],1
  je pdulRightFirst
  cmp dword ptr [rsi],2
  jne pdulMove
  mov ecx,102
  jmp pdulRightScale
pdulRightFirst:
  mov ecx,204
pdulRightScale:
  mov eax,edi
  imul eax,ecx
  xor edx,edx
  mov ecx,96
  div ecx
  mov ecx,[rsp+48h]
  sub ecx,eax
  mov [rsp+20h],ecx
pdulMove:
  mov rcx,[rsp+38h]
  mov edx,[rsp+20h]
  mov r8d,[rsp+24h]
  mov r9d,[rsp+28h]
  mov eax,[rsp+2Ch]
  mov [rsp+20h],rax
  mov qword ptr [rsp+28h],1
  call MoveWindow
  add rsi,20
  dec ebx
  jnz pdulNext
pdulDone:
  add rsp,60h
  pop rdi
  pop rsi
  pop rbx
  ret
PdUiLayout ENDP
