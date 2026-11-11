includelib dxgi.lib
; Single-worker PDH provider. No published frame pointer is retained.
extern PdhOpenQueryW:proc,PdhAddEnglishCounterW:proc,PdhCollectQueryData:proc
extern PdhGetFormattedCounterArrayW:proc,PdhCloseQuery:proc
extern GetTickCount64:proc,GetSystemTimeAsFileTime:proc,OpenProcess:proc
extern GetProcessTimes:proc,GetProcessId:proc,CloseHandle:proc,GetLastError:proc
extern GetProcessHeap:proc,HeapAlloc:proc,HeapFree:proc,CreateDXGIFactory1:proc
.data
align 8
gpApiOpenQuery dq PdhOpenQueryW
gpApiAddCounter dq PdhAddEnglishCounterW
gpApiCollect dq PdhCollectQueryData
gpApiArray dq PdhGetFormattedCounterArrayW
gpApiCloseQuery dq PdhCloseQuery
gpApiClock dq GetTickCount64
gpApiFileTime dq GetSystemTimeAsFileTime
gpApiOpenProcess dq OpenProcess
gpApiProcessTimes dq GetProcessTimes
gpApiProcessId dq GetProcessId
gpApiCloseHandle dq CloseHandle
gpApiLastError dq GetLastError
gpApiAllocate dq HeapAlloc
gpApiFree dq HeapFree
gpApiFactory dq CreateDXGIFactory1
gpProviderQuery dq 0
gpProviderScratch dq 0
gpProviderCapacity dd 0
gpProviderPreviousCount dd 0
gpProviderPreviousTick dq 0
gpProviderPreviousFT dq 0
gpProviderEpoch dq 0
gpProviderRetry dq 0
gpProviderMetadataRetry dq 0
gpProviderMetadataCount dd 0
gpProviderMetadataError dd 0
gpProviderCounters dq 5 dup(0)
gpProviderErrors dd 5 dup(0)
gpProviderPaths dq gpPathEngine,gpPathProcessDedicated,gpPathProcessShared,gpPathAdapterDedicated,gpPathAdapterShared
gpPathEngine dw '\','G','P','U',' ','E','n','g','i','n','e','(','*',')','\','U','t','i','l','i','z','a','t','i','o','n',' ','P','e','r','c','e','n','t','a','g','e',0
gpPathProcessDedicated dw '\','G','P','U',' ','P','r','o','c','e','s','s',' ','M','e','m','o','r','y','(','*',')','\','D','e','d','i','c','a','t','e','d',' ','U','s','a','g','e',0
gpPathProcessShared dw '\','G','P','U',' ','P','r','o','c','e','s','s',' ','M','e','m','o','r','y','(','*',')','\','S','h','a','r','e','d',' ','U','s','a','g','e',0
gpPathAdapterDedicated dw '\','G','P','U',' ','A','d','a','p','t','e','r',' ','M','e','m','o','r','y','(','*',')','\','D','e','d','i','c','a','t','e','d',' ','U','s','a','g','e',0
gpPathAdapterShared dw '\','G','P','U',' ','A','d','a','p','t','e','r',' ','M','e','m','o','r','y','(','*',')','\','S','h','a','r','e','d',' ','U','s','a','g','e',0
align 4
gpIIDFactory dd 770aae78h
 dw 0f26fh,4dbah
 db 0a8h,29h,25h,3ch,83h,0d1h,0b3h,87h
.data?
align 8
gpProviderPrevious db GP_PROCESS_CAPACITY*16 dup(?)
gpProviderMetadata db GP_ADAPTER_CAPACITY*312 dup(?)
.code
GpProviderClose PROC
 sub rsp,28h
 mov rcx,gpProviderQuery
 test rcx,rcx
 jz gppcScratch
 call qword ptr gpApiCloseQuery
gppcScratch:
 mov gpProviderQuery,0
 mov qword ptr gpProviderCounters,0
 mov qword ptr gpProviderCounters+8,0
 mov qword ptr gpProviderCounters+16,0
 mov qword ptr gpProviderCounters+24,0
 mov qword ptr gpProviderCounters+32,0
 mov r8,gpProviderScratch
 test r8,r8
 jz gppcReset
 call GetProcessHeap
 mov rcx,rax
 xor edx,edx
 mov r8,gpProviderScratch
 call qword ptr gpApiFree
