option casemap:none

; TaskManagerX64 - small native MASM x64 process manager foundation.
; Build with build-taskmanager.ps1.  No CRT is used.

includelib kernel32.lib
includelib user32.lib
includelib gdi32.lib
includelib comctl32.lib
includelib psapi.lib
includelib advapi32.lib
includelib shell32.lib

extern GetModuleHandleW:proc
extern ExitProcess:proc
extern GetLastError:proc
extern CreateToolhelp32Snapshot:proc
extern Process32FirstW:proc
extern Process32NextW:proc
extern OpenProcess:proc
extern CloseHandle:proc
extern GetProcessMemoryInfo:proc
extern TerminateProcess:proc
extern SuspendThread:proc
extern ResumeThread:proc
extern CreateThread:proc
extern SetThreadPriority:proc
extern GetWindowLongPtrW:proc
extern SetWindowLongPtrW:proc
extern DefWindowProcW:proc
extern DispatchMessageW:proc
extern TranslateMessage:proc
extern GetMessageW:proc
extern PostQuitMessage:proc
extern RegisterClassExW:proc
extern CreateWindowExW:proc
extern ShowWindow:proc
extern UpdateWindow:proc
extern DestroyWindow:proc
extern SetTimer:proc
extern KillTimer:proc
extern MessageBoxW:proc
extern SendMessageW:proc
extern GetClientRect:proc
extern MoveWindow:proc
extern LoadCursorW:proc
extern LoadIconW:proc
extern GetStockObject:proc
extern SetWindowTextW:proc
extern GetDlgCtrlID:proc
extern GetWindowTextW:proc


; constants
WM_CREATE equ 1h
WM_DESTROY equ 2h
WM_SIZE equ 5h
WM_COMMAND equ 111h
WM_TIMER equ 113h
WM_NOTIFY equ 4Eh
WS_OVERLAPPEDWINDOW equ 00CF0000h
WS_VISIBLE equ 10000000h
WS_CHILD equ 40000000h
WS_VSCROLL equ 00200000h
LVS_REPORT equ 1h
LVS_SINGLESEL equ 4h
LVS_SHOWSELALWAYS equ 8h
LVS_OWNERDATA equ 1000h
LVS_EX_FULLROWSELECT equ 20h
LVS_EX_DOUBLEBUFFER equ 10000h
LVM_SETEXTENDEDLISTVIEWSTYLE equ 1036h
LVM_SETBKCOLOR equ 1001h
LVM_SETCOLUMNWIDTH equ 101Eh
LVM_GETNEXTITEM equ 100Ch
LVM_DELETEALLITEMS equ 1009h
LVM_INSERTCOLUMNW equ 1061h
LVM_INSERTITEMW equ 104Dh
LVM_SETITEMTEXTW equ 1074h
LVNI_SELECTED equ 2h
WS_EX_CLIENTEDGE equ 200h
BS_PUSHBUTTON equ 0h
CW_USEDEFAULT equ 80000000h
SW_SHOW equ 5
IDC_ARROW equ 32512
IDI_APP equ 103
COLOR_WINDOW equ 5
DEFAULT_GUI_FONT equ 17
TH32CS_SNAPPROCESS equ 2h
TH32CS_SNAPTHREAD equ 4h
PROCESS_TERMINATE equ 1h
PROCESS_QUERY_LIMITED_INFORMATION equ 1000h
PROCESS_SET_QUOTA equ 100h
THREAD_SUSPEND_RESUME equ 2h
INVALID_HANDLE_VALUE equ -1
INFINITE equ 0FFFFFFFFh
ID_LIST equ 1001
ID_KILL equ 2001
ID_PAUSE equ 2002
ID_RESUME equ 2003
ID_TRIM equ 2004
TIMER_REFRESH equ 77h

PROCESSENTRY32W struct
  peSize dd ?
  peUsage dd ?
  pePid dd ?
  peHeap dd ?
  peMod dd ?
  peThreads dd ?
  peParent dd ?
  pePri dd ?
  peFlags dd ?
  peExe db 520 dup(?)
PROCESSENTRY32W ends

PROCESS_MEMORY_COUNTERS_EX struct
  pmcbSize dd ?
  pmcbPageFaults dd ?
  pmcbPeakWS dq ?
  pmcbWS dq ?
  pmcbPeakPaged dq ?
  pmcbPaged dq ?
  pmcbPeakNP dq ?
  pmcbNP dq ?
  pmcbPagefile dq ?
  pmcbPrivate dq ?
