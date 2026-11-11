; Actual production owner-data ListView, model arrays, compare, and main routes.
; Own hidden windows and heap frames only. No demand/provider/thread is started.
.data
align 8
gpunStage dd 9500
gpunAssertions dd 0
gpunKeyMode dd 0
gpunHeaderIds dd 1001,1106,1009,1107,1108,1004,1002
gpunControls dd 8,1
gpunFrame dq 0
gpunOldProc dq 0
gpunStatic dw 'S','T','A','T','I','C',0
gpunList dw 'S','y','s','L','i','s','t','V','i','e','w','3','2',0
gpunEngineName dw 'p','i','d','_','%','u','_','l','u','i','d','_','0','x','%','0','8','X','_','0','x','%','0','8','X','_'
 dw 'p','h','y','s','_','%','u','_','e','n','g','_','%','u','_','e','n','g','t','y','p','e','_','%','u',0
gpunMemoryName dw 'p','i','d','_','%','u','_','l','u','i','d','_','0','x','%','0','8','X','_','0','x','%','0','8','X','_','p','h','y','s','_','%','u',0
gpunPercent dw '2','.','0','%',0
gpunPercentRu dw '2',',','0','%',0
gpunMemory dw '4','0','9','6','.','0',' ','M','i','B',0
gpunMemoryRu dw '4','0','9','6',',','0',' ','M','i','B',0
gpunEngine dw 'L','U','I','D',' ','[','0','0','0','0','0','0','0','0',':','0','0','0','0','0','0','0','1',']',' ','/',' ','0',' ','/',' ','2',' ','1','0',0
.data?
align 8
gpunRows db 12288 dup(?)
gpunArray db 1024 dup(?)
gpunItem db 88 dup(?)
gpunNotify db 112 dup(?)
gpunText dw 512 dup(?)
gpunExpected dw 512 dup(?)
gpunColumn db 56 dup(?)
.code
GPUN_EQ MACRO a,b
  cmp a,b
  jne gpunFail
  inc gpunAssertions
ENDM
; Model gets two complete, bounded PDH items with names inside the array.
GpunBuildFrame PROC
  push rbx
  push rsi
  push rdi
  sub rsp,60h
  call GetProcessHeap
  mov rcx,rax
  mov edx,8
  mov r8d,GP_SNAPSHOT_SIZE
  call HeapAlloc
  test rax,rax
  jz gpunBuildBad
  mov gpunFrame,rax
  inc gpWorkerBlocks
  call GetTickCount64
  mov r9,rax
  mov rcx,gpunFrame
  mov edx,1
  mov r8d,7
  call GpModelInit
  xor esi,esi
gpunBuildLane:
  lea rdi,gpunArray
  xor ebx,ebx
gpunBuildItem:
  imul eax,ebx,24
  lea rcx,[rdi+rax]
  imul edx,ebx,384
  lea rax,[rdi+rdx+64]
  mov [rcx],rax
  mov qword ptr [rcx+8],0
  mov rdx,100000002h
  test ebx,ebx
  jz gpunValue
  mov rdx,200000001h
gpunValue:
  test esi,esi
  jnz gpunMemoryValue
  mov rdx,4000000000000000h ; 2 percent
  test ebx,ebx
  jz gpunMemoryValue
  mov rdx,4024000000000000h ; 10 percent
  cmp gpunKeyMode,3
  jne gpunMemoryValue
  mov rdx,4000000000000000h
