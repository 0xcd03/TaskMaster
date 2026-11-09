extern RegCreateKeyExW:proc, RegQueryValueExW:proc, RegSetValueExW:proc, RegDeleteValueW:proc
extern FindFirstFileW:proc, FindNextFileW:proc, FindClose:proc, GetFileAttributesW:proc
extern CoInitializeEx:proc, CoUninitialize:proc, CoCreateInstance:proc
extern SysAllocString:proc, SysFreeString:proc, SysStringLen:proc
.data
startupView dd 0
startupKind dd 0
startupDisabled dd 0
startupOnce dd 0
startupErrors dd 0
startupActionKind dd 0
startupActionDisabled dd 0
startupActionOnce dd 0
startupActionView dd 0
startupActionRoot dq 0
startupActionHash dq 0
startupCom dd 0
startupTestMode dd 0 ; set only by the separate ModelTest entry, never by GUI input
taskService dq 0
taskFolder dq 0
taskDepth dd 0
taskError dd 0
startupTaskStamp dq 0
startupErrorSource dq 0
taskClass dd 0F87369Fh
dw 0A4E5h,4CFCh
db 0BDh,3Eh,73h,0E6h,15h,45h,72h,0DDh
taskIID dd 2FABA4C7h
dw 4DA9h,4013h
db 96h,97h,20h,0CCh,3Fh,0D4h,0Fh,85h
taskRoot dw '\',0
runOnceKey dw 'S','o','f','t','w','a','r','e','\','M','i','c','r','o','s','o','f','t','\','W'
dw 'i','n','d','o','w','s','\','C','u','r','r','e','n','t','V','e','r','s','i','o'
dw 'n','\','R','u','n','O','n','c','e',0
disabledSuffix dw '\','T','a','s','k','M','a','n','a','g','e','r','X','6','4','D','i','s','a','b'
dw 'l','e','d',0
startupSlash dw '\',0
startupWildcard dw '\','*',0
startupRunSource dw 'R','u','n',0
.data?
align 16
taskEmpty db 16 dup(?)
taskIndexVariant db 16 dup(?)
startupPath dw 1024 dup(?)
startupBase dw 1024 dup(?)
startupKeyPath dw 512 dup(?)
startupDestKeyPath dw 512 dup(?)
startupActionName dw 512 dup(?)
startupActionPath dw 512 dup(?)
startupActionBase dw 512 dup(?)
startupFindData db 592 dup(?)
startupLongName dw 16384 dup(?)
startupSourceLabel dw 256 dup(?)
taskString dq ?
.code
; Input: RDX bytes, ECX size, R8D registry type. No Win32/UI calls.
StartupHash PROC
  mov rax,0CBF29CE484222325h
  mov r9,100000001B3h
  xor rax,r8
shashNext:
  test ecx,ecx
  jz shashDone
  movzx r10d,byte ptr [rdx]
  xor rax,r10
  imul rax,r9
  inc rdx
  dec ecx
  jmp shashNext
shashDone:
  ret
StartupHash ENDP

InitTaskScheduler PROC
  sub rsp,38h
  xor ecx,ecx
  xor edx,edx
  call CoInitializeEx
  test eax,eax
  js itsDone
  mov startupCom,1
  lea rcx,taskClass
  xor edx,edx
  mov r8d,1
  lea r9,taskIID
  lea rax,taskService
  mov [rsp+20h],rax
  call CoCreateInstance
  test eax,eax
  js itsError
  mov rcx,taskService
  lea rdx,taskEmpty
  lea r8,taskEmpty
  lea r9,taskEmpty
  lea rax,taskEmpty
  mov [rsp+20h],rax
  mov rax,[rcx]
  call qword ptr [rax+80]
  test eax,eax
  js itsError
  lea rcx,taskRoot
  call SysAllocString
  mov taskString,rax
  mov rcx,taskService
  mov rdx,rax
  lea r8,taskFolder
  mov rax,[rcx]
  call qword ptr [rax+56]
  mov taskError,eax
  mov rcx,taskString
  call SysFreeString
  jmp itsDone
itsError:
  mov taskError,eax
itsDone:
  add rsp,38h
  ret
InitTaskScheduler ENDP

CollectStartupV2 PROC
  sub rsp,28h
  mov backCount,0
  mov startupErrors,0
  mov startupRoot,-2147483647