PROCESS_MEMORY_COUNTERS_EX ends

LVITEMW struct
  lviMask dd ?
  lviItem dd ?
  lviSubItem dd ?
  lviState dd ?
  lviStateMask dd ?
  lviText dq ?
  lviTextMax dd ?
  lviImage dd ?
  lviParam dq ?
  lviIndent dd ?
  lviGroupId dd ?
  lviColumns dd ?
  lviViewId dd ?
LVITEMW ends

LVCOLUMNW struct
  lvcMask dd ?
  lvcFmt dd ?
  lvcWidth dd ?
  lvcText dq ?
  lvcTextMax dd ?
  lvcSubItem dd ?
  lvcImage dd ?
  lvcOrder dd ?
  lvcCxMin dd ?
  lvcCxDefault dd ?
  lvcCxIdeal dd ?
LVCOLUMNW ends

WNDCLASSEXW struct
  wcSize dd ?
  wcStyle dd ?
  wcProc dq ?
  wcClsExtra dd ?
  wcWndExtra dd ?
  wcInst dq ?
  wcIcon dq ?
  wcCursor dq ?
  wcBrush dq ?
  wcMenu dq ?
  wcName dq ?
  wcIconSm dq ?
WNDCLASSEXW ends

MSG struct
  msgHwnd dq ?
  msgMessage dd ?
  msgWParam dq ?
  msgLParam dq ?
  msgTime dd ?
  msgPtX dd ?
  msgPtY dd ?
  msgPrivate dd ?
MSG ends

RECT struct
  rcLeft dd ?
  rcTop dd ?
  rcRight dd ?
  rcBottom dd ?
RECT ends

.data
szClass dw 'T','a','s','k','M','a','n','a','g','e','r','X','6','4',0
szStatic dw 83,84,65,84,73,67,0
szList dw 83,121,115,76,105,115,116,86,105,101,119,51,50,0
szButton dw 66,85,84,84,79,78,0
szKill dw 84,101,114,109,105,110,97,116,101,0
szPause dw 80,97,117,115,101,0
szResume dw 82,101,115,117,109,101,0
szTrim dw 84,114,105,109,32,82,65,77,0
szMemory dw 80,114,105,118,97,116,101,32,82,65,77,0
szStatus dw 83,116,97,116,117,115,0
szRunning dw 82,117,110,110,105,110,103,0
szPaused dw 80,97,117,115,101,100,0
szDenied dw 65,99,99,101,115,115,32,100,101,110,105,101,100,0
szConfirm dw 84,101,114,109,105,110,97,116,101,32,115,101,108,101,99,116,101,100,32,112,114,111,99,101,115,115,63,0
szError dw 79,112,101,114,97,116,105,111,110,32,102,97,105,108,101,100,0
szNoSelection dw 83,101,108,101,99,116,32,97,32,112,114,111,99,101,115,115,32,102,105,114,115,116,46,0
szMemUnit dw 32,77,66,0
szFmtPid db '%u',0
szFmtMem db '%I64u',0
szFmtThreads db '%u',0
szFmtStatus db '%s',0
gInst dq ?
gMain dq ?
gList dq ?
gCount dd ?
gPids dd 4096 dup(?)
gPe PROCESSENTRY32W <>
gMem PROCESS_MEMORY_COUNTERS_EX <>
gBuf db 128 dup(?)
gMsg MSG <>
gRect RECT <>
gClass WNDCLASSEXW <>

.code
SimpleWndProc PROC
  sub rsp,68h
  cmp edx,2E0h
  je dpiChanged
  cmp edx,24h
  je minimumSize
  cmp edx,8001h
  je snapshotReady
  cmp edx,WM_NOTIFY
  je displayNotify
  cmp edx,WM_COMMAND
  je menuCommand
  cmp edx,8002h
  je actionComplete
  cmp edx,231h
  je beginMove
  cmp edx,232h
  je endMove
  cmp edx,WM_CREATE
  je simpleCreate
  cmp edx,WM_SIZE
  je simpleResize
  cmp edx,WM_TIMER
  je simpleTimer
  cmp edx,WM_DESTROY
  jne swp_def
  mov collectorStop,1
  mov rcx,refreshEvent
  call SetEvent
  mov rcx,ackEvent
  call SetEvent
  xor ecx,ecx
  call PostQuitMessage
  xor eax,eax
  add rsp,68h
  ret
swp_def:
  call DefWindowProcW
  add rsp,68h
  ret
