extern SetFilePointerEx:proc, RegDeleteTreeW:proc
.data
modelStage dd 1
modelName dw 'm','o','d','e','l','.','e','x','e',0
modelNeedle dw 'n','e','e','d','l','e','.','e','x','e',0
modelSearch dw 'n','e','e','d','l','e',0
modelHistoryDir dw 'w','o','r','k','\','m','o','d','e','l','-','h','i','s','t','o','r','y',0
modelUnicode dw 442h,435h,441h,442h,0D83Dh,0DE80h,'.','e','x','e',0
modelRegName dw 'T','M','X','_','T','e','s','t',0
modelRegData dw '%','T','E','M','P','%','\',442h,435h,441h,442h,'.','e','x','e',0
modelFileName dw 'e','x','a','m','p','l','e','.','l','n','k',0
modelRegRoot dw 'S','o','f','t','w','a','r','e','\','T','M','X','T','e','s','t','-'
dw 'A','6','3','7','1','F','B','4','-','C','E','9','2','-','4','B','7','4',0
modelZero db 0
.code
ModelMap PROC
  lea rdx,rowMap
  mov rax,frontBuffer
  xor ecx,ecx
mmNext:
  cmp ecx,frontCount
  jae mmDone
  mov [rdx+rcx*8],rax
  add rax,4096
  inc ecx
  jmp mmNext
mmDone:
  mov visibleCount,ecx
  ret
ModelMap ENDP

ModelTest PROC
  push rbx
  push rsi
  push rdi
  push r12
  push r13
  sub rsp,60h
  xor ecx,ecx
  call GetModuleHandleW
  mov gInst,rax
  mov appLanguage,1
  call LoadUiStrings
  cmp word ptr [szProcess],041Fh
  jne mtFail
  mov appLanguage,0
  call LoadUiStrings
  cmp word ptr [szProcess],0050h
  jne mtFail
  mov modelStage,2
  call GetProcessHeap
  mov rcx,rax
  mov edx,8
  mov r8d,33554432
  call HeapAlloc
  test rax,rax
  jz mtFail
  mov frontBuffer,rax
  call GetProcessHeap
  mov rcx,rax
  mov edx,8
  mov r8d,33554432
  call HeapAlloc
  test rax,rax
  jz mtFail
  mov backBuffer,rax
  ; 5000 rows, seven children per parent. No 5000 real processes are created.
  mov frontCount,5000
  mov rdi,frontBuffer
  xor esi,esi
mtRows:
  mov rcx,rdi
  lea rdx,modelName
  call lstrcpyW
  lea eax,[rsi+100]
  mov [rdi+3584],eax
  lea eax,[rsi+1]
  mov [rdi+3600],rax
  mov dword ptr [rdi+3656],1
  mov qword ptr [rdi+3624],1048576
  mov qword ptr [rdi+3632],2097152
  test esi,esi
  jz mtNextRow
  lea eax,[rsi-1]
  xor edx,edx
  mov ecx,7
  div ecx
  add eax,100
  mov [rdi+3588],eax
