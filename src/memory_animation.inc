; UI-thread animation. The worker never paints or holds a UI lock.
extern MoveToEx:proc, LineTo:proc, Ellipse:proc
.data
memoryAnimationState dd 0 ; 0 idle, 1 running, 2 finished, 3 failed
memoryAnimationFrame dd 0
memoryAnimationEnd dq 0
memoryAnimationStreaks dd 8,7,20, 58,11,11, 130,6,16, 31,26,25, 95,30,18, 157,23,9
.data?
memoryAnimationProgressText dw 128 dup(?)
.code
StartMemoryAnimation PROC
  sub rsp,28h
  mov memoryAnimationState,1
  mov memoryAnimationFrame,0
  mov memoryTrimProgress,0
  mov memoryTrimTotal,0
  mov memoryAnimationEnd,0
  mov shellButtonBusy,-1
  call RefreshShellMemory
  mov rcx,shellWnd
  mov edx,902
  mov r8d,33
  xor r9d,r9d
  call SetTimer
  call RepaintMemoryAnimation
  ; First frame is visible before waking the memory worker.
  mov rcx,memoryButton
  call UpdateWindow
  add rsp,28h
  ret
StartMemoryAnimation ENDP

FinishMemoryAnimation PROC
  sub rsp,28h
  cmp memoryAnimationState,1
  jne fmaDone
  mov memoryAnimationState,2
  cmp actionFailed,0
  jne fmaFailed
  cmp actionResult,0
  je fmaResult
fmaFailed:
  mov memoryAnimationState,3
fmaResult:
  call GetTickCount64
  add rax,1000
  mov memoryAnimationEnd,rax
  mov shellButtonBusy,-1
  call RefreshShellMemory
  call RepaintMemoryAnimation
fmaDone:
  add rsp,28h
  ret
FinishMemoryAnimation ENDP

MemoryAnimationTick PROC
  sub rsp,28h
  cmp memoryAnimationState,0
  je matDone
  cmp memoryAnimationState,1
  je matFrame
  call GetTickCount64
  cmp rax,memoryAnimationEnd
  jb matDone
  mov memoryAnimationState,0
  mov rcx,shellWnd
  mov edx,902
  call KillTimer
  mov shellButtonBusy,-1
  call RefreshShellMemory
matFrame:
  inc memoryAnimationFrame
  call RepaintMemoryAnimation
matDone:
  add rsp,28h
  ret
MemoryAnimationTick ENDP

RepaintMemoryAnimation PROC
  sub rsp,28h
  mov rcx,memoryButton
  xor edx,edx
  xor r8d,r8d
  call InvalidateRect
  add rsp,28h
  ret
RepaintMemoryAnimation ENDP

; RCX HDC, already mapped to 96-DPI logical units; button is 172x36.
; Rendered with stock GDI objects, so frame ticks allocate no GDI resources.
DrawMemoryAnimation PROC
  push rbx
  push rsi
  push rdi
  sub rsp,40h
  mov rbx,rcx
  call SaveDC
  mov [rsp+30h],eax
  mov ecx,19 ; DC_PEN
  call GetStockObject
  mov rcx,rbx
  mov rdx,rax
  call SelectObject
  mov ecx,5 ; NULL_BRUSH
  call GetStockObject
  mov rcx,rbx
  mov rdx,rax
  call SelectObject
  cmp memoryAnimationState,1
  jne dmaFinished
  mov rcx,rbx
  mov edx,0C08C50h ; subdued blue speed streaks (COLORREF)
  call SetDCPenColor
  xor esi,esi
dmaStreak:
  lea rax,memoryAnimationStreaks
  mov edx,[rax+rsi]
  mov eax,memoryAnimationFrame
  imul eax,4
  sub edx,eax
  and edx,255
  mov edi,edx
  lea rax,memoryAnimationStreaks
  mov r8d,[rax+rsi+4]
  mov rcx,rbx
  xor r9d,r9d
  call MoveToEx
  lea rax,memoryAnimationStreaks
  mov edx,[rax+rsi+8]
  add edx,edi
  mov r8d,[rax+rsi+4]
  mov rcx,rbx
  call LineTo
  add esi,12
  cmp esi,72
  jb dmaStreak
  ; Central outlined rocket; rhythmic exhaust gives motion even on small DPI.
  mov rcx,rbx
  mov edx,0FFFFFFh
  call SetDCPenColor
  mov rcx,rbx
  mov edx,78
  mov r8d,14
  xor r9d,r9d
  call MoveToEx
  mov rcx,rbx
  mov edx,88
  mov r8d,12
  call LineTo
  mov rcx,rbx
  mov edx,96
  mov r8d,18
  call LineTo
  mov rcx,rbx
  mov edx,88
  mov r8d,24
  call LineTo
  mov rcx,rbx
  mov edx,78
  mov r8d,22
  call LineTo
  mov rcx,rbx
  mov edx,78
  mov r8d,14
  call LineTo
  mov rcx,rbx
  mov edx,84
  mov r8d,15
  mov r9d,90
  mov qword ptr [rsp+20h],21
  call Ellipse
  ; Upper/lower stabilizers.
  mov rcx,rbx
  mov edx,79
  mov r8d,14
  xor r9d,r9d
  call MoveToEx
  mov rcx,rbx
  mov edx,80
  mov r8d,9
  call LineTo
  mov rcx,rbx
  mov edx,86
  mov r8d,12
  call LineTo
  mov rcx,rbx
  mov edx,79
  mov r8d,22
  xor r9d,r9d
  call MoveToEx
  mov rcx,rbx
  mov edx,80
  mov r8d,27
  call LineTo
  mov rcx,rbx
  mov edx,86
  mov r8d,24
  call LineTo
  xor esi,esi
dmaExhaust:
  mov edx,memoryAnimationFrame
  add edx,esi
  and edx,3
  neg edx
  add edx,71
  lea r8d,[rsi+rsi*2+15]
  mov rcx,rbx
  xor r9d,r9d
  call MoveToEx
  mov rcx,rbx
  mov edx,75
  lea r8d,[rsi+rsi*2+15]
  call LineTo
  inc esi
  cmp esi,3
  jb dmaExhaust
  jmp dmaRestore
dmaFinished:
  mov rcx,rbx
  mov edx,0FFFFFFh
  call SetDCPenColor
  cmp memoryAnimationState,3
  je dmaFailure
  mov rcx,rbx
  mov edx,78
  mov r8d,18
  xor r9d,r9d
  call MoveToEx
  mov rcx,rbx
  mov edx,84
  mov r8d,24
  call LineTo
  mov rcx,rbx
  mov edx,95
  mov r8d,12
  call LineTo
  jmp dmaRestore
dmaFailure:
  mov rcx,rbx
  mov edx,86
  mov r8d,10
  xor r9d,r9d
  call MoveToEx
  mov rcx,rbx
  mov edx,86
  mov r8d,21
  call LineTo
  mov rcx,rbx
  mov edx,85
  mov r8d,25
  mov r9d,88
  mov qword ptr [rsp+20h],28
  call Ellipse
dmaRestore:
  mov rcx,rbx
  mov edx,[rsp+30h]
  call RestoreDC
  add rsp,40h
  pop rdi
  pop rsi
  pop rbx
  ret
DrawMemoryAnimation ENDP
