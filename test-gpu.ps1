param()
$ErrorActionPreference='Stop'
$root=$PSScriptRoot
. (Join-Path $root 'native-test-process.ps1')
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if(-not $vs){throw 'Visual Studio C++ Build Tools is required.'}
$vc=Join-Path $vs 'VC/Auxiliary/Build/vcvars64.bat'
$work=Join-Path $root ('work/gpu-tests-'+[Guid]::NewGuid().ToString('N'))
[void][IO.Directory]::CreateDirectory($work)
$entries=@('GpuModelTest','GpuProviderTest','GpuWorkerTest','GpuUiTest','GpuPerformanceTest','GpuChartTest')
$obj=Join-Path $work 'gpu.obj'
$res=Join-Path $work 'gpu.res'
$libs='kernel32.lib user32.lib gdi32.lib comctl32.lib psapi.lib advapi32.lib shell32.lib wtsapi32.lib shlwapi.lib pdh.lib iphlpapi.lib dwmapi.lib dbghelp.lib d2d1.lib dwrite.lib ole32.lib oleaut32.lib uuid.lib uxtheme.lib dxgi.lib'
$cmd="call `"$vc`" >nul && ml64 /nologo /D GP_TESTS /I src /c /Fo`"$obj`" src\taskmanager.asm && rc /nologo /d PREVIEW /fo`"$res`" taskmanager.rc"
foreach($entry in $entries){$exe=Join-Path $work ($entry+'.exe'); $cmd+=" && link /nologo /SUBSYSTEM:CONSOLE /ENTRY:$entry /OUT:`"$exe`" `"$obj`" `"$res`" $libs"}
Write-Output "GPU owned evidence: $work"
Push-Location $root
try {
 & cmd.exe /d /s /c $cmd
 if($LASTEXITCODE){throw "GPU actual-main build failed:$LASTEXITCODE"}
 foreach($entry in $entries){
  $null=Invoke-NativeTestProcess -FilePath (Join-Path $work ($entry+'.exe')) -WorkingDirectory $work -TimeoutMilliseconds 30000 -StandardOutputPath (Join-Path $work ($entry+'.log')) -StandardErrorPath (Join-Path $work ($entry+'-errors.log')) -TestName $entry
  Write-Output "PASS: $entry"
 }
 Write-Output 'PASS: GPU mandatory native gates.'
} finally {Pop-Location}