minimumSize:
  mov eax,uiMinWidth
  mov [r9+32],eax
  mov eax,uiMinHeight
  mov [r9+36],eax
  jmp moveDone
dpiChanged:
  mov eax,r8d
  and eax,0FFFFh
  mov [rsp+40h],eax
  mov rax,r9
  mov r8d,[rax]
  mov r9d,[rax+4]
  mov edx,[rax+8]
  sub edx,r8d
  mov [rsp+20h],rdx
  mov edx,[rax+12]
  sub edx,r9d
  mov [rsp+28h],rdx
  mov qword ptr [rsp+30h],14h
  mov rcx,gMain
  xor edx,edx
  call SetWindowPos
  mov ecx,[rsp+40h]
  call UpdateUiDpi
  call LayoutContent
  jmp moveDone
simpleCreate:
  mov gMain,rcx
  call GetDpiForWindow
  mov ecx,eax
  call UpdateUiDpi
  call InitGui
  call BuildMenus
  call CreateSearch
  call BuildSidebar
  call CreateChart
  call SetModeColumns
  call ApplyTheme
  xor eax,eax
  add rsp,68h
  ret
simpleResize:
  call LayoutContent
  xor eax,eax
  add rsp,68h
  ret
simpleTimer:
  xor eax,eax
  add rsp,68h
  ret
beginMove:
  mov movingWindow,1
  xor eax,eax
  add rsp,68h
  ret
endMove:
  mov movingWindow,0
  cmp snapshotPending,0
  je moveDone
  call ApplySnapshot
moveDone:
  xor eax,eax
  add rsp,68h
  ret
snapshotReady:
  mov snapshotPending,1
  cmp movingWindow,0
  jne moveDone
  call ApplySnapshot
  jmp moveDone
displayNotify:
  cmp dword ptr [r9+16],-3
  jne notTreeDouble
  call ToggleTreeNode
  jmp moveDone
notTreeDouble:
  cmp dword ptr [r9+16],-108
  je columnClicked
  cmp dword ptr [r9+16],-177
  jne moveDone
  mov eax,dword ptr [r9+28]
  cmp eax,visibleCount
  jae moveDone
  lea rdx,rowMap
  mov rax,[rdx+rax*8]
  cmp frontMode,0
  jne displayNormal
  cmp treeEnabled,0
  je displayNormal
  cmp dword ptr [r9+32],4
  je displayTreeCommit
  cmp dword ptr [r9+32],5
  je displayTreeWS
  cmp dword ptr [r9+32],0
  jne displayNormal
  mov rax,[rax+3744]
  mov qword ptr [r9+48],rax
  jmp moveDone
displayTreeCommit:
  lea rax,[rax+3752]
  mov qword ptr [r9+48],rax
  jmp moveDone
displayTreeWS:
  lea rax,[rax+3816]
  mov qword ptr [r9+48],rax
  jmp moveDone
displayNormal:
  mov edx,dword ptr [r9+32]
  cmp edx,7
  jae moveDone
  shl edx,9
  add rax,rdx
  mov qword ptr [r9+48],rax
  jmp moveDone
menuCommand:
  mov eax,r8d
  shr eax,16
  cmp eax,300h
  jne menuNormalRoute
  call RememberSelection
  call BuildRowMap
  jmp moveDone
menuNormalRoute:
  mov ecx,r8d
  and ecx,0FFFFh
  cmp ecx,2300
  jb menuAction
  call SettingsCommand
  jmp moveDone
menuAction:
  call QueueAction
  jmp moveDone
columnClicked:
  mov eax,dword ptr [r9+28]
  cmp eax,7
  jae moveDone
  cmp eax,sortColumn
  jne newSortColumn
  xor sortDescending,1
  jmp doSortColumn
newSortColumn:
  mov sortColumn,eax
  mov sortDescending,0
doSortColumn:
  call RememberSelection
  call BuildRowMap
  jmp moveDone
actionComplete:
  mov rcx,gMain
  lea rdx,actionMessage
  lea r8,szTitle
  mov r9d,40h
  call MessageBoxW
  mov actionBusy,0
  jmp moveDone
SimpleWndProc ENDP