mtNextRow:
  add rdi,4096
  inc esi
  cmp esi,5000
  jb mtRows
  call ModelMap
  call BuildProcessTree
  cmp visibleCount,5000
  jne mtFail
  mov rdi,frontBuffer
  mov rax,5242880000
  cmp [rdi+3720],rax
  jne mtFail
  cmp qword ptr [rdi+3736],5000
  jne mtFail
  mov modelStage,3
  mov collapsedPid,100
  mov collapsedBirth,1
  mov collapsedUsed,1
  call ModelMap
  call BuildProcessTree
  cmp visibleCount,1
  jne mtFail
  mov rcx,frontBuffer
  call CaptureTreeTargets
  cmp treeTargetCount,5000
  jne mtFail
  mov modelStage,4
  mov collapsedPid,0
  lea rcx,searchBuffer
  lea rdx,modelSearch
  call lstrcpyW
  mov rdi,frontBuffer
  add rdi,4096*4999
  mov rcx,rdi
  lea rdx,modelNeedle
  call lstrcpyW
  call ModelMap
  call BuildProcessTree
  cmp visibleCount,2
  jb mtFail
  cmp visibleCount,20
  ja mtFail
  ; A younger reused parent PID is never attached.
  mov modelStage,5
  mov word ptr [searchBuffer],0
  mov rdi,frontBuffer
  mov qword ptr [rdi+3600],9000
  call ModelMap
  call BuildProcessTree
  cmp qword ptr [rdi+4096+3680],0
  jne mtFail
  mov modelStage,6
  lea rcx,modelHistoryDir
  xor edx,edx
  call CreateDirectoryW
  lea rcx,storageDir
  lea rdx,modelHistoryDir
  call lstrcpyW
  mov configReady,1
  call InitHistoryStore
  ; Start clean even after a previous test was interrupted.
  mov historyCount,0
  lea rcx,historyRows
  call ClearRow
  mov rdi,frontBuffer
  mov rcx,rdi
  call ClearRow
  mov rcx,rdi
  lea rdx,modelUnicode
  call lstrcpyW
  mov dword ptr [rdi+3584],12345
  mov qword ptr [rdi+3600],777
  mov qword ptr [rdi+3608],100
  mov dword ptr [rdi+3660],1
  mov qword ptr [rdi+3640],1000
  mov historyNow,10000
  mov rcx,rdi
  call RecordHistory
  mov qword ptr [rdi+3608],200
  mov qword ptr [rdi+3640],4000
  mov historyNow,20000
  mov rcx,rdi
  call RecordHistory
  cmp qword ptr [historyRows+3656],100
  jne mtFail
  cmp qword ptr [historyRows+3680],3000
  jne mtFail
  call SaveHistoryStore
  cmp historySaveError,0
  jne mtFail
  mov modelStage,7
  mov historyCount,0
  call InitHistoryStore
  cmp historyCount,1
  jne mtFail
  cmp word ptr [historyRows],442h
  jne mtFail
  cmp qword ptr [historyRows+3656],100
  jne mtFail
  mov qword ptr [rdi+3608],1000
  mov rcx,rdi
  call RecordHistory
  cmp qword ptr [historyRows+3656],100
  jne mtFail
  ; Corrupt one byte without changing file length: checksum must reject it.
  mov modelStage,8
  lea rcx,historyPath
  mov edx,40000000h
  xor r8d,r8d
  xor r9d,r9d
  mov qword ptr [rsp+20h],3
  mov qword ptr [rsp+28h],80h
  mov qword ptr [rsp+30h],0
  call CreateFileW
  mov rbx,rax
  cmp rax,-1
  je mtFail
  mov rcx,rax
  mov edx,24
  xor r8d,r8d
  xor r9d,r9d
  call SetFilePointerEx
  mov rcx,rbx
  lea rdx,modelZero
  mov r8d,1
  lea r9,historyTransferred
  mov qword ptr [rsp+20h],0
  call WriteFile
  mov rcx,rbx
  call CloseHandle
  call InitHistoryStore
  cmp historyCount,0
  jne mtFail
  cmp historyLoadError,13
  jne mtFail
  ; Registry round trip is isolated from real Run keys by a test-only switch.
  mov modelStage,90
  mov startupTestMode,1
  mov startupOnce,0
  mov startupDisabled,0
  call MakeStartupKey
  mov rcx,-2147483647
  lea rdx,startupKeyPath
  xor r8d,r8d
  xor r9d,r9d
  mov qword ptr [rsp+20h],0
  mov qword ptr [rsp+28h],103h
  mov qword ptr [rsp+30h],0
  lea rax,startupKey
  mov [rsp+38h],rax
  mov qword ptr [rsp+40h],0
  call RegCreateKeyExW
  mov modelStage,eax
  add modelStage,9000
  test eax,eax
  jnz mtFail
  mov rcx,startupKey
  lea rdx,modelRegName
  xor r8d,r8d
  mov r9d,2
  lea rax,modelRegData
  mov [rsp+20h],rax
  mov qword ptr [rsp+28h],32
  call RegSetValueExW
  mov r12d,eax
  mov rcx,startupKey
  call RegCloseKey
  mov modelStage,r12d
  add modelStage,9100
  test r12d,r12d
  jnz mtFail
  mov ecx,32
  lea rdx,modelRegData
  mov r8d,2
  call StartupHash
  mov startupActionHash,rax
  mov startupActionRoot,-2147483647
  mov startupActionView,100h
  mov startupActionOnce,0
  mov startupActionDisabled,0
  lea rcx,startupActionName
  lea rdx,modelRegName
  call lstrcpyW
  mov actionCode,2400
  call ChangeStartupRegistry
  add modelStage,1
  test eax,eax
  jnz mtFail
  mov startupActionDisabled,1
  mov actionCode,2401
  call ChangeStartupRegistry
  add modelStage,1
  test eax,eax
  jnz mtFail
  ; Compare full original type + bytes via the same collector hash.
  mov startupRoot,-2147483647
  mov startupView,100h
  mov startupDisabled,0
  mov startupOnce,0
  mov backCount,0
  call CollectRegistrySource
  mov modelStage,94
  cmp backCount,1
  jne mtFail
  mov rdi,backBuffer
  mov rax,[rdi+3640]
  mov modelStage,95
  cmp rax,startupActionHash
  jne mtFail
  mov modelStage,10
  lea rcx,startupTestFolder
  xor edx,edx
  call CreateDirectoryW
  lea rcx,startupActionName
  lea rdx,modelFileName
  call lstrcpyW
  lea rcx,startupActionPath
  lea rdx,startupTestFolder
  call lstrcpyW
  lea rcx,startupActionPath
  lea rdx,startupSlash
  call lstrcatW
  lea rcx,startupActionPath
  lea rdx,modelFileName
  call lstrcatW
  lea rcx,startupActionPath
  mov edx,40000000h
  xor r8d,r8d
  xor r9d,r9d
  mov qword ptr [rsp+20h],2
  mov qword ptr [rsp+28h],80h
  mov qword ptr [rsp+30h],0
  call CreateFileW
  cmp rax,-1
  je mtFail
  mov rcx,rax
  call CloseHandle
  lea rcx,startupActionPath
  lea rdx,startupFindData
  call FindFirstFileW
  cmp rax,-1
  je mtFail
  mov rcx,rax
  call FindClose
  mov rax,qword ptr [startupFindData+4]
  mov startupActionHash,rax
  mov startupActionKind,2
  mov startupActionDisabled,0
  mov actionCode,2400
  call ChangeStartupFile
  test eax,eax
  jnz mtFail
  ; Destination from disable becomes exact source for restore.
  lea rcx,startupActionPath
  lea rdx,startupPath
  call lstrcpyW
  mov startupActionDisabled,1
  mov actionCode,2401
  call ChangeStartupFile
  test eax,eax
  jnz mtFail
  mov modelStage,11
  mov chartHead,0
  mov chartSamples,0
  xor esi,esi
