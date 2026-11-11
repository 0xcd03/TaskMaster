; Deterministic native model fixtures. Include contract+model first.
; GpModelTest returns zero or a stable failing stage (no OS dependencies).
.data?
gmtFrame db GP_SNAPSHOT_SIZE dup(?)
gmtParsed db GP_PARSED_SIZE dup(?)
.data
gmtStage dd 0
ALIGN 8
gmtItems LABEL BYTE
 dq 0
 dd 0,0
 real8 20.004
 dq 0
 dd 1,0
 real8 30.004
 dq 0
 dd 0,0
 real8 40.0
gmtnEngine LABEL WORD
 dw 112,105,100,95,55,95,108,117,105,100,95,48,120,48,48,48
 dw 48,48,48,48,48,95,48,120,48,48,48,48,48,48,48,49
 dw 95,112,104,121,115,95,48,95,101,110,103,95,50,95,101,110
 dw 103,116,121,112,101,95,51,68,0
gmtnSecond LABEL WORD
 dw 112,105,100,95,56,95,108,117,105,100,95,48,120,48,48,48
 dw 48,48,48,48,48,95,48,120,48,48,48,48,48,48,48,49
 dw 95,112,104,121,115,95,48,95,101,110,103,95,50,95,101,110
 dw 103,116,121,112,101,95,51,68,0
gmtnOther LABEL WORD
 dw 112,105,100,95,55,95,108,117,105,100,95,48,120,48,48,48
 dw 48,48,48,48,48,95,48,120,48,48,48,48,48,48,48,49
 dw 95,112,104,121,115,95,48,95,101,110,103,95,49,95,101,110
 dw 103,116,121,112,101,95,67,111,112,121,0
gmtnMemory LABEL WORD
 dw 112,105,100,95,55,95,108,117,105,100,95,48,120,48,48,48
 dw 48,48,48,48,48,95,48,120,48,48,48,48,48,48,48,49
 dw 95,112,104,121,115,95,48,0
gmtnAdapter LABEL WORD
 dw 108,117,105,100,95,48,120,48,48,48,48,48,48,48,48,95
 dw 48,120,48,48,48,48,48,48,48,49,95,112,104,121,115,95
 dw 48,0
gmtnBad LABEL WORD
 dw 112,105,100,95,52,50,57,52,57,54,55,50,57,54,95,108
 dw 117,105,100,95,48,120,48,48,48,48,48,48,48,48,95,48
 dw 120,48,48,48,48,48,48,48,49,95,112,104,121,115,95,48
 dw 0
gmtItemsEnd LABEL BYTE
.code
GMT_EQ MACRO actual,expected
 cmp actual,expected
 jne gmtFail
ENDM
GmtInit PROC
 lea rcx,gmtFrame
 mov edx,11
 mov r8d,22
 mov r9d,33
 jmp GpModelInit
GmtInit ENDP
GmtArray PROC
 ; EDX source, R9D count; fixture names remain within supplied buffer.
 lea rcx,gmtFrame
 lea r8,gmtItems
 mov eax,r9d
 mov r9d,gmtItemsEnd-gmtItems
 sub rsp,28h
 mov [rsp+20h],eax
 call GpModelArray
 add rsp,28h
 ret
GmtArray ENDP
GmtFinalize PROC
 lea rcx,gmtFrame
 jmp GpModelFinalize
GmtFinalize ENDP
GmtFind PROC
 lea rcx,gmtFrame
 mov edx,7
 jmp GpFindProcess
