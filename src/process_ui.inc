; Process presentation is independent of the seven legacy model text slots.
; The collector queues icon paths; one isolated worker fills a bounded cache.
; The UI only reads immutable rows, including the icon index of that snapshot.
extern SHGetFileInfoW:proc, GetModuleFileNameW:proc, QueryFullProcessImageNameW:proc
extern CreateThread:proc, CreateEventW:proc, WaitForSingleObject:proc, SetEvent:proc
extern SetWindowSubclass:proc, DefSubclassProc:proc, SetDCBrushColor:proc
extern SetTextColor:proc, SetBkMode:proc, SelectObject:proc, FrameRect:proc
extern GetClientRect:proc, GetDC:proc, ReleaseDC:proc, FillRect:proc
.data
processImageList dq 0
processHeader dq 0
processDefaultIcon dd 0
processGenericExe dw 'a','p','p','l','i','c','a','t','i','o','n','.','e','x','e',0
processCpuFormat dw '%','u','.','%','u','%','%',0
processCpuFormatRu dw '%','u',',','%','u','%','%',0
; Compact lower-bound marker leaves room for the percent suffix and NUL in
; the 16-WCHAR aggregate field, including the maximum unsigned %u value.
processCpuPartialFormat dw 2265h,' ','%','u','.','%','u','%','%',0
processCpuPartialFormatRu dw 2265h,' ','%','u',',','%','u','%','%',0
processIntegerFormat dw '%','I','6','4','u',0
processIntegerPartialFormat dw '>','=',' ','%','I','6','4','u',0
processScaledFormat dw '%','I','6','4','u','.','%','u',0
processScaledFormatRu dw '%','I','6','4','u',',','%','u',0
processScaledPartialFormat dw '>','=',' ','%','I','6','4','u','.','%','u',0
processScaledPartialFormatRu dw '>','=',' ','%','I','6','4','u',',','%','u',0
processHeatDark dd 202034h,20203Ch,202046h,202052h,202060h,202070h,202080h,202090h
processHeatLight dd 0E9F6FFh,0DDF0FFh,0CFE7FFh,0BDDCFFh,0A9CFFFh,092BFFFh,078ACFFh,05B95FFh
.data?
align 8
processShellInfo db 696 dup(?)
processIconLength dd ?
processIconPids dd 16384 dup(?)
processIconBirth dq 16384 dup(?)
processIconIndices dd 16384 dup(?)
processPausedPids dd 16384 dup(?)
processPausedBirth dq 16384 dup(?)
processDisplayBuffer dw 32 dup(?)
processHeaderText dw 512 dup(?)
headerClientRect RECT <>
headerLastRect RECT <>
.code
InitProcessImages PROC
  sub rsp,38h
  lea rcx,processGenericExe
  mov edx,80h ; FILE_ATTRIBUTE_NORMAL: generic .exe, never our product icon
  lea r8,processShellInfo
  mov r9d,696
  mov qword ptr [rsp+20h],4011h ; SYSICONINDEX | SMALLICON | USEFILEATTRIBUTES
  call SHGetFileInfoW
  mov processImageList,rax
  test rax,rax
  jz ipiDone
  mov eax,dword ptr [processShellInfo+8]
  mov processDefaultIcon,eax
  mov rcx,gList
  mov edx,1003h ; LVM_SETIMAGELIST, shell retains ownership
  mov r8d,1
  mov r9,processImageList
  call SendMessageW
ipiDone:
  add rsp,38h
  ret
InitProcessImages ENDP



InitProcessListStyle PROC
  sub rsp,28h
  mov rcx,gList
  mov edx,101Fh ; LVM_GETHEADER
  xor r8d,r8d
  xor r9d,r9d
  call SendMessageW
  mov processHeader,rax
  mov rcx,gList
  lea rdx,ProcessListSubclass
  mov r8d,1
  xor r9d,r9d
  call SetWindowSubclass
  mov rcx,processHeader
  lea rdx,ProcessHeaderSubclass
  mov r8d,3
  xor r9d,r9d
  call SetWindowSubclass
  call InitScrollTheme
  add rsp,28h
  ret
