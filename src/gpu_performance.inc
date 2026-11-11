; Collector-owned scratch, no provider or UI calls/frame pointers.
.data
IFDEF GP_TESTS
gpPerfCopy dq GpWorkerCopyAdapters
ENDIF
gpPerfIndex dd -1
gpPerfEmpty dw 0
gpPerfMiB dw 'M','i','B',0
gpPerfPercent dw '%',0
gpPerfMs dw 'm','s',0
gpPerfLabelFormat dw 'G','P','U',' ','%','u',':',' ','%','s',0
gpPerfLuidFormat dw '%','0','8','X',':','%','0','8','X',0
gpPerfIdentityFormat dw '%','0','8','X',':','%','0','8','X',' ','/',' ','%','u',0
gpPerfUnknownFormat dw '%','0','8','X',':','%','0','8','X',' ','/',' ','-',0
.data?
align 8
gpPerfSnapshot db GP_ADAPTER_COPY_SIZE dup(?)
gpPerfCaption dw 200 dup(?)
gpPerfLabel dw 256 dup(?)
gpPerfValue dw 256 dup(?)
gpPerfStatusFormat dw 128 dup(?)
.code
; RCX label,RDX value,R8 unit; bounded native cells, 8192 rows maximum.
GpPerfAppend PROC
 push rbx
 push rsi
 push rdi
 sub rsp,20h
 mov rbx,rcx
 mov rsi,rdx
 mov rdi,r8
 mov eax,backCount
 cmp eax,8192
 jae gppaDone
 cmp backBuffer,0
 je gppaDone
 shl rax,12
 add rax,backBuffer
 mov rcx,rax
 mov rdx,rbx
 mov rbx,rax
 mov r8d,256
 call lstrcpynW
 lea rcx,[rbx+512]
 mov rdx,rsi
 mov r8d,256
 call lstrcpynW
 lea rcx,[rbx+1024]
 mov rdx,rdi
 mov r8d,256
 call lstrcpynW
 inc backCount
gppaDone:
 add rsp,20h
 pop rdi
 pop rsi
 pop rbx
 ret
GpPerfAppend ENDP
; ECX localized resource caption,RDX value,R8 unit.
GpPerfRow PROC
 push rbx
 push rsi
 sub rsp,28h
 mov rbx,rdx
 mov rsi,r8
 add ecx,appLanguage
 mov edx,ecx
 mov rcx,gInst
 lea r8,gpPerfCaption
 mov r9d,200
 mov word ptr [r8],0
 call LoadStringW
 lea rcx,gpPerfLabel
 lea rdx,gpPerfCaption
 cmp gpPerfIndex,-1
 je gpprUnprefixed
 lea rdx,gpPerfLabelFormat
 mov r8d,gpPerfIndex
 lea r9,gpPerfCaption
 call wsprintfW
 jmp gpprAppend
gpprUnprefixed:
 mov r8d,256
 call lstrcpynW
gpprAppend:
 lea rcx,gpPerfLabel
 mov rdx,rbx
 mov r8,rsi
 call GpPerfAppend
 add rsp,28h
 pop rsi
 pop rbx
 ret
GpPerfRow ENDP
; EDX status,R8D error -> RAX collector-owned localized value.
GpPerfStatus PROC
 sub rsp,38h
 mov [rsp+20h],r8d
 mov ecx,52402
 cmp edx,GP_STATUS_WARMUP
 jne gppsUnavailable
 mov ecx,52400
 jmp gppsLoad
gppsUnavailable:
 cmp edx,GP_STATUS_UNAVAILABLE
 jne gppsPartial
 mov ecx,52404
 jmp gppsLoad
gppsPartial:
 cmp edx,GP_STATUS_PARTIAL
 jne gppsStale
 mov ecx,52406
 jmp gppsLoad
gppsStale:
 cmp edx,GP_STATUS_STALE
 jne gppsLoad
 mov ecx,52408
gppsLoad:
 add ecx,appLanguage
 mov edx,ecx
 mov rcx,gInst
 lea r8,gpPerfStatusFormat
 mov r9d,128
 mov word ptr [r8],0
 call LoadStringW
 test eax,eax
 jnz gppsFormat
 mov word ptr [gpPerfStatusFormat],'?'
 mov word ptr [gpPerfStatusFormat+2],0