csvHive:
  mov startupView,100h
csvView:
  mov startupOnce,0
csvKey:
  mov startupDisabled,0
csvState:
  call CollectRegistrySource
  xor startupDisabled,1
  cmp startupDisabled,0
  jne csvState
  xor startupOnce,1
  cmp startupOnce,0
  jne csvKey
  xor startupView,300h
  cmp startupView,100h
  jne csvView
  cmp startupRoot,-2147483646
  je csvFolders
  mov startupRoot,-2147483646
  jmp csvHive
csvFolders:
  mov startupKind,2
  call CollectStartupFolder
  mov startupKind,3
  call CollectStartupFolder
  mov rcx,taskFolder
  test rcx,rcx
  jz csvTaskError
  mov taskDepth,0
  call CollectTaskFolder
  jmp csvDone
csvTaskError:
  lea rdx,startupTaskSource
  mov startupErrorSource,rdx
  mov eax,taskError
  test eax,eax
  jnz csvTaskErrStore
  mov eax,80004005h
csvTaskErrStore:
  call StartupErrorRow
csvDone:
  add rsp,28h
  ret
CollectStartupV2 ENDP

MakeStartupKey PROC
  sub rsp,28h
  lea rcx,startupKeyPath
  cmp startupTestMode,0
  je mskProduction
  lea rdx,startupTestKey
  jmp mskCopy
mskProduction:
  lea rdx,runKey
  cmp startupOnce,0
  je mskCopy
  lea rdx,runOnceKey
mskCopy:
  call lstrcpyW
  cmp startupDisabled,0
  je mskDone
  lea rcx,startupKeyPath
  lea rdx,disabledSuffix
  call lstrcatW
mskDone:
  add rsp,28h
  ret
MakeStartupKey ENDP

CollectRegistrySource PROC
  push rbx
  push rsi
  push rdi
  sub rsp,50h
  call MakeStartupKey
  call MakeStartupSourceLabel
  mov rcx,startupRoot
  lea rdx,startupKeyPath
  xor r8d,r8d
  mov r9d,startupView
  or r9d,1
  lea rax,startupKey
  mov [rsp+20h],rax
  call RegOpenKeyExW
  test eax,eax
  jz crsStart
  cmp eax,2
  je crsDone
  call StartupErrorRow
  jmp crsDone
crsStart:
  mov startupIndex,0
crsNext:
  cmp backCount,8192
  jae crsClose
  mov startupNameSize,16383
  mov startupDataSize,32766
  mov rcx,startupKey
  mov edx,startupIndex
  lea r8,startupLongName
  lea r9,startupNameSize
  mov qword ptr [rsp+20h],0
  lea rax,startupType
  mov [rsp+28h],rax
  lea rax,startupData
  mov [rsp+30h],rax
  lea rax,startupDataSize
  mov [rsp+38h],rax
  call RegEnumValueW
  cmp eax,259
  je crsClose
  test eax,eax
  jz crsRow
  cmp eax,234
  jne crsFail
  mov startupDataSize,0
crsRow:
  mov edi,backCount
  shl rdi,12
  add rdi,backBuffer
  mov rcx,rdi
  call ClearRow
  mov rcx,rdi
  lea rdx,startupLongName
  mov r8d,256
  call lstrcpynW
  mov eax,startupDataSize
  inc eax
  and eax,-2
  lea rdx,startupData
  mov word ptr [rdx+rax],0
  cmp startupType,1
  je crsStringValue
  cmp startupType,2
  je crsStringValue
  lea rdx,typeText
crsStringValue:
  lea rcx,[rdi+512]
  mov r8d,256
  call lstrcpynW
  lea rcx,[rdi+1024]
  lea rdx,startupSourceFmt
  lea r8,hkcuText
  cmp startupRoot,-2147483647
  je crsHiveLabel
  lea r8,hklmText
crsHiveLabel:
  lea r9,startupRunSource
  cmp startupOnce,0
  je crsBits
  lea r9,runOnceLabel
crsBits:
  mov eax,64
  cmp startupView,100h
  je crsFormat
  mov eax,32
crsFormat:
  mov [rsp+20h],eax
  call wsprintfW
  lea rcx,[rdi+1536]
  lea rdx,startupPresent
  cmp startupDisabled,0
  je crsState
  lea rdx,startupOff
