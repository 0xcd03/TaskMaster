; UI owns one immutable GPU frame. Lookup/compare never invoke providers.
.data
align 8
gpUiFront dq 0
gpUiEpoch dq 0
gpUiMask dd 0
gpUiFresh dd 0
gpUiFailure dd 0
gpUiStopping dd 0
gpUiTimer dd 0
IFDEF TM_TESTS
gpUiTestEnable dd 0
ENDIF
gpUiPumping dd 0
align 2
gpUiEngineFormat dw '%','s',' ','[','%','0','8','X',':','%','0','8','X',']',' ','/',' ','%','u',' ','/',' ','%','u',' ','%','s',0
gpUiLuidLabel dw 'L','U','I','D',0
gpUiMemFormat dw '%','s',' ','M','i','B',0
.data?
align 8
gpUiText dw 512 dup(?)
gpUiSmall dw 64 dup(?)
gpUiFormat dw 128 dup(?)
gpUiTakenError dd ?
.code
; RCX process row, EDX stablecolumn -> RAX numeric/engine-record, EDX status,
; R8D error. All effective freshness is frozen by UI pump before sorting.
GpUiRowValue PROC
  push rbx
  push rsi
  sub rsp,28h
  mov rbx,rcx
  mov esi,edx
  mov edx,GP_STATUS_MISSING
  xor r8d,r8d
  test rbx,rbx
  jz gpurvNone
  mov rcx,gpUiFront
  test rcx,rcx
  jz gpurvNoFrame
  cmp gpUiFresh,0
  je gpurvStale
  mov rax,[rcx+GP_S_EPOCH]
  cmp rax,gpUiEpoch
  jne gpurvNone
  cmp qword ptr [rbx+3600],0
  je gpurvNone
  mov edx,[rbx+3584]
  call GpFindProcess
  test rax,rax
  jz gpurvSource
  mov rcx,[rbx+3600]
  cmp [rax+GP_P_BIRTH],rcx
  jne gpurvIdentity
  cmp esi,PC_GPU_DEDICATED
  je gpurvDedicated
  cmp esi,PC_GPU_SHARED
  je gpurvShared
  mov edx,[rax+GP_P_UTIL_STATUS]
  mov r8d,[rax+GP_P_UTIL_ERROR]
  cmp esi,PC_GPU_ENGINE
  je gpurvDone
  cmp esi,PC_GPU
  jne gpurvMissing
  mov eax,[rax+GP_P_UTIL]
  jmp gpurvDone
gpurvDedicated:
  mov edx,[rax+GP_P_DEDICATED_STATUS]
  mov r8d,[rax+GP_P_DEDICATED_ERROR]
  mov rax,[rax+GP_P_DEDICATED]
  jmp gpurvDone
gpurvShared:
  mov edx,[rax+GP_P_SHARED_STATUS]
  mov r8d,[rax+GP_P_SHARED_ERROR]
  mov rax,[rax+GP_P_SHARED]
  jmp gpurvDone
gpurvIdentity:
  cmp qword ptr [rax+GP_P_BIRTH],0
  jne gpurvMissing
  mov r8d,[rax+GP_P_IDENTITY_ERROR]
  test r8d,r8d
  jz gpurvMissing
  mov edx,GP_STATUS_UNAVAILABLE
  jmp gpurvNone
gpurvNoFrame:
  mov r8d,gpUiFailure
  test r8d,r8d
  jz gpurvMissing
  mov edx,GP_STATUS_UNAVAILABLE
  jmp gpurvNone
gpurvStale:
  mov edx,GP_STATUS_STALE
  mov r8d,1460
  jmp gpurvNone
gpurvMissing:
  mov edx,GP_STATUS_MISSING
  xor r8d,r8d
  jmp gpurvNone
gpurvSource:
  mov rcx,gpUiFront
  xor eax,eax
  cmp esi,PC_GPU_DEDICATED
  jne gpurvSourceShared
  mov eax,GP_SOURCE_PROCESS_DEDICATED
gpurvSourceShared:
  cmp esi,PC_GPU_SHARED
  jne gpurvSourceRead
  mov eax,GP_SOURCE_PROCESS_SHARED
gpurvSourceRead:
  mov edx,[rcx+rax*8+GP_S_SOURCES]
  mov r8d,[rcx+rax*8+GP_S_SOURCES+4]
  cmp edx,GP_STATUS_VALID
  je gpurvMissing
gpurvNone:
  xor eax,eax
gpurvDone:
  add rsp,28h
  pop rsi
  pop rbx
  ret
GpUiRowValue ENDP

GpUiSortMetric PROC
  sub rsp,28h
  call GpUiRowValue
  xor ecx,ecx
  cmp edx,GP_STATUS_VALID
  sete cl
  add rsp,28h
  ret
GpUiSortMetric ENDP