gppsFormat:
 lea rcx,gpPerfValue
 lea rdx,gpPerfStatusFormat
 mov r8d,[rsp+20h]
 call wsprintfW
 lea rax,gpPerfValue
 add rsp,38h
 ret
GpPerfStatus ENDP
; RCX bytes,EDX status,R8D error,R9D caption. Usage != capacity.
GpPerfMemory PROC
 push rbx
 sub rsp,20h
 mov ebx,r9d
 cmp edx,GP_STATUS_VALID
 jne gppmStatus
 lea rdx,gpPerfValue
 mov r8d,1048576
 call StoreScaledMetric
 jmp gppmRow
gppmStatus:
 call GpPerfStatus
gppmRow:
 mov ecx,ebx
 lea rdx,gpPerfValue
 lea r8,gpPerfMiB
 call GpPerfRow
 add rsp,20h
 pop rbx
 ret
GpPerfMemory ENDP
GpCollectPerformance PROC
 push rbx
 push rsi
 push rdi
 sub rsp,30h
 lea rcx,gpPerfSnapshot
IFDEF GP_TESTS
 call gpPerfCopy
ELSE
 call GpWorkerCopyAdapters
ENDIF
 mov edi,eax
 mov gpPerfIndex,-1
 mov rcx,gInst
 mov edx,52410
 add edx,appLanguage
 lea r8,gpPerfValue
 mov r9d,256
 mov word ptr [r8],0
 call LoadStringW
 mov ecx,52422
 lea rdx,gpPerfValue
 lea r8,gpPerfEmpty
 call GpPerfRow
 test edi,edi
 jz gpcpCapture
 ; Worker timeout is a successful bounded copy of stale usage plus retained
 ; adapter metadata, not a failed/empty mailbox. Other failures remain errors.
 cmp edi,1460
 jne gpcpFailure
 cmp dword ptr [gpPerfSnapshot+GP_S_UTIL_STATUS],GP_STATUS_STALE
 jne gpcpFailure
gpcpCapture:
 mov rcx,qword ptr [gpPerfSnapshot+GP_S_SEQUENCE]
 lea rdx,gpPerfValue
 call StoreDecimal
 mov ecx,52424
 lea rdx,gpPerfValue
 lea r8,gpPerfEmpty
 call GpPerfRow
 mov rcx,qword ptr [gpPerfSnapshot+GP_S_INTERVAL_MS]
 lea rdx,gpPerfValue
 call StoreDecimal
 mov ecx,52426
 lea rdx,gpPerfValue
 lea r8,gpPerfMs
 call GpPerfRow
 mov esi,dword ptr [gpPerfSnapshot+GP_S_ADAPTER_COUNT]
 test esi,esi
 jz gpcpNoAdapters
 cmp esi,GP_ADAPTER_CAPACITY
 jbe gpcpStart
 mov esi,GP_ADAPTER_CAPACITY
 mov edx,GP_STATUS_PARTIAL
 mov r8d,13
 call GpPerfStatus
 mov ecx,52434
 mov rdx,rax
 lea r8,gpPerfEmpty
 call GpPerfRow
gpcpStart:
 xor edi,edi
 lea rbx,gpPerfSnapshot+GP_HEADER_SIZE
gpcpAdapter:
 mov gpPerfIndex,edi
 test dword ptr [rbx+GP_A_FLAGS],1
 jz gpcpLuidName
 cmp word ptr [rbx+GP_A_DESCRIPTION],0
 je gpcpLuidName
 lea rcx,gpPerfValue
 lea rdx,[rbx+GP_A_DESCRIPTION]
 mov r8d,128
 call lstrcpynW
 jmp gpcpNameRow
gpcpLuidName:
 lea rcx,gpPerfValue
 lea rdx,gpPerfLuidFormat
 mov r8d,[rbx+GP_A_LUID+4]
 mov r9d,[rbx+GP_A_LUID]
 call wsprintfW