crsState:
  call lstrcpyW
  lea rcx,[rdi+2048]
  lea rdx,startupUser
  cmp startupRoot,-2147483647
  je crsRights
  lea rdx,startupAdmin
crsRights:
  call lstrcpyW
  mov rax,startupRoot
  mov [rdi+3600],rax
  mov eax,startupView
  mov [rdi+3608],eax
  mov eax,startupOnce
  mov [rdi+3612],eax
  mov eax,startupDisabled
  mov [rdi+3616],eax
  mov dword ptr [rdi+3620],1
  mov dword ptr [rdi+3624],1
  cmp startupNameSize,255
  ja crsLong
  cmp startupDataSize,0
  je crsLong
  mov ecx,startupDataSize
  lea rdx,startupData
  mov r8d,startupType
  call StartupHash
  mov [rdi+3640],rax
  jmp crsAdded
crsLong:
  mov dword ptr [rdi+3624],0
  lea rcx,[rdi+1536]
  lea rdx,startupNameLong
  call lstrcpyW
crsAdded:
  inc backCount
  inc startupIndex
  jmp crsNext
crsFail:
  call StartupErrorRow
crsClose:
  mov rcx,startupKey
  call RegCloseKey
crsDone:
  add rsp,50h
  pop rdi
  pop rsi
  pop rbx
  ret
CollectRegistrySource ENDP

; EAX Win32/HRESULT. Read failures are visible, never reported as empty sources.
StartupErrorRow PROC
  push rbx
  sub rsp,20h
  inc startupErrors
  mov ebx,eax
  cmp backCount,8192
  jae serDone
  mov eax,backCount
  shl rax,12
  add rax,backBuffer
  mov rcx,rax
  call ClearRow
  mov eax,backCount
  shl rax,12
  add rax,backBuffer
  mov rcx,rax
  lea rdx,startupErrorText
  call lstrcpyW
  mov eax,backCount
  shl rax,12
  add rax,backBuffer
  lea rcx,[rax+1536]
  lea rdx,startupErrorFmt
  mov r8d,ebx
  call wsprintfW
  mov eax,backCount
  shl rax,12
  add rax,backBuffer
  lea rcx,[rax+1024]
  mov rdx,startupErrorSource
  test rdx,rdx
  jz serNoSource
  call lstrcpyW
serNoSource:
  inc backCount
serDone:
  add rsp,20h
  pop rbx
  ret
StartupErrorRow ENDP

CollectStartupFolder PROC
  push rbx
  push rsi
  push rdi
  sub rsp,40h
  lea rax,startupUserFolder
  cmp startupKind,2
  je csfErrorSource
  lea rax,startupCommonFolder
csfErrorSource:
  mov startupErrorSource,rax
  xor ecx,ecx
  mov edx,7
  cmp startupKind,2
  je csfGet
  mov edx,18h
csfGet:
  xor r8d,r8d
  xor r9d,r9d
  lea rax,startupBase
  mov [rsp+20h],rax
  call SHGetFolderPathW
  test eax,eax
  js csfError
  mov startupDisabled,0
csfState:
  lea rcx,startupPath
  lea rdx,startupBase
  call lstrcpyW
  cmp startupDisabled,0
  je csfMask
  lea rcx,startupPath
  lea rdx,disabledSuffix
  call lstrcatW
csfMask:
  lea rcx,startupPath
  lea rdx,startupWildcard
  call lstrcatW
  lea rcx,startupPath
  lea rdx,startupFindData
  call FindFirstFileW
  mov rbx,rax
  cmp rax,-1
  jne csfNext
  call GetLastError
  cmp eax,2
  je csfNextState
  cmp eax,3
  je csfNextState
  call StartupErrorRow
  jmp csfNextState
csfNext:
  test dword ptr [startupFindData],10h
  jnz csfAdvance
  lea rcx,startupBase
  call lstrlenW
  mov esi,eax
  lea rcx,startupFindData+44
  call lstrlenW
  add eax,esi
  add eax,30 ; separator and our disabled-folder suffix
  cmp eax,511
  jbe csfFits
  mov eax,206
  call StartupErrorRow
  jmp csfAdvance
