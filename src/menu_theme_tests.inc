; Integration contract for documented native owner-draw menus: retain the
; original resource names/IDs and MSAA names, support DPI, never touch controls.
extern DestroyMenu:proc
.data?
align 8
menuTestOriginal dw 512 dup(?)
menuTestItem db 80 dup(?)
menuTestMeasure db 40 dup(?)
menuTestSaved db 32 dup(?)
.code
MenuThemeTests PROC
  push rbx
  push rsi
  push rdi
  push r12
  sub rsp,38h
  mov rax,mainMenu
  mov qword ptr [menuTestSaved],rax
  mov eax,menuOwnerReady
  mov dword ptr [menuTestSaved+8],eax
  mov eax,uiDpi
  mov dword ptr [menuTestSaved+12],eax
  mov eax,menuOwnerCount
  mov dword ptr [menuTestSaved+16],eax
  xor r12d,r12d
  mov esi,101
mttLanguage:
  mov modelStage,1501
  mov rcx,gInst
  mov edx,esi
  call LoadMenuW
  mov rbx,rax
  test rax,rax
  jz mttFail
  mov rcx,rbx
  mov edx,2001
  lea r8,menuTestOriginal
  mov r9d,512
  mov qword ptr [rsp+20h],0
  call GetMenuStringW
  test eax,eax
  jz mttCloseFail
  mov menuOwnerCount,0
  mov rcx,rbx
  mov edx,1
  call ThemeMenuTree
  cmp menuOwnerCount,0
  je mttCloseFail
  mov ecx,2001
  call FindMenuOwnerText
  test rax,rax
  jz mttCloseFail
  mov rcx,rax
  lea rdx,menuTestOriginal
  call lstrcmpW
  test eax,eax
  jnz mttCloseFail
  ; Command ID and MSAA data survive theme conversion through Win32 lookup.
  lea rdi,menuTestItem
  xor eax,eax
  mov ecx,10
  rep stosq
  mov dword ptr [menuTestItem],80
  mov dword ptr [menuTestItem+4],122h ; ID | DATA | FTYPE
  mov rcx,rbx
  mov edx,2001
  xor r8d,r8d
  lea r9,menuTestItem
  call GetMenuItemInfoW
  test eax,eax
  jz mttCloseFail
  cmp dword ptr [menuTestItem+16],2001
  jne mttCloseFail
  test dword ptr [menuTestItem+8],100h
  jz mttCloseFail
  mov rdi,qword ptr [menuTestItem+48]
  test rdi,rdi
  jz mttCloseFail
  cmp dword ptr [rdi],0AA0DF00Dh
  jne mttCloseFail
  mov rcx,[rdi+8]
  lea rdx,menuTestOriginal
  call lstrcmpW
  test eax,eax
  jnz mttCloseFail
  mov modelStage,1502
  mov rax,0A5A5A5A5A5A5A5A5h
  mov qword ptr [menuTestMeasure],rax
  mov qword ptr [menuTestMeasure+8],rax
  mov qword ptr [menuTestMeasure+16],rax
  mov qword ptr [menuTestMeasure+24],rdi
  mov qword ptr [menuTestMeasure+32],rax
  mov dword ptr [menuTestMeasure],1
  mov uiDpi,96
  lea r9,menuTestMeasure
  call MenuMeasure
  cmp eax,1
  jne mttCloseFail
  cmp dword ptr [menuTestMeasure+16],30
  jne mttCloseFail
  cmp dword ptr [menuTestMeasure+12],48
  jbe mttCloseFail
  mov rax,0A5A5A5A5A5A5A5A5h
  cmp qword ptr [menuTestMeasure+32],rax
  jne mttCloseFail
  cmp qword ptr [menuTestMeasure+24],rdi
  jne mttCloseFail
  mov uiDpi,192
  lea r9,menuTestMeasure
  call MenuMeasure
  cmp dword ptr [menuTestMeasure+16],60
  jne mttCloseFail
  mov rcx,rbx
  call DestroyMenu
  inc esi
  cmp esi,103
  jb mttLanguage
  jmp mttRestore
mttCloseFail:
  mov rcx,rbx
  call DestroyMenu
mttFail:
  mov r12d,1
mttRestore:
  mov rax,qword ptr [menuTestSaved]
  mov mainMenu,rax
  mov eax,dword ptr [menuTestSaved+8]
  mov menuOwnerReady,eax
  mov eax,dword ptr [menuTestSaved+12]
  mov uiDpi,eax
  mov eax,dword ptr [menuTestSaved+16]
  mov menuOwnerCount,eax
  mov eax,r12d
  add rsp,38h
  pop r12
  pop rdi
  pop rsi
  pop rbx
  ret
MenuThemeTests ENDP