gppcReset:
 mov gpProviderScratch,0
 mov gpProviderCapacity,0
 mov gpProviderPreviousCount,0
 mov gpProviderPreviousTick,0
 mov gpProviderPreviousFT,0
 mov gpProviderEpoch,0
 mov gpProviderRetry,0
 mov gpProviderMetadataRetry,0
 mov gpProviderMetadataCount,0
 mov gpProviderMetadataError,0
 add rsp,28h
 ret
GpProviderClose ENDP

; Refresh bounded DXGI metadata; release factory and adapter on every path.
GpProviderMetadataRefresh PROC
 push rbx
 push rsi
 push rdi
 sub rsp,40h
 mov gpProviderMetadataCount,0
 mov qword ptr [rsp+28h],0
 lea rcx,gpIIDFactory
 lea rdx,[rsp+28h]
 call qword ptr gpApiFactory
 test eax,eax
 js gpmrError
 mov rbx,[rsp+28h]
 xor esi,esi
gpmrNext:
 cmp esi,GP_ADAPTER_CAPACITY
 jae gpmrCapacity
 mov qword ptr [rsp+30h],0
 mov rcx,rbx
 mov edx,esi
 lea r8,[rsp+30h]
 mov rax,[rbx]
 call qword ptr [rax+96] ; IDXGIFactory1::EnumAdapters1
 cmp eax,887a0002h
 je gpmrSuccess
 test eax,eax
 js gpmrReleaseError
 mov rdi,[rsp+30h]
 mov rcx,rdi
 imul edx,esi,312
 lea r8,gpProviderMetadata
 add rdx,r8
 mov rax,[rdi]
 call qword ptr [rax+80] ; IDXGIAdapter1::GetDesc1
 mov [rsp+38h],eax
 mov rcx,rdi
 mov rax,[rdi]
 call qword ptr [rax+16]
 mov eax,[rsp+38h]
 test eax,eax
 js gpmrReleaseError
 inc esi
 mov gpProviderMetadataCount,esi
 jmp gpmrNext
gpmrCapacity:
 mov eax,234
 jmp gpmrReleaseError
gpmrSuccess:
 xor eax,eax
gpmrReleaseError:
 mov [rsp+38h],eax
 mov rcx,rbx
 mov rax,[rbx]
 call qword ptr [rax+16]
 mov eax,[rsp+38h]
gpmrError:
 mov gpProviderMetadataError,eax
 add rsp,40h
 pop rdi
 pop rsi
 pop rbx
 ret
GpProviderMetadataRefresh ENDP

; RCX frame. Map by exact 64-bit LUID, never enumeration ordinal.
GpProviderMetadataApply PROC
 push rbx
 push rsi
 push rdi
 mov rbx,rcx
 mov eax,gpProviderMetadataError
 mov [rbx+GP_S_METADATA_ERROR],eax
 ; DXGI identities remain visible even without a matching PDH observation.
 ; Unknown physical index is FFFFFFFFh, never a guessed phys_0.
 lea rsi,gpProviderMetadata
 mov r10d,gpProviderMetadataCount
gpmaSeedNext:
 test r10d,r10d
 jz gpmaSeedDone
 mov rax,[rsi+296]
 lea rdi,[rbx+GP_S_ADAPTERS]
 mov ecx,[rbx+GP_S_ADAPTER_COUNT]
gpmaSeedSearch:
 test ecx,ecx
 jz gpmaSeedNew
 cmp rax,[rdi+GP_A_LUID]
 je gpmaSeedAdvance
 add rdi,GP_ADAPTER_SIZE
 dec ecx
 jmp gpmaSeedSearch
gpmaSeedNew:
 cmp dword ptr [rbx+GP_S_ADAPTER_COUNT],GP_ADAPTER_CAPACITY
 jae gpmaSeedOverflow
 mov [rdi+GP_A_LUID],rax
 mov dword ptr [rdi+GP_A_PHYS],0FFFFFFFFh
 ; Seeded metadata-only adapters inherit the corresponding PDH lane state.
 ; A valid source with no row remains MISSING; a failed source keeps its error.
 mov eax,[rbx+GP_S_SOURCES+GP_SOURCE_ENGINE*8]
 cmp eax,GP_STATUS_VALID
 jne gpmaSeedEngineState
 mov eax,GP_STATUS_MISSING
