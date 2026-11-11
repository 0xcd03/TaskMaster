; Standalone native picker regression. The fixture owns its hidden windows and
; temporary preference file; it never calls LoadSettings or resolves LocalAppData.
extern GetTempPathW:proc, GetTempFileNameW:proc, DeleteFileW:proc
.data
align 8
pcutStage dd 0
pcutAssertions dd 0
pcutMode dd 0
pcutOpens dd 0
pcutNotices dd 0
pcutWritten dd 0
pcutList dq 0
pcutFormat db 'Process columns UI: exit=%u assertions=%u opens=%u notices=%u',13,10,0
align 2
pcutPrefix dw 'p','c','u',0
pcutCodeFormat dw '%','u',0
pcutRowA dw 'Z',' ','f','i','x','t','u','r','e',0
pcutRowB dw 'A',' ','f','i','x','t','u','r','e',0
align 8
pcutExpected dd 0,1,3,6,4,5,7,8,9,10,11,12,13,14,15,16,17,18,19
.data?
align 8
pcutItem db 88 dup(?)
pcutBefore db PC_LAYOUT_SIZE dup(?)
pcutReadback db PC_LAYOUT_SIZE dup(?)
pcutRows db 8192 dup(?)
pcutTemp dw 1024 dup(?)
pcutOwnedPath dw 1024 dup(?)
pcutTitle dw 256 dup(?)
pcutNoticeCode dw 16 dup(?)
pcutOutput db 192 dup(?)
.code

; ECX index, EDX LVIS_STATEIMAGEMASK value (1000h/2000h).
PcutSetCheck PROC
  sub rsp,28h
  mov dword ptr [pcutItem+12],edx
  mov dword ptr [pcutItem+16],0F000h
  mov r8d,ecx
  mov rcx,pcutList
  mov edx,102Bh
  lea r9,pcutItem
  call SendMessageW
  add rsp,28h
  ret
PcutSetCheck ENDP

; ECX index -> EAX state image index.
PcutGetCheck PROC
  sub rsp,28h
  mov r8d,ecx
  mov rcx,pcutList
  mov edx,102Ch
  mov r9d,0F000h
  call SendMessageW
  shr eax,12
  add rsp,28h
  ret
PcutGetCheck ENDP

; ECX row -> stable ID, read from the actual control's lParam.
PcutGetId PROC
  sub rsp,28h
  mov dword ptr [pcutItem],4
  mov dword ptr [pcutItem+4],ecx
  mov dword ptr [pcutItem+8],0
  mov qword ptr [pcutItem+40],-1
  mov rcx,pcutList
  mov edx,104Bh
  xor r8d,r8d
  lea r9,pcutItem
  call SendMessageW
  test eax,eax
  jz pcutgiBad
  mov eax,dword ptr [pcutItem+40]
  jmp pcutgiDone
pcutgiBad:
  mov eax,-1
pcutgiDone:
  add rsp,28h
  ret
PcutGetId ENDP

; ECX selected row. Native selection notifications reach the real dialog.
PcutSelect PROC
  push rbx
  sub rsp,20h
  mov ebx,ecx
  mov dword ptr [pcutItem+12],0
  mov dword ptr [pcutItem+16],3
  mov rcx,pcutList
  mov edx,102Bh
  mov r8,-1
  lea r9,pcutItem
  call SendMessageW
  mov dword ptr [pcutItem+12],3
  mov rcx,pcutList
  mov edx,102Bh
  mov r8d,ebx
  lea r9,pcutItem
  call SendMessageW
  mov rcx,pcutList
  mov edx,100Ch
  mov r8,-1
  mov r9d,2
  call SendMessageW
  add rsp,20h
  pop rbx
  ret
PcutSelect ENDP

; ECX dialog control ID. Exercise the production WM_COMMAND route.
PcutCommand PROC
  sub rsp,28h
  mov r8d,ecx
  mov rcx,pcDialogWindow
  mov edx,111h
  xor r9d,r9d
  call SendMessageW
  add rsp,28h
  ret
PcutCommand ENDP

