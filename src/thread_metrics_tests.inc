; Isolated native metrics tests. Live sampling reads only this process and
; creates/stops only its own worker thread; no production collector is started.
.data
threadMetricsTestStage dd 0
threadMetricsTestBytes dd 0
threadMetricsAssertions dd 0
threadMetricsLiveStop dd 0
threadMetricsLiveBusy dd 1
threadMetricsLiveTid dd 0
threadMetricsLiveCpu dd 0
threadMetricsIdleCpu dd 0
threadMetricsLiveHandle dq 0
threadMetricsLiveCounter dq 0
threadMetricsLivePreviousCpu dq 0
threadMetricsLivePreviousTick dq 0
threadMetricsLiveDeltaCpu dq 0
threadMetricsLiveDeltaTick dq 0
threadMetricsTestFormat db 'Thread CPU metrics: exit=%u assertions=%u busyTenths=%u idleTenths=%u',13,10,0
threadMetricsClockFormat db 'Live busy interval: CPU100ns=%I64u elapsedMs=%I64u tolerance100ns=%I64u',13,10,0
threadMetricsNa dw 'N','/','A',0
threadMetricsEn dw '1','2','.','5','%',0
threadMetricsRu dw '1','2',',','5','%',0
.data?
threadMetricsTestOutput db 160 dup(?)
align 8
threadMetricsFixtureRow db 4096 dup(?)
threadMetricsSortRow db 4096 dup(?)
.code
ThreadMetricsLiveWorker PROC
  sub rsp,28h
tmlwNext:
  cmp threadMetricsLiveStop,0
  jne tmlwDone
  cmp threadMetricsLiveBusy,0
  je tmlwIdle
  inc threadMetricsLiveCounter
  jmp tmlwNext
tmlwIdle:
  mov ecx,10
  call Sleep
  jmp tmlwNext
tmlwDone:
  xor eax,eax
  add rsp,28h
  ret
ThreadMetricsLiveWorker ENDP

ThreadMetricsTest PROC
  sub rsp,48h
  mov threadMetricsTestStage,3000
  lea rcx,unavailable
  lea rdx,threadMetricsNa
  call lstrcpyW
  call ThreadMetricsFixtureTests
  test eax,eax
  jz tmtFixturesPassed
  mov threadMetricsTestStage,eax
  jmp tmtFinish
tmtFixturesPassed:
  mov threadMetricsTestStage,3000
  call GetProcessHeap
  mov rcx,rax
  mov edx,8
  mov r8d,16777216
  call HeapAlloc
  mov backBuffer,rax
  test rax,rax
  jz tmtFinish
  call GetCurrentProcessId
  mov collectInspectedPid,eax
  call GetCurrentProcess
  mov rcx,rax
  lea rdx,timeCreate
  lea r8,timeExit
  lea r9,timeKernel
  lea rax,timeUser
  mov [rsp+20h],rax
  call GetProcessTimes
  test eax,eax
  jz tmtFinish
  mov rax,timeCreate
  mov collectInspectedBirth,rax
  mov collectMode,1
  mov collectGeneration,1
  xor ecx,ecx
  xor edx,edx
  lea r8,ThreadMetricsLiveWorker
  xor r9d,r9d
  mov qword ptr [rsp+20h],0
  lea rax,threadMetricsLiveTid
  mov [rsp+28h],rax
  call CreateThread
  mov threadMetricsLiveHandle,rax
  test rax,rax
  jz tmtFinish
  call CollectThreads
  call GetLiveThreadMetricsSlot
  mov rcx,[rax+8]
  mov threadMetricsLivePreviousCpu,rcx
  mov rcx,[rax+16]
  mov threadMetricsLivePreviousTick,rcx
  mov ecx,500
  call Sleep
  call CollectThreads
  call GetLiveThreadMetricsSlot
  mov rcx,[rax+8]
  sub rcx,threadMetricsLivePreviousCpu
  mov threadMetricsLiveDeltaCpu,rcx
  mov rcx,[rax+16]
  sub rcx,threadMetricsLivePreviousTick
  mov threadMetricsLiveDeltaTick,rcx
  call FindLiveThreadMetricsRow
  mov threadMetricsTestStage,3001
  test rax,rax
  jz tmtFinish
  cmp dword ptr [rax+3672],1
  jne tmtFinish
  mov ecx,[rax+3900]
  mov threadMetricsLiveCpu,ecx
  test ecx,ecx
  jz tmtFinish
  ; Allow the busy interval to finish before establishing the idle baseline.
  mov threadMetricsLiveBusy,0
  mov ecx,50
  call Sleep
  call CollectThreads
  mov ecx,400
  call Sleep
  call CollectThreads
  call FindLiveThreadMetricsRow
  mov threadMetricsTestStage,3002
  test rax,rax
  jz tmtFinish
  cmp dword ptr [rax+3672],1
  jne tmtFinish
  mov ecx,[rax+3900]
  mov threadMetricsIdleCpu,ecx
  cmp ecx,threadMetricsLiveCpu
  jae tmtFinish
  mov threadMetricsTestStage,0
