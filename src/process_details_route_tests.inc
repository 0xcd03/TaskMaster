; Runs only inside the existing hidden-main regression. Reads only this fixture
; process. Actual menu selection, WndProc and modeless creation remain real.
.data
align 8
pdrtOptionalCollector dq 0
pdrtName dw 'R','o','u','t','e',' ','f','i','x','t','u','r','e',0
.data?
align 8
pdrtItem db 88 dup(?)
pdrtTimes db 32 dup(?)
.code
PdrtHiddenHook PROC
  ret
PdrtHiddenHook ENDP

PdUiRoutingTests PROC
  push rbx
  push rsi
  push rdi
  sub rsp,40h
  mov ebx,7301
  mov rax,pdOptionalWorkerCollector
  mov pdrtOptionalCollector,rax
  lea rax,PdCollectNative ; lane2 returns50; routing never depends on WMI
  mov pdOptionalWorkerCollector,rax
  mov esi,actionBusy
  mov rax,pdUiInitHook
  mov [rsp+30h],rax
  lea rax,PdrtHiddenHook
  mov pdUiInitHook,rax
  mov frontMode,0
  mov frontCount,1
  mov visibleCount,1
  mov rdi,frontBuffer
  mov rcx,rdi
  call ClearRow
  mov rcx,rdi
  lea rdx,pdrtName
  call lstrcpyW
  call GetCurrentProcessId
  mov [rdi+3584],eax
  mov rcx,-1
  lea rdx,pdrtTimes
  lea r8,pdrtTimes+8
  lea r9,pdrtTimes+16
  lea rax,pdrtTimes+24
  mov [rsp+20h],rax
  call GetProcessTimes
  test eax,eax
  jz pdrtDone
  mov rax,qword ptr [pdrtTimes]
  mov [rdi+3600],rax
  mov qword ptr [rowMap],rdi
  call SetModeColumns
  mov rcx,gList
  mov edx,102Fh
  mov r8d,1
  xor r9d,r9d
  call SendMessageW
  lea rdi,pdrtItem
  xor eax,eax
  mov ecx,11
  rep stosq
  mov dword ptr [pdrtItem+12],3
  mov dword ptr [pdrtItem+16],3
  mov rcx,gList
  mov edx,102Bh ; LVM_SETITEMSTATE
  xor r8d,r8d
  lea r9,pdrtItem
  call SendMessageW
  mov rcx,gMain
  mov edx,111h
  mov r8d,2501
  xor r9d,r9d
  call SendMessageW
  mov ebx,7302
  cmp pdWindow,0
  je pdrtDone
  mov rax,pdCookieCounter
  mov [rsp+38h],rax
  mov actionBusy,1
  mov rcx,gMain
  mov edx,111h
  mov r8d,2501
  xor r9d,r9d
  call SendMessageW
  mov ebx,7303
  mov rax,pdCookieCounter
  cmp rax,[rsp+38h]
  jne pdrtDone
  cmp actionBusy,1
  jne pdrtDone
  mov frontMode,2
  mov rcx,gMain
  mov edx,111h
  mov r8d,2501
  xor r9d,r9d
  call SendMessageW
  mov ebx,7304
  mov rax,pdCookieCounter
  cmp rax,[rsp+38h]
  jne pdrtDone
  mov rcx,gMain
  mov edx,117h
  xor r8d,r8d
  xor r9d,r9d
  call SendMessageW
  mov rcx,mainMenu
  mov edx,2501
  xor r8d,r8d
  call GetMenuState
  test eax,3
  jz pdrtDone
  xor ebx,ebx
pdrtDone:
  mov rcx,pdWindow
  test rcx,rcx
  jz pdrtRestore
  call DestroyWindow
pdrtRestore:
  mov rax,pdrtOptionalCollector
  mov pdOptionalWorkerCollector,rax
  mov actionBusy,esi
  mov rax,[rsp+30h]
  mov pdUiInitHook,rax
  mov frontMode,0
  mov frontCount,0
  mov visibleCount,0
  mov qword ptr [rowMap],0
  call SetModeColumns
  mov rcx,gList
  mov edx,102Fh
  xor r8d,r8d
  xor r9d,r9d
  call SendMessageW
  mov eax,ebx
  add rsp,40h
  pop rdi
  pop rsi
  pop rbx
  ret
PdUiRoutingTests ENDP
