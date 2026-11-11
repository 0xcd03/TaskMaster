; Pure single-worker model; include contract first. Fixed numeric scratch.
.data?
gpMDuplicates db 524288*32 dup(?)
gpMEngineRaw dq GP_ENGINE_CAPACITY dup(?)
gpMProcessRaw dq GP_PROCESS_CAPACITY dup(?)
.const
gpMTpid dw 'p','i','d','_',0
gpMTluid dw 'l','u','i','d','_','0','x',0
gpMTlow dw '_','0','x',0
gpMTphys dw '_','p','h','y','s','_',0
gpMTeng dw '_','e','n','g','_',0
gpMTtype dw '_','e','n','g','t','y','p','e','_',0
gpMHundred real8 100.0
gpMHalf real8 0.5
.code
; Cursor RSI, exclusive NUL end RDI. No helper reads beyond this bound.
GpMToken PROC
gmtLoop:
 movzx eax,word ptr [rdx]
 test eax,eax
 jz gmtOk
 cmp rsi,rdi
 jae gmtBad
 movzx ecx,word ptr [rsi]
 cmp ecx,'A'
 jb gmtCompare
 cmp ecx,'Z'
 ja gmtCompare
 or ecx,20h
gmtCompare:
 cmp ecx,eax
 jne gmtBad
 add rdx,2
 add rsi,2
 jmp gmtLoop
gmtOk:
 xor eax,eax
 ret
gmtBad:
 mov eax,13
 ret
GpMToken ENDP
GpMDecimal PROC
 xor r10d,r10d
 xor r11d,r11d
gmdLoop:
 cmp rsi,rdi
 jae gmdEnd
 movzx eax,word ptr [rsi]
 sub eax,'0'
 cmp eax,9
 ja gmdEnd
 imul r10,r10,10
 add r10,rax
 mov eax,0FFFFFFFFh
 cmp r10,rax
 ja gmdBad
 inc r11d
 add rsi,2
 jmp gmdLoop
gmdEnd:
 test r11d,r11d
 jz gmdBad
 mov eax,r10d
 clc
 ret
gmdBad:
 stc
 ret
GpMDecimal ENDP
GpMHex PROC
 xor r10d,r10d
 mov r11d,8
gmhLoop:
 cmp rsi,rdi
 jae gmhBad
 movzx eax,word ptr [rsi]
 or eax,20h
 sub eax,'0'
 cmp eax,9
 jbe gmhDigit
 sub eax,49
 cmp eax,5
 ja gmhBad
 add eax,10
gmhDigit:
 shl r10d,4
 or r10d,eax
 add rsi,2
 dec r11d
 jnz gmhLoop
 mov eax,r10d
 clc
 ret
gmhBad:
 stc
 ret
GpMHex ENDP
GpParseName PROC
 push rbx
 push rsi
 push rdi
 push r12
 sub rsp,0C8h
 mov rbx,rdx
 mov rsi,rcx
 test rcx,rcx
 jz gpnBad
 test rbx,rbx
 jz gpnBad
 test cl,1
 jnz gpnBad
 cmp r8d,GP_NAME_CAPACITY
 ja gpnBad
 test r8d,r8d
 jz gpnBad
 xor eax,eax
gpnScan:
 cmp eax,r8d
 jae gpnBad
 movzx edx,word ptr [rsi+rax*2]
 test edx,edx
 jz gpnFound
 cmp edx,0D800h
 jb gpnNext
 cmp edx,0DFFFh
 ja gpnNext
 cmp edx,0DBFFh
 ja gpnBad
 inc eax
 cmp eax,r8d
 jae gpnBad
 movzx edx,word ptr [rsi+rax*2]
 sub edx,0DC00h
 cmp edx,3FFh
 ja gpnBad
gpnNext:
 inc eax
 jmp gpnScan