; EAX typed engine comparison, R8D/R9D left/right validity for outer stable
; validity and PID/birth tie rules. Display names never affect engine identity.
GpUiCompareEngine PROC
  push rbx
  push rsi
  push rdi
  push r12
  sub rsp,28h
  mov rsi,rdx
  mov edx,PC_GPU_ENGINE
  call GpUiRowValue
  mov rdi,rax
  xor r12d,r12d
  cmp edx,GP_STATUS_VALID
  sete r12b
  mov rcx,rsi
  mov edx,PC_GPU_ENGINE
  call GpUiRowValue
  mov r8d,r12d
  xor r9d,r9d
  cmp edx,GP_STATUS_VALID
  sete r9b
  xor ecx,ecx
  test r8d,r8d
  jz gpuceZero
  test r9d,r9d
  jz gpuceZero
  mov rcx,[rdi+GP_P_LUID]
  cmp rcx,[rax+GP_P_LUID]
  jne gpuceResult
  mov ecx,[rdi+GP_P_PHYS]
  cmp ecx,[rax+GP_P_PHYS]
  jne gpuceResult
  mov ecx,[rdi+GP_P_ENGINE]
  cmp ecx,[rax+GP_P_ENGINE]
  je gpuceZero
gpuceResult:
  mov eax,1
  ja gpuceDone
  mov eax,-1
  jmp gpuceDone
gpuceZero:
  xor eax,eax
gpuceDone:
  add rsp,28h
  pop r12
  pop rdi
  pop rsi
  pop rbx
  ret
GpUiCompareEngine ENDP

; EDX status,R8D error -> localized text in UI-only buffer.
GpUiStatusText PROC
  sub rsp,38h
  mov [rsp+20h],r8d
  mov ecx,52402
  cmp edx,GP_STATUS_WARMUP
  jne gpustUnavailable
  mov ecx,52400
  jmp gpustLoad
gpustUnavailable:
  cmp edx,GP_STATUS_UNAVAILABLE
  jne gpustPartial
  mov ecx,52404
  jmp gpustLoad
gpustPartial:
  cmp edx,GP_STATUS_PARTIAL
  jne gpustStale
  mov ecx,52406
  jmp gpustLoad
gpustStale:
  cmp edx,GP_STATUS_STALE
  jne gpustLoad
  mov ecx,52408
gpustLoad:
  add ecx,appLanguage
  mov edx,ecx
  mov rcx,gInst
  lea r8,gpUiFormat
  mov r9d,128
  mov word ptr [r8],0
  call LoadStringW
  test eax,eax
  jnz gpustFormat
  mov word ptr [gpUiFormat],'?'
  mov word ptr [gpUiFormat+2],0
gpustFormat:
  lea rcx,gpUiText
  lea rdx,gpUiFormat
  mov r8d,[rsp+20h]
  call wsprintfW
  lea rax,gpUiText
  add rsp,38h
  ret
GpUiStatusText ENDP

GpUiCellText PROC
  push rbx
  sub rsp,60h
  mov ebx,edx
  call GpUiRowValue
  cmp edx,GP_STATUS_VALID
  jne gpuctStatus
  cmp ebx,PC_GPU_ENGINE
  je gpuctEngine
  cmp ebx,PC_GPU
  je gpuctUtil
  mov rcx,rax
  lea rdx,gpUiSmall
  mov r8d,1048576
  call StoreScaledMetric
  lea rcx,gpUiText
  lea rdx,gpUiMemFormat
  lea r8,gpUiSmall
  call wsprintfW
  jmp gpuctText
gpuctUtil:
  xor edx,edx
  mov ecx,100
  div ecx
  mov r8d,eax
  mov eax,edx
  xor edx,edx
  mov ecx,10
  div ecx
  mov r9d,eax
  lea rcx,gpUiText
  lea rdx,processCpuFormat
  cmp appLanguage,0
  je gpuctPercent
  lea rdx,processCpuFormatRu
gpuctPercent:
  call wsprintfW
  jmp gpuctText
gpuctEngine:
  mov r10,rax
  lea r8,gpUiLuidLabel
  mov r11,gpUiFront
  xor ecx,ecx
gpuctAdapter:
  cmp ecx,[r11+GP_S_ADAPTER_COUNT]
  jae gpuctEngineFormat
  cmp ecx,GP_ADAPTER_CAPACITY
  jae gpuctEngineFormat
  mov eax,ecx
  shl eax,9
  lea rdx,[r11+rax+GP_S_ADAPTERS]
  mov rax,[r10+GP_P_LUID]
  cmp [rdx+GP_A_LUID],rax
  jne gpuctAdapterNext
  test dword ptr [rdx+GP_A_FLAGS],1
  jz gpuctAdapterNext
  lea r8,[rdx+GP_A_DESCRIPTION]
  jmp gpuctEngineFormat
gpuctAdapterNext:
  inc ecx
  jmp gpuctAdapter
