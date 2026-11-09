.data
pdhDisk dq 0
chartLastTick dq 0
chartElapsed dq 0
diskPath dw '\','P','h','y','s','i','c','a','l','D','i','s','k','(','_','T','o','t','a','l',')','\','D','i','s','k',' ','B','y','t','e','s','/','s','e','c',0
.data?
chartMemStatus db 64 dup(?)
.code
CollectChartMetrics PROC
  sub rsp,28h
  mov eax,cpuValue
  cmp cpuValid,0
  jne ccmCpu
  mov eax,-1
ccmCpu:
  mov chartBack,eax
  mov eax,gpuValue
  cmp gpuValid,0
  jne ccmGpu
  mov eax,-1
ccmGpu:
  mov [chartBack+8],eax
  mov dword ptr [chartMemStatus],64
  lea rcx,chartMemStatus
  call GlobalMemoryStatusEx
  mov eax,dword ptr [chartMemStatus+4]
  mov [chartBack+4],eax
  mov dword ptr [chartBack+12],-1
  mov rcx,pdhDisk
  test rcx,rcx
  jz ccmNet
  mov edx,200h ; PDH_FMT_DOUBLE
  xor r8d,r8d
  lea r9,pdhValue
  call PdhGetFormattedCounterValue
  test eax,eax
  jnz ccmNet
  cmp dword ptr [pdhValue],1
  ja ccmNet
  cvttsd2si rax,qword ptr [pdhValue+8]
  test rax,rax
  js ccmNet
  shr rax,10
  mov ecx,0FFFFFFFEh
  cmp rax,rcx
  jbe ccmDiskFits
  mov eax,0FFFFFFFEh
ccmDiskFits:
  mov [chartBack+12],eax
ccmNet:
  call GetTickCount64
  mov rcx,rax
  sub rax,chartLastTick
  mov rdx,chartLastTick
  mov chartLastTick,rcx
  cmp netValid,0
  je ccmFirst
  test rdx,rdx
  jz ccmFirst
  test rax,rax
  jz ccmFirst
  mov chartElapsed,rax
  mov rax,netReceive
  add rax,netSend
  js ccmFirst
  mov ecx,1000
  mul rcx
  div chartElapsed
  shr rax,10
  mov [chartBack+16],eax
  jmp ccmDone
ccmFirst:
  mov dword ptr [chartBack+16],-1
ccmDone:
  add rsp,28h
  ret
CollectChartMetrics ENDP
