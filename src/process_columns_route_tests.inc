; Process-column integration route fixtures. Runs against the existing hidden main
; and real ListView; all path and model state is restored before returning.
extern GetTempPathW:proc, GetTempFileNameW:proc, DeleteFileW:proc
extern PeekMessageW:proc
.data
align 8
pcrtStage dd 0
pcrtAssertions dd 0
pcrtOpens dd 0
pcrtSavedBusy dd 0
pcrtSavedMode dd 0
pcrtSavedSort dd 0
pcrtSavedDesc dd 0
pcrtSavedCount dd 0
pcrtSavedVisible dd 0
pcrtSavedSelection dd -1
pcrtSavedPending dd 0
pcrtSavedLoadError dd 0
pcrtSavedSaveError dd 0
pcrtWritten dd 0
pcrtHeaderCalls dd 0
pcrtSavedAction dd 0
pcrtSavedDpi dd 0
pcrtSavedTree dd 0
pcrtSavedFront dq 0
pcrtSavedTrack dq 0
pcrtSavedPublished dd 0
pcrtSavedPathReady dd 0
pcrtSavedHook dq 0
pcrtSavedNotice dq 0
pcrtFormat db 'Process columns route: exit=%u assertions=%u opens=%u',13,10,0
align 2
pcrtPrefix dw 'p','c','r',0
pcrtNameA dw 'R','o','u','t','e',' ','A',0
pcrtNameB dw 'R','o','u','t','e',' ','B',0
pcrtExpectedPid dw '2','0','2',0
pcrtExpectedParent dw '7','7','7',0
pcrtExpectedRead dw '4','2','9','4','9','6','7','2','9','7',0
.data?
align 8
pcrtSavedLayout db PC_LAYOUT_SIZE dup(?)
pcrtTestLayout db PC_LAYOUT_SIZE dup(?)
pcrtCaptured db PC_LAYOUT_SIZE dup(?)
pcrtReadback db PC_LAYOUT_SIZE dup(?)
pcrtRows db 8192 dup(?)
pcrtSavedMap dq 8192 dup(?)
pcrtSavedRemember db 32 dup(?)
pcrtSavedModeSort db 64 dup(?)
pcrtMessage db 48 dup(?)
pcrtSavedPath dw 1024 dup(?)
pcrtTemp dw 1024 dup(?)
pcrtPath dw 1024 dup(?)
pcrtNotify db 128 dup(?)
pcrtOut db PC_LAYOUT_SIZE dup(?)
.code

PcrtDialogHook PROC
  sub rsp,28h
  inc pcrtOpens
  mov rcx,pcDialogWindow
  mov edx,2
  call EndDialog
  add rsp,28h
  ret
PcrtDialogHook ENDP

PcrtCompare PROC
  push rsi
  push rdi
  mov rsi,rcx
  mov rdi,rdx
  mov ecx,PC_LAYOUT_SIZE
  repe cmpsb
  setne al
  movzx eax,al
  pop rdi
  pop rsi
  ret
PcrtCompare ENDP

; RCX physical header index, EDX notification code.
PcrtHeaderClick PROC
  sub rsp,28h
  mov dword ptr [pcrtNotify+16],edx
  mov dword ptr [pcrtNotify+28],ecx
  mov rcx,gList
  mov qword ptr [pcrtNotify],rcx
  mov dword ptr [pcrtNotify+8],4300
  mov rcx,gMain
  mov edx,4Eh
  mov r8d,0
  lea r9,pcrtNotify
  call SendMessageW
  add rsp,28h
  ret
PcrtHeaderClick ENDP

; Explicit owned path, never the production settings path.
PcrtMakePath PROC
  sub rsp,28h
  mov ecx,1024
  lea rdx,pcrtTemp
  call GetTempPathW
  test eax,eax
  jz pcrmpDone
  lea rcx,pcrtTemp
  lea rdx,pcrtPrefix
  xor r8d,r8d
  lea r9,pcrtPath
  call GetTempFileNameW
  test eax,eax
  jz pcrmpDone
  mov eax,1
