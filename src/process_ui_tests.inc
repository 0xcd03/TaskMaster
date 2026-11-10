; Regression fixtures exercise the public presentation contract independently
; of the UI. No real processes, devices, or user settings are modified here.
extern lstrcmpW:proc
.data
presentationName dw 'f','i','x','t','u','r','e','.','e','x','e',0
presentationPid dw '9','0','1','2','3',0
presentationThreads dw '1','2',0
presentationCpu dw '1','.','5','%',0
presentationCpuZero dw '0','.','0','%',0
presentationCpuRu dw '1',',','5','%',0
presentationMemory dw '5','1','2','.','5',0
presentationIo dw '8','0','.','0',0
presentationRate dw '9','6','.','0',0
presentationMaximum dw 2265h,' ','4','2','9','4','9','6','7','2','9','5','.','9','%',0
presentationMaximumRu dw 2265h,' ','4','2','9','4','9','6','7','2','9','5',',','9','%',0
presentationCpuHalf dw '5','0','.','0','%',0
presentationCpuFull dw '1','0','0','.','0','%',0
presentationTreeMemory dw '1','1','0','2',0
presentationTreeCpu dw '3','1','.','1','%',0
presentationTreeIo dw '1','1','0','2','.','0',0
; PID, PPID, birth, own MiB, own CPU tenths, own KiB/s.
; Root A and child X consume less themselves than their competing siblings,
; while the X grandchild reverses both displayed aggregate orderings.
presentationTreeData dq 100,0,1,1,10,1
                    dq 101,100,2,1,1,1
                    dq 102,101,3,1000,200,1000
                    dq 103,100,4,100,100,100
                    dq 200,0,5,500,150,500
                    dq 201,200,6,1,1,1
                    dq 300,0,7,50,300,2000
