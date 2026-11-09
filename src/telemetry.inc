extern PdhOpenQueryW:proc, PdhAddEnglishCounterW:proc, PdhCollectQueryData:proc
extern PdhGetFormattedCounterValue:proc, PdhCloseQuery:proc
extern GetIfTable2:proc, FreeMibTable:proc
.data
align 8
pdhQuery dq 0
pdhCpu dq 0
pdhGpu dq 0
pdhGpuMem dq 0
pdhStatus dd 0
cpuValue dd 0
cpuValid dd 0
gpuValue dd 0
gpuValid dd 0
videoMiB dq 0
videoValid dd 0
arrayBytes dd 524288
arrayCount dd 0
.data?
align 8
counterArray db 524288 dup(?)
.data
.data?
align 8
engineValues dq 256 dup(?)
.data
.data?
align 8
engineNames db 131072 dup(?)
.data
engineCount dd 0
gpuSum dq 0
.data?
align 8
pdhValue db 32 dup(?)
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
gpuPath dw '\','G','P','U',' ','E','n','g','i','n','e','(','*',')','\','U','t','i','l','i','z','a','t','i','o','n',' ','P','e','r','c','e','n','t','a','g','e',0
gpuMemPath dw '\','G','P','U',' ','A','d','a','p','t','e','r',' ','M','e','m','o','r','y','(','*',')','\','D','e','d','i','c','a','t','e','d',' ','U','s','a','g','e',0
luidMarker dw 'l','u','i','d','_',0
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
  lea rdx,gpuPath
  xor r8d,r8d
  lea r9,pdhGpu
  call PdhAddEnglishCounterW
  mov rcx,pdhQuery
  lea rdx,gpuMemPath
  xor r8d,r8d
  lea r9,pdhGpuMem
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
  je ctNet
  mov rcx,pdhQuery
  call PdhCollectQueryData
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
  mov rcx,pdhGpu
  call ReadGpuEngines
  mov rcx,pdhGpuMem
  call ReadGpuMemory
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

extern PdhGetFormattedCounterArrayW:proc
ReadGpuEngines PROC
  push rbx
  push rsi
  push rdi
  push r12
  push r13
  sub rsp,30h
  mov gpuValid,0
  mov engineCount,0
  mov arrayBytes,524288
  mov edx,200h
  lea r8,arrayBytes
  lea r9,arrayCount
  lea rax,counterArray
  mov [rsp+20h],rax
  call PdhGetFormattedCounterArrayW
  test eax,eax
  jnz rgeDone
  lea rsi,counterArray
  mov ebx,arrayCount
rgeNext:
  test ebx,ebx
  jz rgeMax
  cmp dword ptr [rsi+8],1
  ja rgeAdvance
  mov rcx,[rsi]
  lea rdx,luidMarker
  call StrStrIW
  test rax,rax
  jz rgeAdvance
  mov rdi,rax
  xor r12d,r12d
  lea r13,engineNames
rgeFind:
  cmp r12d,engineCount
  jae rgeNew
  mov rcx,rdi
  mov rdx,r13
  call StrCmpLogicalW
  test eax,eax
  jz rgeAdd
  inc r12d
  add r13,512
  jmp rgeFind
rgeNew:
  cmp r12d,256
  jae rgeAdvance
  inc engineCount
  lea rdx,engineValues
  mov qword ptr [rdx+r12*8],0
  mov rcx,r13
  mov rdx,rdi
  mov r8d,256
  call lstrcpynW
rgeAdd:
  lea rdx,engineValues
  movsd xmm0,qword ptr [rdx+r12*8]
  addsd xmm0,qword ptr [rsi+16]
  movsd qword ptr [rdx+r12*8],xmm0
rgeAdvance:
  add rsi,24
  dec ebx
  jmp rgeNext
rgeMax:
  xor ecx,ecx
  pxor xmm0,xmm0
  lea rdx,engineValues
rgeMaxNext:
  cmp ecx,engineCount
  jae rgeConvert
  maxsd xmm0,qword ptr [rdx+rcx*8]
  inc ecx
  jmp rgeMaxNext
rgeConvert:
  cvttsd2si eax,xmm0
  cmp eax,100
  jbe rgeStore
  mov eax,100
rgeStore:
  mov gpuValue,eax
  mov gpuValid,1
rgeDone:
  add rsp,30h
  pop r13
  pop r12
  pop rdi
  pop rsi
  pop rbx
  ret
ReadGpuEngines ENDP

ReadGpuMemory PROC
  sub rsp,38h
  mov videoValid,0
  mov arrayBytes,524288
  mov edx,200h
  lea r8,arrayBytes
  lea r9,arrayCount
  lea rax,counterArray
  mov [rsp+20h],rax
  call PdhGetFormattedCounterArrayW
  test eax,eax
  jnz rgmDone
  lea rdx,counterArray
  mov ecx,arrayCount
  pxor xmm0,xmm0
rgmNext:
  test ecx,ecx
  jz rgmConvert
  cmp dword ptr [rdx+8],1
  ja rgmAdvance
  addsd xmm0,qword ptr [rdx+16]
rgmAdvance:
  add rdx,24
  dec ecx
  jmp rgmNext
rgmConvert:
  cvttsd2si rax,xmm0
  shr rax,20
  mov videoMiB,rax
  mov videoValid,1
rgmDone:
  add rsp,38h
  ret
ReadGpuMemory ENDP

