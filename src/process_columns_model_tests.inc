; Explicit isolated working-directory fixtures; no LoadSettings/user preference paths.
extern lstrlenA:proc
.data
align 8
pctExpected dd 544,3,7,0,0,0,0,1,2,3,4,5,6
  dd 57 dup(-1)
  dd 240,130,76,120,125,90,68,90,150,190,140,140,180,360,480,90
  dd 48 dup(90)
  dd 0,0
; offset/value invalidations cover every invariant, including hidden widths.
pctBadCases dd 0,543, 4,2, 8,1, 8,21, 12,7, 12,20, 16,2, 20,1
  dd 24,1, 28,0, 28,20, 48,7, 52,0, 276,0, 280,31, 532,4097, 536,1, 540,1
pctBadCaseCount equ ($-pctBadCases)/8
pctStage dd 7200
pctPass db 'Process columns model/store PASS',13,10,0
pctLoadPass db 'Process columns independent load PASS',13,10,0
pctBadPass db 'Process columns malformed load preserved output PASS',13,10,0
align 2
pctPath dw 'l','a','y','o','u','t','.','b','i','n',0
pctMaxPath dw 'm','a','x','i','m','a','l','.','b','i','n',0
pctInvalidPath dw 'i','n','v','a','l','i','d','.','b','i','n',0
pctMissingPath dw 'm','i','s','s','i','n','g','-','d','i','r','\','l','a','y','o','u','t','.','b','i','n',0
.data?
align 8
pctLayout db PC_LAYOUT_SIZE dup(?)
pctGuardBefore dq ?
pctLoaded db PC_LAYOUT_SIZE dup(?)
pctGuardAfter dq ?
pctRaw db PC_LAYOUT_SIZE dup(?)
pctPathLong dw 1025 dup(?)
pctWritten dd ?
.code
; compare 128 bytes RCX/RDX, EAX=0 equal, 1 different.
PctEqual PROC
  xor r8d,r8d
pteLoop:
  mov rax,[rcx+r8]
  cmp rax,[rdx+r8]
  jne pteBad
  add r8d,8
  cmp r8d,PC_LAYOUT_SIZE
  jb pteLoop
  xor eax,eax
  ret
pteBad:
  mov eax,1
  ret
PctEqual ENDP

PctPrint PROC
  push rbx
  sub rsp,30h
  mov rbx,rcx
  call lstrlenA
  mov [rsp+28h],eax
  mov ecx,-11
  call GetStdHandle
  mov rcx,rax
  mov rdx,rbx
  mov r8d,[rsp+28h]
  lea r9,pctWritten
  mov qword ptr [rsp+20h],0
  call WriteFile
  add rsp,30h
  pop rbx
  ret
PctPrint ENDP

ProcessColumnsModelTest PROC
  push rbx
  push rsi
  push rdi
  sub rsp,40h
  lea rcx,pctLayout
  call PcDefaultLayout
  cmp dword ptr [pctLayout],544
  jne pctFinish
  lea rcx,pctLayout
  lea rdx,pctExpected
  call PctEqual
  test eax,eax
  jnz pctFinish
  inc pctStage
  lea rcx,pctLayout
  call PcValidateLayout
  test eax,eax
  jnz pctFinish
  inc pctStage
  call PcEnsureLayout
  lea rcx,pcLayout
  lea rdx,pctExpected
  call PctEqual
  test eax,eax
  jnz pctFinish
  mov dword ptr [pcLayout+PC_L_WIDTHS],4096
  call PcEnsureLayout
  cmp dword ptr [pcLayout+PC_L_WIDTHS],4096
  jne pctFinish
  inc pctStage
  lea rcx,pctPath
  lea rdx,pctLayout
  call PcSaveLayout
  test eax,eax
  jnz pctFinish
  ; Invalid saves must validate before touching existing target bytes.
  xor esi,esi
