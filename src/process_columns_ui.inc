; Parent-owned picker/persistence integration. Dialog owns a transactional draft.
extern SetWindowLongPtrW:proc
.data
align 8
pcDialogWindow dq 0
pcDialogHook dq 0
pcDialogSaveNotice dq MessageBoxW
pcHeaderTrack dq TrackPopupMenu
pcColumnsPathReady dd 0
pcColumnsLoadError dd 0
pcColumnsSaveError dd 0
pcDialogPopulating dd 0
align 2
pcColumnsSuffix dw '\','p','r','o','c','e','s','s','-','c','o','l','u','m','n','s','-','v','3','.','b','i','n',0
pcColumnsV2Suffix dw '\','p','r','o','c','e','s','s','-','c','o','l','u','m','n','s','-','v','2','.','b','i','n',0
pcColumnsLegacySuffix dw '\','p','r','o','c','e','s','s','-','c','o','l','u','m','n','s','-','v','1','.','b','i','n',0
pcDialogEmpty dw 0
.data?
align 8
pcColumnsPath dw 1024 dup(?)
pcColumnsLegacyPath dw 1024 dup(?)
pcColumnsV2Path dw 1024 dup(?)
pcDialogDraft db PC_LAYOUT_SIZE dup(?)
pcDialogIds dd PC_COLUMN_COUNT dup(?)
pcDialogChecks dd PC_COLUMN_COUNT dup(?)
pcDialogItem db 88 dup(?)
pcDialogColumn db 56 dup(?)
pcDialogText dw 1024 dup(?)
pcDialogNotice dw 1024 dup(?)
.code
PcUpdateMenu PROC
  sub rsp,28h
  mov rcx,mainMenu
  mov edx,2502
  xor r8d,r8d
  cmp frontMode,0
  je pcumSet
  mov r8d,1
pcumSet:
  call EnableMenuItem
  add rsp,28h
  ret
PcUpdateMenu ENDP

; Defer capture until native geometry changes and sorting have completed.
PcQueueLayoutCapture PROC
  sub rsp,28h
  cmp pcCapturePending,0
  jne pcqlDone
  mov pcCapturePending,1
  mov rcx,gMain
  mov edx,PC_LAYOUT_MESSAGE
  xor r8d,r8d
  xor r9d,r9d
  call PostMessageW
  test eax,eax
  jnz pcqlDone
  mov pcCapturePending,0
pcqlDone:
  add rsp,28h
  ret
PcQueueLayoutCapture ENDP

; RCX WM_CONTEXTMENU packed screen coordinates; -1 requests keyboard position.
PcShowHeaderMenu PROC
  push rbx
  sub rsp,50h
  mov [rsp+40h],rcx
  cmp frontMode,0
  jne pcshmDone
  call CreatePopupMenu
  mov rbx,rax
  test rax,rax
  jz pcshmDone
  mov edx,52200
  call PcDialogText
  mov r9,rax
  mov rcx,rbx
  xor edx,edx
  mov r8d,2502
  call AppendMenuW
  mov rax,[rsp+40h]
  cmp rax,-1
  jne pcshmMouse
  mov rcx,processHeader
  lea rdx,[rsp+40h]
  call GetWindowRect
  mov r8d,[rsp+40h]
  mov r9d,[rsp+4Ch]
  jmp pcshmTrack
pcshmMouse:
  movsx r8d,ax
  shr eax,16
  movsx r9d,ax
pcshmTrack:
  mov rcx,rbx
  mov edx,102h ; TPM_RETURNCMD | TPM_RIGHTBUTTON
  mov qword ptr [rsp+20h],0
  mov rax,gMain
  mov [rsp+28h],rax
  mov qword ptr [rsp+30h],0
  call qword ptr pcHeaderTrack
  cmp eax,2502
  jne pcshmDestroy
  mov rcx,gMain
  mov edx,111h
  mov r8d,2502
  xor r9d,r9d
  call SendMessageW
pcshmDestroy:
  mov rcx,rbx
  call DestroyMenu
pcshmDone:
  add rsp,50h
  pop rbx
  ret
PcShowHeaderMenu ENDP

