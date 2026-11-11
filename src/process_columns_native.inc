; UI-thread adapter: physical subitems remain stable while native header order changes.
.data
align 8
pcVisibleIds dd PC_COLUMN_COUNT dup(-1)
pcVisibleCount dd 0
pcAppliedMode dd -1
pcAppliedDpi dd 96
pcApplying dd 0
pcCapturePending dd 0
pcModeSort dd 8 dup(0)
pcModeDesc dd 8 dup(0)
align 2
pcUtcFormat dw '%','0','4','u','-','%','0','2','u','-','%','0','2','u',' ','%','0','2','u',':','%','0','2','u',':','%','0','2','u',' ','U','T','C',0
.data?
align 8
pcOrder dd PC_COLUMN_COUNT dup(?)
pcCaptureRecord db PC_LAYOUT_SIZE dup(?)
pcApplyRecord db PC_LAYOUT_SIZE dup(?)
pcExtraCaptions dw (13*256) dup(?)
pcExtraText dw 64 dup(?)
pcSystemTime dw 8 dup(?)
.code
PcColumnId PROC
  cmp ecx,pcVisibleCount
  jae pcciInvalid
  lea rax,pcVisibleIds
  mov eax,[rax+rcx*4]
  ret
pcciInvalid:
  mov eax,-1
  ret
PcColumnId ENDP

PcColumnCaption PROC
  cmp ecx,PC_PARENT_PID
  jae pcccExtra
  lea rax,szProcess
  test ecx,ecx
  jz pcccDone
  lea rax,processStatusCaption
  cmp ecx,1
  je pcccDone
  lea rax,cpuCaption
  cmp ecx,2
  je pcccDone
  lea rax,processMemoryCaption
  cmp ecx,3
  je pcccDone
  lea rax,processDiskCaption
  cmp ecx,4
  je pcccDone
  lea rax,szThreads
  cmp ecx,5
  je pcccDone
  lea rax,szPid
pcccDone:
  ret
pcccExtra:
  push rbx
  sub rsp,20h
  sub ecx,7
  cmp ecx,12
  ja pcccBad
  mov ebx,ecx
  shl ebx,9
  lea rax,pcExtraCaptions
  add rbx,rax
  lea edx,[rcx*2+52214]
  cmp appLanguage,0
  je pcccLoad
  inc edx
pcccLoad:
  mov rcx,gInst
  mov r8,rbx
  mov r9d,256
  call LoadStringW
  test eax,eax
  jnz pcccLoaded
  mov word ptr [rbx],'?'
  mov word ptr [rbx+2],0
pcccLoaded:
  mov rax,rbx
  jmp pcccReturn
pcccBad:
  lea rax,unavailable
pcccReturn:
  add rsp,20h
  pop rbx
  ret
PcColumnCaption ENDP

PcCaptureLayout PROC
  push rbx
  push rsi
  push rdi
  sub rsp,20h
  cmp pcAppliedMode,0
  jne pccSuccess
  cmp pcVisibleCount,2
  jb pccInvalid
  cmp pcVisibleCount,PC_COLUMN_COUNT
  ja pccInvalid
  lea rsi,pcLayout
  lea rdi,pcCaptureRecord
  mov ecx,PC_LAYOUT_SIZE/8
  rep movsq
  mov eax,pcVisibleCount
  mov dword ptr [pcCaptureRecord+PC_L_COUNT],eax
  mov eax,sortColumn
  mov dword ptr [pcCaptureRecord+PC_L_SORT],eax
  mov eax,sortDescending
  mov dword ptr [pcCaptureRecord+PC_L_DESCENDING],eax
  mov rcx,gList
  mov edx,103Bh ; LVM_GETCOLUMNORDERARRAY
  mov r8d,pcVisibleCount
  lea r9,pcOrder
  call SendMessageW
  test eax,eax
  jz pccInvalid
  xor ebx,ebx
pccLoop:
  lea rax,pcOrder
  mov ecx,[rax+rbx*4]
  call PcColumnId
  cmp eax,PC_COLUMN_COUNT
  jae pccInvalid
  lea rsi,pcCaptureRecord
  mov [rsi+rbx*4+PC_L_IDS],eax
  mov edi,eax
  mov rcx,gList
  mov edx,101Dh
  lea rax,pcOrder
  mov r8d,[rax+rbx*4]
  xor r9d,r9d
  call SendMessageW
  imul eax,96
  xor edx,edx
  mov ecx,pcAppliedDpi
  test ecx,ecx
  jz pccInvalid
  shr ecx,1
  add eax,ecx
  mov ecx,pcAppliedDpi
  div ecx
  cmp eax,PC_WIDTH_MIN
  jae pccMax
  mov eax,PC_WIDTH_MIN
