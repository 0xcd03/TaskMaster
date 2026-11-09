; Working-set trim measurement. All procedures run on the action/collector thread.
; MEMORYSTATUSEX is 64 bytes; PROCESS_MEMORY_COUNTERS_EX is 80 bytes on x64.
; System before/after values are observations, not a causal "bytes freed" claim.
extern SetLastError:proc
.data
align 8
memoryTrimActive dd 0
memoryTrimBeforeValid dd 0
memoryTrimAfterValid dd 0
memoryTrimMeasured dd 0
memoryTrimSkipped dd 0
memoryTrimCancelled dd 0
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
rgmtNext:
  test eax,eax
  jz rgmtEnumerationEnd
  cmp collectorStop,0
  jne rgmtCancelled
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
  mov rcx,rbx
  call TrimProcessMemory
  test eax,eax
  jz rgmtFailed
  inc actionSucceeded
  jmp rgmtClose
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
  mov actionResult,eax
  inc actionFailed
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
  add rsp,28h
  pop rsi
  pop rbx
  ret
RunGlobalMemoryTrim ENDP

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
  lea rcx,actionMessage
  lea rdx,memoryTrimNote
  call lstrcatW
amtrDone:
  add rsp,48h
  ret
AppendMemoryTrimReport ENDP
