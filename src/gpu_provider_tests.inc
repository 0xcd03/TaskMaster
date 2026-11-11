; Owned deterministic provider fixtures; no external process mutation.
extern ExitProcess:proc
.data
align 8
gpTestFrame db GP_SNAPSHOT_SIZE dup(0)
gpTestTick dq 1000
gpTestBirth dq 500
gpTestFT dq 10000
gpTestStage dd 1
gpTestOpenError dd 0
gpTestCollectError dd 0
gpTestArrayMode dd 0
gpTestOpenCount dd 0
gpTestCloseCount dd 0
gpTestAddCount dd 0
gpTestProbeCount dd 0
gpTestReadCount dd 0
gpTestIdentityMode dd 0
gptNameEngine dw 'p','i','d','_','7','7','_','l','u','i','d','_','0','x','0','0','0','0','0','0','0','0','_','0','x','0','0','0','0','1','2','3','4'
 dw '_','p','h','y','s','_','0','_','e','n','g','_','0','_','e','n','g','t','y','p','e','_','3','D',0
gptNameProcess dw 'p','i','d','_','7','7','_','l','u','i','d','_','0','x','0','0','0','0','0','0','0','0','_','0','x','0','0','0','0','1','2','3','4'
 dw '_','p','h','y','s','_','0',0
gptNameAdapter dw 'l','u','i','d','_','0','x','0','0','0','0','0','0','0','0','_','0','x','0','0','0','0','1','2','3','4'
 dw '_','p','h','y','s','_','0',0
