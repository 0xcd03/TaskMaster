; Read-only native SCM regression. Never starts, stops or configures a service.
; An independent large WinAPI page supplies names; a 4 KiB collector page must
; return the identical set, including every ERROR_MORE_DATA partial page.
.data
serviceTestStage dd 180
serviceTestAssertions dd 0
serviceTestExpectedCount dd 0
serviceTestSmallCount dd 0
serviceTestSmallPages dd 0
serviceTestBytes dd 0
serviceTestResume dd 0
serviceTestProbeCount dd 0
serviceTestWritten dd 0
serviceTestManager dq 0
serviceTestFormat db 'Services regression: exit=%u, assertions=%u, reference=%u, smallPageRows=%u, smallPages=%u, lastCaseError=%u',13,10,0
.data?
align 8
serviceTestReference db 262144 dup(?)
serviceTestProbe db 4096 dup(?)
serviceTestOutput db 256 dup(?)
.code
ServiceEnumerationTest PROC
  sub rsp,68h
  xor ecx,ecx
  call GetModuleHandleW
  mov gInst,rax
  call LoadUiStrings
  call GetProcessHeap
  mov rcx,rax
  mov edx,8
  mov r8d,16781312 ; 4096 rows plus one guard row, all initially zero
  call HeapAlloc
  test rax,rax
  jz setReport
  mov backBuffer,rax
  mov serviceTestStage,181
  xor ecx,ecx
  xor edx,edx
  mov r8d,4 ; SC_MANAGER_ENUMERATE_SERVICE only
  call OpenSCManagerW
  test rax,rax
  jz setReport
  mov serviceTestManager,rax
  mov serviceTestStage,182
  mov rcx,rax
  xor edx,edx
  mov r8d,30h ; SERVICE_WIN32
  mov r9d,3 ; SERVICE_STATE_ALL
  lea rax,serviceTestReference
  mov [rsp+20h],rax
  mov qword ptr [rsp+28h],262144
  lea rax,serviceTestBytes
  mov [rsp+30h],rax
  lea rax,serviceTestExpectedCount
  mov [rsp+38h],rax
  lea rax,serviceTestResume
  mov [rsp+40h],rax
  mov qword ptr [rsp+48h],0
  call EnumServicesStatusExW
  test eax,eax
  jz setReport ; a full independent reference page is required
  cmp serviceTestExpectedCount,1
  jbe setReport
  cmp serviceTestExpectedCount,4096
  jae setReport
  inc serviceTestAssertions

  ; Prove the chosen small size really returns a partial native page.
  mov serviceTestStage,183
  mov serviceTestResume,0
  mov rcx,serviceTestManager
  xor edx,edx
  mov r8d,30h
  mov r9d,3
  lea rax,serviceTestProbe
  mov [rsp+20h],rax
  mov qword ptr [rsp+28h],4096
  lea rax,serviceTestBytes
  mov [rsp+30h],rax
  lea rax,serviceTestProbeCount
  mov [rsp+38h],rax
  lea rax,serviceTestResume
  mov [rsp+40h],rax
  mov qword ptr [rsp+48h],0
  call EnumServicesStatusExW
  test eax,eax
  jnz setReport
  call GetLastError
  cmp eax,234 ; ERROR_MORE_DATA, with usable records
  jne setReport
  cmp serviceTestProbeCount,0
  je setReport
  mov eax,serviceTestProbeCount
  cmp eax,serviceTestExpectedCount
  jae setReport
  inc serviceTestAssertions

  mov serviceTestStage,184
  call CollectServices
  cmp serviceEnumError,0
  jne setReport
  call ServiceTestCompareNames
  test eax,eax
  jz setReport
  inc serviceTestAssertions

  mov serviceTestStage,185
  mov ecx,4096
  mov edx,4096
  call CollectServicesWithLimits
  mov eax,backCount
  mov serviceTestSmallCount,eax
  mov eax,serviceEnumPages
  mov serviceTestSmallPages,eax
  cmp serviceEnumError,0
  jne setReport
  cmp serviceEnumPages,1
  jbe setReport
  call ServiceTestCompareNames
  test eax,eax
  jz setReport
  inc serviceTestAssertions

  mov serviceTestStage,186
  mov collectorStop,1
  call CollectServices
  cmp backCount,0
  jne setReport
  cmp serviceEnumPages,0
  jne setReport
  cmp serviceEnumError,1223 ; ERROR_CANCELLED
  jne setReport
  mov collectorStop,0
  inc serviceTestAssertions

  ; A changed navigation generation cancels before opening another SCM page.
  mov serviceTestStage,187
  mov collectorHandle,1 ; cancellation predicate only, no worker is started
  mov desiredMode,2
  mov collectMode,2
  mov desiredGeneration,1
  mov collectGeneration,0
  call CollectServices
  cmp backCount,0
  jne setReport
  cmp serviceEnumPages,0
  jne setReport
  cmp serviceEnumError,1223
  jne setReport
  mov desiredGeneration,0
  mov collectorHandle,0
  inc serviceTestAssertions

  ; A small row limit exercises the production capacity branch and guard row.
  mov serviceTestStage,188
  mov rax,backBuffer
  mov rdx,0A55A1234DEADBEEFh
  mov [rax+12288],rdx
  mov ecx,4096
  mov edx,3
  call CollectServicesWithLimits
  cmp backCount,3
  jne setReport
  cmp serviceEnumTruncated,1
  jne setReport
  cmp serviceEnumError,234
  jne setReport
  mov rax,backBuffer
  mov rdx,0A55A1234DEADBEEFh
  cmp [rax+12288],rdx
  jne setReport
  inc serviceTestAssertions

  ; Too small for even one record: terminate instead of repeating the page.
  mov serviceTestStage,189
  mov ecx,1
  mov edx,4096
  call CollectServicesWithLimits
  cmp backCount,0
  jne setReport
  cmp serviceEnumPages,1
  jne setReport
  cmp serviceEnumError,234
  jne setReport
  inc serviceTestAssertions

  mov serviceTestStage,190
  mov ecx,262145 ; never let a caller exceed the static enumeration buffer
  mov edx,4096
  call CollectServicesWithLimits
  cmp backCount,0
  jne setReport
  cmp serviceEnumPages,0
  jne setReport
  cmp serviceEnumError,87 ; ERROR_INVALID_PARAMETER
  jne setReport
  inc serviceTestAssertions

  mov serviceTestStage,191
  mov ecx,4096
  mov edx,4097 ; the row buffer is always bounded by the production capacity
  call CollectServicesWithLimits
  cmp backCount,0
  jne setReport
  cmp serviceEnumPages,0
  jne setReport
  cmp serviceEnumError,87
  jne setReport
  inc serviceTestAssertions
  mov serviceTestStage,0