pcrmpDone:
  add rsp,28h
  ret
PcrtMakePath ENDP

PcrtPeekCapture PROC
  sub rsp,28h
  mov [rsp+20h],rcx
  lea rcx,pcrtMessage
  mov rdx,gMain
  mov r8d,8063h
  mov r9d,8063h
  call PeekMessageW
  add rsp,28h
  ret
PcrtPeekCapture ENDP

PcrtTrackMenu PROC
  cmp edx,102h
  jne pcrtmBad
  cmp r8d,-17
  jne pcrtmBad
  cmp r9d,223
  jne pcrtmBad
  inc pcrtHeaderCalls
  mov eax,2502
  ret
pcrtmBad:
  xor eax,eax
  ret
PcrtTrackMenu ENDP

PcrtCheckSelection PROC
  sub rsp,28h
  mov rcx,gList
  mov edx,100Ch
  mov r8,-1
  mov r9d,2
  call SendMessageW
  cmp eax,visibleCount
  jae pcrtcsBad
  lea rdx,rowMap
  mov rax,[rdx+rax*8]
  cmp dword ptr [rax+3584],202
  jne pcrtcsBad
  cmp qword ptr [rax+3600],2
  jne pcrtcsBad
  xor eax,eax
  jmp pcrtcsDone
pcrtcsBad:
  mov eax,1
pcrtcsDone:
  add rsp,28h
  ret
PcrtCheckSelection ENDP

; ECX physical subitem, RDX expected literal; actual GETDISPINFO goes to main.
PcrtDisplay PROC
  push rbx
  sub rsp,20h
  mov rbx,rdx
  mov dword ptr [pcrtNotify+32],ecx
  mov rax,gList
  mov qword ptr [pcrtNotify],rax
  mov dword ptr [pcrtNotify+16],-177
  mov dword ptr [pcrtNotify+24],1
  mov dword ptr [pcrtNotify+28],0
  mov qword ptr [pcrtNotify+48],0
  mov rcx,gMain
  mov edx,4Eh
  xor r8d,r8d
  lea r9,pcrtNotify
  call SendMessageW
  mov rcx,qword ptr [pcrtNotify+48]
  test rcx,rcx
  jz pcrtdBad
  mov rdx,rbx
  call lstrcmpW
  jmp pcrtdDone
pcrtdBad:
  mov eax,1
pcrtdDone:
  add rsp,20h
  pop rbx
  ret
PcrtDisplay ENDP