csfFits:
  cmp backCount,8192
  jae csfClose
  mov edi,backCount
  shl rdi,12
  add rdi,backBuffer
  mov rcx,rdi
  call ClearRow
  mov rcx,rdi
  lea rdx,startupFindData+44
  mov r8d,256
  call lstrcpynW
  lea rcx,[rdi+2560]
  lea rdx,startupBase
  call lstrcpyW
  cmp startupDisabled,0
  je csfFullPath
  lea rcx,[rdi+2560]
  lea rdx,disabledSuffix
  call lstrcatW
csfFullPath:
  lea rcx,[rdi+2560]
  lea rdx,startupSlash
  call lstrcatW
  lea rcx,[rdi+2560]
  lea rdx,startupFindData+44
  ; SHGetFolderPath is bounded MAX_PATH; combined full path must fit 512 WCHAR.
  call lstrcatW
  lea rcx,[rdi+512]
  lea rdx,[rdi+2560]
  mov r8d,256
  call lstrcpynW
  lea rcx,[rdi+1024]
  lea rdx,startupUserFolder
  cmp startupKind,2
  je csfSource
  lea rdx,startupCommonFolder
csfSource:
  call lstrcpyW
  lea rcx,[rdi+1536]
  lea rdx,startupPresent
  cmp startupDisabled,0
  je csfStatus
  lea rdx,startupOff
csfStatus:
  call lstrcpyW
  lea rcx,[rdi+2048]
  lea rdx,startupUser
  cmp startupKind,2
  je csfRights
  lea rdx,startupAdmin
csfRights:
  call lstrcpyW
  mov eax,startupKind
  mov [rdi+3620],eax
  mov eax,startupDisabled
  mov [rdi+3616],eax
  mov dword ptr [rdi+3624],1
  mov rax,qword ptr [startupFindData+4]
  mov [rdi+3640],rax ; file creation identity
  inc backCount
csfAdvance:
  mov rcx,rbx
  lea rdx,startupFindData
  call FindNextFileW
  test eax,eax
  jnz csfNext
csfClose:
  mov rcx,rbx
  call FindClose
csfNextState:
  xor startupDisabled,1
  cmp startupDisabled,0
  jne csfState
  jmp csfDone
csfError:
  call StartupErrorRow
csfDone:
  add rsp,40h
  pop rdi
  pop rsi
  pop rbx
  ret
CollectStartupFolder ENDP

; COM calls stay on collector MTA. Recursion capped, every owned interface released.
CollectTaskFolder PROC
  push rbx
  push rsi
  push rdi
  push r12
  push r13
  sub rsp,60h
  mov rbx,rcx
  lea rax,startupTaskSource
  mov startupErrorSource,rax
  cmp taskDepth,64
  jae ctfDone
  inc taskDepth
  mov qword ptr [rsp+30h],0
  mov qword ptr [rsp+38h],0
  mov edx,1 ; include hidden tasks
  lea r8,[rsp+30h]
  mov rax,[rcx]
  call qword ptr [rax+112]
  test eax,eax
  js ctfTasksError
  mov rcx,[rsp+30h]
  lea rdx,[rsp+40h]
  mov rax,[rcx]
  call qword ptr [rax+56]
  test eax,eax
  js ctfFolders
  mov esi,1
ctfTask:
  cmp startupTestMode,0
  jne ctfKeepTasks
  cmp desiredMode,5
  jne ctfFolders
ctfKeepTasks:
  cmp esi,[rsp+40h]
  jg ctfFolders
  cmp backCount,8192
  jae ctfFolders
  mov word ptr [taskIndexVariant],3 ; VT_I4
  mov dword ptr [taskIndexVariant+8],esi
  mov rcx,[rsp+30h]
  lea rdx,taskIndexVariant
  lea r8,[rsp+48h]
  mov rax,[rcx]
  call qword ptr [rax+64]
  test eax,eax
  js ctfTaskNext
  mov r12,[rsp+48h]
  mov edi,backCount
  shl rdi,12
  add rdi,backBuffer
  mov rcx,rdi
  call ClearRow
  mov rcx,r12
  lea rdx,[rsp+50h]
  mov rax,[rcx]
  call qword ptr [rax+56]
  test eax,eax
  js ctfReleaseTask
  mov rcx,rdi
  mov rdx,[rsp+50h]
  mov r8d,256
  call lstrcpynW
  mov rcx,[rsp+50h]
  call SysFreeString
  mov rcx,r12
  lea rdx,[rsp+50h]
  mov rax,[rcx]
  call qword ptr [rax+64]
  test eax,eax
  js ctfReleaseTask
  mov rcx,[rsp+50h]
  call SysStringLen
  cmp eax,511
  ja ctfLongTask
  mov dword ptr [rdi+3624],1