gpnFound:
 lea rdi,[rsi+rax*2]
 lea r12,[rsp+20h]
 mov rdx,rdi
 mov rdi,r12
 xor eax,eax
 mov ecx,GP_PARSED_SIZE/8
 rep stosq
 mov rdi,rdx
 mov dword ptr [r12+GP_N_KIND],3
 cmp rsi,rdi
 jae gpnBad
 movzx eax,word ptr [rsi]
 or eax,20h
 cmp eax,'p'
 jne gpnLuid
 lea rdx,gpMTpid
 call GpMToken
 test eax,eax
 jnz gpnBad
 call GpMDecimal
 jc gpnBad
 mov [r12+GP_N_PID],eax
 cmp rsi,rdi
 jae gpnBad
 cmp word ptr [rsi],'_'
 jne gpnBad
 add rsi,2
 mov dword ptr [r12+GP_N_KIND],2
gpnLuid:
 lea rdx,gpMTluid
 call GpMToken
 test eax,eax
 jnz gpnBad
 call GpMHex
 jc gpnBad
 mov [r12+GP_N_LUID+4],eax
 lea rdx,gpMTlow
 call GpMToken
 test eax,eax
 jnz gpnBad
 call GpMHex
 jc gpnBad
 mov [r12+GP_N_LUID],eax
 lea rdx,gpMTphys
 call GpMToken
 test eax,eax
 jnz gpnBad
 call GpMDecimal
 jc gpnBad
 mov [r12+GP_N_PHYS],eax
 cmp rsi,rdi
 je gpnCommit
 cmp dword ptr [r12+GP_N_KIND],2
 jne gpnBad
 lea rdx,gpMTeng
 call GpMToken
 test eax,eax
 jnz gpnBad
 call GpMDecimal
 jc gpnBad
 mov [r12+GP_N_ENGINE],eax
 lea rdx,gpMTtype
 call GpMToken
 test eax,eax
 jnz gpnBad
 cmp rsi,rdi
 jae gpnBad
 mov dword ptr [r12+GP_N_KIND],1
 mov rdx,rdi
 sub rdx,rsi
 shr rdx,1
 cmp edx,GP_TYPE_CAPACITY-1
 jbe gpnCopyType
 mov edx,GP_TYPE_CAPACITY-1
 mov dword ptr [r12+GP_N_FLAGS],1
 movzx eax,word ptr [rsi+rdx*2-2]
 sub eax,0D800h
 cmp eax,3FFh
 ja gpnCopyType
 dec edx
gpnCopyType:
 xor ecx,ecx
gpnTypeLoop:
 cmp ecx,edx
 jae gpnCommit
 mov ax,[rsi+rcx*2]
 mov [r12+rcx*2+GP_N_TYPE],ax
 inc ecx
 jmp gpnTypeLoop
gpnCommit:
 mov rsi,r12
 mov rdi,rbx
 mov ecx,GP_PARSED_SIZE/8
 rep movsq
 xor eax,eax
 jmp gpnDone
gpnBad:
 mov eax,13
gpnDone:
 add rsp,0C8h
 pop r12
 pop rdi
 pop rsi
 pop rbx
 ret
GpParseName ENDP
GpModelInit PROC
 push rdi
 mov r10,rcx
 mov r11,rdx
 mov rdi,rcx
 xor eax,eax
 mov ecx,GP_SNAPSHOT_SIZE/8
 rep stosq
 mov dword ptr [r10+GP_S_SIZE],GP_SNAPSHOT_SIZE
 mov dword ptr [r10+GP_S_VERSION],GP_VERSION
 mov [r10+GP_S_SEQUENCE],r11
 mov [r10+GP_S_EPOCH],r8
 mov [r10+GP_S_TICK],r9
 lea rdi,gpMEngineRaw
 mov ecx,GP_ENGINE_CAPACITY
 rep stosq
 lea rdi,gpMProcessRaw
 mov ecx,GP_PROCESS_CAPACITY
 rep stosq
 pop rdi
 ret