mtChart:
  mov chartBack,esi
  call AddChartSample
  inc esi
  cmp esi,181
  jb mtChart
  cmp chartSamples,180
  jne mtFail
  cmp chartHead,1
  jne mtFail
  cmp chartData,180
  jne mtFail
  mov modelStage,13
  mov uiDpi,96
  call RecalcUiScale
  cmp uiContentLeft,224
  jne mtFail
  mov uiDpi,120
  call RecalcUiScale
  cmp uiContentLeft,280
  jne mtFail
  mov uiDpi,144
  call RecalcUiScale
  cmp uiContentLeft,336
  jne mtFail
  mov uiDpi,192
  call RecalcUiScale
  cmp uiContentLeft,448
  jne mtFail
  mov uiDpi,96
  call RecalcUiScale
  call ProcessPresentationTests
  test eax,eax
  jnz mtFail
  mov modelStage,20
  call IconQueueTests
  test eax,eax
  jnz mtFail
  call MenuThemeTests
  test eax,eax
  jnz mtFail
  mov modelStage,12
  call InitTaskScheduler
  cmp taskFolder,0
  je mtFail
  mov backCount,0
  mov rcx,taskFolder
  call CollectTaskFolder
  cmp backCount,0
  je mtFail
  mov ecx,backCount
  mov rdx,backBuffer
mtFindTask:
  cmp dword ptr [rdx+3620],4
  je mtTaskFound
  add rdx,4096
  dec ecx
  jnz mtFindTask
  jmp mtFail
mtTaskFound:
  mov rcx,taskFolder
  call ReleaseCom
  mov rcx,taskService
  call ReleaseCom
  call CoUninitialize
  mov modelStage,0
mtFail:
  mov rcx,-2147483647
  lea rdx,modelRegRoot
  call RegDeleteTreeW ; only the named test fixture key
  mov ecx,modelStage
  call ExitProcess
ModelTest ENDP
