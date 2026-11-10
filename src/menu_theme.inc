; Documented owner-draw menus. dwItemData starts with MSAAMENUINFO from
; oleacc.h, retaining RU/EN names for accessibility and action reports.
extern GetMenuItemCount:proc, GetMenuItemInfoW:proc, SetMenuItemInfoW:proc
extern GetSubMenu:proc, SetMenuInfo:proc, DrawMenuBar:proc
extern CheckMenuItem:proc
extern GetDC:proc, ReleaseDC:proc, GetTextExtentPoint32W:proc
.data
menuThemeBrush dq 0
menuThemeSelectedBrush dq 0
menuThemeLineBrush dq 0
menuOwnerCount dd 0
menuOwnerReady dd 0
menuTextColor dd 202020h
menuMutedColor dd 707070h
.data?
align 8
menuOwnerEntries db 64*32 dup(?)
menuOwnerLabels dw 64*512 dup(?)
menuTempText dw 512 dup(?)
.code
ApplyMenuBrushes PROC
  sub rsp,58h
  mov rcx,menuThemeBrush
  test rcx,rcx
  jz ambNoBrush
  call DeleteObject
ambNoBrush:
  mov rcx,menuThemeSelectedBrush
  test rcx,rcx
  jz ambNoSelected
  call DeleteObject
ambNoSelected:
  mov rcx,menuThemeLineBrush
  test rcx,rcx
  jz ambNoLine
  call DeleteObject
ambNoLine:
  mov menuTextColor,202020h
  mov menuMutedColor,707070h
  mov ecx,0FFFFFFh
  cmp themeOption,0
  je ambBackground
  mov ecx,202020h
  mov menuTextColor,0EEEEEEh
  mov menuMutedColor,909090h
ambBackground:
  call CreateSolidBrush
  mov menuThemeBrush,rax
  mov ecx,0F4E5D6h
  cmp themeOption,0
  je ambSelected
  mov ecx,493D30h
ambSelected:
  call CreateSolidBrush
  mov menuThemeSelectedBrush,rax
  mov ecx,0DADADAh
  cmp themeOption,0
  je ambLine
  mov ecx,444444h
ambLine:
  call CreateSolidBrush
  mov menuThemeLineBrush,rax
  mov qword ptr [rsp+30h],40
  mov dword ptr [rsp+34h],80000002h ; MIM_BACKGROUND | APPLYTOSUBMENUS
  mov qword ptr [rsp+38h],0
  mov rax,menuThemeBrush
  mov [rsp+40h],rax
  mov qword ptr [rsp+48h],0
  mov qword ptr [rsp+50h],0
  mov rcx,mainMenu
  test rcx,rcx
  jz ambDone
  lea rdx,[rsp+30h]
  call SetMenuInfo
ambDone:
  add rsp,58h
  ret
ApplyMenuBrushes ENDP

; RCX menu, EDX=1 for top-level menu bar; recursive popup items use zero.
ThemeMenuTree PROC
  push rbx
  push rsi
  push rdi
  push r12
  push r13
  sub rsp,80h
  mov rbx,rcx
  mov r13d,edx
  call GetMenuItemCount
  mov r12d,eax
  xor esi,esi
tmtNext:
  cmp esi,r12d
  jae tmtDone
  lea rdi,[rsp+30h]
  xor eax,eax
  mov ecx,10
  rep stosq
  mov dword ptr [rsp+30h],80
  mov dword ptr [rsp+34h],146h ; FTYPE | ID | SUBMENU | STRING
  lea rax,menuTempText
  mov [rsp+68h],rax ; MENUITEMINFO.dwTypeData +56
  mov dword ptr [rsp+70h],512
  mov rcx,rbx
  mov edx,esi
  mov r8d,1
  lea r9,[rsp+30h]
  call GetMenuItemInfoW
  test eax,eax
  jz tmtAdvance
  mov eax,menuOwnerCount
  cmp eax,64
  jae tmtAdvance
  mov edi,eax
  shl rdi,5
  lea rcx,menuOwnerEntries
  add rdi,rcx
  mov dword ptr [rdi],0AA0DF00Dh ; MSAA_MENU_SIG
  lea rcx,menuTempText
  call lstrlenW
  mov [rdi+4],eax
  mov eax,menuOwnerCount
  shl rax,10
  lea rcx,menuOwnerLabels
  add rcx,rax
  mov [rdi+8],rcx
  lea rdx,menuTempText
  mov r8d,512
  call lstrcpynW
  mov eax,[rsp+40h] ; wID
  mov [rdi+16],eax
  mov eax,[rsp+38h] ; original fType plus private menu-bar flag
  mov [rdi+20],eax
  test r13d,r13d
  jz tmtFlags
  or dword ptr [rdi+20],80000000h