GpModelInit ENDP
GpFindProcess PROC
 mov eax,edx
 imul eax,eax,-61C8864Fh
 and eax,GP_HASH_CAPACITY-1
 lea r8,[rcx+GP_S_HASH]
 mov r9d,GP_HASH_CAPACITY
gmfLoop:
 mov r10d,[r8+rax*4]
 test r10d,r10d
 jz gmfNone
 dec r10d
 shl r10,8
 lea r10,[rcx+r10+GP_S_PROCESSES]
 cmp [r10+GP_P_PID],edx
 je gmfFound
 inc eax
 and eax,GP_HASH_CAPACITY-1
 dec r9d
 jnz gmfLoop
gmfNone:
 xor eax,eax
 ret
gmfFound:
 mov rax,r10
 ret
GpFindProcess ENDP
GpMProcess PROC
 sub rsp,28h
 call GpFindProcess
 add rsp,28h
 test rax,rax
 jnz gmpDone
 mov r11d,[rcx+GP_S_PROCESS_COUNT]
 cmp r11d,GP_PROCESS_CAPACITY
 jae gmpDone
 mov eax,edx
 imul eax,eax,-61C8864Fh
 and eax,GP_HASH_CAPACITY-1
gmpSlot:
 cmp dword ptr [rcx+rax*4+GP_S_HASH],0
 je gmpInsert
 inc eax
 and eax,GP_HASH_CAPACITY-1
 jmp gmpSlot
gmpInsert:
 inc r11d
 mov [rcx+rax*4+GP_S_HASH],r11d
 mov [rcx+GP_S_PROCESS_COUNT],r11d
 dec r11d
 shl r11,8
 lea rax,[rcx+r11+GP_S_PROCESSES]
 mov [rax+GP_P_PID],edx
gmpDone:
 ret
GpMProcess ENDP
GpMAdapter PROC
 lea rax,[rcx+GP_S_ADAPTERS]
 mov r8d,[rcx+GP_S_ADAPTER_COUNT]
 mov r9d,r8d
gmaLoop:
 test r8d,r8d
 jz gmaNew
 mov r10,[rdx+GP_N_LUID]
 cmp [rax+GP_A_LUID],r10
 jne gmaNext
 mov r10d,[rdx+GP_N_PHYS]
 cmp [rax+GP_A_PHYS],r10d
 je gmaDone
gmaNext:
 add rax,GP_ADAPTER_SIZE
 dec r8d
 jmp gmaLoop
gmaNew:
 cmp r9d,GP_ADAPTER_CAPACITY
 jae gmaFail
 inc dword ptr [rcx+GP_S_ADAPTER_COUNT]
 mov r10,[rdx+GP_N_LUID]
 mov [rax+GP_A_LUID],r10
 mov r10d,[rdx+GP_N_PHYS]
 mov [rax+GP_A_PHYS],r10d
gmaDone:
 ret
gmaFail:
 xor eax,eax
 ret
GpMAdapter ENDP
GpMEngine PROC
 lea rax,[rcx+GP_S_ENGINES]
 mov r8d,[rcx+GP_S_ENGINE_COUNT]
 mov r9d,r8d
gmeLoop:
 test r8d,r8d
 jz gmeNew
 mov r10,[rdx+GP_N_LUID]
 cmp [rax+GP_E_LUID],r10
 jne gmeNext
 mov r10,[rdx+GP_N_PHYS]
 cmp [rax+GP_E_PHYS],r10
 je gmeDone
gmeNext:
 add rax,GP_ENGINE_SIZE
 dec r8d
 jmp gmeLoop
