; Owned immutable copy fixture; actual formatter/row append under test.
.data
gpptStage dd 9400
gpptAssertions dd 0
gpptError dd 0
gpptFixture db GP_ADAPTER_COPY_SIZE dup(0)
gpptZero dw '0','.','0',0
gpptUsage dw '2','.','0',0
gpptCapacity dw '8','.','0',0
gpptSequence dw '7',0
gpptInterval dw '1','0','0','0',0
gpptName dw 'F','i','x','t','u','r','e',' ','G','P','U',0
gpptUnknown dw '0','0','0','0','0','0','0','0',':','0','0','0','0','1','2','3','4',' ','/',' ','-',0
gpptLuid dw '0','0','0','0','0','0','0','0',':','0','0','0','0','1','2','3','4',0
gpptPhysical dw '0','0','0','0','0','0','0','0',':','0','0','0','0','1','2','3','4',' ','/',' ','1',0
gpptResourceIds dd 52402,52400,0,52404,52406,52408
gpptLong dw 300 dup('A'),0
gpptExpected dw 256 dup(0)
gpptFormat dw 128 dup(0)
.code
GPPT_EQ MACRO a,b
 cmp a,b
 jne gpptFail
 inc gpptAssertions
ENDM
GpptCopy PROC
 push rsi
 push rdi
 mov rdi,rcx
 lea rsi,gpptFixture
 mov ecx,GP_ADAPTER_COPY_SIZE
 rep movsb
 mov eax,gpptError
 pop rdi
 pop rsi
 ret
GpptCopy ENDP
GPPT_TEXT MACRO row,expected
 mov rcx,backBuffer
 add rcx,row*4096+512
 lea rdx,expected
 call lstrcmpW
 GPPT_EQ eax,0
ENDM
GpuPerformanceTest PROC
 push rbx
 push rsi
 push rdi
 sub rsp,30h
 xor ecx,ecx
 call GetModuleHandleW
 mov gInst,rax
 mov appLanguage,0
 call GetProcessHeap
 mov rcx,rax
 mov edx,8
 mov r8d,8193*4096
 call HeapAlloc
 test rax,rax
 jz gpptFail
 mov backBuffer,rax
 lea rax,GpptCopy
 mov gpPerfCopy,rax
 mov dword ptr [gpptFixture+GP_S_ADAPTER_COUNT],1
 mov qword ptr [gpptFixture+GP_S_SEQUENCE],7
 mov qword ptr [gpptFixture+GP_S_INTERVAL_MS],1000
 lea rbx,gpptFixture+GP_HEADER_SIZE
 mov qword ptr [rbx+GP_A_LUID],1234h
 mov dword ptr [rbx+GP_A_FLAGS],1
 mov dword ptr [rbx+GP_A_UTIL_STATUS],GP_STATUS_VALID
 mov dword ptr [rbx+GP_A_DEDICATED_STATUS],GP_STATUS_VALID
 mov dword ptr [rbx+GP_A_SHARED_STATUS],GP_STATUS_VALID
 mov dword ptr [rbx+GP_A_METADATA_STATUS],GP_STATUS_VALID
 mov qword ptr [rbx+GP_A_DEDICATED],2097152
 mov qword ptr [rbx+GP_A_DEDICATED_CAPACITY],8388608
 lea rcx,[rbx+GP_A_DESCRIPTION]
 lea rdx,gpptName
 call lstrcpyW
 mov backCount,0
 call GpCollectPerformance
 ; Three capture/source rows followed by seven per-adapter rows.
 GPPT_EQ backCount,10
 GPPT_TEXT 1,gpptSequence
 GPPT_TEXT 2,gpptInterval
 ; Source caveat must survive as an accessible full native value cell.
 mov rcx,gInst
 mov edx,52410
 lea r8,gpptExpected
 mov r9d,256
 call LoadStringW
 test eax,eax
 jz gpptFail
 GPPT_TEXT 0,gpptExpected
 mov gpptStage,9401
 mov rcx,backBuffer
 add rcx,5*4096+512
 lea rdx,gpptZero
 call lstrcmpW
 GPPT_EQ eax,0
 mov gpptStage,9402
 GPPT_TEXT 6,gpptUsage
 mov gpptStage,9403
 GPPT_TEXT 8,gpptCapacity
 ; Every invalid status remains distinct, including localized error codes.
 mov gpptStage,9410
 xor edi,edi
