; Working-set trim measurement. All procedures run on the action/collector thread.
; MEMORYSTATUSEX is 64 bytes; PROCESS_MEMORY_COUNTERS_EX is 80 bytes on x64.
; System before/after values are observations, not a causal "bytes freed" claim.
extern SetLastError:proc
extern GetCurrentThread:proc, GetThreadPriority:proc, SetThreadPriority:proc, Sleep:proc
.data
align 8
memoryTrimActive dd 0
memoryTrimBeforeValid dd 0
memoryTrimAfterValid dd 0
memoryTrimMeasured dd 0
memoryTrimSkipped dd 0
memoryTrimCancelled dd 0
memoryTrimForegroundPid dd 0
memoryTrimSession dd 0
memoryTrimTargetLoad dd 0
memoryTrimForegroundSkipped dd 0
memoryTrimSmallSkipped dd 0
memoryTrimStopReason dd 0 ; 1=pressure target, 2=budget
; Single writer (collector), aligned atomic reads by the animation UI. Total is
; the immutable snapshot size; progress counts examined entries, including skips.
memoryTrimProgress dd 0
memoryTrimTotal dd 0
memoryTrimPrioritySaved dd 0
memoryTrimPriorityChanged dd 0
memoryTrimStartTick dq 0
memoryTrimForegroundName dq 0
memoryTrimPolicyFmtEn dw 13,10,83,101,108,101,99,116,105,118,101,32,116,114,105,109,58,32,97,99,116,105,118,101,45,97,112,112,32,115,107,105
dw 112,115,32,37,117,59,32,115,109,97,108,108,32,119,111,114,107,105,110,103,45,115,101,116,32,115,107,105,112,115,32,37
dw 117,59,32,115,116,111,112,32,114,101,97,115,111,110,32,37,117,32,40,48,61,99,111,109,112,108,101,116,101,44,32,49
dw 61,116,97,114,103,101,116,32,114,101,97,99,104,101,100,44,32,50,61,98,117,100,103,101,116,41,46,0
memoryTrimPolicyFmtRu dw 13,10,1042,1099,1073,1086,1088,1086,1095,1085,1072,1103,32,1086,1095,1080,1089,1090,1082,1072,58,32,1087,1088,1086,1087,1091,1097,1077,1085,1086,32
dw 1072,1082,1090,1080,1074,1085,1099,1093,32,37,117,44,32,1084,1072,1083,1099,1093,32,1088,1072,1073,1086,1095,1080,1093,32,1085,1072,1073,1086,1088
dw 1086,1074,32,37,117,46,32,1055,1088,1080,1095,1080,1085,1072,32,1086,1089,1090,1072,1085,1086,1074,1082,1080,32,37,117,32,40,48,61,1079
dw 1072,1074,1077,1088,1096,1077,1085,1086,44,32,49,61,1094,1077,1083,1100,32,1076,1086,1089,1090,1080,1075,1085,1091,1090,1072,44,32,50,61,1083
dw 1080,1084,1080,1090,41,46,0
memoryTrimWsBefore dq 0
memoryTrimWsAfter dq 0
memoryTrimCommitBefore dq 0
memoryTrimCommitAfter dq 0
.data?
align 8
memoryTrimBefore db 64 dup(?)
memoryTrimAfter db 64 dup(?)
memoryTrimProcessBefore db 80 dup(?)
memoryTrimProcessAfter db 80 dup(?)
memoryTrimEntry db 568 dup(?)
memoryTrimCurrent db 64 dup(?)
memoryTrimForegroundPath dw 1024 dup(?)
.code

BeginMemoryTrim PROC
  sub rsp,28h
  mov memoryTrimActive,0
  cmp actionMode,0
  jne bmtDone
  cmp actionCode,2004
  je bmtStart
  cmp actionCode,2005
  je bmtStart
  cmp actionCode,2022
  jne bmtDone
bmtStart:
  mov memoryTrimActive,1
  mov memoryTrimBeforeValid,0
  mov memoryTrimAfterValid,0
  mov memoryTrimMeasured,0
  mov memoryTrimSkipped,0
  mov memoryTrimCancelled,0
  mov memoryTrimForegroundSkipped,0
  mov memoryTrimSmallSkipped,0
  mov memoryTrimStopReason,0
  mov memoryTrimProgress,0
  mov memoryTrimTotal,0
  mov memoryTrimWsBefore,0
  mov memoryTrimWsAfter,0
  mov memoryTrimCommitBefore,0
  mov memoryTrimCommitAfter,0
  mov dword ptr [memoryTrimBefore],64
  lea rcx,memoryTrimBefore
  call GlobalMemoryStatusEx
  mov memoryTrimBeforeValid,eax
bmtDone:
  add rsp,28h
  ret
BeginMemoryTrim ENDP