gpmaSeedEngineState:
 mov [rdi+GP_A_UTIL_STATUS],eax
 mov edx,[rbx+GP_S_SOURCES+GP_SOURCE_ENGINE*8+4]
 mov [rdi+GP_A_UTIL_ERROR],edx
 mov eax,[rbx+GP_S_SOURCES+GP_SOURCE_ADAPTER_DEDICATED*8]
 cmp eax,GP_STATUS_VALID
 jne gpmaSeedDedicatedState
 mov eax,GP_STATUS_MISSING
gpmaSeedDedicatedState:
 mov [rdi+GP_A_DEDICATED_STATUS],eax
 mov edx,[rbx+GP_S_SOURCES+GP_SOURCE_ADAPTER_DEDICATED*8+4]
 mov [rdi+GP_A_DEDICATED_ERROR],edx
 mov eax,[rbx+GP_S_SOURCES+GP_SOURCE_ADAPTER_SHARED*8]
 cmp eax,GP_STATUS_VALID
 jne gpmaSeedSharedState
 mov eax,GP_STATUS_MISSING
gpmaSeedSharedState:
 mov [rdi+GP_A_SHARED_STATUS],eax
 mov edx,[rbx+GP_S_SOURCES+GP_SOURCE_ADAPTER_SHARED*8+4]
 mov [rdi+GP_A_SHARED_ERROR],edx
 inc dword ptr [rbx+GP_S_ADAPTER_COUNT]
 jmp gpmaSeedAdvance
gpmaSeedOverflow:
 mov dword ptr [rbx+GP_S_METADATA_ERROR],234
gpmaSeedAdvance:
 add rsi,312
 dec r10d
 jmp gpmaSeedNext
gpmaSeedDone:
 lea rdi,[rbx+GP_S_ADAPTERS]
 mov r9d,[rbx+GP_S_ADAPTER_COUNT]
gpmaNext:
 test r9d,r9d
 jz gpmaDone
 mov dword ptr [rdi+GP_A_METADATA_STATUS],GP_STATUS_UNAVAILABLE
 mov eax,gpProviderMetadataError
 test eax,eax
 jnz gpmaMissingError
 mov eax,1168
gpmaMissingError:
 mov [rdi+GP_A_METADATA_ERROR],eax
 lea rsi,gpProviderMetadata
 mov r10d,gpProviderMetadataCount
gpmaSearch:
 test r10d,r10d
 jz gpmaAdvance
 mov rax,[rsi+296]
 cmp rax,[rdi+GP_A_LUID]
 je gpmaFound
 add rsi,312
 dec r10d
 jmp gpmaSearch
gpmaFound:
 mov rax,[rsi+272]
 mov [rdi+GP_A_DEDICATED_CAPACITY],rax
 mov rax,[rsi+288]
 mov [rdi+GP_A_SHARED_CAPACITY],rax
 mov eax,[rsi+256]
 mov [rdi+GP_A_VENDOR],eax
 mov eax,[rsi+260]
 mov [rdi+GP_A_DEVICE],eax
 mov eax,[rsi+304]
 and eax,2
 or eax,1
 mov r11d,[rdi+GP_A_FLAGS]
 and r11d,4
 or eax,r11d
 mov [rdi+GP_A_FLAGS],eax
 mov dword ptr [rdi+GP_A_METADATA_STATUS],GP_STATUS_VALID
 mov dword ptr [rdi+GP_A_METADATA_ERROR],0
 xor edx,edx
gpmaCopy:
 mov ax,[rsi+rdx*2]
 mov [rdi+rdx*2+GP_A_DESCRIPTION],ax
 inc edx
 cmp edx,127
 jb gpmaCopy
 mov word ptr [rdi+GP_A_DESCRIPTION+254],0
gpmaAdvance:
 add rdi,GP_ADAPTER_SIZE
 dec r9d
 jmp gpmaNext
gpmaDone:
 pop rdi
 pop rsi
 pop rbx
 ret
GpProviderMetadataApply ENDP

; Read one counter: a zero-size probe precedes every bounded attempt, including
; any retry after MORE_DATA. At most one owned scratch allocation, max 8 MiB.
; RCX frame, EDX source -> EAX model/source error.
GpProviderReadArray PROC
 push rbx
 push rsi
 push rdi
 push r12
 sub rsp,48h
 mov rbx,rcx
 mov esi,edx
 mov edi,GP_ARRAY_RETRIES