InitProcessListStyle ENDP

ProcessListSubclass PROC
  sub rsp,58h
  mov [rsp+20h],rcx
  mov [rsp+28h],rdx
  mov [rsp+30h],r8
  mov [rsp+38h],r9
  cmp frontMode,0
  jne plsStartup
  cmp treeEnabled,0
  je plsNotify
  cmp edx,201h
  je plsTreeMouse
  cmp edx,203h ; double-click on a disclosure must not toggle a second time
  je plsTreeDouble
  cmp edx,100h
  jne plsNotify
  mov ecx,r8d
  call ProcessTreeKey
  test eax,eax
  jnz plsHandled
  jmp plsRestore
plsTreeMouse:
  mov rcx,r9
  xor edx,edx
  call ProcessTreeMouse
  test eax,eax
  jnz plsHandled
  jmp plsRestore
plsTreeDouble:
  mov rcx,r9
  mov edx,1
  call ProcessTreeMouse
  test eax,eax
  jnz plsHandled
  jmp plsRestore
plsStartup:
  cmp frontMode,5
  jne plsNotify
  cmp edx,201h ; WM_LBUTTONDOWN
  jne plsKey
  mov rcx,r9
  call StartupCheckboxMouse
  test eax,eax
  jnz plsHandled
  jmp plsRestore
plsKey:
  cmp edx,100h
  jne plsNotify
  cmp r8d,20h ; SPACE
  jne plsNotify
  mov rcx,gList
  mov edx,100Ch
  mov r8,-1
  mov r9d,2
  call SendMessageW
  mov ecx,eax
  call ToggleStartupCheckbox
plsHandled:
  xor eax,eax
  add rsp,58h
  ret
plsNotify:
  cmp edx,4Eh
  jne plsRestore
  mov rcx,r9
  call PcHeaderNotify
  test edx,edx
  jnz plsReturnHeader
  mov r9,[rsp+38h]
  mov rax,processHeader
  cmp [r9],rax
  jne plsRestore
  cmp dword ptr [r9+16],-12
  jne plsRestore
  call ProcessHeaderCustomDraw
plsReturnHeader:
  add rsp,58h
  ret
plsRestore:
  mov rcx,[rsp+20h]
  mov rdx,[rsp+28h]
  mov r8,[rsp+30h]
  mov r9,[rsp+38h]
  call DefSubclassProc
  add rsp,58h
  ret
ProcessListSubclass ENDP

; Native header leaves the area after the last column in the system window
; background. Repaint that remainder after native WM_PAINT so it cannot become
; a white strip while retaining native header hit testing and sorting.
ProcessHeaderSubclass PROC
  sub rsp,48h
  cmp edx,7Bh ; header owns its context menu, independent of ListView forwarding
  jne phsPaintMessages
  cmp frontMode,0
  jne phsDefault
  mov rcx,r9
  call PcShowHeaderMenu
  xor eax,eax
  add rsp,48h
  ret
phsPaintMessages:
  cmp edx,318h ; WM_PRINTCLIENT: same render path for offscreen control tests
  je phsPrint
  cmp edx,14h ; WM_ERASEBKGND: suppress the default white background
  je phsErase
  cmp edx,0Fh ; WM_PAINT
  jne phsDefault
  call DefSubclassProc
  mov [rsp+30h],rax
  mov rcx,processHeader
  call GetDC
  mov [rsp+38h],rax
  mov rcx,rax
  mov edx,1 ; tail only, native labels already painted
  call PaintHeaderBackground
  mov rcx,processHeader
  mov rdx,[rsp+38h]
  call ReleaseDC
  mov rax,[rsp+30h]
  add rsp,48h
  ret
phsPrint:
  mov [rsp+38h],r8
  call DefSubclassProc
  mov [rsp+30h],rax
  mov rcx,[rsp+38h]
  mov edx,1
  call PaintHeaderBackground
  mov rax,[rsp+30h]
  add rsp,48h
  ret
phsErase:
  mov rcx,r8
  xor edx,edx
  call PaintHeaderBackground
  mov eax,1
  add rsp,48h
  ret
