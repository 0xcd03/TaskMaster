; Pure version-3 layout plus explicit-path atomic store. No settings side effects.
extern GetTickCount:proc
.data
align 8
pcLayout db PC_LAYOUT_SIZE dup(0)
pcLayoutReady dd 0
pcTempSerial dd 0
pcDefaultRecord dd PC_LAYOUT_SIZE,PC_LAYOUT_VERSION,7,0,0,0
  dd 0,1,2,3,4,5,6
  dd 57 dup(-1)
  dd 240,130,76,120,125,90,68,90,150,190,140,140,180,360,480,90
  dd 48 dup(90)
  dd 0,0
align 2
pcTempFormat dw '.','p','c','-','%','0','8','X','-','%','0','8','X','-','%','0','8','X','.','t','m','p',0
.code
PcDefaultLayout PROC
  lea rdx,pcDefaultRecord
  xor eax,eax
pcdCopy:
  mov r8,[rdx+rax]
  mov [rcx+rax],r8
  add eax,8
  cmp eax,PC_LAYOUT_SIZE
  jb pcdCopy
  ret
PcDefaultLayout ENDP

PcValidateLayout PROC
  test rcx,rcx
  jz pcvBad
  cmp dword ptr [rcx+PC_L_SIZE],PC_LAYOUT_SIZE
  jne pcvBad
  cmp dword ptr [rcx+PC_L_VERSION],PC_LAYOUT_VERSION
  jne pcvBad
  mov edx,[rcx+PC_L_COUNT]
  cmp edx,2
  jb pcvBad
  cmp edx,PC_COLUMN_COUNT
  ja pcvBad
  cmp dword ptr [rcx+PC_L_IDS],0
  jne pcvBad
  cmp dword ptr [rcx+PC_L_DESCENDING],1
  ja pcvBad
  cmp dword ptr [rcx+PC_L_RESERVED],0
  jne pcvBad
  cmp qword ptr [rcx+PC_L_RESERVED_TAIL],0
  jne pcvBad
  xor r8d,r8d
  xor r9d,r9d
pcvVisible:
  mov eax,[rcx+r8*4+PC_L_IDS]
  cmp eax,PC_COLUMN_COUNT
  jae pcvBad
  bt r9d,eax
  jc pcvBad
  bts r9d,eax
  inc r8d
  cmp r8d,edx
  jb pcvVisible
  test r9d,40h
  jz pcvBad
  mov eax,[rcx+PC_L_SORT]
  cmp eax,PC_COLUMN_COUNT
  jae pcvBad
  bt r9d,eax
  jnc pcvBad
pcvUnused:
  cmp r8d,PC_LAYOUT_CAPACITY
  jae pcvWidths
  cmp dword ptr [rcx+r8*4+PC_L_IDS],-1
  jne pcvBad
  inc r8d
  jmp pcvUnused
pcvWidths:
  xor r8d,r8d
pcvWidth:
  mov eax,[rcx+r8*4+PC_L_WIDTHS]
  cmp eax,32
  jb pcvBad
  cmp eax,4096
  ja pcvBad
  inc r8d
  cmp r8d,PC_LAYOUT_CAPACITY
  jb pcvWidth
  xor eax,eax
  ret
pcvBad:
  mov eax,87
  ret
PcValidateLayout ENDP

PcValidateLayoutV1 PROC
  test rcx,rcx
  jz pcv1Bad
  cmp dword ptr [rcx+PC_L_SIZE],128
  jne pcv1Bad
  cmp dword ptr [rcx+PC_L_VERSION],1
  jne pcv1Bad
  mov edx,[rcx+PC_L_COUNT]
  cmp edx,2
  jb pcv1Bad
  cmp edx,12
  ja pcv1Bad
  cmp dword ptr [rcx+PC_L_IDS],0
  jne pcv1Bad
  cmp dword ptr [rcx+PC_L_DESCENDING],1
  ja pcv1Bad
  cmp dword ptr [rcx+PC_L_RESERVED],0
  jne pcv1Bad
  cmp qword ptr [rcx+120],0
  jne pcv1Bad
  xor r8d,r8d
  xor r9d,r9d