PcUiRoutingTests PROC
  push rbx
  push rsi
  push rdi
  sub rsp,80h
  mov pcrtStage,7301
  mov eax,actionBusy
  mov pcrtSavedBusy,eax
  mov eax,frontMode
  mov pcrtSavedMode,eax
  mov eax,sortColumn
  mov pcrtSavedSort,eax
  mov eax,sortDescending
  mov pcrtSavedDesc,eax
  mov eax,frontCount
  mov pcrtSavedCount,eax
  mov eax,visibleCount
  mov pcrtSavedVisible,eax
  mov rax,frontBuffer
  mov pcrtSavedFront,rax
  mov eax,pcCapturePending
  mov pcrtSavedPending,eax
  mov eax,pcColumnsLoadError
  mov pcrtSavedLoadError,eax
  mov eax,pcColumnsSaveError
  mov pcrtSavedSaveError,eax
  mov eax,actionCode
  mov pcrtSavedAction,eax
  mov eax,uiDpi
  mov pcrtSavedDpi,eax
  mov eax,treeEnabled
  mov pcrtSavedTree,eax
  mov eax,publishedListCount
  mov pcrtSavedPublished,eax
  mov eax,rememberId
  mov dword ptr [pcrtSavedRemember],eax
  mov rax,rememberBirth
  mov qword ptr [pcrtSavedRemember+8],rax
  mov eax,rememberMode
  mov dword ptr [pcrtSavedRemember+16],eax
  mov eax,rememberItem
  mov dword ptr [pcrtSavedRemember+20],eax
  lea rsi,pcModeSort
  lea rdi,pcrtSavedModeSort
  mov ecx,8 ; eight DWORD sort IDs followed by eight direction DWORDs
  rep movsq
  mov rax,pcHeaderTrack
  mov pcrtSavedTrack,rax
  mov eax,pcColumnsPathReady
  mov pcrtSavedPathReady,eax
  mov rax,pcDialogHook
  mov pcrtSavedHook,rax
  mov rax,pcDialogSaveNotice
  mov pcrtSavedNotice,rax
  lea rsi,pcColumnsPath
  lea rdi,pcrtSavedPath
  mov ecx,256
  rep movsq
  lea rsi,rowMap
  lea rdi,pcrtSavedMap
  mov ecx,8192
  rep movsq
  mov ecx,PC_LAYOUT_SIZE/8
  lea rsi,pcLayout
  lea rdi,pcrtSavedLayout
  rep movsq
  mov rcx,gList
  mov edx,100Ch
  mov r8,-1
  mov r9d,2
  call SendMessageW
  mov pcrtSavedSelection,eax
  mov pcrtStage,7304
  mov pcColumnsPathReady,0
  mov pcCapturePending,0
  lea rax,PcrtDialogHook
  mov pcDialogHook,rax
  mov pcrtOpens,0
  mov frontMode,0
  mov actionBusy,0
  mov rcx,gMain
  mov edx,111h
  mov r8d,2502
  xor r9d,r9d
  call SendMessageW
  mov pcrtStage,7305
  cmp pcrtOpens,1
  jne pcrtFail
  inc pcrtAssertions
  mov actionBusy,1
  mov rcx,gMain
  mov edx,111h
  mov r8d,2502
  xor r9d,r9d
  call SendMessageW
  cmp pcrtOpens,2
  jne pcrtFail
  cmp actionBusy,1
  jne pcrtFail
  mov eax,actionCode
  cmp eax,pcrtSavedAction
  jne pcrtFail
  inc pcrtAssertions
  xor esi,esi
pcrtModes:
  mov frontMode,esi
  cmp esi,0
  je pcrtModeNext
  mov ebx,pcrtOpens
  mov rcx,gMain
  mov edx,111h
  mov r8d,2502
  xor r9d,r9d
  call SendMessageW
  cmp pcrtOpens,ebx
  jne pcrtFail
  inc pcrtAssertions
pcrtModeNext:
  mov rcx,gMain
  mov edx,117h
  xor r8d,r8d
  xor r9d,r9d
  call SendMessageW
  mov rcx,mainMenu
  mov edx,2502
  xor r8d,r8d
  call GetMenuState
  and eax,3
  test esi,esi
  jnz pcrtModeDisabled
  test eax,eax
  jnz pcrtFail
  jmp pcrtMenuPassed
pcrtModeDisabled:
  test eax,eax
  jz pcrtFail