FinishMemoryTrim PROC
  sub rsp,28h
  cmp memoryTrimActive,0
  je fmtDone
  mov dword ptr [memoryTrimAfter],64
  lea rcx,memoryTrimAfter
  call GlobalMemoryStatusEx
  mov memoryTrimAfterValid,eax
fmtDone:
  add rsp,28h
  ret
FinishMemoryTrim ENDP

; RCX = guarded process handle. Return BOOL; preserve the trim API's last error.
; Only complete successful before/after pairs are added to the process totals.
TrimProcessMemory PROC
  push rbx
  sub rsp,30h
  mov rbx,rcx
  lea rdx,memoryTrimProcessBefore
  mov dword ptr [rdx],80
  mov r8d,80
  call GetProcessMemoryInfo
  mov [rsp+20h],eax
  mov rcx,rbx
  call EmptyWorkingSet
  test eax,eax
  jz tpmFailed
  mov rcx,rbx
  lea rdx,memoryTrimProcessAfter
  mov dword ptr [rdx],80
  mov r8d,80
  call GetProcessMemoryInfo
  test eax,eax
  jz tpmSuccess
  cmp dword ptr [rsp+20h],0
  je tpmSuccess
  mov rax,qword ptr [memoryTrimProcessBefore+16]
  add memoryTrimWsBefore,rax
  mov rax,qword ptr [memoryTrimProcessAfter+16]
  add memoryTrimWsAfter,rax
  mov rax,qword ptr [memoryTrimProcessBefore+72]
  add memoryTrimCommitBefore,rax
  mov rax,qword ptr [memoryTrimProcessAfter+72]
  add memoryTrimCommitAfter,rax
  inc memoryTrimMeasured
tpmSuccess:
  mov eax,1
  jmp tpmDone
tpmFailed:
  call GetLastError
  mov ecx,eax
  call SetLastError
  xor eax,eax
tpmDone:
  add rsp,30h
  pop rbx
  ret
TrimProcessMemory ENDP

; Snapshot all current processes rather than treating service/thread UI rows as PIDs.
; Each opened handle is guarded; inaccessible/system/self targets are skipped.
RunGlobalMemoryTrim PROC
  push rbx
  push rsi
  sub rsp,28h
  mov memoryTrimProgress,0
  mov memoryTrimTotal,0
  call BeginTrimScheduling
  call PrepareSelectiveTrim
  mov ecx,2 ; TH32CS_SNAPPROCESS
  xor edx,edx
  call CreateToolhelp32Snapshot
  mov rsi,rax
  cmp rax,-1
  je rgmtSnapshotError
  mov dword ptr [memoryTrimEntry],568
  mov rcx,rsi
  lea rdx,memoryTrimEntry
  call Process32FirstW
rgmtCount:
  test eax,eax
  jz rgmtCountDone
  inc memoryTrimTotal
  cmp collectorStop,0
  jne rgmtCancelled
  mov rcx,rsi
  lea rdx,memoryTrimEntry
  call Process32NextW
  jmp rgmtCount
rgmtCountDone:
  call GetLastError
  cmp eax,18
  jne rgmtEnumerationError
  ; Rewind the same immutable snapshot; no second system snapshot or UI query.
  mov dword ptr [memoryTrimEntry],568
  mov rcx,rsi
  lea rdx,memoryTrimEntry
  call Process32FirstW
rgmtNext:
  test eax,eax
  jz rgmtEnumerationEnd
  cmp collectorStop,0
  jne rgmtCancelled
  call GetTickCount64
  sub rax,memoryTrimStartTick
  cmp rax,3000
  jae rgmtBudget
  cmp actionSucceeded,32
  jae rgmtBudget
  inc memoryTrimProgress
  mov eax,dword ptr [memoryTrimEntry+8]
  cmp eax,memoryTrimForegroundPid
  je rgmtForeground
  mov rdx,memoryTrimForegroundName
  test rdx,rdx
  jz rgmtOpen
  lea rcx,[memoryTrimEntry+44]
  call lstrcmpiW
  test eax,eax
  jz rgmtForeground
rgmtOpen:
  mov ecx,1100h
  xor edx,edx
  mov r8d,dword ptr [memoryTrimEntry+8]
  call OpenProcess
  mov rbx,rax
  test rax,rax
  jz rgmtSkipped
  mov rcx,rax
  mov edx,dword ptr [memoryTrimEntry+8]
  call GuardProcess
  test eax,eax
  jz rgmtGuarded
  mov eax,checkSession
  cmp eax,memoryTrimSession
  jne rgmtGuarded
  mov rcx,rbx
  lea rdx,memoryTrimProcessBefore
  mov dword ptr [rdx],80
  mov r8d,80
  call GetProcessMemoryInfo
  test eax,eax
  jz rgmtGuarded
  cmp qword ptr [memoryTrimProcessBefore+16],16777216 ; <16 MiB
  jb rgmtSmall
  mov dword ptr [rsp+20h],0
  call GetForegroundWindow
  mov rcx,rax
  lea rdx,[rsp+20h]
  call GetWindowThreadProcessId
  mov eax,dword ptr [rsp+20h]
  cmp eax,dword ptr [memoryTrimEntry+8]
  jne rgmtTrim
  inc memoryTrimForegroundSkipped
  jmp rgmtGuarded
