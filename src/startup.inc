extern RegOpenKeyExW:proc, RegEnumValueW:proc, RegCloseKey:proc
.data
align 8
startupKey dq 0
startupRoot dq 0
startupIndex dd 0
startupNameSize dd 0
startupDataSize dd 0
startupType dd 0
.data?
align 8
startupName dw 256 dup(?)
.data
.data?
align 8
startupData dw 16384 dup(?)
.data
runKey dw 'S','o','f','t','w','a','r','e','\','M','i','c','r','o','s','o','f','t','\','W','i','n','d','o','w','s','\','C','u','r','r','e','n','t','V','e','r','s','i','o','n','\','R','u','n',0
startupSource dq 0
.code
CollectStartup PROC
  sub rsp,28h
  mov backCount,0
  mov startupRoot,-2147483647
  lea rax,hkcuText
  mov startupSource,rax
  call CollectRunKey
  mov startupRoot,-2147483646
  lea rax,hklmText
  mov startupSource,rax
  call CollectRunKey
  add rsp,28h
  ret
CollectStartup ENDP

CollectRunKey PROC
  push rbx
  push rsi
  push rdi
  sub rsp,50h
  mov rcx,startupRoot
  lea rdx,runKey
  xor r8d,r8d
  mov r9d,201h
  lea rax,startupKey
  mov [rsp+20h],rax
  call RegOpenKeyExW
  test eax,eax
  jnz crkDone
  mov startupIndex,0
crkNext:
  mov startupNameSize,256
  mov startupDataSize,32766
  mov rcx,startupKey
  mov edx,startupIndex
  lea r8,startupName
  lea r9,startupNameSize
  mov qword ptr [rsp+20h],0
  lea rax,startupType
  mov [rsp+28h],rax
  lea rax,startupData
  mov [rsp+30h],rax
  lea rax,startupDataSize
  mov [rsp+38h],rax
  call RegEnumValueW
  test eax,eax
  jnz crkClose
  cmp backCount,4096
  jae crkClose
  mov edi,backCount
  shl rdi,12
  add rdi,backBuffer
  mov rcx,rdi
  call ClearRow
  mov rcx,rdi
  lea rdx,startupName
  mov r8d,256
  call lstrcpynW
  mov eax,startupDataSize
  and eax,0FFFFFFFEh
  lea rdx,startupData
  mov word ptr [rdx+rax],0
  lea rcx,[rdi+512]
  mov r8d,256
  call lstrcpynW
  lea rcx,[rdi+1024]
  mov rdx,startupSource
  call lstrcpyW
  mov ecx,startupType
  lea rdx,[rdi+1536]
  call StoreDecimal
  mov rax,startupRoot
  mov [rdi+3600],rax
  inc backCount
  inc startupIndex
  jmp crkNext
crkClose:
  mov rcx,startupKey
  call RegCloseKey
crkDone:
  add rsp,50h
  pop rdi
  pop rsi
  pop rbx
  ret
CollectRunKey ENDP