SimpleEntry PROC
  sub rsp,68h
  lea rcx,commonControls
  call InitCommonControlsEx
  xor ecx,ecx
  call GetModuleHandleW
  mov gInst,rax
  call LoadSettings
  mov gClass.wcSize,SIZEOF WNDCLASSEXW
  mov gClass.wcStyle,3
  lea rdx,SimpleWndProc
  mov gClass.wcProc,rdx
  mov rax,gInst
  mov gClass.wcInst,rax
  mov rcx,gInst
  mov edx,IDI_APP
  call LoadIconW
  mov gClass.wcIcon,rax
  mov gClass.wcIconSm,rax
  xor ecx,ecx
  mov edx,IDC_ARROW
  call LoadCursorW
  mov gClass.wcCursor,rax
  mov gClass.wcBrush,COLOR_WINDOW+1
  lea rax,szClass
  mov gClass.wcName,rax
  lea rcx,gClass
  call RegisterClassExW
  xor ecx,ecx
  lea rdx,szClass
  lea r8,szTitle
  mov r9d,WS_OVERLAPPEDWINDOW or WS_VISIBLE
  mov dword ptr [rsp+20h],100
  mov dword ptr [rsp+28h],100
  mov eax,1100
  imul eax,uiDpi
  xor edx,edx
  mov r10d,96
  div r10d
  mov [rsp+30h],rax
  mov eax,700
  imul eax,uiDpi
  xor edx,edx
  div r10d
  mov [rsp+38h],rax
  lea rdx,szClass
  xor eax,eax
  mov [rsp+40h],rax
  mov [rsp+48h],rax
  mov rax,gInst
  mov [rsp+50h],rax
  xor eax,eax
  mov [rsp+58h],rax
  call CreateWindowExW
  mov gMain,rax
  test rax,rax
  jz simple_exit
  mov rcx,rax
  mov edx,SW_SHOW
  call ShowWindow
  mov rcx,gMain
  call UpdateWindow
simple_loop:
  lea rcx,messageBuffer
  xor edx,edx
  xor r8d,r8d
  xor r9d,r9d
  call GetMessageW
  test eax,eax
  jle simple_exit
  lea rcx,messageBuffer
  call TranslateMessage
  lea rcx,messageBuffer
  call DispatchMessageW
  jmp simple_loop
simple_exit:
  mov collectorStop,1
  mov rcx,refreshEvent
  call SetEvent
  mov rcx,ackEvent
  call SetEvent
  mov rcx,collectorHandle
  mov edx,-1
  call WaitForSingleObject
  xor ecx,ecx
  call ExitProcess
SimpleEntry ENDP

extern InitCommonControlsEx:proc
.data
commonControls dd 8,1
.data?
align 8
messageBuffer db 48 dup(?)
.data
.data?
align 8
columnBuffer db 56 dup(?)
.data
.data?
align 8
itemBuffer db 88 dup(?)
.data
.data?
align 8
processBuffer db 568 dup(?)
.data
.data?
align 8
memoryBuffer db 80 dup(?)
.data
.data?
align 8
decimalBuffer dw 32 dup(?)
.data
.code

; Register-only leaf routines avoid MASM parameter home-slot assumptions.
GuiDecimal PROC
  lea r10,decimalBuffer+62
  mov word ptr [r10],0
  mov rax,rcx
  mov r11,10
gdLoop:
  xor edx,edx
  div r11
  add dx,48
  sub r10,2
  mov [r10],dx
  test rax,rax
  jnz gdLoop
  mov rax,r10
  ret
GuiDecimal ENDP

; RCX index, RDX caption, R8D width
GuiColumn PROC
  sub rsp,28h
  mov r11,rdx
  mov eax,r8d
  imul eax,uiDpi
  xor edx,edx
  mov r10d,96
  div r10d
  mov r8d,eax
  mov rdx,r11
  mov dword ptr [columnBuffer],7
  mov dword ptr [columnBuffer+8],r8d
  mov qword ptr [columnBuffer+16],rdx
  mov r8,rcx
  mov rcx,gList
  mov edx,1061h
  lea r9,columnBuffer
  call SendMessageW
  add rsp,28h
  ret
GuiColumn ENDP