presentationTreeMemoryDesc dd 100,101,102,103,200,201,300
presentationTreeMemoryAsc dd 300,200,201,100,103,101,102
presentationTreeCpuDesc dd 100,101,102,103,300,200,201
presentationTreeIoDesc dd 300,100,101,102,103,200,201
.data?
align 8
presentationRows db 8208 dup(?)
presentationDraw db 160 dup(?)
presentationMinMax db 48 dup(?)
presentationSaved db 80 dup(?)
presentationTreeRows db 28672 dup(?)
.code
ProcessPresentationTests PROC
  push rbx
  push rsi
  push rdi
  sub rsp,30h
  lea rsi,presentationSaved
  mov eax,appLanguage
  mov [rsi],eax
  mov eax,treeEnabled
  mov [rsi+4],eax
  mov eax,frontMode
  mov [rsi+8],eax
  mov eax,themeOption
  mov [rsi+12],eax
  mov eax,sortColumn
  mov [rsi+16],eax
  mov eax,sortDescending
  mov [rsi+20],eax
  mov eax,visibleCount
  mov [rsi+24],eax
  mov eax,historyCount
  mov [rsi+28],eax
  mov eax,historyDirty
  mov [rsi+32],eax
  mov rax,rowMap
  mov [rsi+40],rax
  mov rax,historyNow
  mov [rsi+48],rax
  mov rax,sysSampleTotal
  mov [rsi+56],rax
  mov rax,sysDelta
  mov [rsi+64],rax
  mov rax,sysPrev
  mov [rsi+72],rax
  lea rbx,presentationRows
  mov rcx,rbx
  call ClearRow
  lea rcx,[rbx+4096]
  call ClearRow
  mov appLanguage,0
  mov treeEnabled,0
  mov frontMode,0
  mov sortDescending,0
  mov modelStage,1401
  mov rcx,rbx
  lea rdx,presentationName
  call lstrcpyW
  lea rcx,[rbx+512]
  lea rdx,presentationPid
  call lstrcpyW
  lea rcx,[rbx+1536]
  lea rdx,presentationThreads
  call lstrcpyW
  lea rcx,[rbx+3072]
  lea rdx,presentationCpu
  call lstrcpyW
  lea rcx,[rbx+2560]
  lea rdx,presentationMemory
  call lstrcpyW
  lea rcx,[rbx+3912]
  lea rdx,presentationIo
  call lstrcpyW
  mov dword ptr [rbx+3672],1
  mov dword ptr [rbx+3896],1
  mov dword ptr [rbx+3656],1
  mov dword ptr [rbx+3584],90123
  mov dword ptr [rbx+3900],15
  mov qword ptr [rbx+3632],537395200
  mov qword ptr [rbx+3888],81920
  mov rcx,rbx
  xor edx,edx
  call ProcessDisplayText
  mov rcx,rax
  lea rdx,presentationName
  call lstrcmpW
  test eax,eax
  jnz pptFail
  mov rcx,rbx
  mov edx,2
  call ProcessDisplayText
  mov rcx,rax
  lea rdx,presentationCpu
  call lstrcmpW
  test eax,eax
  jnz pptFail
  mov rcx,rbx
  mov edx,3
  call ProcessDisplayText
  mov rcx,rax
  lea rdx,presentationMemory
  call lstrcmpW
  test eax,eax
  jnz pptFail
  mov rcx,rbx
  mov edx,4
  call ProcessDisplayText
  mov rcx,rax
  lea rdx,presentationIo
  call lstrcmpW
  test eax,eax
  jnz pptFail
  mov rcx,rbx
  mov edx,5
  call ProcessDisplayText
  mov rcx,rax
  lea rdx,presentationThreads
  call lstrcmpW
  test eax,eax
  jnz pptFail
  mov rcx,rbx
  mov edx,6
  call ProcessDisplayText
  mov rcx,rax
  lea rdx,presentationPid
  call lstrcmpW
  test eax,eax
  jnz pptFail
  mov dword ptr [rbx+3884],1
  mov rcx,rbx
  mov edx,1
  call ProcessDisplayText
  lea rdx,processPausedText
  cmp rax,rdx
  jne pptFail
  mov dword ptr [rbx+3884],2
  mov rcx,rbx
  mov edx,1
  call ProcessDisplayText
  lea rdx,processDeniedText
  cmp rax,rdx
  jne pptFail
  mov modelStage,1402
  mov word ptr [rbx+1536],'2'
  mov word ptr [rbx+1538],0
  mov dword ptr [rbx+4096+1536],00300031h ; "10"
  mov word ptr [rbx+4096+1540],0
  mov sortColumn,5
  mov rcx,rbx
  lea rdx,[rbx+4096]
  call CompareRows
  test eax,eax
  jge pptFail
  ; The 8 GiB value must not wrap to zero through a 32-bit comparison.
  mov rax,8589934592
  mov [rbx+3632],rax
  mov qword ptr [rbx+4096+3632],1073741824
  mov sortColumn,3
  mov rcx,rbx
  lea rdx,[rbx+4096]
  call CompareRows
  test eax,eax
  jle pptFail
  mov sortDescending,1
  mov rcx,rbx
  lea rdx,[rbx+4096]
  call CompareRows
  test eax,eax
  jge pptFail
  mov sortDescending,0
  mov sortColumn,2
  mov dword ptr [rbx+4096+3900],100
  mov rcx,rbx
  lea rdx,[rbx+4096]
  call CompareRows
  test eax,eax
  jge pptFail
  mov sortColumn,4
  mov qword ptr [rbx+4096+3888],10485760
  mov rcx,rbx
  lea rdx,[rbx+4096]
  call CompareRows
  test eax,eax
  jge pptFail
  mov modelStage,1403
  mov rcx,rbx
  call ClearRow
  mov dword ptr [rbx+3584],999991
  mov qword ptr [rbx+3600],777771
  mov qword ptr [rbx+3608],100
  mov dword ptr [rbx+3660],1
  mov qword ptr [rbx+3640],65536
  mov historyNow,100000000
  mov rcx,rbx
  call RecordHistory
  cmp dword ptr [rbx+3896],0
  jne pptFail
  ; Read +128 KiB and write +64 KiB during 2 seconds => 96 KiB/s.
  mov qword ptr [rbx+3640],196608
  mov qword ptr [rbx+3648],65536
  mov qword ptr [rbx+3608],200
  mov historyNow,120000000
  mov rcx,rbx
  call RecordHistory
  cmp dword ptr [rbx+3896],1
  jne pptFail
  cmp qword ptr [rbx+3888],98304
  jne pptFail
  mov rcx,[rbx+3888]
  lea rdx,[rbx+3912]
  mov r8d,1024
  call StoreScaledMetric
  lea rcx,[rbx+3912]
  lea rdx,presentationRate
  call lstrcmpW
  test eax,eax
  jnz pptFail
  mov dword ptr [rbx+3896],0
  mov rcx,rbx
  call RecordHistory ; identical timestamps cannot produce a valid rate
  cmp dword ptr [rbx+3896],0
  jne pptFail
  mov qword ptr [rbx+3600],777772 ; PID reused: counters belong to a new identity
  mov historyNow,140000000
  mov rcx,rbx
  call RecordHistory
  cmp dword ptr [rbx+3896],0
  jne pptFail
  mov qword ptr [rbx+3640],0 ; a decreasing counter cannot underflow into a rate
  mov historyNow,160000000
  mov rcx,rbx
  call RecordHistory
  cmp dword ptr [rbx+3896],0
  jne pptFail
  mov modelStage,1404
  ; Synthetic x64 NMLVCUSTOMDRAW with canaries in the HDC, RECT and LPARAM.
  ; A 32-bit layout accidentally used here corrupts one of these fields.
  lea rdi,presentationDraw
  mov rax,0A5A5A5A5A5A5A5A5h
  mov ecx,20
  rep stosq
  mov visibleCount,1
  mov rowMap,rbx
  mov themeOption,1
  mov dword ptr [rbx+3672],1
  mov dword ptr [rbx+3900],15
  lea r9,presentationDraw
  mov dword ptr [r9+24],1
  call ProcessListCustomDraw
  cmp eax,20h
  jne pptFail
  lea r9,presentationDraw
  mov dword ptr [r9+24],30001h
  mov qword ptr [r9+56],0
  mov dword ptr [r9+64],0
  mov dword ptr [r9+88],2
  call ProcessListCustomDraw
  test eax,eax
  jnz pptFail
  lea r9,presentationDraw
  cmp dword ptr [r9+80],0EEEEEEh
  jne pptFail
  mov eax,processHeatDark
  cmp [r9+84],eax
  jne pptFail
  mov rax,0A5A5A5A5A5A5A5A5h
  cmp [r9+32],rax
  jne pptFail
  cmp [r9+40],rax
  jne pptFail
  cmp [r9+48],rax
  jne pptFail
  cmp [r9+72],rax
  jne pptFail
  cmp [r9+96],rax
  jne pptFail
  cmp [r9+152],rax
  jne pptFail
  mov dword ptr [r9+64],1 ; native selection colors are left untouched
  mov dword ptr [r9+80],123456h
  mov dword ptr [r9+84],654321h
  call ProcessListCustomDraw
  lea r9,presentationDraw
  cmp dword ptr [r9+80],123456h
  jne pptFail
  cmp dword ptr [r9+84],654321h
  jne pptFail
  mov modelStage,1405
  mov rax,0C3C3C3C3C3C3C3C3h
  mov [rbx+4096],rax
  mov rax,42949672959 ; maximum unsigned %u digits, decimal, lower bound and %
  mov [rbx+4040],rax
  mov qword ptr [rbx+3736],8192
  mov dword ptr [rbx+4056],0
  mov dword ptr [rbx+4060],0AABBCCDDh
  mov rcx,rbx
  call FormatTreeProcessUi
  mov rax,0C3C3C3C3C3C3C3C3h
  cmp [rbx+4096],rax
  jne pptFail
  cmp dword ptr [rbx+4060],0AABBCCDDh
  jne pptFail
  lea rcx,[rbx+4064]
  lea rdx,presentationMaximum
  call lstrcmpW
  test eax,eax
  jnz pptFail
  mov appLanguage,1
  mov rcx,rbx
  call FormatTreeProcessUi
  lea rcx,[rbx+4064]
  lea rdx,presentationMaximumRu
  call lstrcmpW
  test eax,eax
  jnz pptFail
  mov rax,0C3C3C3C3C3C3C3C3h
  cmp [rbx+4096],rax
  jne pptFail
  mov rcx,rbx
  mov edx,15
  call FormatProcessCpu
  lea rcx,[rbx+3072]
  lea rdx,presentationCpuRu
  call lstrcmpW
  test eax,eax
  jnz pptFail
  cmp dword ptr [rbx+3900],15 ; display suffix must not change numeric sorting
  jne pptFail
  mov appLanguage,0
  mov rcx,rbx
  xor edx,edx
  call FormatProcessCpu
  lea rcx,[rbx+3072]
  lea rdx,presentationCpuZero
  call lstrcmpW
  test eax,eax
  jnz pptFail
  mov rax,0B6B6B6B6B6B6B6B6h
  mov [rbx+3880],rax ; following the 64-byte WS aggregate text field
  lea rcx,[rbx+3816]
  lea rdx,treePartialFmt
  mov r8,-1
  call wsprintfW
  mov rax,0B6B6B6B6B6B6B6B6h
  cmp [rbx+3880],rax
  jne pptFail
  mov modelStage,1406
  ; WM_GETMINMAXINFO must set only ptMinTrackSize. A +32/+36 mistake
  ; writes ptMaxTrackSize and pins maximize to a small rectangle.
  lea rdi,presentationMinMax
  mov rax,0A5A5A5A5A5A5A5A5h
  mov ecx,6
  rep stosq
  mov dword ptr [presentationMinMax+8],1920
  mov dword ptr [presentationMinMax+12],1080
  mov dword ptr [presentationMinMax+16],-1920
  mov dword ptr [presentationMinMax+20],0
  mov dword ptr [presentationMinMax+32],3840
  mov dword ptr [presentationMinMax+36],2160
  xor ecx,ecx
  mov edx,24h
  xor r8d,r8d
  lea r9,presentationMinMax
  call SimpleWndProc
  test eax,eax
  jnz pptFail
  mov eax,uiMinWidth
  cmp dword ptr [presentationMinMax+24],eax
  jne pptFail
  mov eax,uiMinHeight
  cmp dword ptr [presentationMinMax+28],eax
  jne pptFail
  cmp dword ptr [presentationMinMax+8],1920
  jne pptFail
  cmp dword ptr [presentationMinMax+12],1080
  jne pptFail
  cmp dword ptr [presentationMinMax+16],-1920
  jne pptFail
  cmp dword ptr [presentationMinMax+20],0
  jne pptFail
  cmp dword ptr [presentationMinMax+32],3840
  jne pptFail
  cmp dword ptr [presentationMinMax+36],2160
  jne pptFail
  mov rax,0A5A5A5A5A5A5A5A5h
  cmp qword ptr [presentationMinMax],rax
  jne pptFail
  cmp qword ptr [presentationMinMax+40],rax
  jne pptFail
  mov modelStage,1407
  mov rcx,rbx
  call ClearRow
  mov dword ptr [rbx+3584],999993
  mov qword ptr [rbx+3600],777773
  mov qword ptr [rbx+3608],100
  mov sysSampleTotal,1000
  mov historyNow,200000000
  mov rcx,rbx
  call RecordHistory
  cmp dword ptr [rbx+3672],0
  jne pptFail
  ; Two cancelled snapshots advance the global total without seeing this PID.
  ; Its next +150 CPU units span +300 system units => 50%, never 150%.
  mov sysSampleTotal,1100
  mov sysDelta,100
  mov sysSampleTotal,1200
  mov sysDelta,100
  mov sysSampleTotal,1300
  mov qword ptr [rbx+3608],250
  mov historyNow,230000000
  mov rcx,rbx
  call RecordHistory
  cmp qword ptr [rbx+4000],300
  jne pptFail
  cmp qword ptr [rbx+3664],150
  jne pptFail
  mov rcx,rbx
  call CalculateProcessCpu
  cmp dword ptr [rbx+3672],1
  jne pptFail
  cmp dword ptr [rbx+3900],500
  jne pptFail
  lea rcx,[rbx+3072]
  lea rdx,presentationCpuHalf
  call lstrcmpW
  test eax,eax
  jnz pptFail
  ; Same system total and rollback cannot validate a sample.
  mov qword ptr [rbx+3608],260
  mov historyNow,240000000
  mov rcx,rbx
  call RecordHistory
  cmp dword ptr [rbx+3672],0
  jne pptFail
  cmp qword ptr [rbx+4000],0
  jne pptFail
  mov sysSampleTotal,1200
  mov qword ptr [rbx+3608],270
  mov historyNow,250000000
  mov rcx,rbx
  call RecordHistory
  cmp dword ptr [rbx+3672],0
  jne pptFail
  mov sysSampleTotal,0 ; failed GetSystemTimes clears the runtime baseline
  mov qword ptr [rbx+3608],280
  mov historyNow,260000000
  mov rcx,rbx
  call RecordHistory
  cmp dword ptr [rbx+3672],0
  jne pptFail
  mov sysSampleTotal,1400
  mov qword ptr [rbx+3608],290
  mov historyNow,270000000
  mov rcx,rbx
  call RecordHistory
  cmp dword ptr [rbx+3672],0
  jne pptFail
  mov sysSampleTotal,1500
  mov qword ptr [rbx+3608],390
  mov historyNow,280000000
  mov rcx,rbx
  call RecordHistory
  mov rcx,rbx
  call CalculateProcessCpu
  cmp dword ptr [rbx+3672],1
  jne pptFail
  cmp dword ptr [rbx+3900],1000
  jne pptFail
  lea rcx,[rbx+3072]
  lea rdx,presentationCpuFull
  call lstrcmpW
  test eax,eax
  jnz pptFail
  mov sysSampleTotal,1600
  mov qword ptr [rbx+3608],591 ; +201 process against +100 system is impossible
  mov historyNow,290000000
  mov rcx,rbx
  call RecordHistory
  mov rcx,rbx
  call CalculateProcessCpu
  cmp dword ptr [rbx+3672],0
  jne pptFail
  cmp dword ptr [rbx+3900],0
  jne pptFail
  ; A monotonic unsigned 64-bit total above INT64_MAX must remain valid.
  mov rax,8000000000000000h
  mov sysSampleTotal,rax
  mov qword ptr [rbx+3608],601
  mov historyNow,300000000
  mov rcx,rbx
  call RecordHistory
  mov rax,8000000000000064h
  mov sysSampleTotal,rax
  mov qword ptr [rbx+3608],701
  mov historyNow,310000000
  mov rcx,rbx
  call RecordHistory
  mov rcx,rbx
  call CalculateProcessCpu
  cmp dword ptr [rbx+3672],1
  jne pptFail
  cmp dword ptr [rbx+3900],1000
  jne pptFail
  mov modelStage,1408
  call ProcessTreePresentationTests
  test eax,eax
  jnz pptFail
  xor edi,edi
  jmp pptRestore