phsDefault:
  call DefSubclassProc
  add rsp,48h
  ret
ProcessHeaderSubclass ENDP

; RCX HDC, EDX tail-only. Does not depend on the parent's shared layout RECT.
PaintHeaderBackground PROC
  push rbx
  push rsi
  sub rsp,38h
  mov rbx,rcx
  mov esi,edx
  test rbx,rbx
  jz phbDone
  mov rcx,processHeader
  lea rdx,headerClientRect
  call GetClientRect
  test esi,esi
  jz phbFill
  mov rcx,processHeader
  mov edx,1200h
  xor r8d,r8d
  xor r9d,r9d
  call SendMessageW
  test eax,eax
  jz phbFill
  mov [rsp+20h],eax
  mov dword ptr [rsp+24h],0
phbRectLoop:
  mov r8d,[rsp+24h]
  mov rcx,processHeader
  mov edx,1207h
  lea r9,headerLastRect
  call SendMessageW
  test eax,eax
  jz phbRectNext
  mov eax,headerLastRect.rcRight
  cmp eax,headerClientRect.rcLeft
  jle phbRectNext
  mov headerClientRect.rcLeft,eax
phbRectNext:
  inc dword ptr [rsp+24h]
  mov eax,[rsp+24h]
  cmp eax,[rsp+20h]
  jb phbRectLoop
phbFill:
  mov rcx,rbx
  mov edx,202020h
  cmp themeOption,0
  jne phbBrush
  mov edx,0F7F7F7h
phbBrush:
  call SetDCBrushColor
  mov ecx,18
  call GetStockObject
  mov r8,rax
  mov rcx,rbx
  lea rdx,headerClientRect
  call FillRect
phbDone:
  add rsp,38h
  pop rsi
  pop rbx
  ret
PaintHeaderBackground ENDP

; Header labels remain native accessible HDITEM text; only painting is custom.
ProcessHeaderCustomDraw PROC
  push rbx
  push rsi
  push rdi
  sub rsp,0C0h
  mov rbx,r9
  cmp dword ptr [rbx+24],1
  je phcdNotify
  cmp dword ptr [rbx+24],10001h
  jne phcdDefault
  mov rsi,[rbx+32]
  mov rcx,rsi
  mov edx,0F7F7F7h
  cmp themeOption,0
  je phcdBackground
  mov edx,202020h
phcdBackground:
  call SetDCBrushColor
  mov ecx,18 ; stock DC_BRUSH does not allocate a GDI object per paint
  call GetStockObject
  mov rdi,rax
  mov rcx,rsi
  lea rdx,[rbx+40]
  mov r8,rdi
  call FillRect
  mov rcx,rsi
  mov edx,0E4E4E4h
  cmp themeOption,0
  je phcdBorder
  mov edx,303030h
phcdBorder:
  call SetDCBrushColor
  mov rcx,rsi
  lea rdx,[rbx+40]
  mov r8,rdi
  call FrameRect
  mov qword ptr [rsp+30h],0
  mov qword ptr [rsp+38h],0
  mov qword ptr [rsp+40h],0
  mov qword ptr [rsp+48h],0
  mov qword ptr [rsp+50h],0
  mov qword ptr [rsp+58h],0
  mov qword ptr [rsp+60h],0
  mov qword ptr [rsp+68h],0
  mov qword ptr [rsp+70h],0
  mov dword ptr [rsp+30h],2 ; HDI_TEXT
  lea rax,processHeaderText
  mov [rsp+38h],rax
  mov dword ptr [rsp+48h],512
  mov rcx,processHeader
  mov edx,120Bh ; HDM_GETITEMW
  mov r8,[rbx+56]
  lea r9,[rsp+30h]
  call SendMessageW
  mov rcx,rsi
  mov rdx,appFont
  call SelectObject
  mov [rsp+78h],rax
  mov rcx,rsi
  mov edx,1 ; TRANSPARENT
  call SetBkMode
  mov [rsp+80h],eax
  mov rcx,rsi
  mov edx,202020h
  cmp themeOption,0
  je phcdInk
  mov edx,0EEEEEEh