gpuctEngineFormat:
  mov rax,[r10+GP_P_LUID]
  mov [rsp+20h],eax
  shr rax,32
  mov r9d,eax
  mov eax,[r10+GP_P_PHYS]
  mov [rsp+28h],rax
  mov eax,[r10+GP_P_ENGINE]
  mov [rsp+30h],rax
  lea rax,[r10+GP_P_TYPE]
  mov [rsp+38h],rax
  lea rcx,gpUiText
  lea rdx,gpUiEngineFormat
  call wsprintfW
gpuctText:
  lea rax,gpUiText
  jmp gpuctDone
gpuctStatus:
  call GpUiStatusText
gpuctDone:
  add rsp,60h
  pop rbx
  ret
GpUiCellText ENDP

GpUiDemand PROC
  push rbx
  push rsi
  sub rsp,28h
  xor ebx,ebx
  cmp gpUiStopping,0
  jne gpudSet
IFDEF TM_TESTS
  cmp gpUiTestEnable,0 ; old fixtures never start unowned live GPU work
  je gpudSet
ELSE
  cmp uiRegressionMode,0
  jne gpudSet
ENDIF
gpudMode:
  cmp frontMode,4
  je gpudGlobal
  cmp frontMode,0
  jne gpudSet
  xor ecx,ecx
  lea rdx,pcVisibleIds
gpudColumns:
  cmp ecx,pcVisibleCount
  jae gpudGlobal
  mov eax,[rdx+rcx*4]
  cmp eax,PC_GPU
  jb gpudNext
  cmp eax,PC_GPU_SHARED
  ja gpudNext
  or ebx,GP_MASK_PROCESS
gpudNext:
  inc ecx
  jmp gpudColumns
gpudGlobal:
  or ebx,GP_MASK_GLOBAL
gpudSet:
  mov gpUiMask,ebx
  mov ecx,ebx
  mov rdx,gMain
  call GpWorkerSetDemand
  cmp rax,gpUiEpoch
  jne gpudEpochChanged
  test edx,edx
  jz gpudTimer
gpudEpochChanged:
  mov gpUiFailure,edx
  mov gpUiEpoch,rax
  mov rcx,gpUiFront
  mov gpUiFront,0
  mov gpUiFresh,0
  call GpWorkerFree
gpudTimer:
  test ebx,ebx
  jz gpudKill
  cmp gpUiTimer,0
  jne gpudDone
  mov rcx,gMain
  test rcx,rcx
  jz gpudDone
  mov edx,GP_TIMER_ID
  mov r8d,250
  xor r9d,r9d
  call SetTimer
  test rax,rax
  setne al
  movzx eax,al
  mov gpUiTimer,eax
  jmp gpudDone
gpudKill:
  cmp gpUiTimer,0
  je gpudDone
  mov rcx,gMain
  mov edx,GP_TIMER_ID
  call KillTimer
  mov gpUiTimer,0
gpudDone:
  add rsp,28h
  pop rsi
  pop rbx
  ret
GpUiDemand ENDP

GpUiPump PROC
  push rbx
  push rsi
  sub rsp,28h
  cmp gpUiPumping,0
  jne gpupDone
  cmp gpUiStopping,0
  jne gpupDone
  mov gpUiPumping,1
  mov esi,gpUiFresh
  lea rcx,gpUiTakenError
  call GpWorkerTake
  test edx,edx
  jz gpupFresh
  mov rbx,rax
  mov eax,gpUiTakenError
  mov gpUiFailure,eax
  mov rcx,gpUiFront
  mov gpUiFront,rbx
  call GpWorkerFree
  mov esi,-1 ; force refresh/order reevaluation
gpupFresh:
  mov gpUiFresh,0
  mov rbx,gpUiFront
  test rbx,rbx
  jz gpupChanged
  call GetTickCount64
  sub rax,[rbx+GP_S_TICK]
  jc gpupChanged
  cmp rax,GP_STALE_MS
  ja gpupChanged
  mov gpUiFresh,1
gpupChanged:
  cmp esi,gpUiFresh
  je gpupClear
  cmp frontMode,0
  jne gpupClear
  cmp sortColumn,PC_GPU
  jb gpupInvalidate
  cmp sortColumn,PC_GPU_SHARED
  ja gpupInvalidate
  call RememberSelection
  call BuildRowMap
gpupInvalidate:
  mov rcx,gList
  xor edx,edx
  xor r8d,r8d
  call InvalidateRect
gpupClear:
  mov gpUiPumping,0
gpupDone:
  add rsp,28h
  pop rsi
  pop rbx
  ret
GpUiPump ENDP

GpUiStop PROC
  sub rsp,28h
  mov gpUiStopping,1
  call GpUiDemand
  call GpWorkerStop
  mov rcx,gpUiFront
  mov gpUiFront,0
  mov gpUiFresh,0
  call GpWorkerFree
  add rsp,28h
  ret
GpUiStop ENDP