pccMax:
  cmp eax,PC_WIDTH_MAX
  jbe pccWidth
  mov eax,PC_WIDTH_MAX
pccWidth:
  mov [rsi+rdi*4+PC_L_WIDTHS],eax
  inc ebx
  cmp ebx,pcVisibleCount
  jb pccLoop
pccUnused:
  cmp ebx,PC_LAYOUT_CAPACITY
  jae pccValidate
  mov dword ptr [rsi+rbx*4+PC_L_IDS],-1
  inc ebx
  jmp pccUnused
pccValidate:
  lea rcx,pcCaptureRecord
  call PcValidateLayout
  test eax,eax
  jnz pccDone
  lea rsi,pcCaptureRecord
  lea rdi,pcLayout
  mov ecx,PC_LAYOUT_SIZE/8
  rep movsq
pccSuccess:
  xor eax,eax
  jmp pccDone
pccInvalid:
  mov eax,87
pccDone:
  add rsp,20h
  pop rdi
  pop rsi
  pop rbx
  ret
PcCaptureLayout ENDP

PcBeforeModeColumns PROC
  sub rsp,28h
  call PcEnsureLayout
  mov eax,pcAppliedMode
  cmp eax,7
  ja pcbMark
  lea rdx,pcModeSort
  mov ecx,sortColumn
  mov [rdx+rax*4],ecx
  lea rdx,pcModeDesc
  mov ecx,sortDescending
  mov [rdx+rax*4],ecx
  cmp eax,0
  jne pcbMark
  cmp pcApplying,0
  jne pcbMark
  call PcCaptureLayout
  test eax,eax
  jnz pcbMark
  call PcPersistCurrent
pcbMark:
  mov pcApplying,1
  mov pcCapturePending,0
  add rsp,28h
  ret
PcBeforeModeColumns ENDP

PcBuildProcessColumns PROC
  push rbx
  push rsi
  push rdi
  sub rsp,20h
  lea rsi,pcLayout
  mov eax,[rsi+PC_L_COUNT]
  mov pcVisibleCount,eax
  xor ebx,ebx
pcbpLoop:
  mov edi,[rsi+rbx*4+PC_L_IDS]
  lea rax,pcVisibleIds
  mov [rax+rbx*4],edi
  mov ecx,edi
  call PcColumnCaption
  mov rdx,rax
  mov ecx,ebx
  mov r8d,[rsi+rdi*4+PC_L_WIDTHS]
  call GuiColumn
  ; GuiColumn's legacy numeric alignment uses physical position. Correct it
  ; explicitly using semantic ID, including Status/PID after reorder.
  mov dword ptr [columnBuffer],1
  xor eax,eax
  cmp edi,PC_GPU_ENGINE
  je pcbpAlign
  cmp edi,2
  jb pcbpAlign
  cmp edi,PC_ACCOUNT
  jb pcbpNumeric
  cmp edi,PC_COMMAND
  jbe pcbpAlign
pcbpNumeric:
  mov eax,1
pcbpAlign:
  mov dword ptr [columnBuffer+4],eax
  mov rcx,gList
  mov edx,1060h ; LVM_SETCOLUMNW
  mov r8d,ebx
  lea r9,columnBuffer
  call SendMessageW
  inc ebx
  cmp ebx,pcVisibleCount
  jb pcbpLoop
  add rsp,20h
  pop rdi
  pop rsi
  pop rbx
  ret
PcBuildProcessColumns ENDP

PcAfterModeColumns PROC
  sub rsp,28h
  mov eax,frontMode
  mov pcAppliedMode,eax
  mov edx,uiDpi
  mov pcAppliedDpi,edx
  cmp eax,0
  jne pcacOther
  mov eax,dword ptr [pcLayout+PC_L_SORT]
  mov sortColumn,eax
  mov eax,dword ptr [pcLayout+PC_L_DESCENDING]
  mov sortDescending,eax
  jmp pcacStyle
pcacOther:
  cmp eax,7
  ja pcacStyle
  lea rdx,pcModeSort
  mov ecx,[rdx+rax*4]
  mov sortColumn,ecx
  lea rdx,pcModeDesc
  mov ecx,[rdx+rax*4]
  mov sortDescending,ecx
pcacStyle:
  mov rcx,gList
  mov edx,1036h
  mov r8d,10h ; LVS_EX_HEADERDRAGDROP
  xor r9d,r9d
  cmp frontMode,0
  jne pcacSend
  mov r9d,10h
pcacSend:
  call SendMessageW
  mov pcApplying,0
  call GpUiDemand
  add rsp,28h
  ret
PcAfterModeColumns ENDP