GmtFind ENDP
GpModelTest PROC
 push rbx
 push rsi
 push rdi
 sub rsp,20h
 mov gmtStage,1
 lea rcx,gmtnEngine
 lea rdx,gmtParsed
 mov r8d,GP_NAME_CAPACITY
 call GpParseName
 GMT_EQ eax,0
 GMT_EQ dword ptr [gmtParsed+GP_N_PID],7
 GMT_EQ dword ptr [gmtParsed+GP_N_KIND],1
 GMT_EQ qword ptr [gmtParsed+GP_N_LUID],1
 GMT_EQ dword ptr [gmtParsed+GP_N_ENGINE],2
 mov gmtStage,2
 lea rdi,gmtParsed
 mov eax,0A5A5A5A5h
 mov ecx,GP_PARSED_SIZE/4
 rep stosd
 lea rcx,gmtnBad
 lea rdx,gmtParsed
 mov r8d,GP_NAME_CAPACITY
 call GpParseName
 GMT_EQ eax,13
 GMT_EQ dword ptr [gmtParsed],0A5A5A5A5h
 GMT_EQ dword ptr [gmtParsed+GP_PARSED_SIZE-4],0A5A5A5A5h
 mov gmtStage,3
 call GmtInit
 GMT_EQ dword ptr [gmtFrame+GP_S_SIZE],GP_SNAPSHOT_SIZE
 GMT_EQ qword ptr [gmtFrame+GP_S_SEQUENCE],11
 GMT_EQ qword ptr [gmtFrame+GP_S_EPOCH],22
 GMT_EQ qword ptr [gmtFrame+GP_S_TICK],33
 lea rax,gmtnEngine
 mov qword ptr [gmtItems],rax
 lea rax,gmtnSecond
 mov qword ptr [gmtItems+24],rax
 lea rax,gmtnOther
 mov qword ptr [gmtItems+48],rax
 xor edx,edx
 mov r9d,3
 call GmtArray
 GMT_EQ eax,0
 call GmtFinalize
 GMT_EQ dword ptr [gmtFrame+GP_S_UTIL],5001
 GMT_EQ dword ptr [gmtFrame+GP_S_UTIL_STATUS],GP_STATUS_VALID
 GMT_EQ dword ptr [gmtFrame+GP_S_ENGINE_COUNT],2
 GMT_EQ dword ptr [gmtFrame+GP_S_ADAPTERS+GP_A_UTIL],5001
 call GmtFind
 test rax,rax
 jz gmtFail
 GMT_EQ dword ptr [rax+GP_P_UTIL],4000
 GMT_EQ dword ptr [rax+GP_P_ENGINE],1
 GMT_EQ qword ptr [rax+GP_P_BIRTH],0
 GMT_EQ dword ptr [rax+GP_P_DEDICATED_STATUS],GP_STATUS_MISSING
 ; Repeated finalization must not accumulate adapter engine counts.
 call GmtFinalize
 GMT_EQ dword ptr [gmtFrame+GP_S_ADAPTERS+GP_A_ENGINE_COUNT],2
 mov gmtStage,4
 call GmtInit
 lea rax,gmtnEngine
 mov qword ptr [gmtItems],rax
 mov qword ptr [gmtItems+24],rax
 xor edx,edx
 mov r9d,2
 call GmtArray
 GMT_EQ eax,13
 ; A failed engine lane cannot invalidate independent process memory.
 lea rax,gmtnMemory
 mov qword ptr [gmtItems],rax
 mov qword ptr [gmtItems+16],123
 mov edx,1
 mov r9d,1
 call GmtArray
 GMT_EQ eax,0
 call GmtFinalize
 GMT_EQ dword ptr [gmtFrame+GP_S_UTIL_STATUS],GP_STATUS_PARTIAL
 call GmtFind
 test rax,rax
 jz gmtFail
 GMT_EQ dword ptr [rax+GP_P_UTIL_STATUS],GP_STATUS_PARTIAL
 GMT_EQ qword ptr [rax+GP_P_DEDICATED],123
 GMT_EQ dword ptr [rax+GP_P_DEDICATED_STATUS],GP_STATUS_VALID
 mov gmtStage,5
 call GmtInit
 xor edx,edx
 xor r9d,r9d
 call GmtArray
 GMT_EQ eax,21
 call GmtFinalize
 GMT_EQ dword ptr [gmtFrame+GP_S_UTIL_STATUS],GP_STATUS_UNAVAILABLE
 mov gmtStage,6
 call GmtInit
 lea rax,gmtnEngine
 mov qword ptr [gmtItems],rax
 mov qword ptr [gmtItems+16],0
 xor edx,edx
 mov r9d,1
 call GmtArray
 GMT_EQ eax,0
 call GmtFinalize
 GMT_EQ dword ptr [gmtFrame+GP_S_UTIL],0
 GMT_EQ dword ptr [gmtFrame+GP_S_UTIL_STATUS],GP_STATUS_VALID
 call GmtFind
 GMT_EQ dword ptr [rax+GP_P_UTIL],0
 GMT_EQ dword ptr [rax+GP_P_UTIL_STATUS],GP_STATUS_VALID
 xor eax,eax
 jmp gmtDone
gmtFail:
 mov eax,gmtStage
gmtDone:
 add rsp,20h
 pop rdi
 pop rsi
 pop rbx
 ret
GpModelTest ENDP
