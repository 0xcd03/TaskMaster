$ErrorActionPreference = 'Stop'
$root = $PSScriptRoot
. (Join-Path $root 'native-test-process.ps1')
$vs = & "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (-not $vs) { throw 'Visual Studio Build Tools C++ is required.' }
$vc = Join-Path $vs 'VC/Auxiliary/Build/vcvars64.bat'
$work = Join-Path $root 'work/process-details-optional-owned'
[void][IO.Directory]::CreateDirectory($work)
$obj = Join-Path $work 'optional-collector.obj'
$exe = Join-Path $work 'optional-collector.exe'
$res = Join-Path $work 'optional-tests.res'
$entries = @(
    @{Entry='PdOptionalCollectorTest';Name='collector';Exe=$exe},
    @{Entry='PdOptionalWorkerTest';Name='worker';Exe=(Join-Path $work 'optional-worker.exe')},
    @{Entry='PdOptionalUiTest';Name='ui';Exe=(Join-Path $work 'optional-ui.exe')}
)
$libs = 'kernel32.lib user32.lib gdi32.lib comctl32.lib psapi.lib advapi32.lib shell32.lib wtsapi32.lib shlwapi.lib pdh.lib iphlpapi.lib dwmapi.lib dbghelp.lib d2d1.lib dwrite.lib ole32.lib oleaut32.lib uuid.lib uxtheme.lib'
$command = "call `"$vc`" >nul && ml64 /nologo /D TM_TESTS /D PD_TESTS /D PD_OPTIONAL_TESTS /I src /c /Fo`"$obj`" src\taskmanager.asm && rc /nologo /d PREVIEW /fo`"$res`" taskmanager.rc"
foreach($entry in $entries) {
    $command += " && link /nologo /SUBSYSTEM:CONSOLE /ENTRY:$($entry.Entry) /OUT:`"$($entry.Exe)`" `"$obj`" `"$res`" $libs"
}
Push-Location $root
try {
    & cmd.exe /d /s /c $command
    if ($LASTEXITCODE) { throw "Optional collector build failed: $LASTEXITCODE" }
    $unicodeWord = -join (@(1055,1088,1080,1074,1077,1090) | ForEach-Object { [char]$_ })
    $literalArguments = '--owned-optional "'+$unicodeWord+' text" "C:\tail\\path" '+('x'*8192)
    foreach($entry in $entries) {
        $log = Join-Path $work ($entry.Name+'.log')
        try {
            $runArguments = if($entry.Name -eq 'collector') { $literalArguments } else { '' }
            $null = Invoke-NativeTestProcess -FilePath $entry.Exe -Arguments $runArguments -WorkingDirectory $work -TimeoutMilliseconds 45000 -StandardOutputPath $log -StandardErrorPath (Join-Path $work ($entry.Name+'-errors.log')) -TestName ('Optional '+$entry.Name)
        } finally { if (Test-Path -LiteralPath $log) { Get-Content -LiteralPath $log } }
    }
} finally { Pop-Location }
