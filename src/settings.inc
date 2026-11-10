extern SHGetFolderPathW:proc, CreateDirectoryW:proc, lstrcatW:proc
extern GetPrivateProfileIntW:proc, WritePrivateProfileStringW:proc
extern LoadMenuW:proc, LoadStringW:proc, CreateFontW:proc
extern DwmSetWindowAttribute:proc, CreateSolidBrush:proc, SetClassLongPtrW:proc
extern RedrawWindow:proc
extern SetWindowTheme:proc, SetBkColor:proc
.data
align 8
appLanguage dd 1
selectedLanguage dd 1
themeOption dd 0
refreshInterval dd 1000
configReady dd 0
appFont dq 0
appBrush dq 0
.data?
align 8
navButtons dq 6 dup(?)
.data
.data?
align 8
localizedBuffer dw 512 dup(?)
.data
.data?
align 8
configPath dw 512 dup(?)
.data
.data?
align 8
storageDir dw 512 dup(?)
.data
configSection dw 'U','I',0
languageKey dw 'L','a','n','g','u','a','g','e',0
themeKey dw 'D','a','r','k',0
intervalKey dw 'I','n','t','e','r','v','a','l',0
configDir dw '\','T','a','s','k','M','a','n','a','g','e','r','X','6','4',0
configFile dw '\','s','e','t','t','i','n','g','s','.','i','n','i',0
fontFace dw 'S','e','g','o','e',' ','U','I',0
zeroText dw '0',0
oneText dw '1',0
oneSecondText dw '1','0','0','0',0
threeSecondText dw '3','0','0','0',0
explorerThemeName dw 'E','x','p','l','o','r','e','r',0
darkExplorerThemeName dw 'D','a','r','k','M','o','d','e','_','E','x','p','l','o','r','e','r',0
.code
LoadSettings PROC
  sub rsp,98h
  xor ecx,ecx
  mov edx,1Ch
  xor r8d,r8d
  xor r9d,r9d
  lea rax,configPath
  mov [rsp+20h],rax
  call SHGetFolderPathW
  test eax,eax
  js lsFont
  lea rcx,configPath
  lea rdx,configDir
  call lstrcatW
  lea rcx,configPath
  xor edx,edx
  call CreateDirectoryW
  lea rcx,storageDir
  lea rdx,configPath
  call lstrcpyW
  lea rcx,configPath
  lea rdx,configFile
  call lstrcatW
  mov configReady,1
  lea rcx,configSection
  lea rdx,languageKey
  mov r8d,1
  lea r9,configPath
  call GetPrivateProfileIntW
  and eax,1
  mov appLanguage,eax
  mov selectedLanguage,eax
  lea rcx,configSection
  lea rdx,themeKey
  xor r8d,r8d
  lea r9,configPath
  call GetPrivateProfileIntW
  and eax,1
  mov themeOption,eax
  lea rcx,configSection
  lea rdx,intervalKey
  mov r8d,1000
  lea r9,configPath
  call GetPrivateProfileIntW
  cmp eax,500
  jb lsFont
  cmp eax,10000
  ja lsFont
  mov refreshInterval,eax
lsFont:
  call LoadMemorySettings
  call GetDpiForSystem
  mov uiDpi,eax
  call RecalcUiScale
  call LoadUiStrings
  mov ecx,-16
  xor edx,edx
  xor r8d,r8d
  xor r9d,r9d
  mov qword ptr [rsp+20h],400
  mov qword ptr [rsp+28h],0
  mov qword ptr [rsp+30h],0
  mov qword ptr [rsp+38h],0
  mov qword ptr [rsp+40h],1
  mov qword ptr [rsp+48h],0
  mov qword ptr [rsp+50h],0
  mov qword ptr [rsp+58h],5
  mov qword ptr [rsp+60h],0
  lea rax,fontFace
  mov [rsp+68h],rax
  call CreateFontW
  mov appFont,rax
  add rsp,98h
  ret
LoadSettings ENDP

; ECX English string resource ID; Russian resources use ID+100.
LocalizedText PROC
  sub rsp,28h
  mov edx,ecx
  cmp appLanguage,0
  je ltCall
  add edx,100
ltCall:
  mov rcx,gInst
  ; LoadStringW pointer mode returns a counted, non-NUL-terminated string.
  ; Copy into our buffer for CreateWindowExW and the search cue banner.
  lea r8,localizedBuffer
  mov r9d,512
  call LoadStringW
  lea rax,localizedBuffer
  add rsp,28h
  ret
LocalizedText ENDP

BuildSidebar PROC
  push rbx
  push rsi
  push rdi
  sub rsp,60h
  call CreateShellSidebar
  xor esi,esi
  lea rdi,navButtons
bsNext:
  mov ecx,100
  add ecx,esi
  cmp esi,4
  jb bsString
  add ecx,2
bsString:
  call LocalizedText
  mov r8,rax
  xor ecx,ecx
  lea rdx,szButton
  mov r9d,5001000Bh
  mov qword ptr [rsp+20h],12
  mov eax,esi
  imul eax,48
  add eax,52
  mov [rsp+28h],rax
  mov qword ptr [rsp+30h],172
  mov qword ptr [rsp+38h],40
  mov rax,shellWnd
  mov [rsp+40h],rax
  xor eax,eax
  test esi,esi
  jz bsId
  lea eax,[rsi+1]
