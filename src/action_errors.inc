extern FormatMessageW:proc, GetMenuStringW:proc
.data
actionNewline dw 13,10,0
.data?
actionLabel dw 256 dup(?)
actionErrorText dw 512 dup(?)
.code
BuildActionReport PROC
  sub rsp,48h
  mov rcx,mainMenu
  mov edx,actionCode
  lea r8,actionLabel
  mov r9d,256
  mov qword ptr [rsp+20h],0
  call GetMenuStringW
  test eax,eax
  jnz barName
  mov ecx,actionCode
  lea rdx,actionLabel
  call StoreDecimal
barName:
  lea rcx,actionMessage
  lea rdx,actionFmt
  lea r8,actionLabel
  mov r9d,actionSucceeded
  mov eax,actionFailed
  mov [rsp+20h],eax
  mov eax,actionResult
  mov [rsp+28h],eax
  call wsprintfW
  mov eax,actionResult
  test eax,eax
  jz barDone
  mov r8d,eax
  and eax,0FFFF0000h
  cmp eax,80070000h
  jne barSystem
  and r8d,0FFFFh
barSystem:
  mov ecx,1200h
  xor edx,edx
  mov r9d,409h
  cmp appLanguage,0
  je barLocale
  mov r9d,419h
barLocale:
  lea rax,actionErrorText
  mov [rsp+20h],rax
  mov qword ptr [rsp+28h],512
  mov qword ptr [rsp+30h],0
  call FormatMessageW
  test eax,eax
  jz barDone
  lea rcx,actionMessage
  lea rdx,actionNewline
  call lstrcatW
  lea rcx,actionMessage
  lea rdx,actionErrorText
  call lstrcatW
barDone:
  add rsp,48h
  ret
BuildActionReport ENDP