tmtFinish:
  mov threadMetricsLiveStop,1
  mov rcx,threadMetricsLiveHandle
  test rcx,rcx
  jz tmtReport
  mov edx,2000
  call WaitForSingleObject
  mov rcx,threadMetricsLiveHandle
  call CloseHandle
tmtReport:
  lea rcx,threadMetricsTestOutput
  lea rdx,threadMetricsTestFormat
  mov r8d,threadMetricsTestStage
  mov r9d,threadMetricsAssertions
  mov eax,threadMetricsLiveCpu
  mov [rsp+20h],rax
  mov eax,threadMetricsIdleCpu
  mov [rsp+28h],rax
  call wsprintfA
  mov [rsp+30h],eax
  mov ecx,-11
  call GetStdHandle
  mov rcx,rax
  lea rdx,threadMetricsTestOutput
  mov r8d,[rsp+30h]
  lea r9,threadMetricsTestBytes
  mov qword ptr [rsp+20h],0
  call WriteFile
  lea rcx,threadMetricsTestOutput
  lea rdx,threadMetricsClockFormat
  mov r8,threadMetricsLiveDeltaCpu
  mov r9,threadMetricsLiveDeltaTick
  mov rax,threadMetricsTolerance100ns
  mov [rsp+20h],rax
  call wsprintfA
  mov [rsp+30h],eax
  mov ecx,-11
  call GetStdHandle
  mov rcx,rax
  lea rdx,threadMetricsTestOutput
  mov r8d,[rsp+30h]
  lea r9,threadMetricsTestBytes
  mov qword ptr [rsp+20h],0
  call WriteFile
  mov ecx,threadMetricsTestStage
  call ExitProcess
ThreadMetricsTest ENDP

FindLiveThreadMetricsRow PROC
  mov ecx,backCount
  mov rax,backBuffer
fltmNext:
  test ecx,ecx
  jz fltmMissing
  mov edx,threadMetricsLiveTid
  cmp edx,[rax+3584]
  je fltmDone
  add rax,4096
  dec ecx
  jmp fltmNext
fltmMissing:
  xor eax,eax
fltmDone:
  ret
FindLiveThreadMetricsRow ENDP

; Diagnostic only: report the exact clock interval the production cache used.
GetLiveThreadMetricsSlot PROC
  mov eax,threadMetricsLiveTid
  shr eax,2
  and eax,8191
  shl eax,6
  lea rcx,threadMetricsCache
  add rax,rcx
  ret
GetLiveThreadMetricsSlot ENDP

; Hand-computed samples: expected tenths never use the production denominator
; or formatter. A wrong identity, clock, arithmetic branch or cache key fails.
TM_CPU_SAMPLE MACRO tickValue,kernelValue,userValue,wantValue,stageValue
  mov threadMetricsTestStage,stageValue
  lea rcx,threadMetricsFixtureRow
  mov rdx,tickValue
  mov r8,kernelValue
  mov r9,userValue
  call RecordThreadCpu
  IF wantValue LT 0
    cmp dword ptr [threadMetricsFixtureRow+3672],0
    jne tmftFail
    lea rcx,threadMetricsFixtureRow+3072
    lea rdx,threadMetricsNa
    call lstrcmpW
    test eax,eax
    jnz tmftFail
  ELSE
    cmp dword ptr [threadMetricsFixtureRow+3672],1
    jne tmftFail
    cmp dword ptr [threadMetricsFixtureRow+3900],wantValue
    jne tmftFail
  ENDIF
  inc threadMetricsAssertions