gmeNew:
 cmp r9d,GP_ENGINE_CAPACITY
 jae gmeFail
 inc dword ptr [rcx+GP_S_ENGINE_COUNT]
 mov r10,[rdx+GP_N_LUID]
 mov [rax+GP_E_LUID],r10
 mov r10,[rdx+GP_N_PHYS]
 mov [rax+GP_E_PHYS],r10
 push rsi
 push rdi
 mov r8,rax
 lea rsi,[rdx+GP_N_TYPE]
 lea rdi,[rax+GP_E_TYPE]
 mov ecx,GP_TYPE_CAPACITY/4
 rep movsq
 mov rax,r8
 pop rdi
 pop rsi
 mov r10d,[rdx+GP_N_FLAGS]
 mov [rax+GP_E_FLAGS],r10d
gmeDone:
 ret
gmeFail:
 xor eax,eax
 ret
GpMEngine ENDP
; Table has >maximum count slots, so bounded input cannot exhaust probing.
GpMDuplicate PROC
 mov r8,[rcx+GP_N_LUID]
 mov r9,[rcx+GP_N_PHYS]
 mov edx,[rcx+GP_N_PID]
 mov rax,r8
 xor rax,r9
 xor rax,rdx
 mov r10,rax
 shr r10,32
 xor eax,r10d
 imul eax,eax,-61C8864Fh
 and eax,524287
 lea r11,gpMDuplicates
gmdpLoop:
 mov r10d,eax
 shl r10,5
 add r10,r11
 cmp dword ptr [r10+24],0
 je gmdpNew
 cmp [r10],r8
 jne gmdpNext
 cmp [r10+8],r9
 jne gmdpNext
 cmp [r10+16],edx
 je gmdpBad
gmdpNext:
 inc eax
 and eax,524287
 jmp gmdpLoop
gmdpNew:
 mov [r10],r8
 mov [r10+8],r9
 mov [r10+16],edx
 mov dword ptr [r10+24],1
 xor eax,eax
 ret
gmdpBad:
 mov eax,13
 ret
GpMDuplicate ENDP
GpMLower PROC
 mov rax,[rcx+GP_N_LUID]
 cmp rax,[rdx+GP_P_LUID]
 jne gmlDone
 mov eax,[rcx+GP_N_PHYS]
 cmp eax,[rdx+GP_P_PHYS]
 jne gmlDone
 mov eax,[rcx+GP_N_ENGINE]
 cmp eax,[rdx+GP_P_ENGINE]
gmlDone:
 ret
GpMLower ENDP
GpModelSourceError PROC
 cmp edx,GP_SOURCES
 jae gmseDone
 mov [rcx+rdx*8+GP_S_SOURCES],r8d
 mov [rcx+rdx*8+GP_S_SOURCES+4],r9d
gmseDone:
 ret
GpModelSourceError ENDP
GpModelArray PROC
 push rbx
 push rbp
 push rsi
 push rdi
 push r12
 push r13
 push r14
 push r15
 sub rsp,0F8h
 mov rbx,rcx
 mov ebp,edx
 mov r12,r8
 mov r9d,r9d ; ABI specifies usedBytes as DWORD; ignore upper register bits.
 mov r14d,[rsp+160h]
 cmp ebp,GP_SOURCES
 jae gmarBadNoSource
 mov dword ptr [rbx+rbp*8+GP_S_SOURCES],GP_STATUS_PARTIAL
 mov dword ptr [rbx+rbp*8+GP_S_SOURCES+4],13
 cmp r9d,GP_ARRAY_BUDGET
 ja gmarBad
 mov eax,r14d
 imul rax,rax,24
 cmp rax,r9
 ja gmarBad
 test r14d,r14d
 jz gmarEmpty
 test r12,r12
 jz gmarBad
 test r12b,7
 jnz gmarBad
 mov r13,r12
 add r13,r9
 jc gmarBad
 lea rdi,gpMDuplicates
 xor eax,eax
 mov ecx,524288*4
 rep stosq
 mov r15,r12
gmarLoop:
 mov eax,[r15+8]
 cmp eax,1
 ja gmarBad
 mov rcx,[r15]
 test cl,1
 jnz gmarBad
 cmp rcx,r12
 jb gmarBad
 cmp rcx,r13
 jae gmarBad
 mov r8,r13
 sub r8,rcx
 shr r8,1
 cmp r8,GP_NAME_CAPACITY
 jbe gmarBound
 mov r8d,GP_NAME_CAPACITY
