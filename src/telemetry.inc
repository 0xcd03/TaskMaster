extern PdhOpenQueryW:proc, PdhAddEnglishCounterW:proc, PdhCollectQueryData:proc
extern PdhGetFormattedCounterValue:proc, PdhCloseQuery:proc
extern GetIfTable2:proc, FreeMibTable:proc
.data
align 8
pdhQuery dq 0
pdhCpu dq 0
pdhStatus dd 0
telemetryPassTick dq 0
gpuTelemetrySequence dq 0
cpuValue dd 0
cpuValid dd 0
gpuValue dd 0
gpuValid dd 0
videoMiB dq 0
videoValid dd 0
.data?
align 8
pdhValue db 32 dup(?)
gpuTelemetryHeader db GP_HEADER_SIZE dup(?)
.data
ifaceTable dq 0
netIn dq 0
netOut dq 0
netPrevIn dq 0
netPrevOut dq 0
netReceive dq 0
netSend dq 0
netValid dd 0
cpuPath dw '\','P','r','o','c','e','s','s','o','r','(','_','T','o','t','a','l',')','\','%',' ','P','r','o','c','e','s','s','o','r',' ','T','i','m','e',0
.code
InitTelemetry PROC
  sub rsp,38h
  xor ecx,ecx
  xor edx,edx
  lea r8,pdhQuery
  call PdhOpenQueryW
  test eax,eax
  jnz itDone
  mov rcx,pdhQuery
  lea rdx,cpuPath
  xor r8d,r8d
  lea r9,pdhCpu
  call PdhAddEnglishCounterW
  mov rcx,pdhQuery
  lea rdx,diskPath
  xor r8d,r8d
  lea r9,pdhDisk
  call PdhAddEnglishCounterW
  mov rcx,pdhQuery
  call PdhCollectQueryData
itDone:
  add rsp,38h
  ret
InitTelemetry ENDP

CollectTelemetry PROC
  sub rsp,48h
  mov cpuValid,0
  mov netValid,0
  cmp pdhQuery,0
  je ctGpu
  mov rcx,pdhQuery
  call PdhCollectQueryData
  mov pdhStatus,eax
  test eax,eax
  jnz ctGpu
  mov rcx,pdhCpu
  mov edx,200h
  xor r8d,r8d
  lea r9,pdhValue
  call PdhGetFormattedCounterValue
  test eax,eax
  jnz ctGpu
  cmp dword ptr [pdhValue],1
  ja ctGpu
  cvttsd2si eax,qword ptr [pdhValue+8]
  mov cpuValue,eax
  mov cpuValid,1
ctGpu:
  call CopyGpuTelemetry
ctNet:
  lea rcx,ifaceTable
  call GetIfTable2
  test eax,eax
  jnz ctDone
  mov netValid,1
  mov rax,ifaceTable
  mov ecx,[rax]
  xor edx,edx
  lea r8,netIn
  lea r9,netOut
  call NetSumRows
  mov rax,netIn
  sub rax,netPrevIn
  mov netReceive,rax
  mov rax,netOut
  sub rax,netPrevOut
  mov netSend,rax
  mov rax,netIn
  mov netPrevIn,rax
  mov rax,netOut
  mov netPrevOut,rax
  mov rcx,ifaceTable
  call FreeMibTable
ctDone:
  call CollectChartMetrics
  add rsp,48h
  ret
CollectTelemetry ENDP
NetSumRows PROC
  mov netIn,0
  mov netOut,0
  mov r10d,[rax]
  lea r11,[rax+8]
nsLoop:
  test r10d,r10d
  jz nsDone
  mov rax,[r11+1208]
  add netIn,rax
  mov rax,[r11+1280]
  add netOut,rax
  add r11,1352
  dec r10d
  jmp nsLoop
nsDone:
  ret
NetSumRows ENDP

; Collector consumes only a bounded copied header, never the UI frame/provider.
CopyGpuTelemetry PROC
  sub rsp,28h
  mov gpuValid,0
  mov videoValid,0
  lea rcx,gpuTelemetryHeader
  call GpWorkerCopySummary
  test eax,eax
  jnz cgtDone
  mov rax,qword ptr [gpuTelemetryHeader+GP_S_SEQUENCE]
  mov gpuTelemetrySequence,rax
  cmp dword ptr [gpuTelemetryHeader+GP_S_UTIL_STATUS],GP_STATUS_VALID
  jne cgtMemory
  mov eax,dword ptr [gpuTelemetryHeader+GP_S_UTIL]
  add eax,50
  xor edx,edx
  mov ecx,100
  div ecx
  mov gpuValue,eax
  mov gpuValid,1
cgtMemory:
  cmp dword ptr [gpuTelemetryHeader+GP_S_DEDICATED_STATUS],GP_STATUS_VALID
  jne cgtDone
  mov rax,qword ptr [gpuTelemetryHeader+GP_S_DEDICATED]
  shr rax,20
  mov videoMiB,rax
  mov videoValid,1
cgtDone:
  add rsp,28h
  ret
CopyGpuTelemetry ENDP

CloseTelemetry PROC
  sub rsp,28h
  mov rcx,pdhQuery
  mov pdhQuery,0
  mov pdhCpu,0
  mov pdhDisk,0
  test rcx,rcx
  jz cltDone
  call PdhCloseQuery
cltDone:
  add rsp,28h
  ret
CloseTelemetry ENDP