phcdInk:
  call SetTextColor
  mov [rsp+88h],eax
  mov eax,[rbx+40]
  add eax,8
  mov [rsp+90h],eax
  mov eax,[rbx+44]
  mov [rsp+94h],eax
  mov eax,[rbx+48]
  sub eax,8
  mov [rsp+98h],eax
  mov eax,[rbx+52]
  mov [rsp+9Ch],eax
  mov eax,8824h ; SINGLELINE | VCENTER | NOPREFIX | END_ELLIPSIS
  cmp frontMode,0
  jne phcdText
  mov [rsp+20h],eax
  mov ecx,[rbx+56]
  call PcColumnId
  mov ecx,eax
  mov eax,[rsp+20h]
  cmp ecx,2
  jb phcdText
  cmp ecx,12
  jae phcdText
  or eax,2 ; DT_RIGHT for the resource figures
phcdText:
  mov [rsp+20h],eax
  mov rcx,rsi
  lea rdx,processHeaderText
  mov r8d,-1
  lea r9,[rsp+90h]
  call DrawTextW
  mov rcx,rsi
  mov edx,[rsp+88h]
  call SetTextColor
  mov rcx,rsi
  mov edx,[rsp+80h]
  call SetBkMode
  mov rcx,rsi
  mov rdx,[rsp+78h]
  call SelectObject
  mov eax,4 ; CDRF_SKIPDEFAULT
  jmp phcdReturn
phcdNotify:
  mov rsi,[rbx+32]
  mov rcx,rsi
  mov edx,202020h
  cmp themeOption,0
  jne phcdFill
  mov edx,0F7F7F7h
phcdFill:
  call SetDCBrushColor
  mov ecx,18
  call GetStockObject
  mov rdi,rax
  mov rcx,rsi
  lea rdx,[rbx+40]
  mov r8,rdi
  call FillRect
  mov eax,20h
  jmp phcdReturn
phcdDefault:
  xor eax,eax
phcdReturn:
  add rsp,0C0h
  pop rdi
  pop rsi
  pop rbx
  ret
ProcessHeaderCustomDraw ENDP

; Collector-only pause index. A label means a suspension owned by this app,
; never an inferred OS-wide thread state.
BuildProcessPauseMap PROC
  push rbx
  push rsi
  push rdi
  lea rdi,processPausedPids
  xor eax,eax
  mov ecx,8192
  rep stosq
  xor esi,esi
  lea rbx,heldHandles
bppmNext:
  cmp esi,4096
  jae bppmDone
  cmp qword ptr [rbx+rsi*8],0
  je bppmAdvance
  lea rdx,heldPids
  mov ecx,[rdx+rsi*4]
  mov eax,ecx
  and eax,16383
  lea rdi,processPausedPids
bppmProbe:
  cmp dword ptr [rdi+rax*4],0
  je bppmInsert
  cmp [rdi+rax*4],ecx
  je bppmInsert
  inc eax
  and eax,16383
  jmp bppmProbe
bppmInsert:
  mov [rdi+rax*4],ecx
  lea rdx,heldBirth
  mov rdx,[rdx+rsi*8]
  lea rdi,processPausedBirth
  mov [rdi+rax*8],rdx
bppmAdvance:
  inc esi
  jmp bppmNext
bppmDone:
  pop rdi
  pop rsi
  pop rbx
  ret
BuildProcessPauseMap ENDP

; RCX row, RDX open process handle. New row tail: icon+3880, state+3884,
; I/O bytes/sec+3888, I/O validity+3896, CPU tenths+3900, I/O text+3912.
CollectProcessPresentation PROC
  push rbx
  push rsi
  push rdi
  sub rsp,30h
  mov rbx,rcx
  mov rsi,rdx
  mov dword ptr [rbx+3884],0
  mov eax,[rbx+3584]
  and eax,16383
  lea rdx,processPausedPids
cppPauseProbe:
  mov ecx,[rdx+rax*4]
  test ecx,ecx
  jz cppIcon
  cmp ecx,[rbx+3584]
  je cppPauseIdentity
  inc eax
  and eax,16383
  jmp cppPauseProbe