ShowProcessColumnsDialog PROC
  push rbx
  push rsi
  push rdi
  sub rsp,30h
  cmp frontMode,0
  jne pcdsNo
  cmp gMain,0
  je pcdsNo
  call PcEnsureLayout
  call PcCaptureLayout
  lea rsi,pcLayout
  lea rdi,pcDialogDraft
  mov ecx,PC_LAYOUT_SIZE/8
  rep movsq
  mov pcColumnsSaveError,0
  mov rcx,gInst
  mov edx,307
  add edx,appLanguage
  mov r8,gMain
  lea r9,PcColumnsDialogProc
  mov qword ptr [rsp+20h],0
  call DialogBoxParamW
  mov rbx,rax
  cmp eax,1
  jne pcdsReturn
  cmp pcColumnsSaveError,0
  je pcdsReturn
  mov edx,52204
  call PcDialogText
  lea rcx,pcDialogNotice
  mov rdx,rax
  mov r8d,pcColumnsSaveError
  call wsprintfW
  mov edx,52200
  call PcDialogText
  mov r8,rax
  mov rcx,gMain
  lea rdx,pcDialogNotice
  mov r9d,30h
  call qword ptr pcDialogSaveNotice
pcdsReturn:
  mov rax,rbx
  jmp pcdsDone
pcdsNo:
  xor eax,eax
pcdsDone:
  add rsp,30h
  pop rdi
  pop rsi
  pop rbx
  ret
ShowProcessColumnsDialog ENDP

PcDialogText PROC
  sub rsp,28h
  add edx,appLanguage
  mov rcx,gInst
  lea r8,pcDialogText
  mov r9d,1024
  call LoadStringW
  lea rax,pcDialogText
  add rsp,28h
  ret
PcDialogText ENDP

PcColumnsDialogProc PROC
  push rbx
  push rsi
  sub rsp,58h
  mov rbx,rcx
  cmp edx,110h
  je pcddInit
  cmp edx,111h
  je pcddCommand
  cmp edx,4Eh
  je pcddNotify
  cmp edx,10h
  je pcddCancel
  cmp edx,2
  je pcddDestroy
  cmp edx,133h
  je pcddColor
  cmp edx,138h
  je pcddColor
pcddNo:
  xor eax,eax
  jmp pcddDone
pcddInit:
  mov pcDialogWindow,rbx
  mov rcx,rbx
  mov edx,4300
  call GetDlgItem
  mov rsi,rax
  mov rcx,rax
  mov edx,1036h
  mov r8d,10024h ; FULLROWSELECT, DOUBLEBUFFER, CHECKBOXES
  mov r9d,10024h
  call SendMessageW
  mov rcx,rsi
  mov edx,30h
  mov r8,appFont
  xor r9d,r9d
  call SendMessageW
  mov dword ptr [pcDialogColumn],7
  mov dword ptr [pcDialogColumn+8],320
  lea rax,pcDialogEmpty
  mov qword ptr [pcDialogColumn+16],rax
  mov rcx,rsi
  mov edx,1061h
  xor r8d,r8d
  lea r9,pcDialogColumn
  call SendMessageW
  mov rcx,rsi
  mov edx,101Eh
  xor r8d,r8d
  mov r9,-2 ; fill actual control width
  call SendMessageW
  mov rcx,rsi
  lea rdx,explorerThemeName
  cmp themeOption,0
  je pcddTheme
  lea rdx,darkExplorerThemeName
pcddTheme:
  xor r8d,r8d
  call SetWindowTheme
  mov rcx,rsi
  mov edx,1024h ; LVM_SETTEXTCOLOR
  xor r8d,r8d
  mov r9d,shellTextColor
  call SendMessageW
  mov r9d,0FFFFFFh
  cmp themeOption,0
  je pcddBackground
  mov r9d,202020h
pcddBackground:
  mov [rsp+40h],r9d
  mov rcx,rsi
  mov edx,1001h
  xor r8d,r8d
  call SendMessageW
  mov rcx,rsi
  mov edx,1026h
  xor r8d,r8d
  mov r9d,[rsp+40h]
  call SendMessageW
  mov edx,52202
  call PcDialogText
  mov rcx,rbx
  mov edx,4304
  mov r8,rax
  call SetDlgItemTextW
  call PcDialogPopulate
  mov rax,pcDialogHook
  test rax,rax
  jz pcddYes
  mov rcx,rbx
  call rax
  jmp pcddYes
pcddCommand:
  and r8d,0FFFFh
  cmp r8d,2
  je pcddCancel
  cmp r8d,1
  je pcddOk
  cmp r8d,4303
  je pcddDefaults
  mov ecx,-1
  cmp r8d,4301
  je pcddMove
  mov ecx,1
  cmp r8d,4302
  jne pcddNo
pcddMove:
  call PcDialogMove
  jmp pcddYes
pcddDefaults:
  lea rcx,pcDialogDraft
  call PcDefaultLayout
  call PcDialogPopulate
  jmp pcddYes