rgmtTrim:
  mov rcx,rbx
  call TrimProcessMemory
  test eax,eax
  jz rgmtFailed
  inc actionSucceeded
  ; Let page-fault recovery and foreground work run between target trims.
  ; Sleep affects only this collector thread; the UI animates independently.
  mov ecx,25
  call Sleep
  mov dword ptr [memoryTrimCurrent],64
  lea rcx,memoryTrimCurrent
  call GlobalMemoryStatusEx
  test eax,eax
  jz rgmtClose
  mov eax,dword ptr [memoryTrimCurrent+4]
  cmp eax,memoryTrimTargetLoad
  ja rgmtClose
  mov memoryTrimStopReason,1
  mov rcx,rbx
  call CloseHandle
  jmp rgmtSnapshotClose
rgmtSmall:
  inc memoryTrimSmallSkipped
  jmp rgmtGuarded
rgmtFailed:
  call GetLastError
  mov actionResult,eax
  inc actionFailed
  jmp rgmtClose
rgmtGuarded:
  inc memoryTrimSkipped
rgmtClose:
  mov rcx,rbx
  call CloseHandle
  jmp rgmtAdvance
rgmtForeground:
  inc memoryTrimForegroundSkipped
rgmtSkipped:
  inc memoryTrimSkipped
rgmtAdvance:
  mov rcx,rsi
  lea rdx,memoryTrimEntry
  call Process32NextW
  jmp rgmtNext
rgmtEnumerationEnd:
  call GetLastError
  cmp eax,18 ; ERROR_NO_MORE_FILES: normal end of snapshot
  je rgmtSnapshotClose
rgmtEnumerationError:
  mov actionResult,eax
  inc actionFailed
  jmp rgmtSnapshotClose
rgmtBudget:
  mov memoryTrimStopReason,2
  jmp rgmtSnapshotClose
rgmtCancelled:
  mov memoryTrimCancelled,1
  mov actionResult,1223 ; ERROR_CANCELLED
rgmtSnapshotClose:
  mov rcx,rsi
  call CloseHandle
  jmp rgmtDone
rgmtSnapshotError:
  call GetLastError
  mov actionResult,eax
  inc actionFailed
rgmtDone:
  call EndTrimScheduling
  add rsp,28h
  pop rsi
  pop rbx
  ret
RunGlobalMemoryTrim ENDP

; Lower only the collector's CPU priority for the global pass. Restore its
; exact prior value on all exits; no process-wide priority or paging setting.
BeginTrimScheduling PROC
  sub rsp,28h
  cmp memoryTrimPriorityChanged,0
  je btsReadPriority
  call EndTrimScheduling
  cmp memoryTrimPriorityChanged,0
  jne btsDone
btsReadPriority:
  call GetCurrentThread
  mov rcx,rax
  call GetThreadPriority
  cmp eax,7FFFFFFFh ; THREAD_PRIORITY_ERROR_RETURN
  je btsDone
  mov memoryTrimPrioritySaved,eax
  ; A caller already below normal should never be promoted by this operation.
  cmp eax,-1
  jle btsDone
  call GetCurrentThread
  mov rcx,rax
  mov edx,-1 ; THREAD_PRIORITY_BELOW_NORMAL
  call SetThreadPriority
  test eax,eax
  jz btsDone
  mov memoryTrimPriorityChanged,1
btsDone:
  add rsp,28h
  ret
BeginTrimScheduling ENDP

EndTrimScheduling PROC
  sub rsp,28h
  cmp memoryTrimPriorityChanged,0
  je etsDone
  call GetCurrentThread
  mov rcx,rax
  mov edx,memoryTrimPrioritySaved
  call SetThreadPriority
  ; Keep the flag if restoration failed; a subsequent call can retry.
  test eax,eax
  jz etsDone
  mov memoryTrimPriorityChanged,0
etsDone:
  add rsp,28h
  ret
EndTrimScheduling ENDP