cppPauseIdentity:
  lea rdx,processPausedBirth
  mov rax,[rdx+rax*8]
  cmp rax,[rbx+3600]
  jne cppIcon
  mov dword ptr [rbx+3884],1
cppIcon:
  call CollectorSnapshotCurrent
  test eax,eax
  jz cppRateText ; navigation changed; skip cold shell icon extraction
  cmp qword ptr [rbx+3600],0
  je cppRateText
  cmp processImageList,0
  je cppRateText
  mov rcx,rbx
  mov rdx,rsi
  call CollectCachedProcessIcon
cppRateText:
  mov rcx,[rbx+3888]
  lea rdx,[rbx+3912]
  mov r8d,1024
  call StoreScaledMetric
  add rsp,30h
  pop rdi
  pop rsi
  pop rbx
  ret
CollectProcessPresentation ENDP

; RCX bytes/rate, RDX destination, R8 divisor. One real decimal, no float CRT.
StoreScaledMetric PROC
  push rbx
  push rsi
  sub rsp,28h
  mov rbx,rdx
  mov esi,r8d
  mov rax,rcx
  xor edx,edx
  div rsi
  mov r8,rax
  mov eax,edx
  imul rax,10
  xor edx,edx
  div rsi
  mov r9d,eax
  mov rcx,rbx
  lea rdx,processScaledFormat
  cmp appLanguage,0
  je ssmFormat
  lea rdx,processScaledFormatRu
ssmFormat:
  call wsprintfW
  add rsp,28h
  pop rsi
  pop rbx
  ret
StoreScaledMetric ENDP

; RCX row, EDX CPU percent in tenths. Worker-only destination.
FormatProcessCpu PROC
  sub rsp,28h
  mov [rcx+3900],edx
  add rcx,3072
  mov eax,edx
  xor edx,edx
  mov r10d,10
  div r10d
  mov r8d,eax
  mov r9d,edx
  lea rdx,processCpuFormat
  cmp appLanguage,0
  je fpcFormat
  lea rdx,processCpuFormatRu
fpcFormat:
  call wsprintfW
  add rsp,28h
  ret
FormatProcessCpu ENDP

; UI-owned tree aggregates preserve every OS process as an actionable row.
FormatTreeProcessUi PROC
  push rbx
  sub rsp,20h
  mov rbx,rcx
  mov rax,[rbx+4040]
  xor edx,edx
  mov r10d,10
  div r10
  lea rcx,[rbx+4064]
  mov r8d,eax
  mov r9d,edx
  lea rdx,processCpuFormat
  cmp appLanguage,0
  je ftpuPartial
  lea rdx,processCpuFormatRu
ftpuPartial:
  mov eax,[rbx+4056]
  cmp rax,[rbx+3736]
  jae ftpuFormat
  lea rdx,processCpuPartialFormat
  cmp appLanguage,0
  je ftpuFormat
  lea rdx,processCpuPartialFormatRu
ftpuFormat:
  call wsprintfW
  add rsp,20h
  pop rbx
  ret
FormatTreeProcessUi ENDP

; RCX row, EDX stable process column -> RAX stable text.
ProcessDisplayText PROC
  sub rsp,28h
  test rcx,rcx
  jz pdtUnavailable
  cmp edx,7
  jae pdtExtra
  mov rax,rcx
  test edx,edx
  jz pdtName
  cmp edx,1
  je pdtState
  cmp edx,2
  je pdtCpu
  cmp edx,3
  je pdtMemory
  cmp edx,4
  je pdtIo
  cmp edx,5
  je pdtThreads
  lea rax,[rcx+512]
  jmp pdtDone
pdtThreads:
  lea rax,[rcx+1536] ; exact OS-process thread count from Toolhelp
  jmp pdtDone
pdtName:
  cmp treeEnabled,0
  je pdtDone
  mov rax,[rcx+3744]
  test rax,rax
  jnz pdtDone
  mov rax,rcx
  jmp pdtDone
pdtState:
  lea rax,processDisplayBuffer
  mov word ptr [rax],0
  cmp dword ptr [rcx+3884],1
  je pdtPaused
  cmp dword ptr [rcx+3884],2
  jne pdtDone
  lea rax,processDeniedText
  jmp pdtDone