pptFail:
  mov edi,1
pptRestore:
  mov eax,[rsi]
  mov appLanguage,eax
  mov eax,[rsi+4]
  mov treeEnabled,eax
  mov eax,[rsi+8]
  mov frontMode,eax
  mov eax,[rsi+12]
  mov themeOption,eax
  mov eax,[rsi+16]
  mov sortColumn,eax
  mov eax,[rsi+20]
  mov sortDescending,eax
  mov eax,[rsi+24]
  mov visibleCount,eax
  mov eax,[rsi+28]
  mov historyCount,eax
  mov eax,[rsi+32]
  mov historyDirty,eax
  mov rax,[rsi+40]
  mov rowMap,rax
  mov rax,[rsi+48]
  mov historyNow,rax
  mov rax,[rsi+56]
  mov sysSampleTotal,rax
  mov rax,[rsi+64]
  mov sysDelta,rax
  mov rax,[rsi+72]
  mov sysPrev,rax
  mov eax,edi
  add rsp,30h
  pop rdi
  pop rsi
  pop rbx
  ret
ProcessPresentationTests ENDP

; Check the public, displayed tree ordering rather than a comparator in
; isolation. The fixture distinguishes own values from aggregate values at
; both the root and child levels, then reverses the requested direction.
ProcessTreePresentationTests PROC
  push rbx
  push rsi
  push rdi
  push r12
  push r13
  sub rsp,0A0h
  mov rax,frontBuffer
  mov [rsp+20h],rax
  mov eax,frontCount
  mov [rsp+28h],eax
  mov eax,visibleCount
  mov [rsp+2Ch],eax
  mov eax,sortColumn
  mov [rsp+30h],eax
  mov eax,sortDescending
  mov [rsp+34h],eax
  mov eax,treeEnabled
  mov [rsp+38h],eax
  mov eax,frontMode
  mov [rsp+3Ch],eax
  mov eax,collapsedUsed
  mov [rsp+40h],eax
  mov ax,word ptr searchBuffer
  mov [rsp+44h],ax
  mov rax,treeRoot
  mov [rsp+48h],rax
  mov eax,treeCount
  mov [rsp+50h],eax
  lea rsi,rowMap
  lea rdi,[rsp+58h]
  mov ecx,7
  rep movsq
  lea rbx,presentationTreeRows
  lea r12,presentationTreeData
  xor esi,esi