pcrtMenuPassed:
  inc pcrtAssertions
  inc esi
  cmp esi,7
  jb pcrtModes
  mov frontMode,0
  mov actionBusy,0
  mov pcrtStage,7310
  lea rax,PcrtTrackMenu
  mov pcHeaderTrack,rax
  mov pcrtHeaderCalls,0
  mov rcx,processHeader
  mov edx,7Bh
  mov r8,rcx
  mov r9d,00DFFFEFh ; packed screen x=-17, y=223
  call SendMessageW
  cmp pcrtHeaderCalls,1
  jne pcrtFail
  cmp pcrtOpens,3
  jne pcrtFail
  inc pcrtAssertions
  mov treeEnabled,0
  mov uiDpi,96
  lea rcx,pcrtRows
  call ClearRow
  lea rcx,pcrtRows+4096
  call ClearRow
  lea rcx,pcrtRows
  lea rdx,pcrtNameA
  call lstrcpyW
  lea rcx,pcrtRows+4096
  lea rdx,pcrtNameB
  call lstrcpyW
  lea rcx,pcrtRows+4096+512
  lea rdx,pcrtExpectedPid
  call lstrcpyW
  mov dword ptr [pcrtRows+3584],101
  mov qword ptr [pcrtRows+3600],1
  mov dword ptr [pcrtRows+4096+3584],202
  mov qword ptr [pcrtRows+4096+3600],2
  mov dword ptr [pcrtRows+4096+3588],777
  mov dword ptr [pcrtRows+4096+3660],1
  mov rax,4294967297
  mov qword ptr [pcrtRows+4096+3640],rax
  lea rax,pcrtRows
  mov frontBuffer,rax
  mov frontCount,2
  ; Reorder real process columns so physical 1 means stable PID (ID6).
  lea rcx,pcrtTestLayout
  call PcDefaultLayout
  mov dword ptr [pcrtTestLayout+PC_L_COUNT],12
  mov dword ptr [pcrtTestLayout+PC_L_IDS+0],PC_NAME
  mov dword ptr [pcrtTestLayout+PC_L_IDS+4],PC_PID
  mov dword ptr [pcrtTestLayout+PC_L_IDS+8],PC_PARENT_PID
  mov dword ptr [pcrtTestLayout+PC_L_IDS+12],PC_PRIVATE_COMMIT
  mov dword ptr [pcrtTestLayout+PC_L_IDS+16],PC_CREATED_UTC
  mov dword ptr [pcrtTestLayout+PC_L_IDS+20],PC_READ_BYTES
  mov dword ptr [pcrtTestLayout+PC_L_IDS+24],PC_WRITE_BYTES
  mov dword ptr [pcrtTestLayout+PC_L_IDS+28],PC_STATUS
  mov dword ptr [pcrtTestLayout+PC_L_IDS+32],PC_CPU
  mov dword ptr [pcrtTestLayout+PC_L_IDS+36],PC_WORKING_SET
  mov dword ptr [pcrtTestLayout+PC_L_IDS+40],PC_IO_RATE
  mov dword ptr [pcrtTestLayout+PC_L_IDS+44],PC_THREADS
  lea rcx,pcrtTestLayout
  call PcApplyLayout
  test eax,eax
  jnz pcrtFail
  mov dword ptr [pcrtNotify+12],3
  mov dword ptr [pcrtNotify+16],3
  mov rcx,gList
  mov edx,102Bh
  mov r8d,1
  lea r9,pcrtNotify
  call SendMessageW
  mov pcrtStage,7320
  mov ecx,1
  mov edx,-108
  call PcrtHeaderClick
  cmp sortColumn,PC_PID
  jne pcrtFail
  inc pcrtAssertions
  call PcrtCheckSelection
  test eax,eax
  jnz pcrtFail
  mov ecx,8 ; previously rejected by hardcoded physical<7 guard.
  mov edx,-108
  call PcrtHeaderClick
  cmp sortColumn,PC_CPU
  jne pcrtFail
  call PcrtCheckSelection
  test eax,eax
  jnz pcrtFail
  inc pcrtAssertions
  ; Stable physical mapping and extra source-backed display callback.
  mov ecx,1
  call PcColumnId
  cmp eax,PC_PID
  jne pcrtFail
  inc pcrtAssertions
  ; Move selected PID202 to row0 using descending PID, then request text.
  mov ecx,1
  mov edx,-108
  call PcrtHeaderClick
  mov ecx,1
  mov edx,-108
  call PcrtHeaderClick
  mov ecx,1
  lea rdx,pcrtExpectedPid
  call PcrtDisplay
  test eax,eax
  jnz pcrtFail
  mov ecx,2
  lea rdx,pcrtExpectedParent
  call PcrtDisplay
  test eax,eax
  jnz pcrtFail
  mov ecx,5
  lea rdx,pcrtExpectedRead
  call PcrtDisplay
  test eax,eax
  jnz pcrtFail
  inc pcrtAssertions
  call PcrtMakePath
  test eax,eax
  jz pcrtFail
  mov pcColumnsPathReady,1
  lea rcx,pcColumnsPath
  lea rdx,pcrtPath
  call lstrcpyW
  ; Resize one real physical column, then exercise coalesced capture/persist.
  mov rcx,gList
  mov edx,101Eh
  mov r8d,1
  mov r9d,377
  call SendMessageW
  ; All earlier click/resize notifications share one deferred message. Drain
  ; that message, then prove two fresh queue requests post only one message.