gmarBound:
 lea rdx,[rsp+40h]
 call GpParseName
 test eax,eax
 jnz gmarBad
 mov eax,2
 test ebp,ebp
 jnz gmarKindMem
 dec eax
 jmp gmarKind
gmarKindMem:
 cmp ebp,3
 jb gmarKind
 inc eax
gmarKind:
 cmp [rsp+40h+GP_N_KIND],eax
 jne gmarBad
 mov rax,[r15+16]
 test ebp,ebp
 jnz gmarLarge
 mov rdx,rax
 shr rdx,52
 and edx,7FFh
 cmp edx,7FFh
 je gmarBad
 test rax,rax
 jns gmarValueOk
 shl rax,1
 jnz gmarBad
 jmp gmarValueOk
gmarLarge:
 test rax,rax
 js gmarBad
gmarValueOk:
 lea rcx,[rsp+40h]
 call GpMDuplicate
 test eax,eax
 jnz gmarBad
 mov rcx,rbx
 lea rdx,[rsp+40h]
 call GpMAdapter
 test rax,rax
 jz gmarBad
 mov rsi,rax
 mov eax,1
 mov ecx,ebp
 shl eax,cl
 or [rsi+356],eax
 cmp ebp,3
 jae gmarAdapterMemory
 mov edx,[rsp+40h+GP_N_PID]
 mov rcx,rbx
 call GpMProcess
 test rax,rax
 jz gmarBad
 mov rdi,rax
 mov eax,1
 mov ecx,ebp
 shl eax,cl
 or [rdi+GP_P_COUNTERS],eax
 test ebp,ebp
 jz gmarEngine
 mov edx,GP_P_DEDICATED
 cmp ebp,1
 je gmarProcessMemory
 mov edx,GP_P_SHARED
gmarProcessMemory:
 mov rax,[r15+16]
 add [rdi+rdx],rax
 jc gmarBad
 jmp gmarNext
gmarAdapterMemory:
 mov edx,GP_A_DEDICATED
 mov ecx,GP_S_DEDICATED
 cmp ebp,3
 je gmarStoreAdapter
 mov edx,GP_A_SHARED
 mov ecx,GP_S_SHARED
gmarStoreAdapter:
 mov rax,[r15+16]
 mov [rsi+rdx],rax
 add [rbx+rcx],rax
 jc gmarBad
 jmp gmarNext
gmarEngine:
 mov rcx,rbx
 lea rdx,[rsp+40h]
 call GpMEngine
 test rax,rax
 jz gmarBad
 inc dword ptr [rax+GP_E_PROCESS_COUNT]
 mov rdx,rax
 sub rdx,rbx
 sub rdx,GP_S_ENGINES
 mov rax,rdx
 xor edx,edx
 mov ecx,GP_ENGINE_SIZE
 div rcx
 lea rdx,gpMEngineRaw
 movsd xmm0,qword ptr [r15+16]
 addsd xmm0,qword ptr [rdx+rax*8]
 movq rcx,xmm0
 shr rcx,52
 and ecx,7FFh
 cmp ecx,7FFh
 je gmarBad
 movsd qword ptr [rdx+rax*8],xmm0
 mov rax,rdi
 sub rax,rbx
 sub rax,GP_S_PROCESSES
 shr rax,8
 mov [rsp+28h],rax
 lea rdx,gpMProcessRaw
 movsd xmm0,qword ptr [r15+16]
 ucomisd xmm0,qword ptr [rdx+rax*8]
 ja gmarWinner
 jb gmarNext
 cmp dword ptr [rdi+GP_P_UTIL_FLAGS],0
 je gmarWinner
 lea rcx,[rsp+40h]
 mov rdx,rdi
 call GpMLower
 jnc gmarNext