ctfLongTask:
  lea rcx,[rdi+2560]
  mov rdx,[rsp+50h]
  mov r8d,512
  call lstrcpynW
  lea rcx,[rdi+512]
  mov rdx,[rsp+50h]
  mov r8d,256
  call lstrcpynW
  mov rcx,[rsp+50h]
  call SysFreeString
  lea rcx,[rdi+1024]
  lea rdx,startupTaskSource
  call lstrcpyW
  mov word ptr [rsp+58h],0
  mov rcx,r12
  lea rdx,[rsp+58h]
  mov rax,[rcx]
  call qword ptr [rax+80]
  test eax,eax
  js ctfReleaseTask
  lea rdx,startupTaskOn
  cmp word ptr [rsp+58h],0
  jne ctfTaskState
  lea rdx,startupTaskOff
  mov dword ptr [rdi+3616],1
ctfTaskState:
  lea rcx,[rdi+1536]
  call lstrcpyW
  lea rcx,[rdi+2048]
  lea rdx,startupAcl
  call lstrcpyW
  mov dword ptr [rdi+3620],4
  inc backCount
ctfReleaseTask:
  mov rcx,r12
  call ReleaseCom
ctfTaskNext:
  inc esi
  jmp ctfTask
ctfTasksError:
  call StartupErrorRow
ctfFolders:
  mov rcx,rbx
  xor edx,edx
  lea r8,[rsp+38h]
  mov rax,[rcx]
  call qword ptr [rax+80]
  test eax,eax
  js ctfRelease
  mov rcx,[rsp+38h]
  lea rdx,[rsp+40h]
  mov rax,[rcx]
  call qword ptr [rax+56]
  test eax,eax
  js ctfRelease
  mov esi,1
ctfFolder:
  cmp startupTestMode,0
  jne ctfKeepFolders
  cmp desiredMode,5
  jne ctfRelease
ctfKeepFolders:
  cmp esi,[rsp+40h]
  jg ctfRelease
  cmp backCount,8192
  jae ctfRelease
  cmp collectorStop,0
  jne ctfRelease
  mov word ptr [taskIndexVariant],3
  mov dword ptr [taskIndexVariant+8],esi
  mov rcx,[rsp+38h]
  lea rdx,taskIndexVariant
  lea r8,[rsp+48h]
  mov rax,[rcx]
  call qword ptr [rax+64]
  test eax,eax
  js ctfFolderNext
  mov r13,[rsp+48h]
  mov rcx,r13
  call CollectTaskFolder
  mov rcx,r13
  call ReleaseCom
ctfFolderNext:
  inc esi
  jmp ctfFolder
ctfRelease:
  mov rcx,[rsp+30h]
  call ReleaseCom
  mov rcx,[rsp+38h]
  call ReleaseCom
  dec taskDepth
ctfDone:
  add rsp,60h
  pop r13
  pop r12
  pop rdi
  pop rsi
  pop rbx
  ret
CollectTaskFolder ENDP
.code
MakeStartupSourceLabel PROC
  sub rsp,28h
  lea rcx,startupSourceLabel
  lea rdx,startupSourceFmt
  lea r8,hkcuText
  cmp startupRoot,-2147483647
  je msslHive
  lea r8,hklmText
msslHive:
  lea r9,startupRunSource
  cmp startupOnce,0
  je msslBits
  lea r9,runOnceLabel
msslBits:
  mov eax,64
  cmp startupView,100h
  je msslFormat
  mov eax,32
msslFormat:
  mov [rsp+20h],eax
  call wsprintfW
  lea rax,startupSourceLabel
  mov startupErrorSource,rax
  add rsp,28h
  ret
MakeStartupSourceLabel ENDP
.data
runOnceLabel dw 'R','u','n','O','n','c','e',0
startupTestKey dw 'S','o','f','t','w','a','r','e','\','T','M','X','T','e','s','t','-'
dw 'A','6','3','7','1','F','B','4','-','C','E','9','2','-','4','B','7','4','\','R','u','n',0
startupTestFolder dw 'w','o','r','k','\','m','o','d','e','l','-','s','t','a','r','t','u','p',0
