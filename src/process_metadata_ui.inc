; UI ownership boundary: workers receive copied identities only; painting and
; comparisons read immutable cache payloads. No process API runs on this path.
.data
align 8
pmUiMask dd 0
pmUiFrameValid dd 0
pmUiPumping dd 0
pmUiTimer dd 0
pmUiTestEnable dd 0
pmUiStopping dd 0
pmUiFairTurns dd PM_LANES dup(0)
.data?
align 8
pmUiRequests db PM_LANES*PD_REQUEST_SIZE dup(?)
pmUiTakenRequest db PD_REQUEST_SIZE dup(?)
pmUiTakenError dd ?
pmUiText dw 256 dup(?)
pmUiFormat dw 256 dup(?)
.code
; Stable column ID -> lane (zero means ordinary snapshot column).
PmColumnLane PROC
  mov eax,PM_LANE_ACCOUNT
  cmp edx,PC_ACCOUNT
  je pmclDone
  mov eax,PM_LANE_PATH
  cmp edx,PC_IMAGE_PATH
  je pmclDone
  mov eax,PM_LANE_COMMAND
  cmp edx,PC_COMMAND
  je pmclDone
  xor eax,eax
pmclDone:
  ret
PmColumnLane ENDP

; RCX row, EDX column -> CacheGet ABI. Never formats the comparison keys.
PmRowValue PROC
  sub rsp,28h
  mov [rsp+20h],rcx
  call PmColumnLane
  test eax,eax
  jz pmrvMissing
  mov r8d,eax
  mov rax,[rsp+20h]
  test rax,rax
  jz pmrvMissing
  mov ecx,[rax+3584]
  mov rdx,[rax+3600]
  call PmCacheGet
  jmp pmrvDone
pmrvMissing:
  xor eax,eax
  xor edx,edx
  xor r8d,r8d
  xor r9d,r9d
pmrvDone:
  add rsp,28h
  ret
PmRowValue ENDP

PmCellText PROC
  sub rsp,38h
  call PmRowValue
  cmp edx,PM_STATUS_VALID
  je pmctDone
  mov [rsp+20h],r8d
  mov ecx,52310
  cmp edx,PM_STATUS_MISSING
  je pmctLoad
  cmp edx,PM_STATUS_LOADING
  je pmctLoad
  mov ecx,52312
  cmp edx,PM_STATUS_TIMEOUT
  je pmctLoad
  mov ecx,52314
pmctLoad:
  add ecx,appLanguage
  mov edx,ecx
  mov rcx,gInst
  lea r8,pmUiFormat
  mov r9d,256
  call LoadStringW
  lea rcx,pmUiText
  lea rdx,pmUiFormat
  mov r8d,[rsp+20h]
  call wsprintfW
  lea rax,pmUiText
pmctDone:
  add rsp,38h
  ret
PmCellText ENDP

; Column/mode transitions change demand; ordinary snapshots do not change epoch.
PmUiDemand PROC
  push rbx
  sub rsp,20h
  xor ebx,ebx
  cmp pmUiStopping,0
  jne pmudSet
  cmp frontMode,0
  jne pmudNoFrame
  cmp uiRegressionMode,0
  je pmudColumns
  cmp pmUiTestEnable,0
  je pmudNoFrame
pmudColumns:
  xor r10d,r10d
  lea r11,pcVisibleIds
pmudColumn:
  cmp r10d,pcVisibleCount
  jae pmudSet
  mov edx,[r11+r10*4]
  cmp edx,PC_ACCOUNT
  jne pmudPath
  or ebx,PM_MASK_ACCOUNT
pmudPath:
  cmp edx,PC_IMAGE_PATH
  jne pmudCommand
  or ebx,PM_MASK_PATH
pmudCommand:
  cmp edx,PC_COMMAND
  jne pmudNext
  or ebx,PM_MASK_COMMAND
pmudNext:
  inc r10d
  jmp pmudColumn
pmudNoFrame:
  mov pmUiFrameValid,0
pmudSet:
  mov pmUiMask,ebx
  mov ecx,ebx
  mov rdx,gMain
  call PmCacheSetDemand
  test ebx,ebx
  jz pmudKill
  cmp pmUiTimer,0
  jne pmudDone
  mov rcx,gMain
  test rcx,rcx
  jz pmudDone
  mov edx,PM_TIMER_ID
  mov r8d,PM_TIMER_MS
  xor r9d,r9d
  call SetTimer
  test rax,rax
  setne al
  movzx eax,al
  mov pmUiTimer,eax
  jmp pmudDone
pmudKill:
  cmp pmUiTimer,0
  je pmudDone
  mov rcx,gMain
  mov edx,PM_TIMER_ID
  call KillTimer
  mov pmUiTimer,0
pmudDone:
  add rsp,20h
  pop rbx
  ret
PmUiDemand ENDP

; Called after the process front-buffer swap and before BuildRowMap.
PmUiFrame PROC
  push rbx
  push rsi
  sub rsp,28h
  call PmUiDemand
  cmp frontMode,0
  jne pmufDone
  cmp uiRegressionMode,0
  je pmufBegin
  cmp pmUiTestEnable,0
  je pmufDone
pmufBegin:
  call GetTickCount64
  mov rcx,rax
  call PmCacheSetClock
  call PmCacheBeginFrame
  mov rsi,frontBuffer
  xor ebx,ebx
pmufMark:
  cmp ebx,frontCount
  jae pmufPrune
  mov ecx,[rsi+3584]
  mov rdx,[rsi+3600]
  call PmCacheMark
  add rsi,4096
  inc ebx
  jmp pmufMark
pmufPrune:
  call PmCachePrune
  mov rsi,frontBuffer
  xor ebx,ebx
