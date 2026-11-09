$ErrorActionPreference = 'Stop'
$root = $PSScriptRoot
$vs = & "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (-not $vs) { throw 'Visual Studio Build Tools C++ is required.' }
$vc = Join-Path $vs 'VC/Auxiliary/Build/vcvars64.bat'
[void][IO.Directory]::CreateDirectory((Join-Path $root 'work'))
$testExe = Join-Path $root 'work/ui-regression.exe'
$testLog = Join-Path $root 'work/ui-regression.log'
$testErrors = Join-Path $root 'work/ui-regression-errors.log'
$libraries = 'kernel32.lib user32.lib gdi32.lib comctl32.lib psapi.lib advapi32.lib shell32.lib wtsapi32.lib shlwapi.lib pdh.lib iphlpapi.lib dwmapi.lib dbghelp.lib d2d1.lib dwrite.lib ole32.lib oleaut32.lib uuid.lib uxtheme.lib'
$command = "call `"$vc`" >nul && ml64 /nologo /I`"$root\src`" /c /Fo`"$root\work\ui-regression.obj`" `"$root\src\taskmanager.asm`" && rc /nologo /d PREVIEW /fo`"$root\work\ui-regression.res`" `"$root\taskmanager.rc`" && link /nologo /SUBSYSTEM:CONSOLE /ENTRY:UiRegressionTest /OUT:`"$testExe`" `"$root\work\ui-regression.obj`" `"$root\work\ui-regression.res`" $libraries"
Push-Location $root
try {
    & cmd.exe /d /s /c $command
    if ($LASTEXITCODE) { throw "UI regression build failed: $LASTEXITCODE" }
    $testProcess = Start-Process -FilePath $testExe -WorkingDirectory $root -PassThru -WindowStyle Hidden -RedirectStandardOutput $testLog -RedirectStandardError $testErrors
    if (-not $testProcess.WaitForExit(15000)) {
        $testProcess.Kill()
        $testProcess.WaitForExit()
        throw 'Hidden UI regression timed out after 15 seconds; only its own test process was stopped.'
    }
    $testProcess.WaitForExit()
    Get-Content -LiteralPath $testLog
    if ($testProcess.ExitCode) { throw "UI regression failed: exit=$($testProcess.ExitCode). See $testLog" }
} finally {
    Pop-Location
}