gmarWinner:
 mov rax,[rsp+28h]
 lea rdx,gpMProcessRaw
 movsd xmm0,qword ptr [r15+16]
 movsd qword ptr [rdx+rax*8],xmm0
 mov dword ptr [rdi+GP_P_UTIL_FLAGS],1
 mov rax,[rsp+40h+GP_N_LUID]
 mov [rdi+GP_P_LUID],rax
 mov rax,[rsp+40h+GP_N_PHYS]
 mov [rdi+GP_P_PHYS],rax
 push rsi
 lea rsi,[rsp+48h+GP_N_TYPE]
 add rdi,GP_P_TYPE
 mov ecx,GP_TYPE_CAPACITY/4
 rep movsq
 pop rsi
gmarNext:
 add r15,24
 dec r14d
 jnz gmarLoop
 mov dword ptr [rbx+rbp*8+GP_S_SOURCES],GP_STATUS_VALID
 mov dword ptr [rbx+rbp*8+GP_S_SOURCES+4],0
 xor eax,eax
 jmp gmarDone
gmarEmpty:
 mov dword ptr [rbx+rbp*8+GP_S_SOURCES],GP_STATUS_UNAVAILABLE
 mov dword ptr [rbx+rbp*8+GP_S_SOURCES+4],21
 mov eax,21
 jmp gmarDone
gmarBad:
gmarBadNoSource:
 mov eax,13
gmarDone:
 add rsp,0F8h
 pop r15
 pop r14
 pop r13
 pop r12
 pop rdi
 pop rsi
 pop rbp
 pop rbx
 ret
GpModelArray ENDP
GpMBasis PROC
 ucomisd xmm0,gpMHundred
 jae gmbClamp
 mulsd xmm0,gpMHundred
 addsd xmm0,gpMHalf
 cvttsd2si eax,xmm0
 ret
gmbClamp:
 mov eax,10000
 ret
GpMBasis ENDP
; RCX record, EDX status offset, R8 source pair, R9D observed bit.
GpMField PROC
 mov eax,[r8]
 mov r10d,[r8+4]
 cmp eax,GP_STATUS_VALID
 jne gmfiStore
 test r9d,r9d
 jnz gmfiStore
 xor eax,eax
 xor r10d,r10d
gmfiStore:
 mov [rcx+rdx],eax
 mov [rcx+rdx+4],r10d
 ret
GpMField ENDP
GpModelFinalize PROC
 push rbx
 push rsi
 push rdi
 push r12
 push r13
 sub rsp,30h
 mov rbx,rcx
 mov dword ptr [rbx+GP_S_UTIL],0
 lea r8,[rbx+GP_S_SOURCES]
 mov edx,GP_S_UTIL_STATUS
 mov r9d,1
 call GpMField
 mov rcx,rbx
 lea r8,[rbx+GP_S_SOURCES+24]
 mov edx,GP_S_DEDICATED_STATUS
 call GpMField
 mov rcx,rbx
 lea r8,[rbx+GP_S_SOURCES+32]
 mov edx,GP_S_SHARED_STATUS
 call GpMField
 lea rsi,[rbx+GP_S_PROCESSES]
 xor r12d,r12d
gmfinProcess:
 cmp r12d,[rbx+GP_S_PROCESS_COUNT]
 jae gmfinAdapters
 mov rcx,rsi
 mov edx,GP_P_UTIL_STATUS
 lea r8,[rbx+GP_S_SOURCES]
 mov r9d,[rsi+GP_P_COUNTERS]
 and r9d,1
 call GpMField
 mov rcx,rsi
 mov edx,GP_P_DEDICATED_STATUS
 lea r8,[rbx+GP_S_SOURCES+8]
 mov r9d,[rsi+GP_P_COUNTERS]
 and r9d,2
 call GpMField
 mov rcx,rsi
 mov edx,GP_P_SHARED_STATUS
 lea r8,[rbx+GP_S_SOURCES+16]
 mov r9d,[rsi+GP_P_COUNTERS]
 and r9d,4
 call GpMField
 cmp dword ptr [rsi+GP_P_UTIL_STATUS],GP_STATUS_VALID
 jne gmfinProcessNext
 lea rax,gpMProcessRaw
 movsd xmm0,qword ptr [rax+r12*8]
 call GpMBasis
 mov [rsi+GP_P_UTIL],eax