AppendMemoryTrimReport PROC
  sub rsp,48h
  cmp memoryTrimActive,0
  je amtrDone
  cmp memoryTrimBeforeValid,0
  je amtrUnavailable
  cmp memoryTrimAfterValid,0
  je amtrUnavailable
  lea rcx,actionErrorText
  lea rdx,memoryTrimSystemFmt
  mov r8d,dword ptr [memoryTrimBefore+4]
  mov r9d,dword ptr [memoryTrimAfter+4]
  mov rax,qword ptr [memoryTrimBefore+16]
  shr rax,20
  mov [rsp+20h],rax
  mov rax,qword ptr [memoryTrimAfter+16]
  shr rax,20
  mov [rsp+28h],rax
  call wsprintfW
  lea rcx,actionMessage
  lea rdx,actionErrorText
  call lstrcatW
  jmp amtrProcesses
amtrUnavailable:
  lea rcx,actionMessage
  lea rdx,memoryTrimUnavailable
  call lstrcatW
amtrProcesses:
  lea rcx,actionErrorText
  lea rdx,memoryTrimSkippedFmt
  mov r8d,memoryTrimSkipped
  call wsprintfW
  lea rcx,actionMessage
  lea rdx,actionErrorText
  call lstrcatW
  cmp memoryTrimMeasured,0
  je amtrNote
  lea rcx,actionErrorText
  lea rdx,memoryTrimProcessFmt
  mov r8d,memoryTrimMeasured
  mov r9,memoryTrimWsBefore
  shr r9,20
  mov rax,memoryTrimWsAfter
  shr rax,20
  mov [rsp+20h],rax
  mov rax,memoryTrimCommitBefore
  shr rax,20
  mov [rsp+28h],rax
  mov rax,memoryTrimCommitAfter
  shr rax,20
  mov [rsp+30h],rax
  call wsprintfW
  lea rcx,actionMessage
  lea rdx,actionErrorText
  call lstrcatW
amtrNote:
  cmp actionCode,2005
  jne amtrGeneralNote
  lea rcx,actionErrorText
  lea rdx,memoryTrimPolicyFmtEn
  cmp appLanguage,0
  je amtrPolicyFormat
  lea rdx,memoryTrimPolicyFmtRu
amtrPolicyFormat:
  mov r8d,memoryTrimForegroundSkipped
  mov r9d,memoryTrimSmallSkipped
  mov eax,memoryTrimStopReason
  mov [rsp+20h],rax
  call wsprintfW
  lea rcx,actionMessage
  lea rdx,actionErrorText
  call lstrcatW
amtrGeneralNote:
  lea rcx,actionMessage
  lea rdx,memoryTrimNote
  call lstrcatW
amtrDone:
  add rsp,48h
  ret
AppendMemoryTrimReport ENDP

; Collector-owned setup captures the active application's executable once.
; Excluding matching executable names also protects its renderer/helper processes.
PrepareSelectiveTrim PROC
  push rbx
  sub rsp,30h
  call GetTickCount64
  mov memoryTrimStartTick,rax
  mov memoryPolicyLastRun,rax
  mov memoryTrimSession,-1
  mov memoryTrimForegroundPid,0
  mov memoryTrimForegroundName,0
  mov memoryTrimTargetLoad,0
  cmp memoryTrimBeforeValid,0
  je pstSession
  mov eax,dword ptr [memoryTrimBefore+4]
  sub eax,5
  jnc pstTarget
  xor eax,eax
pstTarget:
  mov memoryTrimTargetLoad,eax
pstSession:
  call GetCurrentProcessId
  mov ecx,eax
  lea rdx,memoryTrimSession
  call ProcessIdToSessionId
  call GetForegroundWindow
  test rax,rax
  jz pstDone
  mov rcx,rax
  lea rdx,memoryTrimForegroundPid
  call GetWindowThreadProcessId
  call GetCurrentProcessId
  cmp eax,memoryTrimForegroundPid
  jne pstOpen
  mov eax,memoryPolicyForegroundPid
  mov memoryTrimForegroundPid,eax
pstOpen:
  mov ecx,1000h
  xor edx,edx
  mov r8d,memoryTrimForegroundPid
  call OpenProcess
  mov rbx,rax
  test rax,rax
  jz pstDone
  mov dword ptr [rsp+20h],1024
  mov rcx,rbx
  xor edx,edx
  lea r8,memoryTrimForegroundPath
  lea r9,[rsp+20h]
  call QueryFullProcessImageNameW
  test eax,eax
  jz pstClose
  lea rax,memoryTrimForegroundPath
  mov memoryTrimForegroundName,rax
pstName:
  movzx ecx,word ptr [rax]
  test ecx,ecx
  jz pstClose
  add rax,2
  cmp ecx,92
  jne pstName
  mov memoryTrimForegroundName,rax
  jmp pstName
pstClose:
  mov rcx,rbx
  call CloseHandle
pstDone:
  add rsp,30h
  pop rbx
  ret
PrepareSelectiveTrim ENDP

include memory_policy.inc
include memory_trim_tests.inc
include memory_policy_tests.inc