pcrtDrainBeforeQueue:
  mov ecx,1
  call PcrtPeekCapture
  test eax,eax
  jnz pcrtDrainBeforeQueue
  mov pcCapturePending,0
  call PcQueueLayoutCapture
  call PcQueueLayoutCapture
  cmp pcCapturePending,1
  jne pcrtCaptureFail
  mov ecx,1
  call PcrtPeekCapture
  test eax,eax
  jz pcrtCaptureFail
  mov ecx,1
  call PcrtPeekCapture
  test eax,eax
  jnz pcrtCaptureFail
  add pcrtAssertions,2
  mov rcx,gMain
  mov edx,8063h
  xor r8d,r8d
  xor r9d,r9d
  call SendMessageW
  cmp pcColumnsSaveError,0
  jne pcrtCaptureFail
  inc pcrtAssertions
  cmp dword ptr [pcLayout+PC_L_WIDTHS+PC_PID*4],377
  jne pcrtCaptureFail
  cmp dword ptr [pcLayout+PC_L_SORT],PC_PID
  jne pcrtCaptureFail
  cmp dword ptr [pcLayout+PC_L_DESCENDING],1
  jne pcrtCaptureFail
  lea rcx,pcrtPath
  lea rdx,pcrtReadback
  call PcLoadLayout
  test eax,eax
  jnz pcrtCaptureFail
  lea rcx,pcrtReadback
  lea rdx,pcLayout
  call PcrtCompare
  test eax,eax
  jnz pcrtCaptureFail
  inc pcrtAssertions
  lea rsi,pcLayout
  lea rdi,pcrtCaptured
  mov ecx,PC_LAYOUT_SIZE/8
  rep movsq
  mov pcColumnsPathReady,0
  mov frontMode,2
  call SetModeColumns
  mov pcColumnsPathReady,1
  mov pcColumnsSaveError,12345
  mov pcrtStage,7302
  ; A stale queued capture in another mode cannot replace the process draft.
  mov rcx,gMain
  mov edx,8063h
  xor r8d,r8d
  xor r9d,r9d
  call SendMessageW
  lea rcx,pcrtCaptured
  lea rdx,pcLayout
  call PcrtCompare
  test eax,eax
  jnz pcrtFail
  cmp pcColumnsSaveError,12345
  jne pcrtFail
  inc pcrtAssertions
  jmp pcrtOk
pcrtCaptureFail:
  mov pcrtStage,7303
  jmp pcrtFail
pcrtOk:
  mov pcrtStage,0
pcrtFail:
  ; Restore saved model/layout and every route seam before returning.
  mov pcColumnsPathReady,0
  ; Remove this fixture's deferred capture before restoring the original path.