bsId:
  add eax,2100
  mov [rsp+48h],rax
  mov rax,gInst
  mov [rsp+50h],rax
  mov qword ptr [rsp+58h],0
  call CreateWindowExW
  mov [rdi+rsi*8],rax
  mov rcx,rax
  mov edx,30h
  mov r8,appFont
  mov r9d,1
  call SendMessageW
  inc esi
  cmp esi,6
  jb bsNext
  call CreateMemoryButton
  mov rcx,gList
  mov edx,30h
  mov r8,appFont
  mov r9d,1
  call SendMessageW
  mov rcx,searchWnd
  mov edx,30h
  mov r8,appFont
  mov r9d,1
  call SendMessageW
  mov ecx,104
  call LocalizedText
  mov r9,rax
  mov rcx,searchWnd
  mov edx,1501h
  mov r8d,1
  call SendMessageW
  add rsp,60h
  pop rdi
  pop rsi
  pop rbx
  ret
BuildSidebar ENDP

LayoutSearch PROC
  sub rsp,38h
  mov rcx,searchWnd
  mov edx,uiContentLeft
  mov r8d,uiInset
  mov r9d,gRect.rcRight
  sub r9d,uiContentSpace
  mov eax,uiSearchHeight
  mov [rsp+20h],rax
  mov qword ptr [rsp+28h],1
  call MoveWindow
  add rsp,38h
  ret
LayoutSearch ENDP

ApplyTheme PROC
  sub rsp,28h
  call UpdateShellTheme
  call ApplyMenuTheme
  ; Ask the native control theme to paint its nonclient scrollbars and border.
  ; Manual drawing/colors continue to work if a theme part is unavailable.
  mov rcx,gList
  lea rdx,explorerThemeName
  cmp themeOption,0
  je atListTheme
  lea rdx,darkExplorerThemeName
atListTheme:
  xor r8d,r8d
  call SetWindowTheme
  mov rcx,searchWnd
  lea rdx,explorerThemeName
  cmp themeOption,0
  je atSearchTheme
  lea rdx,darkExplorerThemeName
atSearchTheme:
  xor r8d,r8d
  call SetWindowTheme
  mov rcx,actionDetailsText
  lea rdx,explorerThemeName
  cmp themeOption,0
  je atReportTheme
  lea rdx,darkExplorerThemeName
atReportTheme:
  xor r8d,r8d
  call SetWindowTheme
  mov rcx,reportWnd
  xor edx,edx
  xor r8d,r8d
  mov r9d,85h ; invalidate/erase/all children
  call RedrawWindow
  mov rcx,gMain
  mov edx,20
  lea r8,themeOption
  mov r9d,4
  call DwmSetWindowAttribute
  mov r9d,0FFFFFFh
  cmp themeOption,0
  je atBackground
  mov r9d,202020h
atBackground:
  mov rcx,gList
  mov edx,1001h
  xor r8d,r8d
  call SendMessageW
  mov r9d,0FFFFFFh
  cmp themeOption,0
  je atTextBk
  mov r9d,202020h
atTextBk:
  mov rcx,gList
  mov edx,1026h
  xor r8d,r8d
  call SendMessageW
  mov r9d,202020h
  cmp themeOption,0
  je atText
  mov r9d,0EEEEEEh
atText:
  mov rcx,gList
  mov edx,1024h
  xor r8d,r8d
  call SendMessageW
  mov rcx,searchWnd
  xor edx,edx
  mov r8d,1
  call InvalidateRect
  mov rcx,processHeader
  xor edx,edx
  mov r8d,1
  call InvalidateRect
  mov rcx,gList
  xor edx,edx
  mov r8d,1
  call InvalidateRect
  add rsp,28h
  ret
ApplyTheme ENDP

SettingsCommand PROC
  sub rsp,28h
  cmp ecx,2400
  jb scOrdinary
  call QueueStartupAction
  jmp scDone
scOrdinary:
  cmp ecx,2330
  je scIntegration
  cmp ecx,2331
  je scIntegration
  cmp ecx,2340
  je scMemoryPolicy
  cmp ecx,2341
  je scMemoryPolicy
  cmp ecx,2320
  je scHistoryStatus
  cmp ecx,2310
  je scTree
  cmp ecx,2300
  je scRu
  cmp ecx,2301
  je scEn
  cmp ecx,2302
  je scLight
  cmp ecx,2303
  je scDark
  cmp ecx,2305
  je scFast
  cmp ecx,2306
  je scSlow
  jmp scDone
scIntegration:
  call IntegrationCommand
  jmp scDone
scMemoryPolicy:
  call MemorySettingsCommand
  jmp scDone
scHistoryStatus:
  lea rcx,actionMessage
  lea rdx,historyStatusFmt
  mov r8d,historyCount
  mov r9d,historyLoadError
  mov eax,historySaveError
  mov [rsp+20h],eax
  call wsprintfW
  mov rcx,gMain
  lea rdx,actionMessage
  lea r8,szTitle
  mov r9d,40h
  call MessageBoxW
  jmp scDone
scTree:
  xor treeEnabled,1
  call SetModeColumns
  call BuildRowMap
  jmp scDone
scRu:
  mov selectedLanguage,1
  lea rdx,languageKey
  lea r8,oneText
  jmp scSave
scEn:
  mov selectedLanguage,0
  lea rdx,languageKey
  lea r8,zeroText
  jmp scSave
scLight:
  mov themeOption,0
  call ApplyTheme
  lea rdx,themeKey
  lea r8,zeroText
  jmp scSave
scDark:
  mov themeOption,1
  call ApplyTheme
  lea rdx,themeKey
  lea r8,oneText
  jmp scSave
scFast:
  mov refreshInterval,1000
  lea rdx,intervalKey
  lea r8,oneSecondText
  jmp scSave
scSlow:
  mov refreshInterval,3000
  lea rdx,intervalKey
  lea r8,threeSecondText
scSave:
  cmp configReady,0
  je scDone
  lea rcx,configSection
  lea r9,configPath
  call WritePrivateProfileStringW
scDone:
  call RefreshSettingsChecks
  add rsp,28h
  ret
SettingsCommand ENDP
include shell_ui.inc