PcApplyLayout PROC
  push rsi
  push rdi
  sub rsp,28h
  mov rsi,rcx
  call PcValidateLayout
  test eax,eax
  jnz pcalDone
  ; Copy draft before capture: caller may pass the active layout itself.
  lea rdi,pcApplyRecord
  mov ecx,PC_LAYOUT_SIZE/8
  rep movsq
  call PcCaptureLayout
  test eax,eax
  jnz pcalDone
  call RememberSelection
  lea rsi,pcApplyRecord
  lea rdi,pcLayout
  mov ecx,PC_LAYOUT_SIZE/8
  rep movsq
  mov pcApplying,1 ; Before must not capture over the newly installed draft
  call SetModeColumns
  call BuildRowMap
  xor eax,eax
pcalDone:
  add rsp,28h
  pop rdi
  pop rsi
  ret
PcApplyLayout ENDP

PcHeaderNotify PROC
  sub rsp,28h
  xor edx,edx
  mov rax,processHeader
  cmp [rcx],rax
  jne pchnAllow
  cmp frontMode,0
  jne pchnAllow
  cmp pcApplying,0
  jne pchnAllow
  mov eax,[rcx+16]
  cmp eax,-310 ; HDN_BEGINDRAG
  je pchnBegin
  cmp eax,-311 ; HDN_ENDDRAG
  je pchnEnd
  cmp eax,-327 ; HDN_ENDTRACKW
  je pchnPost
  cmp eax,-307 ; HDN_ENDTRACKA
  je pchnPost
  cmp eax,-321 ; HDN_ITEMCHANGEDW
  je pchnPost
  cmp eax,-301
  jne pchnAllow
  jmp pchnPost
pchnBegin:
  cmp dword ptr [rcx+24],0
  je pchnCancel
  jmp pchnHandled
pchnEnd:
  cmp dword ptr [rcx+24],0
  je pchnCancel
  mov rax,[rcx+32]
  test rax,rax
  jz pchnCancel
  test dword ptr [rax],80h ; HDI_ORDER
  jz pchnCancel
  cmp dword ptr [rax+44],0
  jle pchnCancel
pchnPost:
  cmp pcCapturePending,0
  jne pchnHandled
  mov pcCapturePending,1
  mov rcx,gMain
  mov edx,PC_LAYOUT_MESSAGE
  xor r8d,r8d
  xor r9d,r9d
  call PostMessageW
  test eax,eax
  jnz pchnHandled
  mov pcCapturePending,0
pchnHandled:
  mov edx,1
pchnAllow:
  xor eax,eax
  add rsp,28h
  ret
pchnCancel:
  mov edx,1
  mov eax,1
  add rsp,28h
  ret
PcHeaderNotify ENDP

; Extra cells are snapshot-backed. UTC never goes through a locale timezone.
PcDisplayExtra PROC
  sub rsp,68h
  test rcx,rcx
  jz pcdeUnknown
  cmp edx,PC_GPU
  jb pcdeNotGpu
  cmp edx,PC_GPU_SHARED
  ja pcdeUnknown
  call GpUiCellText
  jmp pcdeDone
pcdeNotGpu:
  cmp edx,PC_HANDLES
  je pcdeHandles
  cmp edx,PC_ACCOUNT
  jb pcdeOld
  cmp edx,PC_COMMAND
  ja pcdeUnknown
  call PmCellText
  jmp pcdeDone
pcdeOld:
  cmp edx,7
  je pcdeParent
  cmp edx,8
  je pcdePrivate
  cmp edx,9
  je pcdeUtc
  cmp edx,11
  ja pcdeUnknown
  cmp edx,10
  jb pcdeUnknown
  cmp dword ptr [rcx+3660],0
  je pcdeUnknown
  mov r8,[rcx+3640]
  cmp edx,10
  je pcdeInteger
  mov r8,[rcx+3648]
  jmp pcdeInteger
pcdeParent:
  mov r8d,[rcx+3588]
  jmp pcdeInteger
pcdeHandles:
  cmp dword ptr [rcx+3908],0
  je pcdeUnknown
  mov r8d,[rcx+3904]
  jmp pcdeInteger
pcdePrivate:
  cmp treeEnabled,0
  jne pcdeTreePrivate
  cmp dword ptr [rcx+3656],0
  je pcdeUnknown
  mov rcx,[rcx+3624]
  jmp pcdeScale
pcdeTreePrivate:
  mov eax,[rcx+3712]
  cmp rax,[rcx+3736]
  jae pcdeUnknown
  lea rax,[rcx+3752]
  jmp pcdeDone
pcdeScale:
  lea rdx,pcExtraText
  mov r8d,1048576
  call StoreScaledMetric
  jmp pcdeText