.code
GpuProviderTest PROC
 sub rsp,28h
 lea rax,GptOpen
 mov gpApiOpenQuery,rax
 lea rax,GptAdd
 mov gpApiAddCounter,rax
 lea rax,GptCollect
 mov gpApiCollect,rax
 lea rax,GptArray
 mov gpApiArray,rax
 lea rax,GptClose
 mov gpApiCloseQuery,rax
 lea rax,GptClock
 mov gpApiClock,rax
 lea rax,GptFT
 mov gpApiFileTime,rax
 lea rax,GptProcessOpen
 mov gpApiOpenProcess,rax
 lea rax,GptProcessTimes
 mov gpApiProcessTimes,rax
 lea rax,GptProcessId
 mov gpApiProcessId,rax
 lea rax,GptHandleClose
 mov gpApiCloseHandle,rax
 lea rax,GptLastError
 mov gpApiLastError,rax
 lea rax,GptFactory
 mov gpApiFactory,rax
 mov gpTestOpenError,1234
 call GptSample
 cmp dword ptr [gpTestFrame+GP_S_SIZE],GP_SNAPSHOT_SIZE
 jne gptFail
 cmp qword ptr [gpTestFrame+GP_S_SEQUENCE],12
 jne gptFail
 cmp qword ptr [gpTestFrame+GP_S_EPOCH],33
 jne gptFail
 cmp dword ptr [gpTestFrame+GP_S_SOURCES+4],1234
 jne gptFail
 inc gpTestStage
 call GptSample
 cmp gpTestOpenCount,1 ; failure cooldown
 jne gptFail
 call GpProviderClose
 mov gpTestOpenError,0
 call GptSample
 cmp gpTestAddCount,5
 jne gptFail
 cmp qword ptr [gpTestFrame+GP_S_PROCESSES+GP_P_BIRTH],500
 jne gptFail
 cmp dword ptr [gpTestFrame+GP_S_PROCESSES+GP_P_UTIL_STATUS],GP_STATUS_WARMUP
 jne gptFail
 cmp dword ptr [gpTestFrame+GP_S_PROCESSES+GP_P_SHARED_STATUS],GP_STATUS_VALID
 jne gptFail
 inc gpTestStage
 add gpTestTick,1000
 call GptSample
 cmp dword ptr [gpTestFrame+GP_S_PROCESSES+GP_P_UTIL_STATUS],GP_STATUS_VALID
 jne gptFail
 cmp gpTestAddCount,5
 jne gptFail
  mov gpTestStage,30
 mov gpTestBirth,1000
 add gpTestTick,1000
 call GptSample
 cmp dword ptr [gpTestFrame+GP_S_PROCESSES+GP_P_UTIL_STATUS],GP_STATUS_WARMUP
 jne gptFail
 cmp dword ptr [gpTestFrame+GP_S_PROCESSES+GP_P_SHARED_STATUS],GP_STATUS_VALID
 jne gptFail
 inc gpTestStage
 mov gpTestBirth,20000
 add gpTestTick,1000
 call GptSample
 cmp qword ptr [gpTestFrame+GP_S_PROCESSES+GP_P_BIRTH],20000
 jne gptFail
 cmp dword ptr [gpTestFrame+GP_S_PROCESSES+GP_P_UTIL_STATUS],GP_STATUS_WARMUP
 jne gptFail
 cmp dword ptr [gpTestFrame+GP_S_PROCESSES+GP_P_SHARED_STATUS],GP_STATUS_WARMUP
 jne gptFail
 inc gpTestStage
 mov gpTestIdentityMode,1
 add gpTestTick,1000
 call GptSample
 cmp qword ptr [gpTestFrame+GP_S_PROCESSES+GP_P_BIRTH],0
 jne gptFail
 cmp dword ptr [gpTestFrame+GP_S_PROCESSES+GP_P_IDENTITY_ERROR],5
 jne gptFail
 cmp dword ptr [gpTestFrame+GP_S_PROCESSES+GP_P_SHARED_STATUS],GP_STATUS_UNAVAILABLE
 jne gptFail
 inc gpTestStage
 mov gpTestIdentityMode,0
 mov gpTestBirth,500
 mov gpTestArrayMode,1
 add gpTestTick,1000
 call GptSample
 cmp dword ptr [gpTestFrame+GP_S_SOURCES+4],4321
 jne gptFail
 cmp dword ptr [gpTestFrame+GP_S_SOURCES+16],GP_STATUS_VALID
 jne gptFail
 cmp gpProviderPreviousFT,0
 jne gptFail
 inc gpTestStage
 mov gpTestArrayMode,0
 add gpTestTick,6000
 call GptSample
 cmp dword ptr [gpTestFrame+GP_S_UTIL_STATUS],GP_STATUS_WARMUP
 jne gptFail
 inc gpTestStage
 mov gpTestArrayMode,2
 mov gpTestProbeCount,0
 mov gpTestReadCount,0
 lea rcx,gpTestFrame
 xor edx,edx
 call GpProviderReadArray
 cmp eax,234
 jne gptFail
 cmp gpTestProbeCount,3
 jne gptFail
 cmp gpTestReadCount,3
 jne gptFail
 inc gpTestStage
 mov gpTestArrayMode,3
 mov gpTestReadCount,0
 lea rcx,gpTestFrame
 xor edx,edx
 call GpProviderReadArray
 cmp eax,122
 jne gptFail
 cmp gpTestReadCount,0
 jne gptFail
 inc gpTestStage
 call GpProviderClose
 lea rax,GptNoMemory
 mov gpApiAllocate,rax
 mov gpTestArrayMode,0
 lea rcx,gpTestFrame
 xor edx,edx
 call GpProviderReadArray
 cmp eax,8
 jne gptFail
 cmp gpProviderScratch,0
 jne gptFail
 lea rax,HeapAlloc
 mov gpApiAllocate,rax
 inc gpTestStage
 mov gpTestCollectError,6789
 call GptSample
 cmp dword ptr [gpTestFrame+GP_S_SOURCES+4],6789
 jne gptFail
 cmp dword ptr [gpTestFrame+GP_S_SOURCES+36],6789
 jne gptFail
 cmp gpProviderPreviousFT,0
 jne gptFail
 call GpProviderClose
 cmp gpProviderQuery,0
 jne gptFail
 cmp gpProviderScratch,0
 jne gptFail
 cmp gpProviderPreviousCount,0
 jne gptFail
  ; Metadata must match full LUID, not ordinal; capacities remain distinct.
 mov gpTestStage,20
 lea rcx,gpTestFrame
 mov dword ptr [rcx+GP_S_ADAPTER_COUNT],1
 mov qword ptr [rcx+GP_S_ADAPTERS+GP_A_LUID],1234
 mov qword ptr [gpProviderMetadata+296],1234
 mov qword ptr [gpProviderMetadata+272],9000
 mov qword ptr [gpProviderMetadata+288],8000
 mov dword ptr [gpProviderMetadata+304],2
 mov gpProviderMetadataCount,1
 mov gpProviderMetadataError,0
 call GpProviderMetadataApply
 cmp qword ptr [gpTestFrame+GP_S_ADAPTERS+GP_A_DEDICATED_CAPACITY],9000
 jne gptFail
 cmp qword ptr [gpTestFrame+GP_S_ADAPTERS+GP_A_SHARED_CAPACITY],8000
 jne gptFail
 cmp dword ptr [gpTestFrame+GP_S_ADAPTERS+GP_A_FLAGS],3
 jne gptFail
  mov gpTestStage,23
 lea rcx,gpTestFrame
 mov dword ptr [rcx+GP_S_ADAPTER_COUNT],0
 call GpProviderMetadataApply
 cmp dword ptr [gpTestFrame+GP_S_ADAPTER_COUNT],1
 jne gptFail
 cmp dword ptr [gpTestFrame+GP_S_ADAPTERS+GP_A_PHYS],0FFFFFFFFh
 jne gptFail
 mov gpTestStage,21
 mov gpTestCollectError,0
 mov gpTestIdentityMode,2
 call GptSample
 cmp dword ptr [gpTestFrame+GP_S_PROCESSES+GP_P_IDENTITY_ERROR],13
 jne gptFail
 cmp qword ptr [gpTestFrame+GP_S_PROCESSES+GP_P_BIRTH],0
 jne gptFail
 call GpProviderClose
 mov gpTestStage,22
 mov gpTestAddCount,0
 lea rcx,gpTestFrame
 mov edx,12
 mov r8d,34
 mov r9d,GP_MASK_GLOBAL
 call GpProviderSample
 cmp gpTestAddCount,3
 jne gptFail
 call GpProviderClose
 mov gpTestStage,90
 mov gpTestIdentityMode,0
 mov gpTestFT,10000
 mov gpTestBirth,15000
 add gpTestTick,1000
 lea rax,GptAdvancingFT
 mov gpApiFileTime,rax
 call GptSample
 cmp qword ptr [gpTestFrame+GP_S_BEGIN_FT],10000
 jne gptFail
 cmp qword ptr [gpTestFrame+GP_S_END_FT],20000
 jne gptFail
 cmp dword ptr [gpTestFrame+GP_S_PROCESSES+GP_P_SHARED_STATUS],GP_STATUS_WARMUP
 jne gptFail
 mov gpTestStage,91
 add gpTestTick,1000
 call GptSample
 cmp dword ptr [gpTestFrame+GP_S_PROCESSES+GP_P_UTIL_STATUS],GP_STATUS_WARMUP
 jne gptFail
 call GpProviderClose
 xor ecx,ecx
 call ExitProcess
