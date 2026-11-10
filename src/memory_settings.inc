extern DialogBoxParamW:proc, EndDialog:proc, SetDlgItemInt:proc, GetDlgItemInt:proc
.data
memorySettingFmt dw '%','u',0
memoryThresholdCaptionEn dw 77,101,109,111,114,121,32,116,104,114,101,115,104,111,108,100,58,32,37,117,37,37,46,46,46,0
memoryThresholdCaptionRu dw 1055,1086,1088,1086,1075,32,1079,1072,1085,1103,1090,1086,1081,32,1087,1072,1084,1103,1090,1080,58,32,37,117,37,37,46,46,46,0
autoMemoryKey dw 'A','u','t','o','M','e','m','o','r','y',0
memoryThresholdKey dw 'M','e','m','o','r','y','T','h','r','e','s','h','o','l','d',0
.data?
memorySettingNumber dw 16 dup(?)
.code
LoadMemorySettings PROC
  sub rsp,28h
  cmp configReady,0
  je lmsDone
  lea rcx,configSection
  lea rdx,autoMemoryKey
  xor r8d,r8d
  lea r9,configPath
  call GetPrivateProfileIntW
  and eax,1
  mov memoryPolicyAutoEnabled,eax
  lea rcx,configSection
  lea rdx,memoryThresholdKey
  mov r8d,85
  lea r9,configPath
  call GetPrivateProfileIntW
  cmp eax,50
  jb lmsDone
  cmp eax,95
  ja lmsDone
  mov memoryPolicyThreshold,eax
lmsDone:
  add rsp,28h
  ret
LoadMemorySettings ENDP

MemorySettingsCommand PROC
  sub rsp,38h
  cmp ecx,2341
  je mscThreshold
  xor memoryPolicyAutoEnabled,1
  call MemoryPolicyReset
  lea r8,zeroText
  cmp memoryPolicyAutoEnabled,0
  je mscAutoSave
  lea r8,oneText
mscAutoSave:
  lea rdx,autoMemoryKey
  jmp mscSave
mscThreshold:
  mov rcx,gInst
  mov edx,301
  cmp appLanguage,0
  je mscDialog
  mov edx,302
mscDialog:
  mov r8,gMain
  lea r9,MemoryThresholdProc
  mov qword ptr [rsp+20h],0
  call DialogBoxParamW
  cmp eax,1
  jne mscDone
  call MemoryPolicyReset
  lea rcx,memorySettingNumber
  lea rdx,memorySettingFmt
  mov r8d,memoryPolicyThreshold
  call wsprintfW
  lea r8,memorySettingNumber
  lea rdx,memoryThresholdKey
mscSave:
  cmp configReady,0
  je mscDone
  lea rcx,configSection
  lea r9,configPath
  call WritePrivateProfileStringW
mscDone:
  call RefreshSettingsChecks
  add rsp,38h
  ret
MemorySettingsCommand ENDP

MemoryThresholdProc PROC
  push rbx
  sub rsp,30h
  mov rbx,rcx
  cmp edx,110h
  je mtpInit
  cmp edx,111h
  je mtpCommand
  cmp edx,138h
  je mtpColor
  cmp edx,133h
  je mtpColor
  cmp edx,136h
  je mtpColor
  xor eax,eax
  jmp mtpDone
mtpColor:
  mov [rsp+20h],r8
  mov rcx,r8
  mov edx,shellTextColor
  call SetTextColor
  mov rcx,[rsp+20h]
  mov edx,0FFFFFFh
  cmp themeOption,0
  je mtpBackground
  mov edx,2E2E2Eh
mtpBackground:
  call SetBkColor
  mov rax,shellCardBrush
  jmp mtpDone
mtpInit:
  mov edx,3001
  mov r8d,memoryPolicyThreshold
  xor r9d,r9d
  call SetDlgItemInt
  mov eax,1
  jmp mtpDone
mtpCommand:
  and r8d,0FFFFh
  cmp r8d,2
  je mtpCancel
  cmp r8d,1
  jne mtpIgnored
  mov edx,3001
  lea r8,[rsp+28h]
  xor r9d,r9d
  call GetDlgItemInt
  cmp dword ptr [rsp+28h],0
  je mtpInvalid
  cmp eax,50
  jb mtpInvalid
  cmp eax,95
  ja mtpInvalid
  mov memoryPolicyThreshold,eax
  mov rcx,rbx
  mov edx,1
  call EndDialog
  jmp mtpHandled
mtpInvalid:
  mov rcx,rbx
  mov edx,3001
  mov r8d,memoryPolicyThreshold
  xor r9d,r9d
  call SetDlgItemInt
  jmp mtpHandled
mtpCancel:
  mov rcx,rbx
  xor edx,edx
  call EndDialog
mtpHandled:
  mov eax,1
  jmp mtpDone
mtpIgnored:
  xor eax,eax
mtpDone:
  add rsp,30h
  pop rbx
  ret
MemoryThresholdProc ENDP

UpdateMemoryMenuLabel PROC
  push rbx
  sub rsp,20h
  xor ebx,ebx
ummlNext:
  cmp ebx,menuOwnerCount
  jae ummlDone
  mov eax,ebx
  shl eax,5
  lea rcx,menuOwnerEntries
  add rcx,rax
  cmp dword ptr [rcx+16],2341
  je ummlFound
  inc ebx
  jmp ummlNext
ummlFound:
  mov rbx,rcx
  mov rcx,[rbx+8]
  lea rdx,memoryThresholdCaptionEn
  cmp appLanguage,0
  je ummlText
  lea rdx,memoryThresholdCaptionRu
ummlText:
  mov r8d,memoryPolicyThreshold
  call wsprintfW
  mov [rbx+4],eax
ummlDone:
  add rsp,20h
  pop rbx
  ret
UpdateMemoryMenuLabel ENDP