ptptFixture:
  mov rcx,rbx
  call ClearRow
  mov rcx,rbx
  lea rdx,presentationName
  call lstrcpyW
  mov eax,[r12]
  mov [rbx+3584],eax
  mov eax,[r12+8]
  mov [rbx+3588],eax
  mov rax,[r12+16]
  mov [rbx+3600],rax
  mov rax,[r12+24]
  shl rax,20
  mov [rbx+3624],rax
  mov [rbx+3632],rax
  mov eax,[r12+32]
  mov [rbx+3900],eax
  mov rax,[r12+40]
  shl rax,10
  mov [rbx+3888],rax
  mov dword ptr [rbx+3656],1
  mov dword ptr [rbx+3672],1
  mov dword ptr [rbx+3896],1
  add rbx,4096
  add r12,48
  inc esi
  cmp esi,7
  jb ptptFixture
  lea rbx,presentationTreeRows
  mov frontBuffer,rbx
  mov frontCount,7
  mov treeEnabled,1
  mov frontMode,0
  mov collapsedUsed,0
  mov word ptr searchBuffer,0
  mov sortColumn,3
  mov sortDescending,1
  call ModelMap
  call BuildProcessTree
  lea rcx,presentationTreeMemoryDesc
  call CheckTreePresentationOrder
  test eax,eax
  jnz ptptFail
  ; Tree labels are accessible plain names. Indentation and disclosure boxes
  ; must never leak font-dependent line/arrow characters into the text.
  mov rax,[rbx+3744]
  cmp word ptr [rax],'f'
  jne ptptFail
  mov rax,[rbx+4096+3744]
  cmp word ptr [rax],'f'
  jne ptptFail
  ; The root's displayed 1102 MiB must drive its position, not its own 1 MiB.
  mov rcx,rbx
  mov edx,3
  call ProcessDisplayText
  mov rcx,rax
  lea rdx,presentationTreeMemory
  call lstrcmpW
  test eax,eax
  jnz ptptFail
  mov sortDescending,0
  call ModelMap
  call BuildProcessTree
  lea rcx,presentationTreeMemoryAsc
  call CheckTreePresentationOrder
  test eax,eax
  jnz ptptFail
  mov sortColumn,2
  mov sortDescending,1
  call ModelMap
  call BuildProcessTree
  lea rcx,presentationTreeCpuDesc
  call CheckTreePresentationOrder
  test eax,eax
  jnz ptptFail
  mov rcx,rbx
  mov edx,2
  call ProcessDisplayText
  mov rcx,rax
  lea rdx,presentationTreeCpu
  call lstrcmpW
  test eax,eax
  jnz ptptFail
  mov sortColumn,4
  call ModelMap
  call BuildProcessTree
  lea rcx,presentationTreeIoDesc
  call CheckTreePresentationOrder
  test eax,eax
  jnz ptptFail
  mov rcx,rbx
  mov edx,4
  call ProcessDisplayText
  mov rcx,rax
  lea rdx,presentationTreeIo
  call lstrcmpW
  test eax,eax
  jnz ptptFail
  xor r13d,r13d
  jmp ptptRestore