gptFail:
 mov ecx,gpTestStage
 call ExitProcess
GpuProviderTest ENDP
GptSample PROC
 sub rsp,28h
 lea rcx,gpTestFrame
 mov edx,12
 mov r8d,33
 mov r9d,GP_MASK_ALL
 call GpProviderSample
 add rsp,28h
 ret
GptSample ENDP
GptOpen PROC
 inc gpTestOpenCount
 mov eax,gpTestOpenError
 test eax,eax
 jnz gptoDone
 mov qword ptr [r8],123
gptoDone:
 ret
GptOpen ENDP
GptAdd PROC
 inc gpTestAddCount
  xor eax,eax
 lea r10,gpProviderPaths
gptAddFind:
 cmp rdx,[r10+rax*8]
 je gptAddFound
 inc eax
 cmp eax,5
 jb gptAddFind
gptAddFound:
 inc eax
 mov [r9],rax
 xor eax,eax
 ret
GptAdd ENDP
GptCollect PROC
 mov eax,gpTestCollectError
 ret
GptCollect ENDP
GptArray PROC
 cmp qword ptr [rsp+28h],0
 jne gptaRead
 inc gpTestProbeCount
 mov dword ptr [r8],256
 mov dword ptr [r9],1
 cmp gpTestArrayMode,3
 jne gptaMore
 mov dword ptr [r8],GP_ARRAY_BUDGET+1
gptaMore:
 mov eax,800007d2h
 ret
gptaRead:
 inc gpTestReadCount
 cmp gpTestArrayMode,2
 je gptaMore
 cmp gpTestArrayMode,1
 jne gptaOkay
 cmp edx,8200h
 jne gptaOkay
 mov eax,4321
 ret
gptaOkay:
 mov dword ptr [r8],256
 mov dword ptr [r9],1
 mov r10,[rsp+28h]
 lea r11,[r10+24]
 mov [r10],r11
 mov dword ptr [r10+8],0
 mov qword ptr [r10+16],4096
 lea rdx,gptNameProcess
 cmp ecx,4
 jb gptaMaybeEngine
 lea rdx,gptNameAdapter
 jmp gptaCopy
gptaMaybeEngine:
 cmp ecx,1
 jne gptaCopy
 mov rax,4029000000000000h ; 12.5%
 mov [r10+16],rax
 lea rdx,gptNameEngine
gptaCopy:
 mov ax,[rdx]
 mov [r11],ax
 add rdx,2
 add r11,2
 test ax,ax
 jnz gptaCopy
 xor eax,eax
 ret
GptArray ENDP
GptClose PROC
 inc gpTestCloseCount
 xor eax,eax
 ret
GptClose ENDP
GptClock PROC
 mov rax,gpTestTick
 ret
GptClock ENDP
GptFT PROC
 mov rax,gpTestFT
 mov [rcx],rax
 ret
GptFT ENDP
GptAdvancingFT PROC
 mov rax,gpTestFT
 mov [rcx],rax
 add gpTestFT,10000
 ret
GptAdvancingFT ENDP
GptProcessOpen PROC
 xor eax,eax
 cmp gpTestIdentityMode,1
 je gtpoDone
 mov eax,1234
gtpoDone:
 ret
GptProcessOpen ENDP
GptProcessId PROC
 mov eax,77
 cmp gpTestIdentityMode,2
 jne gptpidDone
 inc eax
gptpidDone:
 ret
GptProcessId ENDP
GptProcessTimes PROC
 mov rax,gpTestBirth
 mov [rdx],rax
 mov eax,1
 ret
GptProcessTimes ENDP
GptHandleClose PROC
 mov eax,1
 ret
GptHandleClose ENDP
GptLastError PROC
 mov eax,5
 ret
GptLastError ENDP
GptFactory PROC
 mov eax,80004005h
 ret
GptFactory ENDP
GptNoMemory PROC
 xor eax,eax
 ret
GptNoMemory ENDP