pmufEnsure:
  cmp ebx,frontCount
  jae pmufReady
  mov ecx,[rsi+3584]
  mov rdx,[rsi+3600]
  call PmCacheEnsure
  add rsi,4096
  inc ebx
  jmp pmufEnsure
pmufReady:
  mov pmUiFrameValid,1
pmufDone:
  add rsp,28h
  pop rsi
  pop rbx
  ret
PmUiFrame ENDP

; RCX visible index, EDX lane, R8 copied request output.
PmUiPrepareIndex PROC
  sub rsp,28h
  cmp ecx,visibleCount
  jae pmupiNone
  lea rax,rowMap
  mov rax,[rax+rcx*8]
  test rax,rax
  jz pmupiNone
  mov r9,r8
  mov r8d,edx
  mov ecx,[rax+3584]
  mov rdx,[rax+3600]
  call PmCachePrepare
  jmp pmupiDone
pmupiNone:
  mov eax,259
pmupiDone:
  add rsp,28h
  ret
PmUiPrepareIndex ENDP

; One selected row, a bounded viewport, then the fair cache cursor.
PmUiPrepare PROC
  push rbx
  push rsi
  push rdi
  push r12
  sub rsp,28h
  mov ebx,ecx
  mov rsi,rdx
  ; Reserve every second admission for the fair cursor, even when visible
  ; providers stay slow enough that viewport entries are continuously due.
  lea rax,pmUiFairTurns
  lea ecx,[rbx-1]
  xor dword ptr [rax+rcx*4],1
  cmp dword ptr [rax+rcx*4],0
  jne pmupPriority
  mov ecx,ebx
  mov rdx,rsi
  call PmCacheNext
  test eax,eax
  jz pmupDone
pmupPriority:
  mov rcx,gList
  mov edx,100Ch
  mov r8,-1
  mov r9d,2
  call SendMessageW
  mov ecx,eax
  mov edx,ebx
  mov r8,rsi
  call PmUiPrepareIndex
  test eax,eax
  jz pmupDone
  mov rcx,gList
  mov edx,1027h ; LVM_GETTOPINDEX
  xor r8d,r8d
  xor r9d,r9d
  call SendMessageW
  mov edi,eax
  mov rcx,gList
  mov edx,1028h ; LVM_GETCOUNTPERPAGE
  xor r8d,r8d
  xor r9d,r9d
  call SendMessageW
  inc eax
  cmp eax,128
  jbe pmupBound
  mov eax,128
pmupBound:
  mov r12d,eax
pmupViewport:
  test r12d,r12d
  jz pmupFair
  mov ecx,edi
  mov edx,ebx
  mov r8,rsi
  call PmUiPrepareIndex
  test eax,eax
  jz pmupDone
  inc edi
  dec r12d
  jmp pmupViewport
pmupFair:
  mov ecx,ebx
  mov rdx,rsi
  call PmCacheNext
pmupDone:
  add rsp,28h
  pop r12
  pop rdi
  pop rsi
  pop rbx
  ret
PmUiPrepare ENDP

; UI-only pump. Per lane: one active request, one immutable result, no join.
PmUiPump PROC
  push rbx
  push rsi
  push rdi
  sub rsp,30h
  cmp pmUiPumping,0
  jne pmupmDone
  cmp pmUiStopping,0
  jne pmupmDone
  mov pmUiPumping,1
  call GetTickCount64
  mov rcx,rax
  call PmCacheSetClock
  mov ebx,1
  lea rsi,pmUiRequests
pmupmLane:
  mov rcx,rsi
  call PmCacheExpire
  mov ecx,ebx
  lea rdx,pmUiTakenRequest
  lea r8,pmUiTakenError
  call PmWorkerTake
  test edx,edx
  jz pmupmSchedule
  mov rdi,rax
  lea rcx,pmUiTakenRequest
  mov rdx,rdi
  mov r8d,pmUiTakenError
  call PmCacheComplete
  mov rcx,rdi
  call PmWorkerFree
pmupmSchedule:
  cmp pmUiFrameValid,0
  je pmupmNext
  mov ecx,ebx
  dec ecx
  mov eax,1
  shl eax,cl
  test pmUiMask,eax
  jz pmupmNext
  mov ecx,ebx
  call PmWorkerBusy
  test eax,eax
  jnz pmupmNext
  mov ecx,ebx
  mov rdx,rsi
  call PmUiPrepare
  test eax,eax
  jnz pmupmNext
  mov rcx,rsi
  call PmWorkerSubmit
  test eax,eax
  jz pmupmNext
  mov r8d,eax
  mov rcx,rsi
  xor edx,edx
  call PmCacheComplete
pmupmNext:
  add rsi,PD_REQUEST_SIZE
  inc ebx
  cmp ebx,PM_LANES
  jbe pmupmLane
  ; Advancing the clock can expire another cached row while a provider is
  ; blocked. Re-evaluate text ordering using effective freshness at this tick.
  cmp pmUiMask,0
  je pmupmClear
  cmp pmUiFrameValid,0
  je pmupmClear
  cmp sortColumn,PC_ACCOUNT
  jb pmupmInvalidate
  cmp sortColumn,PC_COMMAND
  ja pmupmInvalidate
  call RememberSelection
  call BuildRowMap
pmupmInvalidate:
  mov rcx,gList
  xor edx,edx
  xor r8d,r8d
  call InvalidateRect
pmupmClear:
  mov pmUiPumping,0
pmupmDone:
  add rsp,30h
  pop rdi
  pop rsi
  pop rbx
  ret
PmUiPump ENDP

PmUiStop PROC
  sub rsp,28h
  mov pmUiStopping,1
  call PmUiDemand
  call PmWorkerStop
  call PmCacheReset
  add rsp,28h
  ret
PmUiStop ENDP