; Narrow MessageBoxW seam: suppress presentation, count the real error notice.
PcutSaveNotice PROC
  push rbx
  sub rsp,20h
  mov rbx,rdx
  lea rcx,pcutNoticeCode
  lea rdx,pcutCodeFormat
  mov r8d,pcColumnsSaveError
  call wsprintfW
  mov rcx,rbx
  lea rdx,pcutNoticeCode
  call StrStrIW
  test rax,rax
  jz pcutNoticeDone
  inc pcutNotices
pcutNoticeDone:
  mov eax,1
  add rsp,20h
  pop rbx
  ret
PcutSaveNotice ENDP

; Called by the real WM_INITDIALOG after population. Every path posts a closing
; command so assertion failures cannot leave a visible or stranded modal dialog.
PcutDialogHook PROC
  push rbx
  push rsi
  sub rsp,28h
  mov rbx,rcx
  inc pcutOpens
  xor edx,edx
  call ShowWindow
  mov pcutStage,8101
  mov rcx,rbx
  mov edx,4300
  call GetDlgItem
  mov pcutList,rax
  test rax,rax
  jz pcutHookFail
  mov rcx,rax
  mov edx,1004h
  xor r8d,r8d
  xor r9d,r9d
  call SendMessageW
  cmp eax,PC_COLUMN_COUNT
  jne pcutHookFail
  inc pcutAssertions
  mov pcutStage,8102
  xor ecx,ecx
  call PcutGetId
  test eax,eax
  jnz pcutHookFail
  inc pcutAssertions
  mov rcx,rbx
  lea rdx,pcutTitle
  mov r8d,256
  call GetWindowTextW
  test eax,eax
  jz pcutHookFail
  cmp appLanguage,0
  jne pcutHookRuTitle
  cmp word ptr [pcutTitle],'C'
  jne pcutHookFail
  jmp pcutHookTitleOk
pcutHookRuTitle:
  cmp word ptr [pcutTitle],412h ; Cyrillic Ve, resource308.
  jne pcutHookFail
pcutHookTitleOk:
  inc pcutAssertions
  mov rcx,pcutList
  mov edx,31h ; WM_GETFONT: font is assigned to the actual control.
  xor r8d,r8d
  xor r9d,r9d
  call SendMessageW
  test rax,rax
  jz pcutHookFail
  inc pcutAssertions
  mov rcx,pcutList
  mov edx,1000h ; LVM_GETBKCOLOR
  xor r8d,r8d
  xor r9d,r9d
  call SendMessageW
  mov ecx,0FFFFFFh
  cmp themeOption,0
  je pcutHookThemeColor
  mov ecx,202020h
pcutHookThemeColor:
  cmp eax,ecx
  jne pcutHookFail
  mov rcx,pcutList
  mov edx,1023h ; LVM_GETTEXTCOLOR
  xor r8d,r8d
  xor r9d,r9d
  call SendMessageW
  cmp eax,shellTextColor
  jne pcutHookFail
  mov rcx,rbx
  mov edx,4304
  lea r8,pcutTitle
  mov r9d,256
  call GetDlgItemTextW
  test eax,eax
  jz pcutHookFail
  add pcutAssertions,3
  ; Defaults/Cancel is also exercised after the eleven-column custom layout.
  cmp pcutMode,2
  je pcutHookDefaults
  cmp pcutMode,3
  je pcutHookDefaults
  mov pcutStage,81030
  xor ecx,ecx
  call PcutGetCheck
  cmp eax,2
  jne pcutHookFail
  mov pcutStage,8103
  xor ecx,ecx
  mov edx,1000h
  call PcutSetCheck
  xor ecx,ecx
  call PcutGetCheck
  mov pcutStage,81031
  cmp eax,2
  jne pcutHookFail
  mov ecx,6
  mov edx,1000h
  call PcutSetCheck
  mov ecx,6
  call PcutGetCheck
  mov pcutStage,81032
  cmp eax,2
  jne pcutHookFail
  add pcutAssertions,2
  mov pcutStage,8104
  xor ecx,ecx
  call PcutSelect
  mov ecx,4302
  call PcutCommand
  xor ecx,ecx
  call PcutGetId
  test eax,eax
  jnz pcutHookFail
  inc pcutAssertions
  ; CPU moves below working set; PID then moves ahead of IO/threads.
  mov ecx,2
  call PcutSelect
  mov pcutStage,81040
  cmp eax,2
  jne pcutHookFail
  mov ecx,4302
  call PcutCommand
  mov ecx,3
  call PcutGetId
  mov pcutStage,81041
  cmp eax,2
  jne pcutHookFail
  mov ecx,6
  call PcutSelect
  mov pcutStage,81043
  cmp eax,6
  jne pcutHookFail
  mov ecx,4301
  call PcutCommand
  mov ecx,4301
  call PcutCommand
  mov ecx,4
  call PcutGetId
  mov pcutStage,81042
  cmp eax,6
  jne pcutHookFail
  add pcutAssertions,2
  mov ecx,3
  mov edx,1000h
  call PcutSetCheck
  mov esi,7