pcddOk:
  call PcDialogCommit
  test eax,eax
  jz pcddAccepted
  mov [rsp+40h],eax
  mov edx,52206
  call PcDialogText
  lea rcx,pcDialogNotice
  mov rdx,rax
  mov r8d,[rsp+40h]
  call wsprintfW
  mov rcx,rbx
  mov edx,4304
  lea r8,pcDialogNotice
  call SetDlgItemTextW
  jmp pcddYes
pcddAccepted:
  mov rcx,rbx
  mov edx,1
  call EndDialog
  jmp pcddYes
pcddCancel:
  mov rcx,rbx
  mov edx,2
  call EndDialog
  jmp pcddYes
pcddNotify:
  cmp pcDialogPopulating,0
  jne pcddNo
  cmp qword ptr [r9+8],4300
  jne pcddNo
  cmp dword ptr [r9+16],-101
  je pcddChanged
  cmp dword ptr [r9+16],-100 ; LVN_ITEMCHANGING: reject unchecking mandatory IDs before commit
  jne pcddNo
  mov esi,[r9+24]
  cmp esi,PC_COLUMN_COUNT
  jae pcddNo
  lea rax,pcDialogIds
  mov eax,[rax+rsi*4]
  cmp eax,PC_NAME
  je pcddChangingMandatory
  cmp eax,PC_PID
  jne pcddNo
pcddChangingMandatory:
  test dword ptr [r9+40],8 ; uChanged: LVIF_STATE
  jz pcddNo
  mov eax,[r9+32]
  xor eax,[r9+36]
  test eax,0F000h ; selection/focus changes must remain usable
  jz pcddNo
  mov eax,[r9+32] ; proposed state image
  and eax,0F000h
  cmp eax,2000h
  je pcddNo
  mov rcx,rbx
  xor edx,edx ; DWLP_MSGRESULT: dialog notification result, not DLGPROC boolean
  mov r8d,1
  call SetWindowLongPtrW
  jmp pcddYes
pcddChanged:
  mov esi,[r9+24]
  cmp esi,PC_COLUMN_COUNT
  jae pcddNo
  lea rax,pcDialogIds
  mov eax,[rax+rsi*4]
  cmp eax,PC_NAME
  je pcddMandatory
  cmp eax,PC_PID
  jne pcddNo
pcddMandatory:
  mov pcDialogPopulating,1
  mov rcx,rbx
  mov edx,4300
  call GetDlgItem
  mov rcx,rax
  mov edx,102Bh
  mov r8d,esi
  mov dword ptr [pcDialogItem+12],2000h
  mov dword ptr [pcDialogItem+16],0F000h
  lea r9,pcDialogItem
  call SendMessageW
  mov pcDialogPopulating,0
  jmp pcddYes
pcddDestroy:
  mov pcDialogWindow,0
  jmp pcddNo
pcddColor:
  mov rsi,r8
  mov rcx,r8
  mov edx,shellTextColor
  call SetTextColor
  mov rcx,rsi
  mov edx,0FFFFFFh
  cmp themeOption,0
  je pcddSetBg
  mov edx,202020h
pcddSetBg:
  call SetBkColor
  mov rax,shellCardBrush
  test rax,rax
  jnz pcddDone
  mov ecx,15
  call GetSysColorBrush
  jmp pcddDone
pcddYes:
  mov eax,1
pcddDone:
  add rsp,58h
  pop rsi
  pop rbx
  ret
PcColumnsDialogProc ENDP

; Rebuild row order/checks from the current draft (initial open or Defaults).
PcDialogPopulate PROC
  push rbx
  push rsi
  push rdi
  sub rsp,20h
  lea rdi,pcDialogChecks
  xor eax,eax
  mov ecx,PC_COLUMN_COUNT
  rep stosd
  lea rsi,pcDialogDraft
  xor ebx,ebx
pcdpVisible:
  cmp ebx,[rsi+PC_L_COUNT]
  jae pcdpHiddenStart
  mov eax,[rsi+rbx*4+PC_L_IDS]
  lea rdx,pcDialogIds
  mov [rdx+rbx*4],eax
  lea rdx,pcDialogChecks
  mov dword ptr [rdx+rax*4],1
  inc ebx
  jmp pcdpVisible
pcdpHiddenStart:
  xor edi,edi
pcdpHidden:
  lea rax,pcDialogChecks
  cmp dword ptr [rax+rdi*4],0
  jne pcdpNext
  lea rax,pcDialogIds
  mov [rax+rbx*4],edi
  inc ebx
