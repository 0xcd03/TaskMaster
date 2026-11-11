; Hidden common-control fixture. Never loads or writes production preferences.
.data
align 2
pcntControls dd 8,1
pcntStatic dw 'S','T','A','T','I','C',0
pcntListClass dw 'S','y','s','L','i','s','t','V','i','e','w','3','2',0
pcntUtcExpected dw '1','6','0','1','-','0','1','-','0','1',' ','0','0',':','0','0',':','0','1',' ','U','T','C',0
pcntCounterExpected dw '4','2','9','4','9','6','7','2','9','8',0
align 8
pcntGpuExpected dq pcntGpu0,pcntGpu1,pcntGpu2,pcntGpu3,pcntGpu4,pcntGpu5,pcntGpu6,pcntGpu7
pcntGpu0 dw 71,80,85,32,40,37,41,0
pcntGpu1 dw 1043,1055,32,40,37,41,0
pcntGpu2 dw 71,80,85,32,101,110,103,105,110,101,0
pcntGpu3 dw 1071,1076,1088,1086,32,1043,1055,0
pcntGpu4 dw 68,101,100,105,99,97,116,101,100,32,71,80,85,32,109,101,109,111,114,121,32,40,77,105,66,41,0
pcntGpu5 dw 1042,1099,1076,1077,1083,1077,1085,1085,1072,1103,32,1087,1072,1084,1103,1090,1100,32,1043,1055,32,40,1052,1080,1041,41,0
pcntGpu6 dw 83,104,97,114,101,100,32,71,80,85,32,109,101,109,111,114,121,32,40,77,105,66,41,0
pcntGpu7 dw 1054,1073,1097,1072,1103,32,1087,1072,1084,1103,1090,1100,32,1043,1055,32,40,1052,1080,1041,41,0
.data?
align 8
pcntLayout db PC_LAYOUT_SIZE dup(?)
pcntOrder dd PC_COLUMN_COUNT dup(?)
pcntRows db 8192 dup(?)
pcntNotify db 40 dup(?)
pcntHeaderItem db 72 dup(?)
pcntState db 88 dup(?)
pcntGpuColumn db 56 dup(?)
pcntGpuText dw 256 dup(?)
.code
; Equal-metric identity order is stable in both directions and antisymmetric.
; RBX points to two adjacent fixture rows, already prepared by the caller.
PcntCheckIdentityOrder PROC
  push rsi
  sub rsp,20h
  xor esi,esi
pcntTieDirection:
  mov sortDescending,esi
  mov rcx,rbx
  lea rdx,[rbx+4096]
  call CompareRows
  test eax,eax
  jns pcntTieFail
  lea rcx,[rbx+4096]
  mov rdx,rbx
  call CompareRows
  test eax,eax
  jle pcntTieFail
  inc esi
  cmp esi,2
  jb pcntTieDirection
  mov sortDescending,0
  xor eax,eax
  jmp pcntTieDone
pcntTieFail:
  mov eax,1
pcntTieDone:
  add rsp,20h
  pop rsi
  ret
PcntCheckIdentityOrder ENDP

; Verify actual common-control text and alignment for all GPU fields in EN/RU.
PcntGpuHeaders PROC
  push rbx
  push rsi
  sub rsp,28h
  mov appLanguage,0
pcntGpuLanguage:
  call SetModeColumns
  mov ebx,PC_GPU
pcntGpuHeader:
  mov dword ptr [pcntGpuColumn],5 ; LVCF_FMT | LVCF_TEXT
  lea rax,pcntGpuText
  mov qword ptr [pcntGpuColumn+16],rax
  mov dword ptr [pcntGpuColumn+24],256
  mov rcx,gList
  mov edx,105Fh ; LVM_GETCOLUMNW
  mov r8d,ebx
  lea r9,pcntGpuColumn
  call SendMessageW
  test eax,eax
  jz pcntGpuBad
  mov eax,dword ptr [pcntGpuColumn+4]
  and eax,3
  xor edx,edx
  cmp ebx,PC_GPU_ENGINE
  je pcntGpuFmt
  inc edx
pcntGpuFmt:
  cmp eax,edx
  jne pcntGpuBad
  lea esi,[rbx-PC_GPU]
  shl esi,1
  add esi,appLanguage
  lea rax,pcntGpuExpected
  mov rdx,[rax+rsi*8]
  lea rcx,pcntGpuText
  call lstrcmpW
  test eax,eax
  jnz pcntGpuBad
  inc ebx
  cmp ebx,PC_COLUMN_COUNT
  jb pcntGpuHeader
  inc appLanguage
  cmp appLanguage,2
  jb pcntGpuLanguage
  mov appLanguage,0
  call SetModeColumns
  xor eax,eax
  jmp pcntGpuDone