pcutHookEnable:
  mov ecx,esi
  mov edx,2000h
  call PcutSetCheck
  inc esi
  cmp esi,PC_COLUMN_COUNT
  jb pcutHookEnable
  cmp pcutMode,0
  je pcutHookDefaults
  jmp pcutHookOk
pcutHookDefaults:
  mov ecx,4303
  call PcutCommand
  mov pcutStage,8105
  xor esi,esi
pcutHookDefaultsRows:
  mov ecx,esi
  call PcutGetId
  cmp eax,esi
  jne pcutHookFail
  mov ecx,esi
  call PcutGetCheck
  mov edx,1
  cmp esi,7
  jae pcutHookDefaultsState
  mov edx,2
pcutHookDefaultsState:
  cmp eax,edx
  jne pcutHookFail
  inc pcutAssertions
  inc esi
  cmp esi,PC_COLUMN_COUNT
  jb pcutHookDefaultsRows
  cmp pcutMode,3
  je pcutHookOk
  mov r8d,2
  jmp pcutHookPost
pcutHookOk:
  mov r8d,1
pcutHookPost:
  mov pcutStage,0
  jmp pcutHookClose
pcutHookFail:
  mov r8d,2
pcutHookClose:
  mov rcx,rbx
  mov edx,111h
  cmp pcutStage,0
  jne pcutHookPostClose
  cmp pcutMode,2
  jne pcutHookPostClose
  ; The actual modal message loop translates Escape into Cancel.
  mov edx,100h
  mov r8d,1Bh
pcutHookPostClose:
  xor r9d,r9d
  call PostMessageW
  add rsp,28h
  pop rsi
  pop rbx
  ret
PcutDialogHook ENDP

; Compare all bytes, including hidden widths and sentinel/reserved fields.
PcutCompare PROC
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
PcutCompare ENDP

PcutSnapshot PROC
  push rsi
  push rdi
  lea rsi,pcLayout
  lea rdi,pcutBefore
  mov ecx,PC_LAYOUT_SIZE/8
  rep movsq
  pop rdi
  pop rsi
  ret
PcutSnapshot ENDP

; Selection is verified by PID+birth, independently of the current sort index.
PcutCheckSelection PROC
  sub rsp,28h
  mov rcx,gList
  mov edx,100Ch
  mov r8,-1
  mov r9d,2
  call SendMessageW
  cmp eax,visibleCount
  jae pcutcsBad
  lea rdx,rowMap
  mov rax,[rdx+rax*8]
  cmp dword ptr [rax+3584],202
  jne pcutcsBad
  cmp qword ptr [rax+3600],2
  jne pcutcsBad
  xor eax,eax
  jmp pcutcsDone
pcutcsBad:
  mov eax,1
pcutcsDone:
  add rsp,28h
  ret
PcutCheckSelection ENDP