ENDM

ThreadMetricsFixtureTests PROC
  sub rsp,28h
  mov appLanguage,0
  mov ecx,42
  mov edx,100
  mov r8d,7
  mov r9d,1
  call SetThreadMetricsContext
  mov dword ptr [threadMetricsFixtureRow+3584],100
  mov dword ptr [threadMetricsFixtureRow+3588],42
  mov qword ptr [threadMetricsFixtureRow+3600],500
  mov qword ptr [threadMetricsFixtureRow+3616],100
  ; First observation is unknown. 1,250,000 CPU ticks / 10,000,000 = 12.5%.
  TM_CPU_SAMPLE 1000,10000,0,-1,3101
  TM_CPU_SAMPLE 2000,1260000,0,125,3102
  mov threadMetricsTestStage,3103
  lea rcx,threadMetricsFixtureRow+3072
  lea rdx,threadMetricsEn
  call lstrcmpW
  test eax,eax
  jnz tmftFail
  inc threadMetricsAssertions
  mov appLanguage,1
  TM_CPU_SAMPLE 3000,2510000,0,125,3104
  mov threadMetricsTestStage,3105
  lea rcx,threadMetricsFixtureRow+3072
  lea rdx,threadMetricsRu
  call lstrcmpW
  test eax,eax
  jnz tmftFail
  inc threadMetricsAssertions
  mov appLanguage,0
  TM_CPU_SAMPLE 4000,2510000,0,0,3106
  TM_CPU_SAMPLE 4500,7510000,0,1000,3139
  ; One full CPU equals 12.5% of an eight-CPU machine.
  mov ecx,42
  mov edx,100
  mov r8d,8
  mov r9d,8
  call SetThreadMetricsContext
  TM_CPU_SAMPLE 5000,10000000,0,-1,3107
  TM_CPU_SAMPLE 6000,20000000,0,125,3108
  ; New TID, reused TID, and a direct-map collision all start unknown.
  mov dword ptr [threadMetricsFixtureRow+3584],104
  mov qword ptr [threadMetricsFixtureRow+3600],501
  TM_CPU_SAMPLE 7000,100,0,-1,3109
  TM_CPU_SAMPLE 8000,1000100,0,12,3110
  mov qword ptr [threadMetricsFixtureRow+3600],502
  TM_CPU_SAMPLE 9000,100,0,-1,3111
  TM_CPU_SAMPLE 10000,100,0,0,3112
  mov dword ptr [threadMetricsFixtureRow+3584],32872 ; 104 + 8192*4
  mov qword ptr [threadMetricsFixtureRow+3600],503
  TM_CPU_SAMPLE 11000,100,0,-1,3113
  mov dword ptr [threadMetricsFixtureRow+3584],104
  mov qword ptr [threadMetricsFixtureRow+3600],502
  TM_CPU_SAMPLE 12000,100,0,-1,3114
  ; Context epoch resets cover owner PID, owner birth, navigation generation.
  mov ecx,43
  mov edx,100
  mov r8d,8
  mov r9d,8
  call SetThreadMetricsContext
  mov dword ptr [threadMetricsFixtureRow+3588],43
  TM_CPU_SAMPLE 13000,100,0,-1,3115
  TM_CPU_SAMPLE 14000,100,0,0,3116
  mov ecx,43
  mov edx,101
  mov r8d,8
  mov r9d,8
  call SetThreadMetricsContext
  mov qword ptr [threadMetricsFixtureRow+3616],101
  TM_CPU_SAMPLE 15000,100,0,-1,3117
  mov ecx,43
  mov edx,101
  mov r8d,9
  mov r9d,8
  call SetThreadMetricsContext
  TM_CPU_SAMPLE 16000,100,0,-1,3118
  ; Topology changes, failed reads, zero/rollback clocks and CPU rollbacks.
  mov threadMetricsProcessorCount,1
  TM_CPU_SAMPLE 17000,100,0,-1,3119
  lea rcx,threadMetricsFixtureRow
  call InvalidateThreadCpu
  TM_CPU_SAMPLE 18000,100,0,-1,3120
  TM_CPU_SAMPLE 18000,100,0,-1,3121
  TM_CPU_SAMPLE 17000,100,0,-1,3122
  TM_CPU_SAMPLE 18000,90,0,-1,3123
  TM_CPU_SAMPLE 19000,100,0,0,3124
  ; Long gaps rebaseline rather than displaying an old interval as current CPU.
  TM_CPU_SAMPLE 49001,100,0,-1,3125
  TM_CPU_SAMPLE 50001,100,0,0,3126
  ; Quantized clocks can exceed one-CPU wall time by up to two clock quanta.
  ; Clamp a small 15.625ms excess, but reject a gross 100ms excess.
  mov threadMetricsTolerance100ns,312500
  TM_CPU_SAMPLE 51001,10156350,0,1000,3127
  TM_CPU_SAMPLE 52001,21156350,0,-1,3138
  ; Overflowing kernel+user, invalid topology/time/identity.
  TM_CPU_SAMPLE 53001,0FFFFFFFFFFFFFFFFh,1,-1,3128
  TM_CPU_SAMPLE 54001,100,0,-1,3129
  mov threadMetricsProcessorCount,0
  TM_CPU_SAMPLE 55001,100,0,-1,3130
  mov threadMetricsProcessorCount,1
  TM_CPU_SAMPLE 0,100,0,-1,3131
  mov qword ptr [threadMetricsFixtureRow+3600],0
  TM_CPU_SAMPLE 55001,100,0,-1,3132
  mov qword ptr [threadMetricsFixtureRow+3600],502
  mov dword ptr [threadMetricsFixtureRow+3588],99
  TM_CPU_SAMPLE 56001,100,0,-1,3133
  mov dword ptr [threadMetricsFixtureRow+3588],43
  TM_CPU_SAMPLE 57001,100,0,-1,3134
  ; Kernel and user deltas both contribute; highest CPU count stays bounded.
  TM_CPU_SAMPLE 58001,625100,625000,125,3135
  mov threadMetricsProcessorCount,0FFFFFFFFh
  TM_CPU_SAMPLE 59001,625100,625000,-1,3136
  TM_CPU_SAMPLE 60001,10625100,625000,0,3137
  ; CPU numeric sorting, availability, direction, and full-identity ties.
  mov threadMetricsTestStage,3140
  mov frontMode,1
  mov sortColumn,6
  mov sortDescending,0
  mov dword ptr [threadMetricsFixtureRow+3672],1
  mov dword ptr [threadMetricsSortRow+3672],1
  lea rcx,threadMetricsFixtureRow
  mov edx,99
  call FormatProcessCpu
  lea rcx,threadMetricsSortRow
  mov edx,100
  call FormatProcessCpu
  lea rcx,threadMetricsFixtureRow
  lea rdx,threadMetricsSortRow
  call CompareRows
  cmp eax,-1
  jne tmftFail
  inc threadMetricsAssertions
  mov sortDescending,1
  lea rcx,threadMetricsFixtureRow
  lea rdx,threadMetricsSortRow
  call CompareRows
  cmp eax,1
  jne tmftFail
  inc threadMetricsAssertions
  mov sortDescending,0
  mov dword ptr [threadMetricsSortRow+3672],0
  lea rcx,threadMetricsFixtureRow
  lea rdx,threadMetricsSortRow
  call CompareRows
  cmp eax,-1
  jne tmftFail
  inc threadMetricsAssertions
  mov dword ptr [threadMetricsSortRow+3672],1
  mov dword ptr [threadMetricsSortRow+3900],99
  mov dword ptr [threadMetricsSortRow+3584],104
  mov qword ptr [threadMetricsSortRow+3600],503
  lea rcx,threadMetricsFixtureRow
  lea rdx,threadMetricsSortRow
  call CompareRows
  cmp eax,-1
  jne tmftFail
  inc threadMetricsAssertions
  mov dword ptr [threadMetricsSortRow+3584],100
  lea rcx,threadMetricsFixtureRow
  lea rdx,threadMetricsSortRow
  call CompareRows
  cmp eax,1
  jne tmftFail
  inc threadMetricsAssertions
  mov threadMetricsTestStage,0
tmftFail:
  mov appLanguage,0
  mov frontMode,0
  mov sortColumn,0
  mov sortDescending,0
  mov eax,threadMetricsTestStage
  add rsp,28h
  ret
ThreadMetricsFixtureTests ENDP