gpunMemoryValue:
  mov [rcx+16],rdx
  mov rcx,rax
  lea rdx,gpunMemoryName
  lea r8d,[rbx+42]
  xor r9d,r9d
  mov qword ptr [rsp+20h],1 ; low LUID
  mov qword ptr [rsp+28h],0 ; physical adapter
  cmp gpunKeyMode,1
  jne gpunKeyPhys
  lea r9d,[rbx+1] ; full LUID high word must dominate low word
  mov eax,2
  sub eax,ebx
  mov [rsp+20h],rax
 gpunKeyPhys:
  cmp gpunKeyMode,2
  jne gpunKeyEngine
  mov qword ptr [rsp+28h],2
  test ebx,ebx
  jz gpunKeyEngine
  mov qword ptr [rsp+28h],10
 gpunKeyEngine:
  test esi,esi
  jnz gpunName
  lea rdx,gpunEngineName
  mov qword ptr [rsp+30h],2
  mov qword ptr [rsp+38h],10 ; deliberately opposed textual type
  test ebx,ebx
  jnz gpunNameSecond
  cmp gpunKeyMode,0
  jne gpunNameFirst
  jmp gpunName
 gpunNameSecond:
  mov qword ptr [rsp+30h],10
  mov qword ptr [rsp+38h],2
  cmp gpunKeyMode,0
  je gpunName
  mov qword ptr [rsp+30h],2
  jmp gpunName
 gpunNameFirst:
  mov qword ptr [rsp+30h],10
gpunName:
  cmp gpunKeyMode,3
  jne gpunNameFormat
  mov qword ptr [rsp+30h],2
  mov qword ptr [rsp+38h],10
 gpunNameFormat:
  call wsprintfW
  inc ebx
  cmp ebx,2
  jb gpunBuildItem
  mov rcx,gpunFrame
  mov edx,esi
  mov r8,rdi
  mov r9d,1024
  mov dword ptr [rsp+20h],2
  call GpModelArray
  test eax,eax
  jnz gpunBuildBad
  inc esi
  cmp esi,3
  jb gpunBuildLane
  mov rcx,gpunFrame
  call GpModelFinalize
  mov esi,42
gpunBirth:
  mov rcx,gpunFrame
  mov edx,esi
  call GpFindProcess
  test rax,rax
  jz gpunBuildBad
  lea edx,[rsi+58]
  mov [rax+GP_P_BIRTH],rdx
  inc esi
  cmp esi,44
  jb gpunBirth
  xor eax,eax
  jmp gpunBuildDone
gpunBuildBad:
  mov eax,1
gpunBuildDone:
  add rsp,60h
  pop rdi
  pop rsi
  pop rbx
  ret
GpunBuildFrame ENDP

; Actual common-control requests text, triggering native LVN_GETDISPINFOW and
; SimpleWndProc -> PcColumnId -> ProcessDisplayText. RCX expected, EDX column.
GpunCheckCell PROC
  push rbx
  sub rsp,20h
  mov rbx,rcx
  mov dword ptr [gpunItem+8],edx
  lea rax,gpunText
  mov qword ptr [gpunItem+24],rax
  mov dword ptr [gpunItem+32],512
  mov word ptr [gpunText],0
  mov rcx,gList
  mov edx,1073h ; LVM_GETITEMTEXTW
  xor r8d,r8d
  lea r9,gpunItem
  call SendMessageW
  lea rcx,gpunText
  mov rdx,rbx
  call lstrcmpW
  add rsp,20h
  pop rbx
  ret
GpunCheckCell ENDP

; Independently specified resource IDs verify all twenty physical captions.
GpunHeaders PROC
  push rbx
  sub rsp,20h
  xor ebx,ebx
 gpunHeaderEach:
  mov ecx,ebx
  call PcColumnId
  cmp eax,ebx
  jne gpunHeaderBad
  mov dword ptr [gpunColumn],5
  lea rax,gpunText
  mov qword ptr [gpunColumn+16],rax
  mov dword ptr [gpunColumn+24],512
  mov rcx,gList
  mov edx,105Fh
  mov r8d,ebx
  lea r9,gpunColumn
  call SendMessageW
  test eax,eax
  jz gpunHeaderBad
  cmp ebx,PC_GPU
  jb gpunHeaderCaption
  mov eax,dword ptr [gpunColumn+4]
  and eax,3
  xor edx,edx
  cmp ebx,PC_GPU_ENGINE
  je gpunHeaderFormat
  inc edx
 gpunHeaderFormat:
  cmp eax,edx
  jne gpunHeaderBad
 gpunHeaderCaption:
  cmp ebx,7
  jae gpunHeaderExtra
  lea rax,gpunHeaderIds
  mov edx,[rax+rbx*4]
  cmp appLanguage,0
  je gpunHeaderLoad
  add edx,2000
  jmp gpunHeaderLoad
 gpunHeaderExtra:
  lea edx,[rbx*2+52200]
  add edx,appLanguage
 gpunHeaderLoad:
  mov rcx,gInst
  lea r8,gpunExpected
  mov r9d,512
  call LoadStringW
  test eax,eax
  jz gpunHeaderBad
  lea rcx,gpunText
  lea rdx,gpunExpected
  call lstrcmpW
  test eax,eax
  jnz gpunHeaderBad
  inc gpunAssertions
  inc ebx
  cmp ebx,PC_COLUMN_COUNT
  jb gpunHeaderEach
  xor eax,eax
  jmp gpunHeaderDone
 gpunHeaderBad:
  mov eax,1
 gpunHeaderDone:
  add rsp,20h
  pop rbx
  ret