pcntGpuBad:
  mov eax,1
pcntGpuDone:
  add rsp,28h
  pop rsi
  pop rbx
  ret
PcntGpuHeaders ENDP

ProcessColumnsNativeTest PROC
  push rbx
  push rsi
  sub rsp,68h
  xor ecx,ecx
  call GetModuleHandleW
  mov gInst,rax
  lea rcx,pcntControls
  call InitCommonControlsEx
  call LoadUiStrings
  xor ecx,ecx
  lea rdx,pcntStatic
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
  jz pcntFail
  mov [rsp+40h],rax
  xor ecx,ecx
  lea rdx,pcntListClass
  xor r8d,r8d
  mov r9d,40001001h ; hidden child report/owner-data
  call CreateWindowExW
  mov gList,rax
  test rax,rax
  jz pcntFail
  mov rcx,rax
  mov edx,101Fh
  xor r8d,r8d
  xor r9d,r9d
  call SendMessageW
  mov processHeader,rax
  call InitProcessListStyle
  ; No production settings, workers, or desktop-visible window.
  mov modelStage,1801
  call PcEnsureLayout
  mov frontMode,0
  mov uiDpi,96
  mov pcAppliedMode,-1
  lea rcx,pcLayout
  call PcDefaultLayout
  mov dword ptr [pcLayout+PC_L_COUNT],PC_COLUMN_COUNT
  xor ebx,ebx
  lea rsi,pcLayout
pcntIds:
  mov [rsi+rbx*4+PC_L_IDS],ebx
  inc ebx
  cmp ebx,PC_COLUMN_COUNT
  jb pcntIds
  call SetModeColumns
  call PcntGpuHeaders
  test eax,eax
  jnz pcntFail
  mov rcx,processHeader
  mov edx,1200h
  xor r8d,r8d
  xor r9d,r9d
  call SendMessageW
  cmp eax,PC_COLUMN_COUNT
  jne pcntFail
  mov ecx,11
  call PcColumnId
  cmp eax,PC_WRITE_BYTES
  jne pcntFail
  ; A real header operation must traverse the production ListView subclass,
  ; update native column geometry, and post one deferred capture notification.
  mov pcCapturePending,0
  mov dword ptr [pcntHeaderItem],1 ; HDI_WIDTH
  mov dword ptr [pcntHeaderItem+4],131
  mov rcx,processHeader
  mov edx,120Ch ; HDM_SETITEMW
  mov r8d,6 ; initial physical PID column
  lea r9,pcntHeaderItem
  call SendMessageW
  test eax,eax
  jz pcntFail
  cmp pcCapturePending,1
  jne pcntFail
  mov rcx,gList
  mov edx,101Dh ; LVM_GETCOLUMNWIDTH
  mov r8d,6
  xor r9d,r9d
  call SendMessageW
  cmp eax,131
  jne pcntFail
  ; Restore the original 96-DPI width for the following geometry assertions.
  mov dword ptr [pcntHeaderItem+4],68
  mov rcx,processHeader
  mov edx,120Ch
  mov r8d,6
  lea r9,pcntHeaderItem
  call SendMessageW
  test eax,eax
  jz pcntFail
  mov pcCapturePending,0
  ; Visual move PID to position 1 does not alter physical subitem mapping.
  lea rsi,pcntOrder
  mov dword ptr [rsi],0
  mov dword ptr [rsi+4],6
  mov ebx,2
pcntOrderLoop:
  lea eax,[rbx-1]
  cmp ebx,7
  jb pcntOrderStore
  mov eax,ebx
