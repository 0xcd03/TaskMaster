; Isolated animation lifecycle + offscreen artwork regression.
; Uses only the hidden test window. Never queues an optimization or trims RAM.
extern IsWindowEnabled:proc, CreateDIBSection:proc, GdiFlush:proc
extern SetViewportOrgEx:proc
.data
memoryAnimationTestStage dd 0
memoryAnimationTestAssertions dd 0
memoryAnimationPreviewPath dw 'w','o','r','k','\','m','e','m','o','r','y','-','a','n','i','m','a','t','i','o','n','-','p','r','e','v','i','e','w','.','b','m','p',0
memoryAnimationPreviewHeader db 'B','M'
dd 99126 ; 54-byte header + 688*36*4
dw 0,0
dd 54
memoryAnimationPreviewInfo dd 40,688,-36
dw 1,32
dd 0,99072,0,0,0,0
.data?
memoryAnimationTestSaved dd 8 dup(?)
memoryAnimationTestDc dq ?
memoryAnimationTestBitmap dq ?
memoryAnimationTestOldBitmap dq ?
memoryAnimationTestBits dq ?
memoryAnimationTestBytes dd ?
.code
MemoryAnimationTests PROC
  push rbx
  push rsi
  sub rsp,38h
  mov eax,actionBusy
  mov memoryAnimationTestSaved,eax
  mov eax,actionCode
  mov memoryAnimationTestSaved+4,eax
  mov eax,actionFailed
  mov memoryAnimationTestSaved+8,eax
  mov eax,actionResult
  mov memoryAnimationTestSaved+12,eax
  mov eax,reportAvailable
  mov memoryAnimationTestSaved+16,eax
  mov eax,reportExpanded
  mov memoryAnimationTestSaved+20,eax
  mov memoryAnimationTestStage,180
  mov actionBusy,1
  mov actionCode,2005
  mov actionFailed,0
  mov actionResult,0
  call StartMemoryAnimation
  cmp memoryAnimationState,1
  jne matestCleanup
  cmp memoryAnimationFrame,0
  jne matestCleanup
  mov rcx,memoryButton
  call IsWindowEnabled
  test eax,eax
  jnz matestCleanup
  inc memoryAnimationTestAssertions
  mov memoryAnimationTestStage,181
  ; Exercise actual WM_TIMER dispatch while busy, not the painter alone.
  xor ebx,ebx
matestTicks:
  mov rcx,shellWnd
  mov edx,113h
  mov r8d,902
  xor r9d,r9d
  call SendMessageW
  inc ebx
  cmp ebx,8
  jb matestTicks
  cmp memoryAnimationFrame,8
  jne matestCleanup
  cmp memoryAnimationState,1
  jne matestCleanup
  cmp actionBusy,1
  jne matestCleanup
  inc memoryAnimationTestAssertions
  ; Actual completion message owns the transition and re-enables the button.
  mov memoryAnimationTestStage,182
  mov rcx,gMain
  mov edx,8002h
  xor r8d,r8d
  xor r9d,r9d
  call SendMessageW
  cmp memoryAnimationState,2
  jne matestCleanup
  cmp actionBusy,0
  jne matestCleanup
  mov rcx,memoryButton
  call IsWindowEnabled
  test eax,eax
  jz matestCleanup
  cmp memoryAnimationEnd,0
  je matestCleanup
  inc memoryAnimationTestAssertions
  mov memoryAnimationTestStage,183
  mov memoryAnimationEnd,0
  call MemoryAnimationTick
  cmp memoryAnimationState,0
  jne matestCleanup
  mov ebx,memoryAnimationFrame
  call MemoryAnimationTick
  cmp ebx,memoryAnimationFrame
  jne matestCleanup
  inc memoryAnimationTestAssertions
  mov memoryAnimationTestStage,184
  mov actionBusy,1
  mov actionFailed,1
  mov actionResult,5
  call StartMemoryAnimation
  mov rcx,gMain
  mov edx,8002h
  xor r8d,r8d
  xor r9d,r9d
  call SendMessageW
  cmp memoryAnimationState,3
  jne matestCleanup
  mov memoryAnimationEnd,0
  call MemoryAnimationTick
  cmp memoryAnimationState,0
  jne matestCleanup
  inc memoryAnimationTestAssertions
  mov memoryAnimationTestStage,185
  call RenderMemoryAnimationPreview
  test eax,eax
  jz matestCleanup
  inc memoryAnimationTestAssertions
  mov memoryAnimationTestStage,0
matestCleanup:
  mov rcx,shellWnd
  mov edx,902
  call KillTimer
  mov memoryAnimationState,0
  mov memoryAnimationFrame,0
  mov memoryAnimationEnd,0
  mov eax,memoryAnimationTestSaved
  mov actionBusy,eax
  mov eax,memoryAnimationTestSaved+4
  mov actionCode,eax
  mov eax,memoryAnimationTestSaved+8
  mov actionFailed,eax
  mov eax,memoryAnimationTestSaved+12
  mov actionResult,eax
  mov eax,memoryAnimationTestSaved+16
  mov reportAvailable,eax
  mov eax,memoryAnimationTestSaved+20
  mov reportExpanded,eax
  mov shellButtonBusy,-1
  call RefreshShellMemory
  mov rcx,reportWnd
  xor edx,edx
  cmp reportAvailable,0
  je matestPanel
  mov edx,5
