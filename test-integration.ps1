$ErrorActionPreference = 'Stop'
$root = $PSScriptRoot
$vs = & "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (-not $vs) { throw 'Visual Studio Build Tools C++ is required.' }
$vc = Join-Path $vs 'VC/Auxiliary/Build/vcvars64.bat'
[void][IO.Directory]::CreateDirectory((Join-Path $root 'work'))
$exe = Join-Path $root 'work/integration-tests.exe'
$libs = 'kernel32.lib user32.lib gdi32.lib comctl32.lib psapi.lib advapi32.lib shell32.lib wtsapi32.lib shlwapi.lib pdh.lib iphlpapi.lib dwmapi.lib dbghelp.lib d2d1.lib dwrite.lib ole32.lib oleaut32.lib uuid.lib uxtheme.lib'
$command = "call `"$vc`" >nul && ml64 /nologo /I`"$root\src`" /c /Fo`"$root\work\integration-tests.obj`" `"$root\src\taskmanager.asm`" && link /nologo /SUBSYSTEM:CONSOLE /ENTRY:IntegrationTest /OUT:`"$exe`" `"$root\work\integration-tests.obj`" $libs"
Push-Location $root
try {
    & cmd.exe /d /s /c $command
    if ($LASTEXITCODE) { throw "Integration test build failed: $LASTEXITCODE" }
    $test = Start-Process -FilePath $exe -WorkingDirectory $root -WindowStyle Hidden -PassThru
    if (-not $test.WaitForExit(10000)) {
        $test.Kill()
        $test.WaitForExit()
        throw 'Integration test exceeded 10 seconds; only its own test process was stopped.'
    }
    if ($test.ExitCode) { throw "Integration test failed at stage $($test.ExitCode)." }
    'PASS: reversible replacement in isolated HKCU fixture; foreign Debugger and sibling value preservation; named mutex contention.'
    'Production taskmgr.exe IFEO settings were not modified.'
} finally {
    Pop-Location
}