pdtPaused:
  lea rax,processPausedText
  jmp pdtDone
pdtCpu:
  cmp treeEnabled,0
  jne pdtTreeCpu
  cmp dword ptr [rcx+3672],0
  je pdtUnavailable
  lea rax,[rcx+3072]
  jmp pdtDone
pdtTreeCpu:
  cmp dword ptr [rcx+4056],0
  je pdtUnavailable
  lea rax,[rcx+4064]
  jmp pdtDone
pdtMemory:
  cmp treeEnabled,0
  jne pdtTreeMemory
  lea rax,[rcx+2560]
  jmp pdtDone
pdtTreeMemory:
  lea rax,[rcx+3816]
  jmp pdtDone
pdtIo:
  cmp treeEnabled,0
  jne pdtTreeIo
  cmp dword ptr [rcx+3896],0
  je pdtUnavailable
  lea rax,[rcx+3912]
  jmp pdtDone
pdtTreeIo:
  cmp dword ptr [rcx+4060],0
  je pdtUnavailable
  mov rax,[rcx+4048]
  xor edx,edx
  mov r10d,1024
  div r10
  mov r8,rax
  mov eax,edx
  imul eax,10
  xor edx,edx
  div r10
  mov r9d,eax
  lea rdx,processScaledFormat
  cmp appLanguage,0
  je pdtTreeIoPartial
  lea rdx,processScaledFormatRu
pdtTreeIoPartial:
  mov eax,[rcx+4060]
  cmp rax,[rcx+3736]
  jae pdtTreeIoFormat
  lea rdx,processScaledPartialFormat
  cmp appLanguage,0
  je pdtTreeIoFormat
  lea rdx,processScaledPartialFormatRu
pdtTreeIoFormat:
  lea rcx,processDisplayBuffer
  call wsprintfW
  lea rax,processDisplayBuffer
  jmp pdtDone
pdtExtra:
  call PcDisplayExtra
  jmp pdtDone
pdtUnavailable:
  lea rax,unavailable
pdtDone:
  add rsp,28h
  ret
ProcessDisplayText ENDP

; RCX/RDX rows. Numeric resource sorting, no lexicographic decimal errors.
CompareProcessRows PROC
  push rbx
  push rsi
  push rdi
  push r12
  sub rsp,28h
  mov rbx,rcx
  mov rsi,rdx
  cmp sortColumn,0
  je cprNames
  cmp sortColumn,PC_GPU_ENGINE
  je cprGpuEngine
  cmp sortColumn,PC_ACCOUNT
  jb cprNumeric
  cmp sortColumn,PC_COMMAND
  jbe cprMetadata
cprNumeric:
  mov edx,sortColumn
  call PcSortMetric
  mov rdi,rax
  mov r12d,ecx
  mov rcx,rsi
  mov edx,sortColumn
  call PcSortMetric
  cmp r12d,ecx
  jne cprValidity
  test ecx,ecx
  jz cprTie
  cmp rdi,rax
  je cprTie
  mov eax,1
  ja cprDone
  mov eax,-1
  jmp cprDone
cprGpuEngine:
  call GpUiCompareEngine
  mov r12d,r8d
  mov ecx,r9d
  cmp r12d,ecx
  jne cprValidity
  test eax,eax
  jz cprTie
  jmp cprDone
cprMetadata:
  mov edx,sortColumn
  call PmRowValue
  mov rdi,rax
  xor r12d,r12d
  cmp edx,2 ; PM_STATUS_VALID
  sete r12b
  mov rcx,rsi
  mov edx,sortColumn
  call PmRowValue
  xor ecx,ecx
  cmp edx,2
  sete cl
  cmp r12d,ecx
  jne cprValidity
  test ecx,ecx
  jz cprTie
  mov rcx,rdi
  mov rdx,rax
  call StrCmpLogicalW
  test eax,eax
  jz cprTie
  jmp cprDone
cprNames:
  call StrCmpLogicalW
  test eax,eax
  jnz cprDone
