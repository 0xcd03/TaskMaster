$ErrorActionPreference='Stop'
$root=$PSScriptRoot
[void][IO.Directory]::CreateDirectory((Join-Path $root 'outputs'))
[void][IO.Directory]::CreateDirectory((Join-Path $root 'work'))
$vs=& "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$vc=Join-Path $vs 'VC/Auxiliary/Build/vcvars64.bat'
$script="call `"$vc`" >nul && rc /nologo /d PREVIEW /fo`"$root\work\test.res`" `"$root\taskmanager.rc`" && link /nologo /SUBSYSTEM:CONSOLE /ENTRY:TargetEntry /OUT:`"$root\outputs\test-target.exe`" `"$root\taskmanager.obj`" kernel32.lib user32.lib gdi32.lib comctl32.lib psapi.lib advapi32.lib shell32.lib wtsapi32.lib shlwapi.lib pdh.lib iphlpapi.lib dwmapi.lib dbghelp.lib d2d1.lib dwrite.lib ole32.lib oleaut32.lib uuid.lib && link /nologo /SUBSYSTEM:CONSOLE /ENTRY:SelfTest /OUT:`"$root\outputs\selftest.exe`" `"$root\taskmanager.obj`" kernel32.lib user32.lib gdi32.lib comctl32.lib psapi.lib advapi32.lib shell32.lib wtsapi32.lib shlwapi.lib pdh.lib iphlpapi.lib dwmapi.lib dbghelp.lib d2d1.lib dwrite.lib ole32.lib oleaut32.lib uuid.lib"
$script += " && link /nologo /SUBSYSTEM:CONSOLE /ENTRY:ModelTest /OUT:`"$root\outputs\model-tests.exe`" `"$root\taskmanager.obj`" `"$root\work\test.res`" kernel32.lib user32.lib gdi32.lib comctl32.lib psapi.lib advapi32.lib shell32.lib wtsapi32.lib shlwapi.lib pdh.lib iphlpapi.lib dwmapi.lib dbghelp.lib d2d1.lib dwrite.lib ole32.lib oleaut32.lib uuid.lib"
$script += " && link /nologo /SUBSYSTEM:CONSOLE /ENTRY:TaskMutationTest /OUT:`"$root\outputs\task-mutation-test.exe`" `"$root\taskmanager.obj`" kernel32.lib user32.lib gdi32.lib comctl32.lib psapi.lib advapi32.lib shell32.lib wtsapi32.lib shlwapi.lib pdh.lib iphlpapi.lib dwmapi.lib dbghelp.lib d2d1.lib dwrite.lib ole32.lib oleaut32.lib uuid.lib"
Push-Location $root
try { & cmd /d /s /c $script; if($LASTEXITCODE){throw "Build failed $LASTEXITCODE"}; & "$root\outputs\selftest.exe"; if($LASTEXITCODE){throw "Selftest failed $LASTEXITCODE"}; 'PASS: stale PID identity, pause/resume, trim, priority, terminate.'; & "$root\outputs\model-tests.exe"; if($LASTEXITCODE){throw "Model test stage failed: $LASTEXITCODE"}; 'PASS: RU/EN resources, 5000-row topology, collapse, ancestor search, PID reuse, persistent history, checksum corruption, registry/folder round trips, chart ring, Task Scheduler enumeration.' }
finally{Pop-Location}
& (Join-Path $root 'test-scheduler.ps1')