pcv1Visible:
  mov eax,[rcx+r8*4+PC_L_IDS]
  cmp eax,12
  jae pcv1Bad
  bt r9d,eax
  jc pcv1Bad
  bts r9d,eax
  inc r8d
  cmp r8d,edx
  jb pcv1Visible
  test r9d,40h
  jz pcv1Bad
  mov eax,[rcx+PC_L_SORT]
  cmp eax,12
  jae pcv1Bad
  bt r9d,eax
  jnc pcv1Bad
pcv1Unused:
  cmp r8d,12
  jae pcv1Widths
  cmp dword ptr [rcx+r8*4+PC_L_IDS],-1
  jne pcv1Bad
  inc r8d
  jmp pcv1Unused
pcv1Widths:
  xor r8d,r8d
pcv1Width:
  mov eax,[rcx+r8*4+72]
  cmp eax,32
  jb pcv1Bad
  cmp eax,4096
  ja pcv1Bad
  inc r8d
  cmp r8d,12
  jb pcv1Width
  xor eax,eax
  ret
pcv1Bad:
  mov eax,87
  ret
PcValidateLayoutV1 ENDP

PcValidateLayoutV2 PROC
  test rcx,rcx
  jz pcv2Bad
  cmp dword ptr [rcx+PC_L_SIZE],160
  jne pcv2Bad
  cmp dword ptr [rcx+PC_L_VERSION],2
  jne pcv2Bad
  mov edx,[rcx+PC_L_COUNT]
  cmp edx,2
  jb pcv2Bad
  cmp edx,16
  ja pcv2Bad
  cmp dword ptr [rcx+PC_L_IDS],0
  jne pcv2Bad
  cmp dword ptr [rcx+PC_L_DESCENDING],1
  ja pcv2Bad
  cmp dword ptr [rcx+PC_L_RESERVED],0
  jne pcv2Bad
  cmp qword ptr [rcx+152],0
  jne pcv2Bad
  xor r8d,r8d
  xor r9d,r9d
pcv2Visible:
  mov eax,[rcx+r8*4+PC_L_IDS]
  cmp eax,16
  jae pcv2Bad
  bt r9d,eax
  jc pcv2Bad
  bts r9d,eax
  inc r8d
  cmp r8d,edx
  jb pcv2Visible
  test r9d,40h
  jz pcv2Bad
  mov eax,[rcx+PC_L_SORT]
  cmp eax,16
  jae pcv2Bad
  bt r9d,eax
  jnc pcv2Bad
pcv2Unused:
  cmp r8d,16
  jae pcv2Widths
  cmp dword ptr [rcx+r8*4+PC_L_IDS],-1
  jne pcv2Bad
  inc r8d
  jmp pcv2Unused
pcv2Widths:
  xor r8d,r8d
pcv2Width:
  mov eax,[rcx+r8*4+88]
  cmp eax,32
  jb pcv2Bad
  cmp eax,4096
  ja pcv2Bad
  inc r8d
  cmp r8d,16
  jb pcv2Width
  xor eax,eax
  ret
pcv2Bad:
  mov eax,87
  ret
PcValidateLayoutV2 ENDP

PcEnsureLayout PROC
  sub rsp,28h
  cmp pcLayoutReady,0
  jne pceDone
  lea rcx,pcLayout
  call PcDefaultLayout
  mov pcLayoutReady,1
pceDone:
  add rsp,28h
  ret
PcEnsureLayout ENDP

; Bounded path probe: returns length or -1. At most 1024 WCHAR reads.
PcPathLength PROC
  test rcx,rcx
  jz pcplBad
  xor eax,eax
pcplNext:
  cmp word ptr [rcx+rax*2],0
  je pcplEnd
  inc eax
  cmp eax,1024
  jb pcplNext
pcplBad:
  mov eax,-1
  ret
pcplEnd:
  test eax,eax
  jz pcplBad
  ret
PcPathLength ENDP