gpraProbe:
 mov dword ptr [rsp+30h],0
 mov dword ptr [rsp+34h],0
 lea rax,gpProviderCounters
 mov rcx,[rax+rsi*8]
 mov edx,400h
 test esi,esi
 jnz gpraProbeFormat
 mov edx,8200h
gpraProbeFormat:
 lea r8,[rsp+30h]
 lea r9,[rsp+34h]
 mov qword ptr [rsp+20h],0
 call qword ptr gpApiArray
 cmp eax,800007d2h
 jne gpraError
 mov r12d,[rsp+30h]
 test r12d,r12d
 jz gpraInvalid
 cmp r12d,GP_ARRAY_BUDGET
 ja gpraTooBig
 cmp r12d,gpProviderCapacity
 jbe gpraRead
 mov rcx,gpProviderScratch
 test rcx,rcx
 jz gpraAlloc
 call GetProcessHeap
 mov rcx,rax
 xor edx,edx
 mov r8,gpProviderScratch
 call qword ptr gpApiFree
 mov gpProviderScratch,0
 mov gpProviderCapacity,0
gpraAlloc:
 call GetProcessHeap
 mov rcx,rax
 xor edx,edx
 mov r8d,r12d
 call qword ptr gpApiAllocate
 test rax,rax
 jz gpraNoMemory
 mov gpProviderScratch,rax
 mov gpProviderCapacity,r12d
gpraRead:
 mov eax,gpProviderCapacity
 mov [rsp+30h],eax
 lea rax,gpProviderCounters
 mov rcx,[rax+rsi*8]
 mov edx,400h
 test esi,esi
 jnz gpraReadFormat
 mov edx,8200h
gpraReadFormat:
 lea r8,[rsp+30h]
 lea r9,[rsp+34h]
 mov rax,gpProviderScratch
 mov [rsp+20h],rax
 call qword ptr gpApiArray
 cmp eax,800007d2h
 jne gpraReadResult
 dec edi
 jnz gpraProbe
 mov eax,234
 jmp gpraError
gpraReadResult:
 test eax,eax
 jnz gpraError
 mov eax,[rsp+30h]
 cmp eax,gpProviderCapacity
 ja gpraInvalid
 mov rcx,rbx
 mov edx,esi
 mov r8,gpProviderScratch
 mov r9d,[rsp+30h]
 mov eax,[rsp+34h]
 mov [rsp+20h],eax
 call GpModelArray
 jmp gpraDone
gpraNoMemory:
 mov eax,8
 jmp gpraError
gpraTooBig:
 mov eax,122
 jmp gpraError
gpraInvalid:
 mov eax,13
gpraError:
 test eax,eax
 jnz gpraSetError
 mov eax,21
gpraSetError:
 mov r12d,eax
 mov rcx,rbx
 mov edx,esi
 mov r8d,GP_STATUS_UNAVAILABLE
 mov r9d,eax
 call GpModelSourceError
 mov eax,r12d
gpraDone:
 add rsp,48h
 pop r12
 pop rdi
 pop rsi
 pop rbx
 ret
GpProviderReadArray ENDP

; Resolve actual PID identity after all arrays. A held handle covers both
; GetProcessId and GetProcessTimes. Current compact map overwrites prior only
; after all prior comparisons, never points into the immutable frame.
GpProviderIdentity PROC
 push rbx
 push rsi
 push rdi
 push r12
 push r13
 push r14
 sub rsp,68h
 mov rbx,rcx
 lea rsi,[rbx+GP_S_PROCESSES]
 mov r12d,[rbx+GP_S_PROCESS_COUNT]
 xor r13d,r13d
gpiNext:
 cmp r13d,r12d
 jae gpiStore
 mov dword ptr [rsi+GP_P_IDENTITY_ERROR],0
 mov edi,[rsi+GP_P_PID]
 test edi,edi
 jz gpiUnknown
 mov ecx,1000h
 xor edx,edx
 mov r8d,edi
 call qword ptr gpApiOpenProcess
 test rax,rax
 jz gpiLastError
 mov r14,rax
 mov rcx,r14
 call qword ptr gpApiProcessId
 cmp eax,edi
 jne gpiMismatch
 mov rcx,r14
 lea rdx,[rsp+30h]
 lea r8,[rsp+38h]
 lea r9,[rsp+40h]
 lea rax,[rsp+48h]
 mov [rsp+20h],rax
 call qword ptr gpApiProcessTimes
 test eax,eax
 jz gpiHandleError
 mov rcx,r14
 call qword ptr gpApiCloseHandle
 mov rax,[rsp+30h]
 mov [rsi+GP_P_BIRTH],rax
 test rax,rax
 jz gpiUnknown
 cmp rax,[rbx+GP_S_BEGIN_FT]
 ja gpiNewMemory
 ; Util requires a valid interval plus exact previous PID/birth.
 cmp qword ptr [rbx+GP_S_BASELINE_FT],0
 je gpiNewUtil
 cmp rax,[rbx+GP_S_BASELINE_FT]
 ja gpiNewUtil
 lea rdx,gpProviderPrevious
 mov ecx,gpProviderPreviousCount
