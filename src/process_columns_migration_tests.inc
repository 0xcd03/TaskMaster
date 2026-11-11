; Isolated working-directory fixtures; never resolves settings/user preference paths.
extern lstrcmpW:proc
.data
pcmNew dw 'p','r','o','c','e','s','s','-','c','o','l','u','m','n','s','-','v','3','.','b','i','n',0
pcmV2 dw 'p','r','o','c','e','s','s','-','c','o','l','u','m','n','s','-','v','2','.','b','i','n',0
pcmOld dw 'p','r','o','c','e','s','s','-','c','o','l','u','m','n','s','-','v','1','.','b','i','n',0
pcmExpectedPath dw 'e','x','p','e','c','t','e','d','.','b','i','n',0
pcmMissingNew dw 'm','i','s','s','i','n','g','-','d','i','r','\','v','2','.','b','i','n',0
pcmUseMissing dd 0
pcmPass db 'Process column migration paths PASS',13,10,0
.data?
align 8
pcmExpected db (PC_LAYOUT_SIZE+4) dup(?) ; Win32 error followed by independently authored V2 bytes.
pcmBefore dq ?
pcmActual db PC_LAYOUT_SIZE dup(?)
pcmAfter dq ?
pcmBytes dd ?
.code
PcmReadExpected PROC
  push rbx
  sub rsp,40h
  lea rcx,pcmExpectedPath
  mov edx,80000000h
  mov r8d,1
  xor r9d,r9d
  mov qword ptr [rsp+20h],3
  mov qword ptr [rsp+28h],80h
  mov qword ptr [rsp+30h],0
  call CreateFileW
  cmp rax,-1
  je pcmFixtureBad
  mov rbx,rax
  mov rcx,rbx
  lea rdx,pcmExpected
  mov r8d,PC_LAYOUT_SIZE+4
  lea r9,pcmBytes
  mov qword ptr [rsp+20h],0
  call ReadFile
  mov rcx,rbx
  call CloseHandle
  cmp pcmBytes,PC_LAYOUT_SIZE+4
  jne pcmFixtureBad
  add rsp,40h
  pop rbx
  ret
pcmFixtureBad:
  mov ecx,7399
  call ExitProcess
PcmReadExpected ENDP

ProcessColumnsMigrationPathsTest PROC
  sub rsp,28h
  call PcmReadExpected
  mov pcmBefore,13579h
  mov pcmAfter,24680h
  lea rcx,pcmActual
  call PcDefaultLayout
  ; A nondefault existing destination must survive any error byte-for-byte.
  mov dword ptr [pcmActual+PC_L_WIDTHS],777
  lea rcx,pcColumnsV2Path
  lea rdx,pcmV2
  call lstrcpyW
  lea rcx,pcmNew
  cmp pcmUseMissing,0
  je pcmPathsCall
  lea rcx,pcmMissingNew
pcmPathsCall:
  lea rdx,pcmOld
  lea r8,pcmActual
  call PcLoadColumnPaths
  cmp eax,dword ptr [pcmExpected]
  jne pcmPathsFail
  cmp pcmBefore,13579h
  jne pcmPathsFail
  cmp pcmAfter,24680h
  jne pcmPathsFail
  lea rcx,pcmActual
  lea rdx,pcmExpected+4
  call PcutCompare
  test eax,eax
  jnz pcmPathsFail
  lea rcx,pcmPass
  call PctPrint
  xor ecx,ecx
  call ExitProcess
pcmPathsFail:
  mov ecx,7390
  call ExitProcess
ProcessColumnsMigrationPathsTest ENDP

ProcessColumnsMigrationMissingDirectoryTest PROC
  mov pcmUseMissing,1
  jmp ProcessColumnsMigrationPathsTest
ProcessColumnsMigrationMissingDirectoryTest ENDP

ProcessColumnsMigrationStartupTest PROC
  sub rsp,28h
  call PcmReadExpected
  lea rcx,pcLayout
  call PcDefaultLayout
  mov dword ptr [pcLayout+PC_L_WIDTHS],777
  mov pcLayoutReady,1
  ; Only our process working directory is installed as the storage root.
  mov ecx,480
  lea rdx,storageDir
  call GetCurrentDirectoryW
  test eax,eax
  jz pcmStartupFail
  cmp eax,480
  jae pcmStartupFail
  mov configReady,1
  call PcLoadCurrent
  mov eax,pcColumnsLoadError
  cmp eax,dword ptr [pcmExpected]
  jne pcmStartupFail
  cmp pcColumnsPathReady,1
  jne pcmStartupFail
  lea rcx,pcLayout
  lea rdx,pcmExpected+4
  call PcutCompare
  test eax,eax
  jnz pcmStartupFail
  ; Persistence always targets the newly named V2 destination, even after fallback.
  lea rcx,pcColumnsPath
  call lstrlenW
  sub eax,23
  lea rcx,pcColumnsPath
  lea rcx,[rcx+rax*2]
  lea rdx,pcColumnsSuffix
  call lstrcmpW
  test eax,eax
  jnz pcmStartupFail
  lea rcx,pcmPass
  call PctPrint
  xor ecx,ecx
  call ExitProcess
pcmStartupFail:
  mov ecx,7391
  call ExitProcess
ProcessColumnsMigrationStartupTest ENDP