pctInvalidLoop:
  lea r10,pctBadCases
  mov ebx,[r10+rsi*8]
  lea r10,pctBadCases
  mov eax,[r10+rsi*8+4]
  lea r10,pctLayout
  mov edi,[r10+rbx]
  lea r10,pctLayout
  mov [r10+rbx],eax
  lea rcx,pctLayout
  call PcValidateLayout
  cmp eax,87
  jne pctFinish
  lea rcx,pctPath
  lea rdx,pctLayout
  call PcSaveLayout
  cmp eax,87
  jne pctFinish
  lea r10,pctLayout
  mov [r10+rbx],edi
  inc esi
  inc pctStage
  cmp esi,pctBadCaseCount
  jb pctInvalidLoop
  ; Both inclusive width bounds accepted for all twelve slots.
  xor esi,esi
pctWidthLoop:
  lea r10,pctLayout
  mov edi,[r10+rsi*4+PC_L_WIDTHS]
  lea r10,pctLayout
  mov dword ptr [r10+rsi*4+PC_L_WIDTHS],32
  lea rcx,pctLayout
  call PcValidateLayout
  test eax,eax
  jnz pctFinish
  lea r10,pctLayout
  mov dword ptr [r10+rsi*4+PC_L_WIDTHS],4096
  lea rcx,pctLayout
  call PcValidateLayout
  test eax,eax
  jnz pctFinish
  lea r10,pctLayout
  mov [r10+rsi*4+PC_L_WIDTHS],edi
  inc esi
  cmp esi,PC_LAYOUT_CAPACITY
  jb pctWidthLoop
  ; Minimal legal layout and maximal legal layout, with a non-name sort.
  lea rcx,pctLayout
  call PcDefaultLayout
  mov dword ptr [pctLayout+PC_L_COUNT],2
  mov dword ptr [pctLayout+PC_L_SORT],6
  mov dword ptr [pctLayout+PC_L_DESCENDING],1
  mov dword ptr [pctLayout+PC_L_IDS+4],6
  mov esi,2
pctTwoUnused:
  lea r10,pctLayout
  mov dword ptr [r10+rsi*4+PC_L_IDS],-1
  inc esi
  cmp esi,PC_LAYOUT_CAPACITY
  jb pctTwoUnused
  lea rcx,pctLayout
  call PcValidateLayout
  test eax,eax
  jnz pctFinish
  mov dword ptr [pctLayout+PC_L_COUNT],PC_COLUMN_COUNT
  mov dword ptr [pctLayout+PC_L_SORT],PC_GPU_SHARED
  xor esi,esi
pctTwelveIds:
  lea r10,pctLayout
  mov dword ptr [r10+rsi*4+PC_L_IDS],esi
  inc esi
  cmp esi,PC_COLUMN_COUNT
  jb pctTwelveIds
  lea rcx,pctLayout
  call PcValidateLayout
  test eax,eax
  jnz pctFinish
  lea rcx,pctMaxPath
  lea rdx,pctLayout
  call PcSaveLayout
  test eax,eax
  jnz pctFinish
  lea rcx,pctMaxPath
  lea rdx,pctLoaded
  call PcLoadLayout
  test eax,eax
  jnz pctFinish
  lea rcx,pctLoaded
  lea rdx,pctLayout
  call PctEqual
  test eax,eax
  jnz pctFinish
  lea rcx,pctLayout
  call PcDefaultLayout
  lea rcx,pctMissingPath
  lea rdx,pctLayout
  call PcSaveLayout
  test eax,eax
  jz pctFinish
  inc pctStage
  ; Block destination replacement with our own opened handle (no delete sharing).
  lea rcx,pctPath
  mov edx,80000000h
  mov r8d,1
  xor r9d,r9d
  mov qword ptr [rsp+20h],3
  mov qword ptr [rsp+28h],80h
  mov qword ptr [rsp+30h],0
  call CreateFileW
  mov rbx,rax
  cmp rax,-1
  je pctFinish
  lea r10,pctLayout
  mov dword ptr [r10+PC_L_WIDTHS],241
  lea rcx,pctPath
  lea rdx,pctLayout
  call PcSaveLayout
  mov esi,eax
  mov rcx,rbx
  call CloseHandle
  test esi,esi
  jz pctFinish
  lea r10,pctLayout
  mov dword ptr [r10+PC_L_WIDTHS],240
  lea rcx,pctPath
  lea rdx,pctLoaded
  call PcLoadLayout
  test eax,eax
  jnz pctFinish
  lea rcx,pctLoaded
  lea rdx,pctExpected
  call PctEqual
  test eax,eax
  jnz pctFinish
  ; Successful replacement (as well as initial creation) is independently loaded.
  mov dword ptr [pctLayout+PC_L_WIDTHS],241
  lea rcx,pctPath
  lea rdx,pctLayout
  call PcSaveLayout
  test eax,eax
  jnz pctFinish
  lea rcx,pctPath
  lea rdx,pctLoaded
  call PcLoadLayout
  test eax,eax
  jnz pctFinish
  lea rcx,pctLoaded
  lea rdx,pctLayout
  call PctEqual
  test eax,eax
  jnz pctFinish
  mov dword ptr [pctLayout+PC_L_WIDTHS],240
  lea rcx,pctPath
  lea rdx,pctLayout
  call PcSaveLayout
  test eax,eax
  jnz pctFinish
  inc pctStage
  ; Explicit path cap: 1024 non-NUL characters rejected before API access.
  xor eax,eax