PcLoadLayout PROC
  ; Disjoint stack regions: source 040h..25Fh, converted 280h..49Fh,
  ; I/O count/EOF 4C0h/4C8h, file size 4D0h, legacy metadata 4D8h/4DCh.
  push rbx
  push rsi
  push rdi
  sub rsp,500h
  mov rsi,rcx
  mov rdi,rdx
  test rdi,rdi
  jz pclArgument
  call PcPathLength
  cmp eax,-1
  je pclArgument
  mov rcx,rsi
  mov edx,80000000h
  mov r8d,1
  xor r9d,r9d
  mov qword ptr [rsp+20h],3
  mov qword ptr [rsp+28h],80h
  mov qword ptr [rsp+30h],0
  call CreateFileW
  cmp rax,-1
  je pclOpenError
  mov rbx,rax
  mov rcx,rbx
  lea rdx,[rsp+4D0h]
  call GetFileSizeEx
  test eax,eax
  jz pclIoError
  cmp qword ptr [rsp+4D0h],128
  je pclRead
  cmp qword ptr [rsp+4D0h],160
  je pclRead
  cmp qword ptr [rsp+4D0h],PC_LAYOUT_SIZE
  jne pclInvalid
pclRead:
  mov rcx,rbx
  lea rdx,[rsp+40h]
  mov r8d,[rsp+4D0h]
  lea r9,[rsp+4C0h]
  mov qword ptr [rsp+20h],0
  call ReadFile
  test eax,eax
  jz pclIoError
  mov eax,[rsp+4D0h]
  cmp dword ptr [rsp+4C0h],eax
  jne pclInvalid
  mov rcx,rbx
  lea rdx,[rsp+4C8h]
  mov r8d,1
  lea r9,[rsp+4C0h]
  mov qword ptr [rsp+20h],0
  call ReadFile
  test eax,eax
  jz pclIoError
  cmp dword ptr [rsp+4C0h],0
  jne pclInvalid
  lea rcx,[rsp+40h]
  cmp qword ptr [rsp+4D0h],128
  je pclLegacyV1
  cmp qword ptr [rsp+4D0h],160
  je pclLegacyV2
  call PcValidateLayout
  test eax,eax
  jnz pclInvalid
  jmp pclValidated
pclLegacyV1:
  call PcValidateLayoutV1
  mov dword ptr [rsp+4D8h],12
  mov dword ptr [rsp+4DCh],72
  jmp pclLegacyChecked
pclLegacyV2:
  call PcValidateLayoutV2
  mov dword ptr [rsp+4D8h],16
  mov dword ptr [rsp+4DCh],88
pclLegacyChecked:
  test eax,eax
  jnz pclInvalid
  lea rcx,[rsp+280h]
  call PcDefaultLayout
  mov eax,[rsp+48h]
  mov [rsp+288h],eax
  mov eax,[rsp+4Ch]
  mov [rsp+28Ch],eax
  mov eax,[rsp+50h]
  mov [rsp+290h],eax
  xor r8d,r8d
  mov r9d,[rsp+4DCh]
  lea r9,[rsp+r9+40h]
pclConvert:
  mov eax,[rsp+r8*4+58h]
  mov [rsp+r8*4+298h],eax
  mov eax,[r9+r8*4]
  mov [rsp+r8*4+398h],eax
  inc r8d
  cmp r8d,[rsp+4D8h]
  jb pclConvert
  xor eax,eax
pclConvertCopy:
  mov rdx,[rsp+rax+280h]
  mov [rsp+rax+40h],rdx
  add eax,8
  cmp eax,PC_LAYOUT_SIZE
  jb pclConvertCopy
pclValidated:
  mov rcx,rbx
  call CloseHandle
  test eax,eax
  jz pclOpenError
  xor eax,eax
pclCopy:
  mov rdx,[rsp+rax+40h]
  mov [rdi+rax],rdx
  add eax,8
  cmp eax,PC_LAYOUT_SIZE
  jb pclCopy
  xor eax,eax
  jmp pclDone
pclInvalid:
  mov esi,13
  jmp pclClose
pclIoError:
  call GetLastError
  mov esi,eax
pclClose:
  mov rcx,rbx
  call CloseHandle
  mov eax,esi
  jmp pclDone