gmfinProcessNext:
 inc r12d
 add rsi,GP_PROCESS_SIZE
 jmp gmfinProcess
gmfinAdapters:
 lea rsi,[rbx+GP_S_ADAPTERS]
 xor r12d,r12d
gmfinAdapter:
 cmp r12d,[rbx+GP_S_ADAPTER_COUNT]
 jae gmfinEngines
 mov dword ptr [rsi+GP_A_UTIL],0
 mov dword ptr [rsi+GP_A_ENGINE_COUNT],0
 mov rcx,rsi
 mov edx,GP_A_UTIL_STATUS
 lea r8,[rbx+GP_S_SOURCES]
 mov r9d,[rsi+356]
 and r9d,1
 call GpMField
 mov rcx,rsi
 mov edx,GP_A_DEDICATED_STATUS
 lea r8,[rbx+GP_S_SOURCES+24]
 mov r9d,[rsi+356]
 and r9d,8
 call GpMField
 mov rcx,rsi
 mov edx,GP_A_SHARED_STATUS
 lea r8,[rbx+GP_S_SOURCES+32]
 mov r9d,[rsi+356]
 and r9d,16
 call GpMField
 inc r12d
 add rsi,GP_ADAPTER_SIZE
 jmp gmfinAdapter
gmfinEngines:
 lea rsi,[rbx+GP_S_ENGINES]
 xor r12d,r12d
gmfinEngine:
 cmp r12d,[rbx+GP_S_ENGINE_COUNT]
 jae gmfinDone
 mov eax,[rbx+GP_S_SOURCES]
 mov [rsi+GP_E_STATUS],eax
 mov eax,[rbx+GP_S_SOURCES+4]
 mov [rsi+GP_E_ERROR],eax
 cmp dword ptr [rsi+GP_E_STATUS],GP_STATUS_VALID
 jne gmfinEngineNext
 lea rax,gpMEngineRaw
 movsd xmm0,qword ptr [rax+r12*8]
 call GpMBasis
 mov [rsi+GP_E_UTIL],eax
 cmp eax,[rbx+GP_S_UTIL]
 jbe gmfinAdapterValue
 mov [rbx+GP_S_UTIL],eax
gmfinAdapterValue:
 lea rdi,[rbx+GP_S_ADAPTERS]
 mov ecx,[rbx+GP_S_ADAPTER_COUNT]
gmfinAdapterFind:
 test ecx,ecx
 jz gmfinEngineNext
 mov rax,[rsi+GP_E_LUID]
 cmp [rdi+GP_A_LUID],rax
 jne gmfinAdapterFindNext
 mov eax,[rsi+GP_E_PHYS]
 cmp [rdi+GP_A_PHYS],eax
 jne gmfinAdapterFindNext
 inc dword ptr [rdi+GP_A_ENGINE_COUNT]
 mov eax,[rsi+GP_E_UTIL]
 cmp eax,[rdi+GP_A_UTIL]
 jbe gmfinEngineNext
 mov [rdi+GP_A_UTIL],eax
 jmp gmfinEngineNext
gmfinAdapterFindNext:
 add rdi,GP_ADAPTER_SIZE
 dec ecx
 jmp gmfinAdapterFind
gmfinEngineNext:
 inc r12d
 add rsi,GP_ENGINE_SIZE
 jmp gmfinEngine
gmfinDone:
 add rsp,30h
 pop r13
 pop r12
 pop rdi
 pop rsi
 pop rbx
 ret
GpModelFinalize ENDP