gpiFind:
 test ecx,ecx
 jz gpiNewUtil
 cmp edi,[rdx]
 jne gpiFindNext
 cmp rax,[rdx+8]
 je gpiAdvance
 jmp gpiNewUtil
gpiFindNext:
 add rdx,16
 dec ecx
 jmp gpiFind
gpiMismatch:
 mov dword ptr [rsp+50h],13
 jmp gpiCloseError
gpiHandleError:
 call qword ptr gpApiLastError
 mov [rsp+50h],eax
gpiCloseError:
 mov rcx,r14
 call qword ptr gpApiCloseHandle
 mov eax,[rsp+50h]
 jmp gpiMarkUnknown
gpiLastError:
 call qword ptr gpApiLastError
gpiMarkUnknown:
 test eax,eax
 jnz gpiSaveError
 mov eax,13
gpiSaveError:
 mov [rsi+GP_P_IDENTITY_ERROR],eax
gpiUnknown:
 mov qword ptr [rsi+GP_P_BIRTH],0
 mov dword ptr [rsi+GP_P_DEDICATED_STATUS],GP_STATUS_UNAVAILABLE
 mov dword ptr [rsi+GP_P_SHARED_STATUS],GP_STATUS_UNAVAILABLE
 mov eax,[rsi+GP_P_IDENTITY_ERROR]
 test eax,eax
 jnz gpiUnknownCode
 mov eax,13
gpiUnknownCode:
 ; Preserve the source's own error when that lane was already invalid.
 mov r8d,[rbx+GP_S_SOURCES+GP_SOURCE_PROCESS_DEDICATED*8]
 cmp r8d,GP_STATUS_VALID
 jne gpiPreserveDedicated
 mov [rsi+GP_P_DEDICATED_ERROR],eax
gpiPreserveDedicated:
 mov r8d,[rbx+GP_S_SOURCES+GP_SOURCE_PROCESS_SHARED*8]
 cmp r8d,GP_STATUS_VALID
 jne gpiPreserveShared
 mov [rsi+GP_P_SHARED_ERROR],eax
gpiPreserveShared:
 mov r8d,[rbx+GP_S_SOURCES]
 cmp r8d,GP_STATUS_VALID
 jne gpiPreserveUtil
 mov [rsi+GP_P_UTIL_ERROR],eax
gpiPreserveUtil:
 mov dword ptr [rsi+GP_P_UTIL_STATUS],GP_STATUS_UNAVAILABLE
 jmp gpiAdvance
gpiNewMemory:
 cmp dword ptr [rsi+GP_P_DEDICATED_STATUS],GP_STATUS_VALID
 jne gpiNewShared
 mov dword ptr [rsi+GP_P_DEDICATED_STATUS],GP_STATUS_WARMUP
gpiNewShared:
 cmp dword ptr [rsi+GP_P_SHARED_STATUS],GP_STATUS_VALID
 jne gpiNewUtil
 mov dword ptr [rsi+GP_P_SHARED_STATUS],GP_STATUS_WARMUP
gpiNewUtil:
 cmp dword ptr [rsi+GP_P_UTIL_STATUS],GP_STATUS_VALID
 jne gpiAdvance
 mov dword ptr [rsi+GP_P_UTIL_STATUS],GP_STATUS_WARMUP
gpiAdvance:
 add rsi,GP_PROCESS_SIZE
 inc r13d
 jmp gpiNext
gpiStore:
 lea rsi,[rbx+GP_S_PROCESSES]
 lea rdi,gpProviderPrevious
 xor ecx,ecx
 xor edx,edx