pclArgument:
  mov eax,87
  jmp pclDone
pclOpenError:
  call GetLastError
pclDone:
  add rsp,500h
  pop rdi
  pop rsi
  pop rbx
  ret
PcLoadLayout ENDP

PcSaveLayout PROC
  ; Record 040h..25Fh; written/path length 280h/290h; temporary path at
  ; 300h has room for 1024 WCHAR plus the complete collision-safe suffix.
  push rbx
  push rsi
  push rdi
  push r12
  push r13
  sub rsp,0C00h
  mov rsi,rcx
  mov rdi,rdx
  test rdi,rdi
  jz pcsArgument
  call PcPathLength
  cmp eax,-1
  je pcsArgument
  mov [rsp+290h],eax
  xor edx,edx
pcsPathCopy:
  mov cx,[rsi+rdx*2]
  mov [rsp+rdx*2+300h],cx
  inc edx
  cmp edx,eax
  jbe pcsPathCopy
  ; Snapshot the complete record before validation or filesystem writes.
  xor eax,eax
pcsCopy:
  mov rdx,[rdi+rax]
  mov [rsp+rax+40h],rdx
  add eax,8
  cmp eax,PC_LAYOUT_SIZE
  jb pcsCopy
  lea rcx,[rsp+40h]
  call PcValidateLayout
  test eax,eax
  jnz pcsDone
  call GetCurrentProcessId
  mov r12d,eax
  mov r13d,64
pcsCreate:
  mov eax,1
  lock xadd pcTempSerial,eax
  mov ebx,eax
  call GetTickCount
  mov [rsp+20h],eax
  mov eax,[rsp+290h]
  lea rcx,[rsp+300h]
  lea rcx,[rcx+rax*2]
  lea rdx,pcTempFormat
  mov r8d,ebx
  mov r9d,r12d
  call wsprintfW
  lea rcx,[rsp+300h]
  mov edx,40000000h
  xor r8d,r8d
  xor r9d,r9d
  mov qword ptr [rsp+20h],1 ; CREATE_NEW: never truncates an unowned file
  mov qword ptr [rsp+28h],80h
  mov qword ptr [rsp+30h],0
  call CreateFileW
  cmp rax,-1
  jne pcsCreated
  call GetLastError
  cmp eax,80
  je pcsCollision
  cmp eax,183
  jne pcsDone
pcsCollision:
  dec r13d
  jnz pcsCreate
  jmp pcsDone
pcsCreated:
  mov rbx,rax
  mov rcx,rbx
  lea rdx,[rsp+40h]
  mov r8d,PC_LAYOUT_SIZE
  lea r9,[rsp+280h]
  mov qword ptr [rsp+20h],0
  call WriteFile
  test eax,eax
  jz pcsIoError
  cmp dword ptr [rsp+280h],PC_LAYOUT_SIZE
  jne pcsShort
  mov rcx,rbx
  call FlushFileBuffers
  test eax,eax
  jz pcsIoError
  mov rcx,rbx
  call CloseHandle
  test eax,eax
  jz pcsClosedError
  lea rcx,[rsp+300h]
  mov rdx,rsi
  mov r8d,9 ; REPLACE_EXISTING | WRITE_THROUGH
  call MoveFileExW
  test eax,eax
  jz pcsClosedError
  xor eax,eax
  jmp pcsDone
pcsShort:
  mov edi,29 ; ERROR_WRITE_FAULT
  jmp pcsClose
pcsIoError:
  call GetLastError
  mov edi,eax
pcsClose:
  mov rcx,rbx
  call CloseHandle
  jmp pcsDelete
pcsClosedError:
  call GetLastError
  mov edi,eax
pcsDelete:
  ; This branch is reachable only after CREATE_NEW succeeded for this operation.
  lea rcx,[rsp+300h]
  call DeleteFileW
  mov eax,edi
  jmp pcsDone
pcsArgument:
  mov eax,87
pcsDone:
  add rsp,0C00h
  pop r13
  pop r12
  pop rdi
  pop rsi
  pop rbx
  ret
PcSaveLayout ENDP