gpcpNameRow:
 mov ecx,52428
 lea rdx,gpPerfValue
 lea r8,gpPerfEmpty
 call GpPerfRow
 lea rcx,gpPerfValue
 lea rdx,gpPerfIdentityFormat
 mov eax,[rbx+GP_A_PHYS]
 mov [rsp+20h],rax
 cmp eax,-1
 jne gpcpIdentity
 lea rdx,gpPerfUnknownFormat
gpcpIdentity:
 mov r8d,[rbx+GP_A_LUID+4]
 mov r9d,[rbx+GP_A_LUID]
 call wsprintfW
 mov ecx,52430
 lea rdx,gpPerfValue
 lea r8,gpPerfEmpty
 call GpPerfRow
 mov edx,[rbx+GP_A_UTIL_STATUS]
 mov r8d,[rbx+GP_A_UTIL_ERROR]
 cmp edx,GP_STATUS_VALID
 jne gpcpUtilStatus
 mov ecx,[rbx+GP_A_UTIL]
 lea rdx,gpPerfValue
 mov r8d,100
 call StoreScaledMetric
 jmp gpcpUtilRow
gpcpUtilStatus:
 call GpPerfStatus
gpcpUtilRow:
 mov ecx,52432
 lea rdx,gpPerfValue
 lea r8,gpPerfPercent
 call GpPerfRow
 mov rcx,[rbx+GP_A_DEDICATED]
 mov edx,[rbx+GP_A_DEDICATED_STATUS]
 mov r8d,[rbx+GP_A_DEDICATED_ERROR]
 mov r9d,52412
 call GpPerfMemory
 mov rcx,[rbx+GP_A_SHARED]
 mov edx,[rbx+GP_A_SHARED_STATUS]
 mov r8d,[rbx+GP_A_SHARED_ERROR]
 mov r9d,52414
 call GpPerfMemory
 mov edx,[rbx+GP_A_METADATA_STATUS]
 test dword ptr [rbx+GP_A_FLAGS],1
 jnz gpcpDedicatedCapacity
 mov edx,GP_STATUS_MISSING
gpcpDedicatedCapacity:
 mov rcx,[rbx+GP_A_DEDICATED_CAPACITY]
 mov r8d,[rbx+GP_A_METADATA_ERROR]
 mov r9d,52416
 call GpPerfMemory
 mov edx,[rbx+GP_A_METADATA_STATUS]
 test dword ptr [rbx+GP_A_FLAGS],1
 jnz gpcpSharedCapacity
 mov edx,GP_STATUS_MISSING
gpcpSharedCapacity:
 mov rcx,[rbx+GP_A_SHARED_CAPACITY]
 mov r8d,[rbx+GP_A_METADATA_ERROR]
 mov r9d,52418
 call GpPerfMemory
 test dword ptr [rbx+GP_A_FLAGS],2
 jz gpcpNext
 mov rcx,gInst
 mov edx,52420
 add edx,appLanguage
 lea r8,gpPerfValue
 mov r9d,256
 call LoadStringW
 mov ecx,52436
 lea rdx,gpPerfValue
 lea r8,gpPerfEmpty
 call GpPerfRow
gpcpNext:
 add rbx,GP_ADAPTER_SIZE
 inc edi
 cmp edi,esi
 jb gpcpAdapter
 jmp gpcpDone
gpcpFailure:
 mov edx,GP_STATUS_UNAVAILABLE
 mov r8d,edi
 jmp gpcpStatus
gpcpNoAdapters:
 mov edx,dword ptr [gpPerfSnapshot+GP_S_UTIL_STATUS]
 mov r8d,dword ptr [gpPerfSnapshot+GP_S_UTIL_ERROR]
 cmp edx,GP_STATUS_VALID
 jne gpcpStatus
 mov edx,GP_STATUS_MISSING
gpcpStatus:
 call GpPerfStatus
 mov ecx,52434
 mov rdx,rax
 lea r8,gpPerfEmpty
 call GpPerfRow
gpcpDone:
 add rsp,30h
 pop rdi
 pop rsi
 pop rbx
 ret
GpCollectPerformance ENDP
