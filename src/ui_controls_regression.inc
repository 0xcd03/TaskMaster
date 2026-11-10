; Native controls with disposable in-memory rows. Never modifies real startup.
extern GetWindowLongPtrW:proc, GetWindowTextLengthW:proc, GetMenuState:proc
.data
uiStartupCapturedCode dd 0
uiStartupCapturedCount dd 0
uiControlsAssertions dd 0
uiControlsStage dd 0
uiControlsReport dw 'T','e','s','t',13,10,'D','e','t','a','i','l','s',13,10,'R','e','p','o','r','t',0
.data?
uiControlsRect RECT <>
.code
UiControlsRegression PROC
  push rbx
  push rsi
  sub rsp,38h
  mov uiControlsStage,140
  mov rcx,actionBannerDetails
  mov edx,-16
  call GetWindowLongPtrW
  test eax,10000000h ; no visible Details button before any action
  jnz ucrDone
  inc uiControlsAssertions
  mov frontMode,5
  mov visibleCount,3
  mov frontCount,3
  xor esi,esi
ucrRows:
  mov rbx,frontBuffer
  mov eax,esi
  shl rax,12
  add rbx,rax
  mov rcx,rbx
  call ClearRow
  lea rax,rowMap
  mov [rax+rsi*8],rbx
  mov word ptr [rbx],41h
  mov dword ptr [rbx+3624],1 ; controllable
  mov eax,esi
  mov [rbx+3616],eax ; enabled, disabled, read-only
  inc esi
  cmp esi,3
  jb ucrRows
  mov dword ptr [rbx+3624],0
  call SetModeColumns
  mov rcx,gList
  mov edx,102Fh ; LVM_SETITEMCOUNT
  mov r8d,3
  xor r9d,r9d
  call SendMessageW
  mov uiControlsStage,141
  mov rcx,gList
  mov edx,1037h ; LVM_GETEXTENDEDLISTVIEWSTYLE
  xor r8d,r8d
  xor r9d,r9d
  call SendMessageW
  test eax,4
  jz ucrDone
  inc uiControlsAssertions
  xor esi,esi
ucrStates:
  mov rcx,gList
  mov edx,102Ch ; LVM_GETITEMSTATE, exercises real GETDISPINFO callback
  mov r8d,esi
  mov r9d,0F000h
  call SendMessageW
  mov edx,2000h
  test esi,esi
  jz ucrCompareState
  mov edx,1000h
  cmp esi,1
  je ucrCompareState
  xor edx,edx
ucrCompareState:
  cmp eax,edx
  jne ucrDone
  inc uiControlsAssertions
  inc esi
  cmp esi,3
  jb ucrStates
  mov uiControlsStage,142
  ; A click on row 1 must enable that row, regardless of prior selection.
  mov dword ptr uiControlsRect,0 ; LVIR_BOUNDS
  mov rcx,gList
  mov edx,100Eh ; LVM_GETITEMRECT
  mov r8d,1
  lea r9,uiControlsRect
  call SendMessageW
  test eax,eax
  jz ucrDone
  mov eax,uiControlsRect.rcTop
  add eax,3
  shl eax,16
  or eax,6
  mov r9d,eax
  mov rcx,gList
  mov edx,201h
  mov r8d,1
  call SendMessageW
  cmp uiStartupCapturedCount,1
  jne ucrDone
  cmp uiStartupCapturedCode,2401
  jne ucrDone
  cmp startupActionDisabled,1
  jne ucrDone
  inc uiControlsAssertions
  mov ecx,0
  call ToggleStartupCheckbox
  cmp uiStartupCapturedCount,2
  jne ucrDone
  cmp uiStartupCapturedCode,2400
  jne ucrDone
  cmp startupActionDisabled,0
  jne ucrDone
  inc uiControlsAssertions
  ; Space goes through the native list subclass and current selection.
  mov rcx,gList
  mov edx,100h
  mov r8d,20h
  xor r9d,r9d
  call SendMessageW
  cmp uiStartupCapturedCount,3
  jne ucrDone
  cmp uiStartupCapturedCode,2400
  jne ucrDone
  inc uiControlsAssertions
  mov ecx,2
  call ToggleStartupCheckbox
  cmp uiStartupCapturedCount,3
  jne ucrDone
  inc uiControlsAssertions
  mov uiControlsStage,143
  lea rcx,actionMessage
  lea rdx,uiControlsReport
  call lstrcpyW
  mov rcx,gMain
  mov edx,8002h
  xor r8d,r8d
  xor r9d,r9d
  call SendMessageW
  cmp reportAvailable,1
  jne ucrDone
  cmp reportExpanded,0
  jne ucrDone
  mov rcx,actionBannerDetails
  mov edx,-16
  call GetWindowLongPtrW
  test eax,10000000h
  jz ucrDone
  inc uiControlsAssertions
  mov rcx,gMain
  mov edx,111h
  mov r8d,2600
  xor r9d,r9d
  call SendMessageW
  cmp reportExpanded,1
  jne ucrDone
  mov rcx,actionDetailsText
  mov edx,-16
  call GetWindowLongPtrW
  and eax,10200804h ; VISIBLE | VSCROLL | READONLY | MULTILINE
  cmp eax,10200804h
  jne ucrDone
  mov rcx,actionDetailsText
  call GetWindowTextLengthW
  cmp eax,21
  jb ucrDone
  mov rcx,actionDetailsText
  lea rdx,uiControlsRect
  call GetClientRect
  mov eax,uiControlsRect.rcBottom
  cmp eax,uiStatusHeight
  jle ucrDone
  inc uiControlsAssertions
  mov rcx,gMain
  mov edx,111h
  mov r8d,2600
  xor r9d,r9d
  call SendMessageW
  cmp reportExpanded,0
  jne ucrDone
  inc uiControlsAssertions
  mov reportAvailable,0
  mov rcx,actionBannerDetails
  xor edx,edx
  call ShowWindow
  mov rcx,actionBanner
  xor edx,edx
  call ShowWindow
  mov frontMode,0
  mov frontCount,0
  mov visibleCount,0
  call SetModeColumns
  call LayoutContent
  mov rcx,gList
  mov edx,102Fh
  xor r8d,r8d
  xor r9d,r9d
  call SendMessageW
  mov uiControlsStage,144
  mov rcx,gList
  mov edx,1037h
  xor r8d,r8d
  xor r9d,r9d
  call SendMessageW
  test eax,4
  jnz ucrDone
  inc uiControlsAssertions
  mov uiControlsStage,0
ucrDone:
  mov eax,uiControlsStage
  add rsp,38h
  pop rsi
  pop rbx
  ret
UiControlsRegression ENDP