gpiStoreNext:
 cmp ecx,r12d
 jae gpiStored
 mov rax,[rsi+GP_P_BIRTH]
 test rax,rax
 jz gpiStoreAdvance
 mov r8d,[rsi+GP_P_PID]
 mov [rdi],r8d
 mov [rdi+8],rax
 add rdi,16
 inc edx
gpiStoreAdvance:
 add rsi,GP_PROCESS_SIZE
 inc ecx
 jmp gpiStoreNext
gpiStored:
 mov gpProviderPreviousCount,edx
 add rsp,68h
 pop r14
 pop r13
 pop r12
 pop rdi
 pop rsi
 pop rbx
 ret
GpProviderIdentity ENDP

GpProviderSample PROC
 push rbx
 push rsi
 push rdi
 push r12
 push r13
 push r14
 push r15
 sub rsp,40h
 mov rbx,rcx
 mov r12,rdx
 mov r13,r8
 mov r14d,r9d
 call qword ptr gpApiClock
 mov r15,rax
 mov rcx,rbx
 mov rdx,r12
 mov r8,r13
 mov r9,r15
 call GpModelInit
 mov [rbx+GP_S_COVERAGE],r14d
 lea rcx,[rbx+GP_S_BEGIN_FT]
 call qword ptr gpApiFileTime
 cmp r13,gpProviderEpoch
 jne gpsResetBaseline
 mov rax,r15
 sub rax,gpProviderPreviousTick
 cmp rax,GP_MAX_GAP_MS
 ja gpsResetBaseline
 mov [rbx+GP_S_INTERVAL_MS],eax
 mov rax,gpProviderPreviousFT
 mov [rbx+GP_S_BASELINE_FT],rax
 jmp gpsQuery
gpsResetBaseline:
 mov gpProviderPreviousFT,0
 mov gpProviderPreviousCount,0
 mov gpProviderEpoch,r13
gpsQuery:
 cmp gpProviderQuery,0
 jne gpsCounters
 cmp r15,gpProviderRetry
 jb gpsNoQuery
 xor ecx,ecx
 xor edx,edx
 lea r8,gpProviderQuery
 call qword ptr gpApiOpenQuery
 test eax,eax
 jz gpsCounters
 mov esi,eax
 lea rax,[r15+30000]
 mov gpProviderRetry,rax
 jmp gpsAllError
gpsNoQuery:
 mov esi,21
 jmp gpsAllError
gpsCounters:
 ; Add any missing lanes no more than once in 30 seconds.
 cmp r15,gpProviderRetry
 jb gpsCollect
 lea rax,[r15+30000]
 mov gpProviderRetry,rax
 xor esi,esi
gpsAdd:
 test r14d,GP_MASK_PROCESS
 jnz gpsAddEnabled
 cmp esi,1
 je gpsAddNext
 cmp esi,2
 je gpsAddNext
gpsAddEnabled:
 lea rax,gpProviderCounters
 cmp qword ptr [rax+rsi*8],0
 jne gpsAddNext
 mov rcx,gpProviderQuery
 lea rax,gpProviderPaths
 mov rdx,[rax+rsi*8]
 xor r8d,r8d
 lea r9,gpProviderCounters
 lea r9,[r9+rsi*8]
 call qword ptr gpApiAddCounter
 lea rdx,gpProviderErrors
 mov [rdx+rsi*4],eax
gpsAddNext:
 inc esi
 cmp esi,GP_SOURCES
 jb gpsAdd
gpsCollect:
 mov rcx,gpProviderQuery
 call qword ptr gpApiCollect
 test eax,eax
 jz gpsArrays
 mov esi,eax
 mov gpProviderPreviousFT,0
 mov gpProviderPreviousCount,0
 jmp gpsAllError
gpsArrays:
 xor esi,esi
gpsArrayNext:
 test r14d,GP_MASK_PROCESS
 jnz gpsArrayEnabled
 cmp esi,1
 je gpsArrayAdvance
 cmp esi,2
 je gpsArrayAdvance
gpsArrayEnabled:
 lea rax,gpProviderCounters
 cmp qword ptr [rax+rsi*8],0
 je gpsMissing
 mov rcx,rbx
 mov edx,esi
 call GpProviderReadArray
 jmp gpsArrayAdvance
gpsMissing:
 mov rcx,rbx
 mov edx,esi
 mov r8d,GP_STATUS_MISSING
 lea rax,gpProviderErrors
 mov r9d,[rax+rsi*4]
 call GpModelSourceError