gpptLanguage:
 mov appLanguage,edi
 xor esi,esi
gpptStatus:
 cmp esi,GP_STATUS_VALID
 je gpptNextStatus
 mov [rbx+GP_A_UTIL_STATUS],esi
 mov dword ptr [rbx+GP_A_UTIL_ERROR],87
 mov backCount,0
 call GpCollectPerformance
 mov rcx,gInst
 lea rax,gpptResourceIds
 mov edx,[rax+rsi*4]
 add edx,edi
 lea r8,gpptFormat
 mov r9d,128
 call LoadStringW
 test eax,eax
 jz gpptFail
 lea rcx,gpptExpected
 lea rdx,gpptFormat
 mov r8d,87
 call wsprintfW
 GPPT_TEXT 5,gpptExpected
gpptNextStatus:
 inc esi
 cmp esi,6
 jb gpptStatus
 inc edi
 cmp edi,2
 jb gpptLanguage
 mov appLanguage,0
 ; Empty capture explicitly exposes warmup instead of hiding GPU monitoring.
 mov gpptStage,9411
 mov dword ptr [gpptFixture+GP_S_ADAPTER_COUNT],0
 mov dword ptr [gpptFixture+GP_S_UTIL_STATUS],GP_STATUS_WARMUP
 mov backCount,0
 call GpCollectPerformance
 GPPT_EQ backCount,4
 mov rcx,gInst
 mov edx,52400
 lea r8,gpptExpected
 mov r9d,256
 call LoadStringW
 GPPT_TEXT 3,gpptExpected
 mov dword ptr [gpptFixture+GP_S_ADAPTER_COUNT],1
 ; Real worker ABI: timeout1460 preserves stale header + adapter metadata.
 mov gpptStage,9420
 mov gpptError,1460
 mov dword ptr [gpptFixture+GP_S_UTIL_STATUS],GP_STATUS_STALE
 mov dword ptr [gpptFixture+GP_S_UTIL_ERROR],1460
 mov dword ptr [rbx+GP_A_UTIL_STATUS],GP_STATUS_STALE
 mov dword ptr [rbx+GP_A_DEDICATED_STATUS],GP_STATUS_STALE
 mov dword ptr [rbx+GP_A_SHARED_STATUS],GP_STATUS_STALE
 xor edi,edi
gpptStaleLanguage:
 mov appLanguage,edi
 mov backCount,0
 call GpCollectPerformance
 GPPT_EQ backCount,10
 GPPT_TEXT 1,gpptSequence
 GPPT_TEXT 2,gpptInterval
 GPPT_TEXT 3,gpptName
 mov rcx,gInst
 mov edx,52408
 add edx,edi
 lea r8,gpptExpected
 mov r9d,256
 call LoadStringW
 GPPT_TEXT 5,gpptExpected
 GPPT_TEXT 6,gpptExpected
 GPPT_TEXT 7,gpptExpected
 ; Both capacity cells remain numeric while all usage cells are stale.
 ; Numeric separators follow the current language.
 mov ax,'.'
 test edi,edi
 jz gpptStaleDecimal
 mov ax,','