tmtFlags:
  mov rax,[rsp+48h] ; hSubMenu
  mov [rdi+24],rax
  inc menuOwnerCount
  mov dword ptr [rsp+34h],120h ; FTYPE | DATA; preserves MIIM_STRING
  or dword ptr [rsp+38h],100h ; MFT_OWNERDRAW
  mov [rsp+60h],rdi ; dwItemData
  mov rcx,rbx
  mov edx,esi
  mov r8d,1
  lea r9,[rsp+30h]
  call SetMenuItemInfoW
  mov rcx,[rdi+24]
  test rcx,rcx
  jz tmtAdvance
  xor edx,edx
  call ThemeMenuTree
tmtAdvance:
  inc esi
  jmp tmtNext
tmtDone:
  add rsp,80h
  pop r13
  pop r12
  pop rdi
  pop rsi
  pop rbx
  ret
ThemeMenuTree ENDP

RefreshSettingsChecks PROC
  push rbx
  sub rsp,20h
  mov rcx,mainMenu
  mov edx,4
  call GetSubMenu
  mov rbx,rax
  test rax,rax
  jz rscDone
  ; MF_BYCOMMAND | MF_CHECKED (or MF_UNCHECKED).
  mov rcx,rbx
  mov edx,2300
  xor r8d,r8d
  cmp appLanguage,1
  jne rscRu
  mov r8d,8
rscRu:
  call CheckMenuItem
  mov rcx,rbx
  mov edx,2301
  xor r8d,r8d
  cmp appLanguage,0
  jne rscEn
  mov r8d,8
rscEn:
  call CheckMenuItem
  mov rcx,rbx
  mov edx,2302
  xor r8d,r8d
  cmp themeOption,0
  jne rscLight
  mov r8d,8
rscLight:
  call CheckMenuItem
  mov rcx,rbx
  mov edx,2303
  xor r8d,r8d
  cmp themeOption,1
  jne rscDark
  mov r8d,8
rscDark:
  call CheckMenuItem
  mov rcx,rbx
  mov edx,2305
  xor r8d,r8d
  cmp refreshInterval,1000
  jne rscFast
  mov r8d,8
rscFast:
  call CheckMenuItem
  mov rcx,rbx
  mov edx,2306
  xor r8d,r8d
  cmp refreshInterval,3000
  jne rscSlow
  mov r8d,8
rscSlow:
  call CheckMenuItem
  mov rcx,rbx
  mov edx,2310
  xor r8d,r8d
  cmp treeEnabled,0
  je rscTree
  mov r8d,8
rscTree:
  call CheckMenuItem
rscDone:
  add rsp,20h
  pop rbx
  ret
RefreshSettingsChecks ENDP


ApplyMenuTheme PROC
  sub rsp,28h
  call ApplyMenuBrushes
  cmp uiRegressionMode,0
  jne amtSkipChecks
  call RefreshSettingsChecks
amtSkipChecks:
  cmp menuOwnerReady,0
  jne amtPaint
  mov menuOwnerCount,0
  mov rcx,mainMenu
  test rcx,rcx
  jz amtDone
  mov edx,1
  call ThemeMenuTree
  mov menuOwnerReady,1
amtPaint:
  mov rcx,gMain
  test rcx,rcx
  jz amtDone
  call DrawMenuBar
amtDone:
  add rsp,28h
  ret
ApplyMenuTheme ENDP

; R9 MEASUREITEMSTRUCT. Only ODT_MENU is accepted by the parent route.
MenuMeasure PROC
  push rbx
  push rsi
  sub rsp,68h
  mov rbx,r9
  mov rsi,[rbx+24]
  test rsi,rsi
  jz mmDefault
  cmp dword ptr [rsi],0AA0DF00Dh
  jne mmDefault
  test dword ptr [rsi+20],800h ; separator
  jnz mmSeparator
  mov rcx,gMain
  call GetDC
  mov [rsp+30h],rax
  test rax,rax
  jz mmFallback
  mov rcx,rax
  mov rdx,appFont
  call SelectObject
  mov [rsp+38h],rax
  mov rcx,[rsp+30h]
  mov rdx,[rsi+8]
  mov r8d,[rsi+4]
  mov qword ptr [rsp+40h],0
  lea r9,[rsp+40h]
  call GetTextExtentPoint32W
  mov rcx,[rsp+30h]
  mov rdx,[rsp+38h]
  call SelectObject
  mov rcx,gMain
  mov rdx,[rsp+30h]
  call ReleaseDC
  mov eax,[rsp+40h]
  jmp mmPadding
