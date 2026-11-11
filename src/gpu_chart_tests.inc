.code
GpuChartTest PROC
  sub rsp,28h
  mov chartHead,0
  mov chartSamples,0
  mov chartAcceptedTick,0
  mov chartWasPaused,0
  mov frontMode,2
  mov telemetryPassTick,1000
  mov dword ptr [chartBack],23
  call ApplyTelemetryChart
  cmp chartSamples,0
  jne gpctFail1
  mov frontMode,4
  call ApplyTelemetryChart
  cmp chartSamples,1
  jne gpctFail1
  cmp dword ptr [chartData],23
  jne gpctFail1
  cmp qword ptr [chartTimes],1000
  jne gpctFail1
  call ApplyTelemetryChart
  cmp chartSamples,1
  jne gpctFail2
  mov frontMode,1
  mov telemetryPassTick,2000
  call ApplyTelemetryChart
  cmp chartSamples,1
  jne gpctFail2
  mov frontMode,0
  mov telemetryPassTick,3000
  mov dword ptr [chartBack],29
  call ApplyTelemetryChart
  cmp chartSamples,3
  jne gpctFail2
  cmp dword ptr [chartData+4],-1
  jne gpctFail2
  cmp dword ptr [chartData+8],29
  jne gpctFail2
  mov telemetryPassTick,9001
  call ApplyTelemetryChart
  cmp chartSamples,5
  jne gpctFail2
  cmp dword ptr [chartData+12],-1
  jne gpctFail2
  cmp qword ptr [chartTimes+32],9001
  jne gpctFail2
  xor ecx,ecx
  call ExitProcess
gpctFail1:
  mov ecx,9300
  call ExitProcess
gpctFail2:
  mov ecx,9301
  call ExitProcess
GpuChartTest ENDP