pcdpNext:
  inc edi
  cmp edi,PC_COLUMN_COUNT
  jb pcdpHidden
  call PcDialogRenderRows
  add rsp,20h
  pop rdi
  pop rsi
  pop rbx
  ret
PcDialogPopulate ENDP

PcDialogRenderRows PROC
  push rbx
  push rsi
  push rdi
  sub rsp,20h
  mov pcDialogPopulating,1
  mov rcx,pcDialogWindow
  mov edx,4300
  call GetDlgItem
  mov rbx,rax
  mov rcx,rax
  mov edx,1009h
  xor r8d,r8d
  xor r9d,r9d
  call SendMessageW
  xor esi,esi
pcdrrLoop:
  lea rax,pcDialogIds
  mov edi,[rax+rsi*4]
  mov ecx,edi
  call PcColumnCaption
  mov dword ptr [pcDialogItem],0Dh ; TEXT|PARAM|STATE
  mov dword ptr [pcDialogItem+4],esi
  mov dword ptr [pcDialogItem+8],0
  mov qword ptr [pcDialogItem+24],rax
  mov qword ptr [pcDialogItem+40],rdi
  mov dword ptr [pcDialogItem+16],0F000h
  lea rax,pcDialogChecks
  mov eax,[rax+rdi*4]
  inc eax
  shl eax,12
  mov dword ptr [pcDialogItem+12],eax
  mov rcx,rbx
  mov edx,104Dh
  xor r8d,r8d
  lea r9,pcDialogItem
  call SendMessageW
  ; LVS_EX_CHECKBOXES initializes newly inserted items unchecked. Set the
  ; desired state after insertion while notifications remain suppressed.
  lea rax,pcDialogChecks
  mov eax,[rax+rdi*4]
  inc eax
  shl eax,12
  mov dword ptr [pcDialogItem+12],eax
  mov dword ptr [pcDialogItem+16],0F000h
  mov rcx,rbx
  mov edx,102Bh
  mov r8d,esi
  lea r9,pcDialogItem
  call SendMessageW
  inc esi
  cmp esi,PC_COLUMN_COUNT
  jb pcdrrLoop
  mov pcDialogPopulating,0
  add rsp,20h
  pop rdi
  pop rsi
  pop rbx
  ret
PcDialogRenderRows ENDP

PcDialogReadChecks PROC
  push rbx
  push rsi
  sub rsp,28h
  mov rcx,pcDialogWindow
  mov edx,4300
  call GetDlgItem
  mov rbx,rax
  xor esi,esi
pcdrcLoop:
  mov rcx,rbx
  mov edx,102Ch
  mov r8d,esi
  mov r9d,0F000h
  call SendMessageW
  shr eax,12
  cmp eax,2
  sete r8b
  movzx r8d,r8b
  lea rdx,pcDialogIds
  mov eax,[rdx+rsi*4]
  lea rdx,pcDialogChecks
  mov [rdx+rax*4],r8d
  inc esi
  cmp esi,PC_COLUMN_COUNT
  jb pcdrcLoop
  mov dword ptr [pcDialogChecks+PC_NAME*4],1
  mov dword ptr [pcDialogChecks+PC_PID*4],1
  add rsp,28h
  pop rsi
  pop rbx
  ret
PcDialogReadChecks ENDP

PcDialogMove PROC
  push rbx
  push rsi
  push rdi
  sub rsp,20h
  mov edi,ecx
  mov rcx,pcDialogWindow
  mov edx,4300
  call GetDlgItem
  mov rbx,rax
  mov rcx,rax
  mov edx,100Ch
  mov r8,-1
  mov r9d,2
  call SendMessageW
  test eax,eax
  jle pcdmDone
  mov esi,eax
  add edi,eax
  cmp edi,1
  jl pcdmDone
  cmp edi,PC_COLUMN_COUNT
  jae pcdmDone
  call PcDialogReadChecks
  lea rdx,pcDialogIds
  mov eax,[rdx+rsi*4]
  mov ecx,[rdx+rdi*4]
  mov [rdx+rsi*4],ecx
  mov [rdx+rdi*4],eax
  call PcDialogRenderRows
  mov rcx,rbx
  mov edx,102Bh
  mov r8d,edi
  mov dword ptr [pcDialogItem+12],3
  mov dword ptr [pcDialogItem+16],3
  lea r9,pcDialogItem
  call SendMessageW
pcdmDone:
  add rsp,20h
  pop rdi
  pop rsi
  pop rbx
  ret