setReport:
  mov rcx,serviceTestManager
  test rcx,rcx
  jz setOutput
  call CloseServiceHandle
setOutput:
  lea rcx,serviceTestOutput
  lea rdx,serviceTestFormat
  mov r8d,serviceTestStage
  mov r9d,serviceTestAssertions
  mov eax,serviceTestExpectedCount
  mov [rsp+20h],rax
  mov eax,serviceTestSmallCount
  mov [rsp+28h],rax
  mov eax,serviceTestSmallPages
  mov [rsp+30h],rax
  mov eax,serviceEnumError
  mov [rsp+38h],rax
  call wsprintfA
  mov [rsp+50h],eax
  mov ecx,-11
  call GetStdHandle
  mov rcx,rax
  lea rdx,serviceTestOutput
  mov r8d,[rsp+50h]
  lea r9,serviceTestWritten
  mov qword ptr [rsp+20h],0
  call WriteFile
  mov ecx,serviceTestStage
  call ExitProcess
ServiceEnumerationTest ENDP

; Equal count plus every unique reference name found detects lost/duplicate
; pages without depending on SCM enumeration order or changing service state.
ServiceTestCompareNames PROC
  push rbx
  push rsi
  push rdi
  push r12
  sub rsp,28h
  mov eax,backCount
  cmp eax,serviceTestExpectedCount
  jne stcnFail
  lea rdi,serviceTestReference
  xor r12d,r12d
stcnReference:
  mov rbx,backBuffer
  xor esi,esi
stcnFind:
  mov rcx,[rdi]
  mov rdx,rbx
  call lstrcmpW
  test eax,eax
  jz stcnFound
  add rbx,4096
  inc esi
  cmp esi,backCount
  jb stcnFind
stcnFail:
  xor eax,eax
  jmp stcnDone
stcnFound:
  add rdi,56
  inc r12d
  cmp r12d,serviceTestExpectedCount
  jb stcnReference
  mov eax,1
stcnDone:
  add rsp,28h
  pop r12
  pop rdi
  pop rsi
  pop rbx
  ret
ServiceTestCompareNames ENDP
