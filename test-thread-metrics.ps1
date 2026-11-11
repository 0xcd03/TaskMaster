$ErrorActionPreference = 'Stop'
$root = $PSScriptRoot
. (Join-Path $root 'native-test-process.ps1')
$vs = & "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (-not $vs) { throw 'Visual Studio Build Tools C++ is required.' }
$vc = Join-Path $vs 'VC/Auxiliary/Build/vcvars64.bat'
[void][IO.Directory]::CreateDirectory((Join-Path $root 'work'))
$exe = Join-Path $root 'work/thread-metrics.exe'
$log = Join-Path $root 'work/thread-metrics.log'
$errors = Join-Path $root 'work/thread-metrics-errors.log'
$libs = 'kernel32.lib user32.lib gdi32.lib comctl32.lib psapi.lib advapi32.lib shell32.lib wtsapi32.lib shlwapi.lib pdh.lib iphlpapi.lib dwmapi.lib dbghelp.lib d2d1.lib dwrite.lib ole32.lib oleaut32.lib uuid.lib uxtheme.lib'
$command = "call `"$vc`" >nul && ml64 /nologo /D TM_TESTS /I`"$root\src`" /c /Fo`"$root\work\thread-metrics.obj`" `"$root\src\taskmanager.asm`" && link /nologo /SUBSYSTEM:CONSOLE /ENTRY:ThreadMetricsTest /OUT:`"$exe`" `"$root\work\thread-metrics.obj`" $libs"
Push-Location $root
try {
    & cmd.exe /d /s /c $command
    if ($LASTEXITCODE) { throw "Thread metrics build failed: $LASTEXITCODE" }
    try {
        $null = Invoke-NativeTestProcess -FilePath $exe -WorkingDirectory $root -TimeoutMilliseconds 15000 -StandardOutputPath $log -StandardErrorPath $errors -TestName 'Thread CPU metrics'
    } finally {
        if (Test-Path -LiteralPath $log) { Get-Content -LiteralPath $log }
    }
} finally {
    Pop-Location
}