PcutCheckUnchanged PROC
  push rbx
  push rsi
  sub rsp,28h
  lea rcx,pcLayout
  lea rdx,pcutBefore
  call PcutCompare
  test eax,eax
  jnz pcutcuDone
  lea rcx,pcutOwnedPath
  lea rdx,pcutReadback
  call PcLoadLayout
  test eax,eax
  jnz pcutcuDone
  lea rcx,pcutReadback
  lea rdx,pcutBefore
  call PcutCompare
  test eax,eax
  jnz pcutcuDone
  mov ebx,dword ptr [pcutBefore+PC_L_COUNT]
  cmp pcVisibleCount,ebx
  jne pcutcuBad
  mov rcx,gList
  mov edx,101Fh ; LVM_GETHEADER
  xor r8d,r8d
  xor r9d,r9d
  call SendMessageW
  mov rcx,rax
  mov edx,1200h ; HDM_GETITEMCOUNT
  xor r8d,r8d
  xor r9d,r9d
  call SendMessageW
  cmp eax,ebx
  jne pcutcuBad
  xor esi,esi
pcutcuColumn:
  lea rdx,pcutBefore+PC_L_IDS
  mov eax,[rdx+rsi*4]
  lea rdx,pcVisibleIds
  cmp [rdx+rsi*4],eax
  jne pcutcuBad
  lea rdx,pcutBefore+PC_L_WIDTHS
  mov eax,[rdx+rax*4]
  mov [rsp+20h],eax
  mov rcx,gList
  mov edx,101Dh ; LVM_GETCOLUMNWIDTH
  mov r8d,esi
  xor r9d,r9d
  call SendMessageW
  cmp eax,[rsp+20h]
  jne pcutcuBad
  inc esi
  cmp esi,ebx
  jb pcutcuColumn
  call PcutCheckSelection
  jmp pcutcuDone
pcutcuBad:
  mov eax,1
pcutcuDone:
  add rsp,28h
  pop rsi
  pop rbx
  ret
PcutCheckUnchanged ENDP

ProcessColumnsUiTest PROC
  push rbx
  push rsi
  push rdi
  sub rsp,70h
  xor ecx,ecx
  call GetModuleHandleW
  mov gInst,rax
  lea rcx,commonControls
  call InitCommonControlsEx
  mov pcutStage,8001
  ; STATIC owner deliberately has no application startup/timers/collector.
  xor ecx,ecx
  lea rdx,szStatic
  xor r8d,r8d
  xor r9d,r9d
  mov qword ptr [rsp+20h],0
  mov qword ptr [rsp+28h],0
  mov qword ptr [rsp+30h],900
  mov qword ptr [rsp+38h],600
  mov qword ptr [rsp+40h],0
  mov qword ptr [rsp+48h],0
  mov rax,gInst
  mov [rsp+50h],rax
  mov qword ptr [rsp+58h],0
  call CreateWindowExW
  mov gMain,rax
  test rax,rax
  jz pcutReport
  xor ecx,ecx
  lea rdx,szList
  xor r8d,r8d
  mov r9d,4001100Dh ; hidden child, report, single select, owner data
  mov qword ptr [rsp+20h],0
  mov qword ptr [rsp+28h],0
  mov qword ptr [rsp+30h],900
  mov qword ptr [rsp+38h],500
  mov rax,gMain
  mov [rsp+40h],rax
  mov qword ptr [rsp+48h],1001
  mov rax,gInst
  mov [rsp+50h],rax
  mov qword ptr [rsp+58h],0
  call CreateWindowExW
  mov gList,rax
  test rax,rax
  jz pcutReport
  mov ecx,17
  call GetStockObject
  mov appFont,rax
  mov frontMode,0
  mov uiDpi,96
  lea rcx,pcutRows
  call ClearRow
  lea rcx,pcutRows+4096
  call ClearRow
  lea rcx,pcutRows
  lea rdx,pcutRowA
  call lstrcpyW
  lea rcx,pcutRows+4096
  lea rdx,pcutRowB
  call lstrcpyW
  mov dword ptr [pcutRows+3584],101
  mov qword ptr [pcutRows+3600],1
  mov dword ptr [pcutRows+4096+3584],202
  mov qword ptr [pcutRows+4096+3600],2
  lea rax,pcutRows
  mov frontBuffer,rax
  mov frontCount,2
  mov pcutStage,8002
  mov ecx,1024
  lea rdx,pcutTemp
  call GetTempPathW
  test eax,eax
  jz pcutReport
  cmp eax,900
  jae pcutReport
  lea rcx,pcutTemp
  lea rdx,pcutPrefix
  xor r8d,r8d
  lea r9,pcutOwnedPath
  call GetTempFileNameW
  test eax,eax
  jz pcutReport
  lea rcx,pcColumnsPath
  lea rdx,pcutOwnedPath
  call lstrcpyW
  mov pcColumnsPathReady,1
  lea rax,PcutDialogHook
  mov pcDialogHook,rax
  lea rax,PcutSaveNotice
  mov pcDialogSaveNotice,rax
  mov themeOption,0