ptptFail:
  mov r13d,1
ptptRestore:
  mov rax,[rsp+20h]
  mov frontBuffer,rax
  mov eax,[rsp+28h]
  mov frontCount,eax
  mov eax,[rsp+2Ch]
  mov visibleCount,eax
  mov eax,[rsp+30h]
  mov sortColumn,eax
  mov eax,[rsp+34h]
  mov sortDescending,eax
  mov eax,[rsp+38h]
  mov treeEnabled,eax
  mov eax,[rsp+3Ch]
  mov frontMode,eax
  mov eax,[rsp+40h]
  mov collapsedUsed,eax
  mov ax,[rsp+44h]
  mov word ptr searchBuffer,ax
  mov rax,[rsp+48h]
  mov treeRoot,rax
  mov eax,[rsp+50h]
  mov treeCount,eax
  lea rsi,[rsp+58h]
  lea rdi,rowMap
  mov ecx,7
  rep movsq
  mov eax,r13d
  add rsp,0A0h
  pop r13
  pop r12
  pop rdi
  pop rsi
  pop rbx
  ret
ProcessTreePresentationTests ENDP

CheckTreePresentationOrder PROC
  cmp visibleCount,7
  jne ctpoFail
  lea r8,rowMap
  xor edx,edx
ctpoNext:
  mov rax,[r8+rdx*8]
  mov eax,[rax+3584]
  cmp eax,[rcx+rdx*4]
  jne ctpoFail
  inc edx
  cmp edx,7
  jb ctpoNext
  xor eax,eax
  ret
ctpoFail:
  mov eax,1
  ret
CheckTreePresentationOrder ENDP

include tree_control_tests.inc