mmFallback:
  mov eax,[rsi+4]
  imul eax,8
mmPadding:
  mov ecx,48
  test dword ptr [rsi+20],80000000h
  jz mmWidth
  mov ecx,24
mmWidth:
  imul ecx,uiDpi
  add ecx,48
  mov [rsp+50h],eax
  mov eax,ecx
  xor edx,edx
  mov ecx,96
  div ecx
  add eax,[rsp+50h]
  mov [rbx+12],eax
  mov eax,30
  imul eax,uiDpi
  add eax,48
  xor edx,edx
  div ecx
  mov [rbx+16],eax
  jmp mmHandled
mmSeparator:
  mov dword ptr [rbx+12],1
  mov eax,9
  imul eax,uiDpi
  add eax,48
  xor edx,edx
  mov ecx,96
  div ecx
  mov [rbx+16],eax
mmHandled:
  mov eax,1
  jmp mmDone
mmDefault:
  xor eax,eax
mmDone:
  add rsp,68h
  pop rsi
  pop rbx
  ret
MenuMeasure ENDP

MenuDraw PROC
  push rbx
  push rsi
  push rdi
  sub rsp,70h
  mov rbx,r9
  mov rdi,[rbx+56]
  test rdi,rdi
  jz mdDefault
  cmp dword ptr [rdi],0AA0DF00Dh
  jne mdDefault
  mov rsi,[rbx+32]
  mov rcx,rsi
  call SaveDC
  mov [rsp+30h],eax
  mov r8,menuThemeBrush
  test dword ptr [rbx+16],41h ; ODS_SELECTED | ODS_HOTLIGHT
  jz mdBackground
  test dword ptr [rbx+16],6 ; ODS_DISABLED | ODS_GRAYED
  jnz mdBackground
  mov r8,menuThemeSelectedBrush
mdBackground:
  mov rcx,rsi
  lea rdx,[rbx+40]
  call FillRect
  mov eax,[rbx+40]
  mov [rsp+40h],eax
  mov eax,[rbx+44]
  mov [rsp+44h],eax
  mov eax,[rbx+48]
  mov [rsp+48h],eax
  mov eax,[rbx+52]
  mov [rsp+4Ch],eax
  test dword ptr [rdi+20],800h
  jnz mdSeparator
  mov rcx,rsi
  mov edx,menuTextColor
  test dword ptr [rbx+16],6
  jz mdInk
  mov edx,menuMutedColor
mdInk:
  call SetTextColor
  mov rcx,rsi
  mov edx,1
  call SetBkMode
  mov rcx,rsi
  mov rdx,appFont
  call SelectObject
  mov eax,12
  imul eax,uiDpi
  xor edx,edx
  mov ecx,96
  div ecx
  add [rsp+40h],eax
  sub [rsp+48h],eax
  mov dword ptr [rsp+20h],8824h ; SINGLELINE | VCENTER | NOPREFIX | ELLIPSIS
  test dword ptr [rdi+20],80000000h
  jz mdText
  or dword ptr [rsp+20h],1 ; DT_CENTER menu bar labels
mdText:
  mov rcx,rsi
  mov rdx,[rdi+8]
  mov r8d,[rdi+4]
  lea r9,[rsp+40h]
  call DrawTextW
  jmp mdRestore
mdSeparator:
  mov eax,[rsp+44h]
  add eax,[rsp+4Ch]
  sar eax,1
  mov [rsp+44h],eax
  inc eax
  mov [rsp+4Ch],eax
  add dword ptr [rsp+40h],12
  sub dword ptr [rsp+48h],12
  mov rcx,rsi
  lea rdx,[rsp+40h]
  mov r8,menuThemeLineBrush
  call FillRect
mdRestore:
  mov rcx,rsi
  mov edx,[rsp+30h]
  call RestoreDC
  mov eax,1
  jmp mdDone
mdDefault:
  xor eax,eax
mdDone:
  add rsp,70h
  pop rdi
  pop rsi
  pop rbx
  ret
MenuDraw ENDP

; ECX command ID -> RAX immutable original localized label (or zero).
FindMenuOwnerText PROC
  xor eax,eax
  xor edx,edx
  lea r8,menuOwnerEntries
fmotNext:
  cmp edx,menuOwnerCount
  jae fmotDone
  cmp ecx,[r8+16]
  je fmotFound
  add r8,32
  inc edx
  jmp fmotNext
fmotFound:
  mov rax,[r8+8]
fmotDone:
  ret
FindMenuOwnerText ENDP