pcrtDrainCapture:
  lea rcx,pcrtMessage
  mov rdx,gMain
  mov r8d,8063h
  mov r9d,8063h
  mov qword ptr [rsp+20h],1
  call PeekMessageW
  test eax,eax
  jnz pcrtDrainCapture
  mov frontMode,0
  lea rcx,pcrtSavedLayout
  call PcApplyLayout
  mov eax,pcrtSavedBusy
  mov actionBusy,eax
  mov eax,pcrtSavedMode
  mov frontMode,eax
  mov eax,pcrtSavedSort
  mov sortColumn,eax
  mov eax,pcrtSavedDesc
  mov sortDescending,eax
  mov eax,pcrtSavedDpi
  mov uiDpi,eax
  mov eax,pcrtSavedTree
  mov treeEnabled,eax
  mov rax,pcrtSavedFront
  mov frontBuffer,rax
  mov eax,pcrtSavedCount
  mov frontCount,eax
  mov pcApplying,1
  call SetModeColumns
  call PcUpdateMenu
  mov eax,pcrtSavedVisible
  mov visibleCount,eax
  lea rsi,pcrtSavedMap
  lea rdi,rowMap
  mov ecx,8192
  rep movsq
  mov rcx,gList
  mov edx,102Fh
  mov r8d,pcrtSavedVisible
  xor r9d,r9d
  call SendMessageW
  mov dword ptr [pcrtNotify+12],0
  mov dword ptr [pcrtNotify+16],3
  mov rcx,gList
  mov edx,102Bh
  mov r8,-1
  lea r9,pcrtNotify
  call SendMessageW
  mov eax,pcrtSavedSelection
  cmp eax,-1
  je pcrtRestoreNoSelection
  mov dword ptr [pcrtNotify+12],3
  mov rcx,gList
  mov edx,102Bh
  mov r8d,eax
  lea r9,pcrtNotify
  call SendMessageW
pcrtRestoreNoSelection:
  lea rsi,pcrtSavedModeSort
  lea rdi,pcModeSort
  mov ecx,8
  rep movsq
  mov eax,pcrtSavedPublished
  mov publishedListCount,eax
  mov eax,dword ptr [pcrtSavedRemember]
  mov rememberId,eax
  mov rax,qword ptr [pcrtSavedRemember+8]
  mov rememberBirth,rax
  mov eax,dword ptr [pcrtSavedRemember+16]
  mov rememberMode,eax
  mov eax,dword ptr [pcrtSavedRemember+20]
  mov rememberItem,eax
  lea rsi,pcrtSavedPath
  lea rdi,pcColumnsPath
  mov ecx,256
  rep movsq
  mov eax,pcrtSavedPending
  mov pcCapturePending,eax
  test eax,eax
  jz pcrtRestoreNotPending
  mov rcx,gMain
  mov edx,8063h
  xor r8d,r8d
  xor r9d,r9d
  call PostMessageW
pcrtRestoreNotPending:
  mov eax,pcrtSavedLoadError
  mov pcColumnsLoadError,eax
  mov eax,pcrtSavedSaveError
  mov pcColumnsSaveError,eax
  mov rax,pcrtSavedTrack
  mov pcHeaderTrack,rax
  mov eax,pcrtSavedPathReady
  mov pcColumnsPathReady,eax
  mov rax,pcrtSavedHook
  mov pcDialogHook,rax
  mov rax,pcrtSavedNotice
  mov pcDialogSaveNotice,rax
  lea rcx,pcrtPath
  call DeleteFileW
  lea rcx,pcrtOut
  lea rdx,pcrtFormat
  mov r8d,pcrtStage
  mov r9d,pcrtAssertions
  mov eax,pcrtOpens
  mov [rsp+20h],rax
  call wsprintfA
  mov ebx,eax
  mov ecx,-11
  call GetStdHandle
  mov rcx,rax
  lea rdx,pcrtOut
  mov r8d,ebx
  lea r9,pcrtWritten
  mov qword ptr [rsp+20h],0
  call WriteFile
  mov eax,pcrtStage
  add rsp,80h
  pop rdi
  pop rsi
  pop rbx
  ret
PcUiRoutingTests ENDP