pcutTheme:
  call UpdateShellTheme
  mov appLanguage,0
pcutLanguage:
  mov pcutStage,8003
  lea rcx,pcutReadback
  call PcDefaultLayout
  mov pcLayoutReady,1
  mov dword ptr [pcutReadback+PC_L_SORT],PC_CPU
  mov dword ptr [pcutReadback+PC_L_DESCENDING],1
  mov dword ptr [pcutReadback+PC_L_WIDTHS+PC_CPU*4],333
  mov dword ptr [pcutReadback+PC_L_WIDTHS+PC_PID*4],444
  lea rcx,pcutReadback
  call PcApplyLayout
  test eax,eax
  jnz pcutReport
  xor esi,esi
pcutSelectIdentity:
  cmp esi,visibleCount
  jae pcutReport
  lea rdx,rowMap
  mov rax,[rdx+rsi*8]
  cmp dword ptr [rax+3584],202
  je pcutSelectFound
  inc esi
  jmp pcutSelectIdentity
pcutSelectFound:
  mov dword ptr [pcutItem+12],3
  mov dword ptr [pcutItem+16],3
  mov rcx,gList
  mov edx,102Bh
  mov r8d,esi
  lea r9,pcutItem
  call SendMessageW
  call PcutCheckSelection
  test eax,eax
  jnz pcutReport
  lea rcx,pcutOwnedPath
  lea rdx,pcLayout
  call PcSaveLayout
  test eax,eax
  jnz pcutReport
  call PcutSnapshot
  mov pcutMode,0
  call ShowProcessColumnsDialog
  cmp pcutStage,0
  jne pcutReport
  mov pcutStage,8201
  call PcutCheckUnchanged
  test eax,eax
  jnz pcutReport
  cmp pcVisibleCount,7
  jne pcutReport
  inc pcutAssertions
  mov pcutMode,1
  call ShowProcessColumnsDialog
  cmp pcutStage,0
  jne pcutReport
  mov pcutStage,8202
  cmp eax,1
  jne pcutReport
  cmp dword ptr [pcLayout+PC_L_COUNT],19
  jne pcutReport
  cmp pcVisibleCount,19
  jne pcutReport
  call PcutCheckSelection
  test eax,eax
  jnz pcutReport
  cmp dword ptr [pcLayout+PC_L_SORT],0
  jne pcutReport
  cmp dword ptr [pcLayout+PC_L_DESCENDING],0
  jne pcutReport
  cmp dword ptr [pcLayout+PC_L_IDS+76],-1
  jne pcutReport
  cmp dword ptr [pcLayout+PC_L_WIDTHS+PC_CPU*4],333
  jne pcutReport
  cmp dword ptr [pcLayout+PC_L_WIDTHS+PC_PID*4],444
  jne pcutReport
  xor esi,esi