GpunHeaders ENDP
GpuUiNativeTest PROC
  push rbx
  push rsi
  push rdi
  sub rsp,60h
  xor ecx,ecx
  call GetModuleHandleW
  mov gInst,rax
  lea rcx,gpunControls
  call InitCommonControlsEx
  mov appLanguage,0
  call LoadUiStrings
  xor ecx,ecx
  lea rdx,gpunStatic
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
  jz gpunFail
  mov [rsp+40h],rax
  xor ecx,ecx
  lea rdx,gpunList
  xor r8d,r8d
  mov r9d,40001001h
  call CreateWindowExW
  mov gList,rax
  test rax,rax
  jz gpunFail
  mov rcx,rax
  mov edx,101Fh
  xor r8d,r8d
  xor r9d,r9d
  call SendMessageW
  mov processHeader,rax
  call InitProcessListStyle
  call PcEnsureLayout
  mov frontMode,0
  mov uiDpi,96
  mov pcAppliedMode,-1
  lea rcx,pcLayout
  call PcDefaultLayout
  mov dword ptr [pcLayout+PC_L_COUNT],PC_COLUMN_COUNT
  xor ebx,ebx
  lea rsi,pcLayout
gpunColumns:
  mov [rsi+rbx*4+PC_L_IDS],ebx
  inc ebx
  cmp ebx,PC_COLUMN_COUNT
  jb gpunColumns
  call SetModeColumns
  mov appLanguage,0
 gpunHeaderLanguage:
  call LoadUiStrings
  call SetModeColumns
  call GpunHeaders
  GPUN_EQ eax,0
  inc appLanguage
  cmp appLanguage,2
  jb gpunHeaderLanguage
  mov appLanguage,0
  call LoadUiStrings
  call SetModeColumns
  ; Attach production WndProc after WM_CREATE to avoid real app initialization.
  mov rcx,gMain
  mov edx,-4
  lea r8,SimpleWndProc
  call SetWindowLongPtrW
  mov gpunOldProc,rax
  test rax,rax
  jz gpunFail
  mov rcx,gList
  mov edx,2005h ; CCM_SETUNICODEFORMAT
  mov r8d,1
  xor r9d,r9d
  call SendMessageW
  lea rbx,gpunRows
  mov rcx,rbx
  call ClearRow
  lea rcx,[rbx+4096]
  call ClearRow
  lea rcx,[rbx+8192]
  call ClearRow
  mov dword ptr [rbx+3584],42
  mov qword ptr [rbx+3600],100
  mov dword ptr [rbx+4096+3584],43
  mov qword ptr [rbx+4096+3600],101
  mov dword ptr [rbx+8192+3584],44 ; absent, never zero-valued valid
  mov qword ptr [rbx+8192+3600],102
  mov frontBuffer,rbx
  mov frontCount,3
  mov treeEnabled,0
  mov sortColumn,PC_GPU
  mov sortDescending,0
  mov gpUiEpoch,7
  mov gpWorkerEpoch,7
  mov gpUiMask,GP_MASK_ALL
  call GpunBuildFrame
  GPUN_EQ eax,0
  ; Publish into the owned mailbox, no worker or provider has been started.
  lea rcx,gpWorkerLock
  call AcquireSRWLockExclusive
  mov rax,gpunFrame
  mov gpWorkerReady,rax
  lea rcx,gpWorkerLock
  call ReleaseSRWLockExclusive
  mov rcx,gMain
  mov edx,GP_MESSAGE
  xor r8d,r8d
  xor r9d,r9d
  call SendMessageW
  mov rax,gpunFrame
  GPUN_EQ gpUiFront,rax
  GPUN_EQ gpUiFresh,1
  GPUN_EQ visibleCount,3
  lea rax,gpunRows
  GPUN_EQ rowMap,rax ; 2 numerically before 10
  mov gpunStage,9501
  lea rcx,gpunPercent
  mov edx,PC_GPU
  call GpunCheckCell
  GPUN_EQ eax,0
  mov gpunStage,9511
  lea rcx,gpunMemory
  mov edx,PC_GPU_DEDICATED
  call GpunCheckCell
  GPUN_EQ eax,0
  mov gpunStage,9512
  lea rcx,gpunMemory
  mov edx,PC_GPU_SHARED
  call GpunCheckCell
  GPUN_EQ eax,0
  mov gpunStage,9513
  lea rcx,gpunEngine
  mov edx,PC_GPU_ENGINE
  call GpunCheckCell
  GPUN_EQ eax,0
  mov appLanguage,1
  call SetModeColumns
  lea rcx,gpunPercentRu
  mov edx,PC_GPU
  call GpunCheckCell
  GPUN_EQ eax,0
  lea rcx,gpunMemoryRu
  mov edx,PC_GPU_DEDICATED
  call GpunCheckCell
  GPUN_EQ eax,0
  mov appLanguage,0
  call SetModeColumns
  mov gpunStage,9502
  mov sortColumn,PC_GPU_DEDICATED
  call BuildRowMap
  lea rax,gpunRows
  GPUN_EQ rowMap,rax ; full byte counter low bits matter above 4 GiB
  mov sortColumn,PC_GPU_SHARED
  call BuildRowMap
  lea rax,gpunRows
  GPUN_EQ rowMap,rax
  mov sortColumn,PC_GPU_ENGINE
  call BuildRowMap
  lea rax,gpunRows
  GPUN_EQ rowMap,rax ; eng2 before eng10 despite opposed type labels
  mov sortDescending,1
  call BuildRowMap
  lea rax,gpunRows+4096
  GPUN_EQ rowMap,rax
  lea rax,gpunRows+8192
  GPUN_EQ qword ptr [rowMap+16],rax ; missing still last descending
  mov sortColumn,PC_GPU
  mov sortDescending,0
  call BuildRowMap
  ; Selection survives a new frame that changes sorted order.
  mov dword ptr [gpunItem+12],3
  mov dword ptr [gpunItem+16],3
  mov rcx,gList
  mov edx,102Bh
  xor r8d,r8d
  lea r9,gpunItem
  call SendMessageW
  mov gpunStage,9503
  call GpunBuildFrame
  GPUN_EQ eax,0
  ; Different sequence but same exact identities; switch direction before pump.
  mov sortDescending,1
  lea rcx,gpWorkerLock
  call AcquireSRWLockExclusive
  mov rax,gpunFrame
  mov gpWorkerReady,rax
  lea rcx,gpWorkerLock
  call ReleaseSRWLockExclusive
  mov rcx,gMain
  mov edx,WM_TIMER
  mov r8d,GP_TIMER_ID
  xor r9d,r9d
  call SendMessageW
  GPUN_EQ gpWorkerBlocks,1 ; replaced front freed exactly once
  GPUN_EQ rememberId,42
  GPUN_EQ rememberBirth,100
  mov rcx,gList
  mov edx,100Ch
  mov r8,-1
  mov r9d,2
  call SendMessageW
  GPUN_EQ eax,1
  ; Force stale clock age through real timer route; identity selection persists.
  mov rax,gpUiFront
  sub qword ptr [rax+GP_S_TICK],GP_STALE_MS+100
  mov rcx,gMain
  mov edx,WM_TIMER
  mov r8d,GP_TIMER_ID
  xor r9d,r9d
  call SendMessageW
  GPUN_EQ gpUiFresh,0
  mov rcx,gList
  mov edx,100Ch
  mov r8,-1
  mov r9d,2
  call SendMessageW
  GPUN_EQ eax,0 ; all stale -> stable PID order
  GPUN_EQ gpWorkerThreads,0
  GPUN_EQ gpUiTestEnable,0
  ; GPU metrics stay per process in tree mode, ignoring aggregate CPU/memory.
  mov gpUiFresh,1
  mov treeEnabled,1
  mov qword ptr [rbx+3736],999
  mov qword ptr [rbx+3720],0FFFFFFFFh
  mov rcx,rbx
  mov edx,PC_GPU
  call ProcessDisplayText
  mov rcx,rax
  lea rdx,gpunPercent
  call lstrcmpW
  GPUN_EQ eax,0
  mov treeEnabled,0
  ; LUID high/low and physical adapter dominate opposing engine numbers.
  mov gpunStage,9505
  mov gpunKeyMode,1
 gpunTypedKey:
  call GpunBuildFrame
  GPUN_EQ eax,0
  mov rcx,gpUiFront
  call GpWorkerFree
  mov rax,gpunFrame
  mov gpUiFront,rax
  mov sortColumn,PC_GPU_ENGINE
  mov sortDescending,0
  call BuildRowMap
  lea rax,gpunRows
  GPUN_EQ rowMap,rax
  mov sortDescending,1
  call BuildRowMap
  lea rax,gpunRows+4096
  GPUN_EQ rowMap,rax
  inc gpunKeyMode
  cmp gpunKeyMode,3
  jb gpunTypedKey
  ; Equal valid utilization uses ascending PID, independently of direction.
  call GpunBuildFrame
  GPUN_EQ eax,0
  mov rcx,gpUiFront
  call GpWorkerFree
  mov rax,gpunFrame
  mov gpUiFront,rax
  mov sortColumn,PC_GPU
  xor esi,esi
 gpunValidTie:
  mov sortDescending,esi
  mov rcx,rbx
  lea rdx,[rbx+4096]
  call CompareRows
  test eax,eax
  jns gpunFail
  inc gpunAssertions
  lea rcx,[rbx+4096]
  mov rdx,rbx
  call CompareRows
  test eax,eax
  jle gpunFail
  inc gpunAssertions
  inc esi
  cmp esi,2
  jb gpunValidTie
  ; Validity remains first for numeric metrics in either direction.
  mov gpunStage,9506
  mov edi,PC_GPU
 gpunNumericColumn:
  cmp edi,PC_GPU_ENGINE
  je gpunNumericNext
  xor esi,esi
 gpunNumericDirection:
  mov sortColumn,edi
  mov sortDescending,esi
  call BuildRowMap
  lea rax,gpunRows+8192
  GPUN_EQ qword ptr [rowMap+16],rax
  inc esi
  cmp esi,2
  jb gpunNumericDirection
 gpunNumericNext:
  inc edi
  cmp edi,PC_GPU_SHARED
  jbe gpunNumericColumn
  ; Full birth tie-break for invalid same-PID identities, both directions.
  mov gpunStage,9504
  mov dword ptr [rbx+8192+3584],42
  mov rax,200000001h
  mov [rbx+8192+3600],rax
  mov rax,100000002h
  mov [rbx+3600],rax
  xor esi,esi
gpunTie:
  mov sortDescending,esi
  mov rcx,rbx
  lea rdx,[rbx+8192]
  call CompareRows
  test eax,eax
  jns gpunFail
  inc gpunAssertions
  inc esi
  cmp esi,2
  jb gpunTie
  mov rcx,gMain
  mov edx,-4
  mov r8,gpunOldProc
  call SetWindowLongPtrW
  mov rcx,gMain
  call DestroyWindow
  mov rcx,gpUiFront
  mov gpUiFront,0
  call GpWorkerFree
  GPUN_EQ gpWorkerBlocks,0
  xor ecx,ecx
  call ExitProcess
gpunFail:
  mov ecx,-11
  call GetStdHandle
  mov rcx,rax
  lea rdx,gpunText
  mov r8d,256
  lea r9,gpunNotify
  mov qword ptr [rsp+20h],0
  call WriteFile
  mov ecx,gpunStage
  call ExitProcess
GpuUiNativeTest ENDP