pcntOrderStore:
  mov [rsi+rbx*4],eax
  inc ebx
  cmp ebx,PC_COLUMN_COUNT
  jb pcntOrderLoop
  mov rcx,gList
  mov edx,103Ah
  mov r8d,PC_COLUMN_COUNT
  mov r9,rsi
  call SendMessageW
  test eax,eax
  jz pcntFail
  mov ecx,1
  call PcColumnId
  cmp eax,PC_STATUS
  jne pcntFail
  call PcCaptureLayout
  test eax,eax
  jnz pcntFail
  cmp dword ptr [pcLayout+PC_L_IDS+4],PC_PID
  jne pcntFail
  mov modelStage,1802
  ; Removal must use actual header count (12), including nonprocess panels.
  mov frontMode,2
  call SetModeColumns
  mov rcx,processHeader
  mov edx,1200h
  xor r8d,r8d
  xor r9d,r9d
  call SendMessageW
  cmp eax,7
  jne pcntFail
  mov frontMode,0
  mov uiDpi,144
  call SetModeColumns
  mov ecx,1
  call PcColumnId
  cmp eax,PC_PID
  jne pcntFail
  mov rcx,gList
  mov edx,101Dh
  mov r8d,1
  xor r9d,r9d
  call SendMessageW
  cmp eax,102
  jne pcntFail
  mov uiDpi,192
  call SetModeColumns
  mov rcx,gList
  mov edx,101Dh
  mov r8d,1
  xor r9d,r9d
  call SendMessageW
  cmp eax,136
  jne pcntFail
  mov rcx,gList
  mov edx,101Eh
  mov r8d,1
  mov r9d,260
  call SendMessageW
  call PcCaptureLayout
  test eax,eax
  jnz pcntFail
  cmp dword ptr [pcLayout+PC_L_WIDTHS+PC_PID*4],130
  jne pcntFail
  mov uiDpi,96
  call SetModeColumns
  mov rcx,gList
  mov edx,101Dh
  mov r8d,1
  xor r9d,r9d
  call SendMessageW
  cmp eax,130
  jne pcntFail
  mov modelStage,1803
  lea rbx,pcntRows
  mov rcx,rbx
  call ClearRow
  lea rcx,[rbx+4096]
  call ClearRow
  mov treeEnabled,0
  mov sortColumn,PC_READ_BYTES
  mov sortDescending,0
  mov dword ptr [rbx+3660],1
  mov dword ptr [rbx+4096+3660],1
  mov qword ptr [rbx+3640],2
  mov qword ptr [rbx+4096+3640],10
  mov rcx,rbx
  lea rdx,[rbx+4096]
  call CompareRows
  test eax,eax
  jns pcntFail
  mov qword ptr [rbx+3640],10
  mov qword ptr [rbx+4096+3640],100
  mov rcx,rbx
  lea rdx,[rbx+4096]
  call CompareRows
  test eax,eax
  jns pcntFail
  mov rax,100000002h
  mov [rbx+3640],rax
  mov rax,10000000Ah
  mov [rbx+4096+3640],rax
  mov rcx,rbx
  lea rdx,[rbx+4096]
  call CompareRows
  test eax,eax
  jns pcntFail
  mov sortDescending,1
  mov dword ptr [rbx+3660],0
  mov rcx,rbx
  lea rdx,[rbx+4096]
  call CompareRows
  test eax,eax
  jle pcntFail
  mov sortDescending,0
  mov rcx,rbx
  lea rdx,[rbx+4096]
  call CompareRows
  test eax,eax
  jle pcntFail
  mov modelStage,1806
  ; Equal full-width numeric values use PID before birth, in both directions.
  mov rax,100000002h
  mov [rbx+3640],rax
  mov [rbx+4096+3640],rax
  mov dword ptr [rbx+3660],1
  mov dword ptr [rbx+4096+3660],1
  mov dword ptr [rbx+3584],2
  mov dword ptr [rbx+4096+3584],100
  mov rax,200000002h
  mov [rbx+3600],rax
  mov rax,100000001h
  mov [rbx+4096+3600],rax
  call PcntCheckIdentityOrder
  test eax,eax
  jnz pcntFail
  ; Same PID: compare all 64 birth bits, not only the low DWORD.
  mov dword ptr [rbx+4096+3584],2
  mov rax,100000002h
  mov [rbx+3600],rax
  mov rax,200000001h
  mov [rbx+4096+3600],rax
  call PcntCheckIdentityOrder
  test eax,eax
  jnz pcntFail
  ; Two unavailable metrics still have the same deterministic identity order.
  mov dword ptr [rbx+3660],0
  mov dword ptr [rbx+4096+3660],0
  call PcntCheckIdentityOrder
  test eax,eax
  jnz pcntFail
  mov dword ptr [rbx+4096+3584],100
  mov rax,200000002h
  mov [rbx+3600],rax
  mov rax,100000001h
  mov [rbx+4096+3600],rax
  call PcntCheckIdentityOrder
  test eax,eax
  jnz pcntFail
  mov modelStage,1804
  mov dword ptr [rbx+3660],1
  mov rcx,rbx
  mov edx,PC_WRITE_BYTES
  call PcDisplayExtra
  mov rcx,rax
  ; Valid zero stays a number. Invalidity is carried by the separate flag.
  cmp word ptr [rcx],'0'
  jne pcntFail
  mov dword ptr [rbx+3660],1
  mov rcx,rbx
  mov edx,PC_READ_BYTES
  call PcDisplayExtra
  mov rcx,rax
  lea rdx,pcntCounterExpected
  call lstrcmpW
  test eax,eax
  jnz pcntFail
  mov qword ptr [rbx+3600],10000000
  mov rcx,rbx
  mov edx,PC_CREATED_UTC
  call PcDisplayExtra
  mov rcx,rax
  lea rdx,pcntUtcExpected
  call lstrcmpW
  test eax,eax
  jnz pcntFail
  mov dword ptr [rbx+3660],0
  ; Exact own counters and explicit invalid display, never a fabricated zero.
  mov rcx,rbx
  mov edx,PC_READ_BYTES
  call PcDisplayExtra
  lea rdx,unavailable
  mov rcx,rax
  call lstrcmpW
  test eax,eax
  jnz pcntFail
  mov treeEnabled,1
  mov qword ptr [rbx+3736],2
  mov dword ptr [rbx+3712],0
  mov qword ptr [rbx+3720],104857600
  mov rcx,rbx
  mov edx,PC_PRIVATE_COMMIT
  call PcSortMetric
  cmp ecx,1
  jne pcntFail
  cmp eax,104857600
  jne pcntFail
  mov dword ptr [rbx+3712],2
  mov rcx,rbx
  mov edx,PC_PRIVATE_COMMIT
  call PcSortMetric
  test ecx,ecx
  jnz pcntFail
  mov treeEnabled,0
  ; Header notifications: Name fixed, PID cannot precede it, capture deferred.
  lea rsi,pcntNotify
  mov rax,processHeader
  mov [rsi],rax
  mov dword ptr [rsi+16],-310
  mov dword ptr [rsi+24],0
  mov rcx,rsi
  call PcHeaderNotify
  cmp eax,1
  jne pcntFail
  mov dword ptr [rsi+16],-311
  mov dword ptr [rsi+24],1
  lea rax,pcntHeaderItem
  mov [rsi+32],rax
  mov dword ptr [rax],80h
  mov dword ptr [rax+44],0
  mov rcx,rsi
  call PcHeaderNotify
  cmp eax,1
  jne pcntFail
  mov dword ptr [pcntHeaderItem+44],2
  mov rcx,rsi
  call PcHeaderNotify
  test eax,eax
  jnz pcntFail
  cmp pcCapturePending,1
  jne pcntFail
  mov modelStage,1805
  ; PcApplyLayout preserves selected PID+birth while sorting the row map.
  lea rax,pcntRows
  mov frontBuffer,rax
  mov frontCount,2
  mov dword ptr [rbx+3584],100
  mov dword ptr [rbx+4096+3584],2
  mov qword ptr [rbx+3600],123
  mov qword ptr [rbx+4096+3600],456
  mov sortColumn,PC_PID
  mov sortDescending,0
  call BuildRowMap
  mov dword ptr [pcntState+12],3
  mov dword ptr [pcntState+16],3
  mov rcx,gList
  mov edx,102Bh
  mov r8d,1 ; PID100 after PID2
  lea r9,pcntState
  call SendMessageW
  lea rcx,pcntLayout
  call PcDefaultLayout
  mov dword ptr [pcntLayout+PC_L_SORT],PC_PID
  mov dword ptr [pcntLayout+PC_L_DESCENDING],1
  lea rcx,pcntLayout
  call PcApplyLayout
  test eax,eax
  jnz pcntFail
  cmp rememberId,100
  jne pcntFail
  cmp rememberBirth,123
  jne pcntFail
  mov rcx,gList
  mov edx,100Ch
  mov r8,-1
  mov r9d,2
  call SendMessageW
  cmp eax,0 ; PID100 now first
  jne pcntFail
  xor eax,eax
  jmp pcntDone
pcntFail:
  mov eax,1
pcntDone:
  mov ebx,eax
  mov rcx,gMain
  call DestroyWindow
  mov ecx,ebx
  test ecx,ecx
  jz pcntExit
  mov ecx,modelStage
pcntExit:
  call ExitProcess
ProcessColumnsNativeTest ENDP