cprTie:
  mov eax,[rbx+3584]
  cmp eax,[rsi+3584]
  jne cprTieResult
  mov rax,[rbx+3600]
  cmp rax,[rsi+3600]
  je cprZero
cprTieResult:
  mov eax,1
  ja cprStable
  mov eax,-1
  jmp cprStable
cprValidity:
  ; Valid first for both sort directions. CompareRows performs final inversion.
  mov eax,-1
  cmp r12d,ecx
  ja cprStable
  mov eax,1
cprStable:
  cmp sortDescending,0
  je cprDone
  neg eax
  jmp cprDone
cprZero:
  xor eax,eax
cprDone:
  add rsp,28h
  pop r12
  pop rdi
  pop rsi
  pop rbx
  ret
CompareProcessRows ENDP

; NMLVCUSTOMDRAW x64 offsets: item+56, state+64, text colors+80/+84,
; subitem+88. Returning a color never changes the immutable process model.
ProcessListCustomDraw PROC
  mov eax,[r9+24]
  cmp eax,1
  je plcdNotify
  cmp eax,10001h
  je plcdNotify
  cmp eax,30001h
  jne plcdDefault
  cmp frontMode,0
  jne plcdNativeSelection
  cmp dword ptr [r9+88],0
  je PaintProcessNameCell
plcdNativeSelection:
  test dword ptr [r9+64],1 ; native selection stays visible
  jnz plcdDefault
  mov eax,0FFFFFFh
  mov edx,202020h
  cmp themeOption,0
  je plcdBase
  mov eax,202020h
  mov edx,0EEEEEEh
plcdBase:
  mov [r9+84],eax
  mov [r9+80],edx
  cmp frontMode,0
  jne plcdDefault
  mov eax,[r9+56]
  cmp eax,visibleCount
  jae plcdDefault
  lea rdx,rowMap
  mov r10,[rdx+rax*8]
  sub rsp,38h
  mov [rsp+20h],r9
  mov [rsp+28h],r10
  mov ecx,[r9+88]
  call PcColumnId
  mov r9,[rsp+20h]
  mov r10,[rsp+28h]
  add rsp,38h
  cmp eax,2
  je plcdCpu
  cmp eax,3
  je plcdMemory
  cmp eax,4
  jne plcdDefault
  cmp treeEnabled,0
  jne plcdIoTree
  cmp dword ptr [r10+3896],0
  je plcdDefault
  mov rax,[r10+3888]
  jmp plcdIoScale
plcdIoTree:
  cmp dword ptr [r10+4060],0
  je plcdDefault
  mov rax,[r10+4048]
plcdIoScale:
  shr rax,20
  jmp plcdHeat
plcdCpu:
  cmp treeEnabled,0
  jne plcdCpuTree
  cmp dword ptr [r10+3672],0
  je plcdDefault
  mov eax,[r10+3900]
  jmp plcdCpuScale
plcdCpuTree:
  cmp dword ptr [r10+4056],0
  je plcdDefault
  mov rax,[r10+4040]
plcdCpuScale:
  xor edx,edx
  mov ecx,50
  div rcx
  jmp plcdHeat
plcdMemory:
  cmp treeEnabled,0
  jne plcdMemoryTree
  cmp dword ptr [r10+3656],0
  je plcdDefault
  mov rax,[r10+3632]
  jmp plcdMemoryScale
plcdMemoryTree:
  mov eax,[r10+3712]
  cmp rax,[r10+3736]
  jae plcdDefault
  mov rax,[r10+3728]
plcdMemoryScale:
  shr rax,27 ; one step per 128 MiB, saturating
plcdHeat:
  cmp rax,7
  jbe plcdPalette
  mov eax,7
plcdPalette:
  lea rdx,processHeatLight
  cmp themeOption,0
  je plcdColor
  lea rdx,processHeatDark
plcdColor:
  mov eax,[rdx+rax*4]
  mov [r9+84],eax
plcdDefault:
  xor eax,eax
  ret
plcdNotify:
  mov eax,20h ; notify item, then subitem draw
  ret
ProcessListCustomDraw ENDP

include tree_paint.inc
include icon_worker.inc
include scroll_theme.inc