pcdeInteger:
  lea rcx,pcExtraText
  lea rdx,processIntegerFormat
  call wsprintfW
  jmp pcdeText
pcdeUtc:
  cmp qword ptr [rcx+3600],0
  je pcdeUnknown
  add rcx,3600
  lea rdx,pcSystemTime
  call FileTimeToSystemTime
  test eax,eax
  jz pcdeUnknown
  movzx r8d,word ptr [pcSystemTime]
  movzx r9d,word ptr [pcSystemTime+2]
  movzx eax,word ptr [pcSystemTime+6]
  mov [rsp+20h],rax
  movzx eax,word ptr [pcSystemTime+8]
  mov [rsp+28h],rax
  movzx eax,word ptr [pcSystemTime+10]
  mov [rsp+30h],rax
  movzx eax,word ptr [pcSystemTime+12]
  mov [rsp+38h],rax
  lea rcx,pcExtraText
  lea rdx,pcUtcFormat
  call wsprintfW
pcdeText:
  lea rax,pcExtraText
  jmp pcdeDone
pcdeUnknown:
  lea rax,unavailable
pcdeDone:
  add rsp,68h
  ret
PcDisplayExtra ENDP

; RCX immutable row, EDX stable ID -> unsigned RAX, ECX boolean validity.
; Tree sources are selected only for the three explicitly aggregated metrics.
PcSortMetric PROC
  cmp edx,PC_GPU
  jb pcsmNotGpu
  cmp edx,PC_GPU_SHARED
  ja pcsmInvalid
  jmp GpUiSortMetric
pcsmNotGpu:
  mov r8,rcx
  mov ecx,1
  cmp edx,PC_HANDLES
  je pcsmHandles
  cmp edx,1
  je pcsmStatus
  cmp edx,2
  je pcsmCpu
  cmp edx,3
  je pcsmWs
  cmp edx,4
  je pcsmIo
  cmp edx,5
  je pcsmThreads
  cmp edx,6
  je pcsmPid
  cmp edx,7
  je pcsmParent
  cmp edx,8
  je pcsmPrivate
  cmp edx,9
  je pcsmBirth
  cmp edx,10
  je pcsmRead
  cmp edx,11
  jne pcsmInvalid
  mov rax,[r8+3648]
  cmp dword ptr [r8+3660],0
  setne cl
  ret
pcsmRead:
  mov rax,[r8+3640]
  cmp dword ptr [r8+3660],0
  setne cl
  ret
pcsmHandles:
  mov eax,[r8+3904]
  cmp dword ptr [r8+3908],0
  setne cl
  ret
pcsmBirth:
  mov rax,[r8+3600]
  test rax,rax
  setne cl
  ret
pcsmPrivate:
  cmp treeEnabled,0
  jne pcsmTreePrivate
  mov rax,[r8+3624]
  cmp dword ptr [r8+3656],0
  setne cl
  ret
pcsmTreePrivate:
  mov rax,[r8+3720]
  mov edx,[r8+3712]
  cmp rdx,[r8+3736]
  setb cl
  ret
pcsmParent:
  mov eax,[r8+3588]
  ret
pcsmPid:
  mov eax,[r8+3584]
  ret
pcsmStatus:
  mov eax,[r8+3884]
  ret
pcsmCpu:
  cmp treeEnabled,0
  jne pcsmTreeCpu
  mov eax,[r8+3900]
  cmp dword ptr [r8+3672],0
  setne cl
  ret
pcsmTreeCpu:
  mov rax,[r8+4040]
  cmp dword ptr [r8+4056],0
  setne cl
  ret
pcsmWs:
  cmp treeEnabled,0
  jne pcsmTreeWs
  mov rax,[r8+3632]
  cmp dword ptr [r8+3656],0
  setne cl
  ret
pcsmTreeWs:
  mov rax,[r8+3728]
  mov edx,[r8+3712]
  cmp rdx,[r8+3736]
  setb cl
  ret
pcsmIo:
  cmp treeEnabled,0
  jne pcsmTreeIo
  mov rax,[r8+3888]
  cmp dword ptr [r8+3896],0
  setne cl
  ret
pcsmTreeIo:
  mov rax,[r8+4048]
  cmp dword ptr [r8+4060],0
  setne cl
  ret
pcsmThreads:
  add r8,1536
  xor eax,eax
pcsmDigit:
  movzx edx,word ptr [r8]
  test edx,edx
  jz pcsmEnd
  sub edx,'0'
  cmp edx,9
  ja pcsmInvalid
  imul rax,10
  add rax,rdx
  add r8,2
  jmp pcsmDigit
pcsmEnd:
  ret
pcsmInvalid:
  xor eax,eax
  xor ecx,ecx
  ret
PcSortMetric ENDP