pctLongLoop:
  lea r10,pctPathLong
  mov word ptr [r10+rax*2],'x'
  inc eax
  cmp eax,1024
  jb pctLongLoop
  lea r10,pctPathLong
  mov word ptr [r10+2048],0
  lea rcx,pctPathLong
  lea rdx,pctLayout
  call PcSaveLayout
  cmp eax,87
  jne pctFinish
  lea rcx,pctPathLong
  lea rdx,pctLoaded
  call PcLoadLayout
  cmp eax,87
  jne pctFinish
  lea rcx,pctPass
  call PctPrint
  mov pctStage,0
pctFinish:
  mov ecx,pctStage
  call ExitProcess
ProcessColumnsModelTest ENDP

ProcessColumnsLoadTest PROC
  sub rsp,28h
  lea rcx,pctPath
  lea rdx,pctLoaded
  call PcLoadLayout
  test eax,eax
  jnz pctLoadFail
  lea rcx,pctLoaded
  lea rdx,pctExpected
  call PctEqual
  test eax,eax
  jnz pctLoadFail
  lea rcx,pctLoadPass
  call PctPrint
  xor ecx,ecx
  call ExitProcess
pctLoadFail:
  mov ecx,7290
  call ExitProcess
ProcessColumnsLoadTest ENDP

ProcessColumnsBadLoadTest PROC
  sub rsp,28h
  mov pctGuardBefore,13579h
  mov pctGuardAfter,24680h
  lea rcx,pctLoaded
  call PcDefaultLayout
  lea rcx,pctInvalidPath
  lea rdx,pctLoaded
  call PcLoadLayout
  test eax,eax
  jz pctBadFail
  cmp pctGuardBefore,13579h
  jne pctBadFail
  cmp pctGuardAfter,24680h
  jne pctBadFail
  lea rcx,pctLoaded
  lea rdx,pctExpected
  call PctEqual
  test eax,eax
  jnz pctBadFail
  lea rcx,pctBadPass
  call PctPrint
  xor ecx,ecx
  call ExitProcess
pctBadFail:
  mov ecx,7291
  call ExitProcess
ProcessColumnsBadLoadTest ENDP

.data
pctLegacyPath dw 'l','e','g','a','c','y','.','b','i','n',0
pctConvertedPath dw 'c','o','n','v','e','r','t','e','d','.','b','i','n',0
.code
ProcessColumnsMigrationTest PROC
 sub rsp,28h
 mov pctGuardBefore,13579h
 mov pctGuardAfter,24680h
 lea rcx,pctLegacyPath
 lea rdx,pctLoaded
 call PcLoadLayout
 test eax,eax
 jnz pctMigrationFail
 cmp dword ptr [pctLoaded],544
 jne pctMigrationFail
 cmp dword ptr [pctLoaded+4],3
 jne pctMigrationFail
 cmp pctGuardBefore,13579h
 jne pctMigrationFail
 cmp pctGuardAfter,24680h
 jne pctMigrationFail
 lea rcx,pctConvertedPath
 lea rdx,pctLoaded
 call PcSaveLayout
 mov ecx,eax
 call ExitProcess
pctMigrationFail:
 mov ecx,7292
 call ExitProcess
ProcessColumnsMigrationTest ENDP