PcDialogMove ENDP

PcDialogCommit PROC
  push rbx
  push rsi
  push rdi
  sub rsp,20h
  call PcDialogReadChecks
  xor ebx,ebx
  xor esi,esi
  lea rdi,pcDialogDraft
pcdcLoop:
  lea rax,pcDialogIds
  mov eax,[rax+rsi*4]
  lea rdx,pcDialogChecks
  cmp dword ptr [rdx+rax*4],0
  je pcdcNext
  mov [rdi+rbx*4+PC_L_IDS],eax
  inc ebx
pcdcNext:
  inc esi
  cmp esi,PC_COLUMN_COUNT
  jb pcdcLoop
  mov [rdi+PC_L_COUNT],ebx
pcdcUnused:
  cmp ebx,PC_LAYOUT_CAPACITY
  jae pcdcSort
  mov dword ptr [rdi+rbx*4+PC_L_IDS],-1
  inc ebx
  jmp pcdcUnused
pcdcSort:
  mov eax,[rdi+PC_L_SORT]
  lea rdx,pcDialogChecks
  cmp dword ptr [rdx+rax*4],1
  je pcdcApply
  mov dword ptr [rdi+PC_L_SORT],PC_NAME
  mov dword ptr [rdi+PC_L_DESCENDING],0
pcdcApply:
  mov rcx,rdi
  call PcApplyLayout
  test eax,eax
  jnz pcdcDone
  call PcPersistCurrent
  mov pcColumnsSaveError,eax
  xor eax,eax ; active layout stays applied even if persistence failed
pcdcDone:
  add rsp,20h
  pop rdi
  pop rsi
  pop rbx
  ret
PcDialogCommit ENDP

PcPersistCurrent PROC
  sub rsp,28h
  cmp pcColumnsPathReady,1
  jne pcpcUnavailable
  lea rcx,pcColumnsPath
  lea rdx,pcLayout
  call PcSaveLayout
  jmp pcpcDone
pcpcUnavailable:
  mov eax,3
pcpcDone:
  mov pcColumnsSaveError,eax
  add rsp,28h
  ret
PcPersistCurrent ENDP

; RCX=V3 path, RDX=V1 path, R8=V3 destination. Separate pcColumnsV2Path.
; Only ERROR_FILE_NOT_FOUND permits proceeding V3 -> V2 -> V1.
PcLoadColumnPaths PROC
  push rbx
  push rsi
  sub rsp,28h
  mov rbx,rdx
  mov rsi,r8
  mov rdx,r8
  call PcLoadLayout
  cmp eax,2
  jne pclpDone
  lea rcx,pcColumnsV2Path
  cmp word ptr [rcx],0
  je pclpV1
  mov rdx,rsi
  call PcLoadLayout
  cmp eax,2
  jne pclpDone
pclpV1:
  mov rcx,rbx
  mov rdx,rsi
  call PcLoadLayout
pclpDone:
  add rsp,28h
  pop rsi
  pop rbx
  ret
PcLoadColumnPaths ENDP

PcLoadCurrent PROC
  sub rsp,28h
  call PcEnsureLayout
  lea rcx,pcLayout
  call PcDefaultLayout
  mov pcColumnsPathReady,0
  mov pcColumnsLoadError,0
  cmp configReady,0
  je pclcDone
  lea rcx,storageDir
  call lstrlenW
  cmp eax,480 ; storageDir is 512 WCHAR; reserve the complete suffix and NUL.
  jbe pclcBuild
  mov pcColumnsLoadError,206
  jmp pclcDone
pclcBuild:
  lea rcx,pcColumnsPath
  lea rdx,storageDir
  call lstrcpyW
  lea rcx,pcColumnsPath
  lea rdx,pcColumnsSuffix
  call lstrcatW
  lea rcx,pcColumnsV2Path
  lea rdx,storageDir
  call lstrcpyW
  lea rcx,pcColumnsV2Path
  lea rdx,pcColumnsV2Suffix
  call lstrcatW
  lea rcx,pcColumnsLegacyPath
  lea rdx,storageDir
  call lstrcpyW
  lea rcx,pcColumnsLegacyPath
  lea rdx,pcColumnsLegacySuffix
  call lstrcatW
  mov pcColumnsPathReady,1
  lea rcx,pcColumnsPath
  lea rdx,pcColumnsLegacyPath
  lea r8,pcLayout
  call PcLoadColumnPaths
  mov pcColumnsLoadError,eax
pclcDone:
  add rsp,28h
  ret
PcLoadCurrent ENDP
