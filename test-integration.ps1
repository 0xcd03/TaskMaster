$ErrorActionPreference = 'Stop'
$root = $PSScriptRoot
. (Join-Path $root 'native-test-process.ps1')
$vs = & "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (-not $vs) { throw 'Visual Studio Build Tools C++ is required.' }
$vc = Join-Path $vs 'VC/Auxiliary/Build/vcvars64.bat'
[void][IO.Directory]::CreateDirectory((Join-Path $root 'work'))
$exe = Join-Path $root 'work/integration-tests.exe'
$testLog = Join-Path $root 'work/integration-tests.log'
$testErrors = Join-Path $root 'work/integration-tests-errors.log'
$libs = 'kernel32.lib user32.lib gdi32.lib comctl32.lib psapi.lib advapi32.lib shell32.lib wtsapi32.lib shlwapi.lib pdh.lib iphlpapi.lib dwmapi.lib dbghelp.lib d2d1.lib dwrite.lib ole32.lib oleaut32.lib uuid.lib uxtheme.lib'
$command = "call `"$vc`" >nul && ml64 /nologo /D TM_TESTS /I`"$root\src`" /c /Fo`"$root\work\integration-tests.obj`" `"$root\src\taskmanager.asm`" && link /nologo /SUBSYSTEM:CONSOLE /ENTRY:IntegrationTest /OUT:`"$exe`" `"$root\work\integration-tests.obj`" $libs"
Push-Location $root
try {
    & cmd.exe /d /s /c $command
    if ($LASTEXITCODE) { throw "Integration test build failed: $LASTEXITCODE" }
    try {
        $null = Invoke-NativeTestProcess -FilePath $exe -WorkingDirectory $root -TimeoutMilliseconds 10000 -StandardOutputPath $testLog -StandardErrorPath $testErrors -TestName 'Integration test'
    } finally {
        if (Test-Path -LiteralPath $testLog) { Get-Content -LiteralPath $testLog }
    }
    'PASS: reversible replacement in isolated HKCU fixture; foreign Debugger and sibling value preservation; named mutex contention.'
    'Production taskmgr.exe IFEO settings were not modified.'
} finally {
    Pop-Location
}