InitGui PROC
  sub rsp,68h
  xor ecx,ecx
  lea rdx,szList
  xor r8d,r8d
  mov r9d,5001100Dh
  mov qword ptr [rsp+20h],16
  mov qword ptr [rsp+28h],16
  mov qword ptr [rsp+30h],1000
  mov qword ptr [rsp+38h],600
  mov rax,gMain
  mov [rsp+40h],rax
  mov qword ptr [rsp+48h],1001
  mov rax,gInst
  mov [rsp+50h],rax
  mov qword ptr [rsp+58h],0
  call CreateWindowExW
  mov gList,rax
  mov rcx,rax
  mov edx,1036h
  mov r8d,10020h
  mov r9d,10020h
  call SendMessageW
  xor ecx,ecx
  lea rdx,szProcess
  mov r8d,340
  call GuiColumn
  mov ecx,1
  lea rdx,szPid
  mov r8d,90
  call GuiColumn
  mov ecx,2
  lea rdx,szParent
  mov r8d,90
  call GuiColumn
  mov ecx,3
  lea rdx,szThreads
  mov r8d,90
  call GuiColumn
  mov ecx,4
  lea rdx,privateCommitCol
  mov r8d,220
  call GuiColumn
  mov ecx,5
  lea rdx,wsCaption
  mov r8d,150
  call GuiColumn
  mov ecx,6
  lea rdx,cpuCaption
  mov r8d,90
  call GuiColumn
  call StartCollector
  add rsp,68h
  ret
InitGui ENDP

; RAX text, EDX subitem
GuiCell PROC
  sub rsp,28h
  mov qword ptr [itemBuffer+24],rax
  mov dword ptr [itemBuffer+8],edx
  mov rcx,gList
  mov edx,1074h
  mov r8d,gCount
  lea r9,itemBuffer
  call SendMessageW
  add rsp,28h
  ret
GuiCell ENDP

SnapshotGui PROC
  push rbx
  push rsi
  push rdi
  sub rsp,20h
  mov rcx,gList
  mov edx,0Bh
  xor r8d,r8d
  xor r9d,r9d
  call SendMessageW
  mov rcx,gList
  mov edx,1009h
  xor r8d,r8d
  xor r9d,r9d
  call SendMessageW
  mov gCount,0
  mov ecx,2
  xor edx,edx
  call CreateToolhelp32Snapshot
  mov rbx,rax
  cmp rax,-1
  je sgEnd
  mov dword ptr [processBuffer],568
  mov rcx,rbx
  lea rdx,processBuffer
  call Process32FirstW
sgNext:
  test eax,eax
  jz sgClose
  mov dword ptr [itemBuffer],1
  mov eax,gCount
  mov dword ptr [itemBuffer+4],eax
  mov dword ptr [itemBuffer+8],0
  lea rax,processBuffer+44
  mov qword ptr [itemBuffer+24],rax
  mov rcx,gList
  mov edx,104Dh
  xor r8d,r8d
  lea r9,itemBuffer
  call SendMessageW
  mov ecx,dword ptr [processBuffer+8]
  call GuiDecimal
  mov edx,1
  call GuiCell
  mov ecx,dword ptr [processBuffer+32]
  call GuiDecimal
  mov edx,2
  call GuiCell
  mov ecx,dword ptr [processBuffer+28]
  call GuiDecimal
  mov edx,3
  call GuiCell
  mov ecx,410h
  xor edx,edx
  mov r8d,dword ptr [processBuffer+8]
  call OpenProcess
  mov rsi,rax
  lea rdi,unavailable
  test rax,rax
  jz sgMemory
  mov rcx,rax
  lea rdx,memoryBuffer
  mov r8d,80
  mov dword ptr [memoryBuffer],80
  call GetProcessMemoryInfo
  test eax,eax
  jz sgCloseProcess
  mov rcx,qword ptr [memoryBuffer+72]
  shr rcx,20
  call GuiDecimal
  mov rdi,rax
sgCloseProcess:
  mov rcx,rsi
  call CloseHandle
sgMemory:
  mov rax,rdi
  mov edx,4
  call GuiCell
  inc gCount
  mov rcx,rbx
  lea rdx,processBuffer
  call Process32NextW
  jmp sgNext
sgClose:
  mov rcx,rbx
  call CloseHandle
sgEnd:
  mov rcx,gList
  mov edx,0Bh
  mov r8d,1
  xor r9d,r9d
  call SendMessageW
  add rsp,20h
  pop rdi
  pop rsi
  pop rbx
  ret
SnapshotGui ENDP
include collector.inc
include actions.inc
include threads.inc
include selftest.inc
include panels.inc
include view.inc
include telemetry.inc
include settings.inc
include startup.inc
include history.inc
include extended.inc
include tree.inc
include history_store.inc
include charts.inc
include chart_metrics.inc
include startup_v2.inc
include startup_control.inc
include tree_control.inc
include ui_strings.inc
include model_tests.inc
include application_roots.inc
include dpi.inc
include task_test.inc
include action_errors.inc
END