gpsArrayAdvance:
 inc esi
 cmp esi,GP_SOURCES
 jb gpsArrayNext
 jmp gpsFinalize
gpsAllError:
 xor edi,edi
gpsErrorNext:
 mov rcx,rbx
 mov edx,edi
 mov r8d,GP_STATUS_UNAVAILABLE
 mov r9d,esi
 call GpModelSourceError
 inc edi
 cmp edi,GP_SOURCES
 jb gpsErrorNext
gpsFinalize:
 lea rcx,[rbx+GP_S_END_FT]
 call qword ptr gpApiFileTime
 mov rcx,rbx
 call GpModelFinalize
 ; Only a complete, successful engine array advances the rate baseline.
 cmp dword ptr [rbx+GP_S_SOURCES],GP_STATUS_VALID
 jne gpsInvalidateBaseline
 cmp qword ptr [rbx+GP_S_BASELINE_FT],0
 jne gpsIdentity
 call GpProviderWarmup
 jmp gpsIdentity
gpsInvalidateBaseline:
 mov qword ptr [rbx+GP_S_BASELINE_FT],0
 mov gpProviderPreviousCount,0
gpsIdentity:
 mov rcx,rbx
 call GpProviderIdentity
 cmp dword ptr [rbx+GP_S_SOURCES],GP_STATUS_VALID
 jne gpsNoBaseline
 ; A PID born during this read cannot own the preceding raw baseline.
 mov rax,[rbx+GP_S_BEGIN_FT]
 mov gpProviderPreviousFT,rax
 mov gpProviderPreviousTick,r15
 jmp gpsMetadata
gpsNoBaseline:
 mov gpProviderPreviousFT,0
 mov gpProviderPreviousCount,0
gpsMetadata:
 cmp r15,gpProviderMetadataRetry
 jb gpsApplyMetadata
 lea rax,[r15+30000]
 mov gpProviderMetadataRetry,rax
 call GpProviderMetadataRefresh
gpsApplyMetadata:
 mov rcx,rbx
 call GpProviderMetadataApply
 xor eax,eax
 add rsp,40h
 pop r15
 pop r14
 pop r13
 pop r12
 pop rdi
 pop rsi
 pop rbx
 ret
GpProviderSample ENDP

; RBX initialized frame. Preserve independent source errors while suppressing
; all rate values during initial collection/epoch change/gap.
GpProviderWarmup PROC
 cmp dword ptr [rbx+GP_S_UTIL_STATUS],GP_STATUS_VALID
 jne gpwuProcesses
 mov dword ptr [rbx+GP_S_UTIL_STATUS],GP_STATUS_WARMUP
gpwuProcesses:
 lea rdx,[rbx+GP_S_PROCESSES]
 mov ecx,[rbx+GP_S_PROCESS_COUNT]
gpwuP:
 test ecx,ecx
 jz gpwuAdapters
 cmp dword ptr [rdx+GP_P_UTIL_STATUS],GP_STATUS_VALID
 jne gpwuPNext
 mov dword ptr [rdx+GP_P_UTIL_STATUS],GP_STATUS_WARMUP
gpwuPNext:
 add rdx,GP_PROCESS_SIZE
 dec ecx
 jmp gpwuP
gpwuAdapters:
 lea rdx,[rbx+GP_S_ADAPTERS]
 mov ecx,[rbx+GP_S_ADAPTER_COUNT]
gpwuA:
 test ecx,ecx
 jz gpwuEngines
 cmp dword ptr [rdx+GP_A_UTIL_STATUS],GP_STATUS_VALID
 jne gpwuANext
 mov dword ptr [rdx+GP_A_UTIL_STATUS],GP_STATUS_WARMUP
gpwuANext:
 add rdx,GP_ADAPTER_SIZE
 dec ecx
 jmp gpwuA
gpwuEngines:
 lea rdx,[rbx+GP_S_ENGINES]
 mov ecx,[rbx+GP_S_ENGINE_COUNT]
gpwuE:
 test ecx,ecx
 jz gpwuDone
 cmp dword ptr [rdx+GP_E_STATUS],GP_STATUS_VALID
 jne gpwuENext
 mov dword ptr [rdx+GP_E_STATUS],GP_STATUS_WARMUP
gpwuENext:
 add rdx,GP_ENGINE_SIZE
 dec ecx
 jmp gpwuE
gpwuDone:
 ret
GpProviderWarmup ENDP