matestPanel:
  call ShowWindow
  call LayoutContent
  mov eax,memoryAnimationTestStage
  add rsp,38h
  pop rsi
  pop rbx
  ret
MemoryAnimationTests ENDP

; Produces a real GDI-rendered image of four states, using the production painter.
; Returns BOOL. File contains no screenshot, user data or app process metrics.
RenderMemoryAnimationPreview PROC
  push rbx
  push rsi
  push rdi
  sub rsp,50h
  mov memoryAnimationTestDc,0
  mov memoryAnimationTestBitmap,0
  mov memoryAnimationTestOldBitmap,0
  mov dword ptr [rsp+40h],0 ; return BOOL
  xor ecx,ecx
  call CreateCompatibleDC
  mov memoryAnimationTestDc,rax
  test rax,rax
  jz rmapCleanup
  mov rcx,rax
  lea rdx,memoryAnimationPreviewInfo
  xor r8d,r8d
  lea r9,memoryAnimationTestBits
  mov qword ptr [rsp+20h],0
  mov qword ptr [rsp+28h],0
  call CreateDIBSection
  mov memoryAnimationTestBitmap,rax
  test rax,rax
  jz rmapCleanup
  mov rcx,memoryAnimationTestDc
  mov rdx,rax
  call SelectObject
  mov memoryAnimationTestOldBitmap,rax
  mov dword ptr [rsp+30h],0
  mov dword ptr [rsp+34h],0
  mov dword ptr [rsp+38h],688
  mov dword ptr [rsp+3Ch],36
  mov rcx,memoryAnimationTestDc
  lea rdx,[rsp+30h]
  mov r8,shellAccentBrush
  call FillRect
  xor ebx,ebx
rmapFrame:
  mov rcx,memoryAnimationTestDc
  call SaveDC
  mov esi,eax
  mov rcx,memoryAnimationTestDc
  imul edx,ebx,172
  xor r8d,r8d
  xor r9d,r9d
  call SetViewportOrgEx
  mov rcx,memoryAnimationTestDc
  xor edx,edx
  xor r8d,r8d
  mov r9d,172
  mov qword ptr [rsp+20h],36
  call IntersectClipRect
  mov memoryAnimationState,1
  mov memoryAnimationFrame,0
  test ebx,ebx
  jz rmapDraw
  mov memoryAnimationFrame,8
  cmp ebx,1
  je rmapDraw
  mov memoryAnimationState,ebx
rmapDraw:
  mov rcx,memoryAnimationTestDc
  call DrawMemoryAnimation
  mov rcx,memoryAnimationTestDc
  mov edx,esi
  call RestoreDC
  inc ebx
  cmp ebx,4
  jb rmapFrame
  call GdiFlush
  ; Same geometry/background, different animation frame must change pixels.
  mov rbx,memoryAnimationTestBits
  xor esi,esi
rmapCompareRow:
  xor edi,edi
rmapComparePixel:
  mov eax,[rbx+rdi*4]
  cmp eax,[rbx+rdi*4+688]
  jne rmapDifferent
  inc edi
  cmp edi,172
  jb rmapComparePixel
  add rbx,2752
  inc esi
  cmp esi,36
  jb rmapCompareRow
  jmp rmapCleanup
rmapDifferent:
  lea rcx,memoryAnimationPreviewPath
  mov edx,40000000h ; GENERIC_WRITE
  xor r8d,r8d
  xor r9d,r9d
  mov qword ptr [rsp+20h],2 ; CREATE_ALWAYS
  mov qword ptr [rsp+28h],80h
  mov qword ptr [rsp+30h],0
  call CreateFileW
  mov rbx,rax
  cmp rax,-1
  je rmapCleanup
  mov rcx,rbx
  lea rdx,memoryAnimationPreviewHeader
  mov r8d,54
  lea r9,memoryAnimationTestBytes
  mov qword ptr [rsp+20h],0
  call WriteFile
  test eax,eax
  jz rmapClose
  cmp memoryAnimationTestBytes,54
  jne rmapClose
  mov rcx,rbx
  mov rdx,memoryAnimationTestBits
  mov r8d,99072
  lea r9,memoryAnimationTestBytes
  mov qword ptr [rsp+20h],0
  call WriteFile
  test eax,eax
  jz rmapClose
  cmp memoryAnimationTestBytes,99072
  jne rmapClose
  mov dword ptr [rsp+40h],1
rmapClose:
  mov rcx,rbx
  call CloseHandle
rmapCleanup:
  mov rcx,memoryAnimationTestDc
  mov rdx,memoryAnimationTestOldBitmap
  test rcx,rcx
  jz rmapBitmap
  test rdx,rdx
  jz rmapDeleteDc
  call SelectObject
rmapDeleteDc:
  mov rcx,memoryAnimationTestDc
  call DeleteDC
rmapBitmap:
  mov rcx,memoryAnimationTestBitmap
  test rcx,rcx
  jz rmapDone
  call DeleteObject
rmapDone:
  mov eax,[rsp+40h]
  add rsp,50h
  pop rdi
  pop rsi
  pop rbx
  ret
RenderMemoryAnimationPreview ENDP