pcutOrder:
  lea rdx,pcutExpected
  mov eax,dword ptr [rdx+rsi*4]
  lea rdx,pcLayout+PC_L_IDS
  cmp dword ptr [rdx+rsi*4],eax
  jne pcutReport
  lea rdx,pcVisibleIds
  cmp dword ptr [rdx+rsi*4],eax
  jne pcutReport
  inc pcutAssertions
  inc esi
  cmp esi,19
  jb pcutOrder
  call PcutSnapshot
  call PcutCheckUnchanged
  test eax,eax
  jnz pcutReport
  mov pcutMode,2
  call ShowProcessColumnsDialog
  cmp pcutStage,0
  jne pcutReport
  mov pcutStage,8203
  call PcutCheckUnchanged
  test eax,eax
  jnz pcutReport
  cmp pcVisibleCount,19
  jne pcutReport
  inc pcutAssertions
  mov pcutMode,3
  call ShowProcessColumnsDialog
  cmp pcutStage,0
  jne pcutReport
  mov pcutStage,8204
  cmp dword ptr [pcLayout+PC_L_COUNT],7
  jne pcutReport
  cmp pcVisibleCount,7
  jne pcutReport
  call PcutCheckSelection
  test eax,eax
  jnz pcutReport
  lea rcx,pcutReadback
  call PcDefaultLayout
  lea rcx,pcLayout
  lea rdx,pcutReadback
  call PcutCompare
  test eax,eax
  jnz pcutReport
  inc pcutAssertions
  ; Existing owned FILE used as parent directory guarantees a save error.
  lea rcx,pcColumnsPath
  lea rdx,pcutOwnedPath
  call lstrcpyW
  lea rdi,pcColumnsPath
pcutPathEnd:
  cmp word ptr [rdi],0
  je pcutBadPath
  add rdi,2
  jmp pcutPathEnd
pcutBadPath:
  mov word ptr [rdi],5Ch
  mov word ptr [rdi+2],'x'
  mov word ptr [rdi+4],0
  mov pcutNotices,0
  mov pcutMode,4
  call ShowProcessColumnsDialog
  cmp pcutStage,0
  jne pcutReport
  mov pcutStage,8205
  cmp eax,1
  jne pcutReport
  cmp pcutNotices,1
  jne pcutReport
  cmp pcColumnsSaveError,0
  je pcutReport
  cmp dword ptr [pcLayout+PC_L_COUNT],19
  jne pcutReport
  cmp pcVisibleCount,19
  jne pcutReport
  call PcutCheckSelection
  test eax,eax
  jnz pcutReport
  lea rcx,pcutOwnedPath
  lea rdx,pcutReadback
  call PcLoadLayout
  test eax,eax
  jnz pcutReport
  lea rcx,pcutBefore
  call PcDefaultLayout
  lea rcx,pcutReadback
  lea rdx,pcutBefore
  call PcutCompare
  test eax,eax
  jnz pcutReport
  add pcutAssertions,5
  lea rcx,pcColumnsPath
  lea rdx,pcutOwnedPath
  call lstrcpyW
  inc appLanguage
  cmp appLanguage,2
  jb pcutLanguage
  inc themeOption
  cmp themeOption,2
  jb pcutTheme
  ; A non-process panel must not even invoke the dialog observer.
  mov pcutStage,8206
  mov ebx,pcutOpens
  mov frontMode,2
  call ShowProcessColumnsDialog
  cmp pcutOpens,ebx
  jne pcutReport
  inc pcutAssertions
  mov pcutStage,0
pcutReport:
  mov pcDialogHook,0
  mov pcColumnsPathReady,0
  lea rcx,pcutOwnedPath
  cmp word ptr [rcx],0
  je pcutReportWindow
  call DeleteFileW
pcutReportWindow:
  mov rcx,gMain
  test rcx,rcx
  jz pcutReportText
  call DestroyWindow
pcutReportText:
  lea rcx,pcutOutput
  lea rdx,pcutFormat
  mov r8d,pcutStage
  mov r9d,pcutAssertions
  mov eax,pcutOpens
  mov [rsp+20h],rax
  mov eax,pcutNotices
  mov [rsp+28h],rax
  call wsprintfA
  mov ebx,eax
  mov ecx,-11
  call GetStdHandle
  mov rcx,rax
  lea rdx,pcutOutput
  mov r8d,ebx
  lea r9,pcutWritten
  mov qword ptr [rsp+20h],0
  call WriteFile
  mov ecx,pcutStage
  call ExitProcess
ProcessColumnsUiTest ENDP