gpptStaleDecimal:
 mov [gpptCapacity+2],ax
 mov [gpptZero+2],ax
 GPPT_TEXT 8,gpptCapacity
 GPPT_TEXT 9,gpptZero
 inc edi
 cmp edi,2
 jb gpptStaleLanguage
 mov appLanguage,0
 mov word ptr [gpptCapacity+2],'.'
 mov word ptr [gpptZero+2],'.'
 mov gpptError,0
 ; Same LUID, distinct phys, plus software and unknown physical index.
 mov gpptStage,9421
 lea rdi,[rbx+GP_ADAPTER_SIZE]
 mov rsi,rbx
 mov ecx,GP_ADAPTER_SIZE
 rep movsb
 mov dword ptr [rbx+GP_ADAPTER_SIZE+GP_A_PHYS],1
 mov dword ptr [rbx+GP_A_PHYS],-1
 mov dword ptr [rbx+GP_A_FLAGS],3
 mov dword ptr [gpptFixture+GP_S_ADAPTER_COUNT],2
 mov backCount,0
 call GpCollectPerformance
 GPPT_EQ backCount,18
 GPPT_TEXT 4,gpptUnknown
 GPPT_TEXT 12,gpptPhysical
 mov rcx,gInst
 mov edx,52420
 lea r8,gpptExpected
 mov r9d,256
 call LoadStringW
 GPPT_TEXT 10,gpptExpected
 ; Metadata flag is mandatory even when a stale capacity number remains.
 mov gpptStage,9422
 mov dword ptr [rbx+GP_A_FLAGS],0
 mov dword ptr [gpptFixture+GP_S_ADAPTER_COUNT],1
 mov backCount,0
 call GpCollectPerformance
 mov rcx,gInst
 mov edx,52402
 lea r8,gpptExpected
 mov r9d,256
 call LoadStringW
 GPPT_TEXT 8,gpptExpected
 GPPT_TEXT 3,gpptLuid
 ; Nonzero worker failure must clear adapter rows, never silently disappear.
 mov gpptStage,9430
 mov gpptError,5
 mov dword ptr [gpptFixture+GP_S_UTIL_STATUS],GP_STATUS_UNAVAILABLE
 mov dword ptr [gpptFixture+GP_S_UTIL_ERROR],5
 mov dword ptr [gpptFixture+GP_S_ADAPTER_COUNT],0
 mov backCount,0
 call GpCollectPerformance
 GPPT_EQ backCount,2
 mov rcx,gInst
 mov edx,52404
 lea r8,gpptFormat
 mov r9d,128
 call LoadStringW
 lea rcx,gpptExpected
 lea rdx,gpptFormat
 mov r8d,5
 call wsprintfW
 GPPT_TEXT 1,gpptExpected
 mov gpptError,0
 ; Overlarge adapter count clamps before any record read.
 mov gpptStage,9440
 lea rdi,gpptFixture
 xor eax,eax
 mov ecx,GP_ADAPTER_COPY_SIZE
 rep stosb
 mov dword ptr [gpptFixture+GP_S_ADAPTER_COUNT],-1
 mov backCount,0
 call GpCollectPerformance
 GPPT_EQ backCount,452
 ; Max-size nonterminated DXGI description cannot bleed into vendor fields.
 mov gpptStage,9441
 mov dword ptr [gpptFixture+GP_S_ADAPTER_COUNT],1
 mov dword ptr [rbx+GP_A_FLAGS],1
 lea rdi,[rbx+GP_A_DESCRIPTION]
 mov ax,'A'
 mov ecx,128
 rep stosw
 mov dword ptr [rbx+GP_A_VENDOR],42424242h
 mov backCount,0
 call GpCollectPerformance
 mov rcx,backBuffer
 add rcx,3*4096+512
 call lstrlenW
 GPPT_EQ eax,127
 ; Full row and cell guards: index8192 remains untouched.
 mov gpptStage,9450
 mov rdi,backBuffer
 mov rax,1234567812345678h
 mov [rdi+8192*4096],rax
 mov backCount,8191
 lea rcx,gpptLong
 lea rdx,gpptLong
 lea r8,gpptLong
 call GpPerfAppend
 GPPT_EQ backCount,8192
 GPPT_EQ word ptr [rdi+8191*4096+510],0
 GPPT_EQ word ptr [rdi+8191*4096+1022],0
 GPPT_EQ word ptr [rdi+8191*4096+1534],0
 lea rcx,gpptLong
 lea rdx,gpptLong
 lea r8,gpptLong
 call GpPerfAppend
 GPPT_EQ backCount,8192
 mov rax,1234567812345678h
 GPPT_EQ qword ptr [rdi+8192*4096],rax
 call GpCollectPerformance
 GPPT_EQ backCount,8192
 mov rax,1234567812345678h
 GPPT_EQ qword ptr [rdi+8192*4096],rax
 mov backCount,-1
 lea rcx,gpptLong
 lea rdx,gpptLong
 lea r8,gpptLong
 call GpPerfAppend
 GPPT_EQ backCount,-1
 xor ecx,ecx
 call ExitProcess
gpptFail:
 mov ecx,gpptStage
 call ExitProcess
GpuPerformanceTest ENDP
