$ErrorActionPreference = 'Stop'
$root = $PSScriptRoot
. (Join-Path $root 'native-test-process.ps1')
$vs = & "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (-not $vs) { throw 'Visual Studio Build Tools C++ is required.' }
$vc = Join-Path $vs 'VC/Auxiliary/Build/vcvars64.bat'
$work = Join-Path $root 'work/process-details-owned'
[void][IO.Directory]::CreateDirectory($work)
$obj = Join-Path $work 'process-details-native.obj'
$exe = Join-Path $work 'process-details-native.exe'
$fixture = Join-Path $work 'process-details-fixture.exe'
$res = Join-Path $work 'process-details-tests.res'
$entries = @(
    @{ Entry='ProcessDetailsNativeTest'; Name='native'; Exe=$exe },
    @{ Entry='PdWorkerTest'; Name='worker'; Exe=(Join-Path $work 'process-details-worker.exe') },
    @{ Entry='PdRatesTest'; Name='rates'; Exe=(Join-Path $work 'process-details-rates.exe') },
    @{ Entry='PdUiTest'; Name='ui'; Exe=(Join-Path $work 'process-details-ui.exe') },
    @{ Entry='UiRegressionTest'; Name='ui-route'; Exe=(Join-Path $work 'process-details-ui-route.exe'); WorkingDirectory=$root }
)
$libs = 'kernel32.lib user32.lib gdi32.lib comctl32.lib psapi.lib advapi32.lib shell32.lib wtsapi32.lib shlwapi.lib pdh.lib iphlpapi.lib dwmapi.lib dbghelp.lib d2d1.lib dwrite.lib ole32.lib oleaut32.lib uuid.lib uxtheme.lib'
$command = "call `"$vc`" >nul && ml64 /nologo /D TM_TESTS /D PD_TESTS /I src /c /Fo`"$obj`" src\taskmanager.asm && rc /nologo /d PREVIEW /fo`"$res`" taskmanager.rc && link /nologo /SUBSYSTEM:CONSOLE /ENTRY:ProcessDetailsFixtureEntry /OUT:`"$fixture`" `"$obj`" $libs"
foreach ($entry in $entries) {
    $command += " && link /nologo /SUBSYSTEM:CONSOLE /ENTRY:$($entry.Entry) /OUT:`"$($entry.Exe)`" `"$obj`" `"$res`" $libs"
}
Push-Location $root
try {
    & cmd.exe /d /s /c $command
    if ($LASTEXITCODE) { throw "Process details build failed: $LASTEXITCODE" }
    foreach ($entry in $entries) {
        $log = Join-Path $work ($entry.Name+'.log')
        try {
            $entryCwd = if ($entry.WorkingDirectory) { $entry.WorkingDirectory } else { $work }
            $null = Invoke-NativeTestProcess -FilePath $entry.Exe -WorkingDirectory $entryCwd -TimeoutMilliseconds 30000 -StandardOutputPath $log -StandardErrorPath (Join-Path $work ($entry.Name+'-errors.log')) -TestName ('Process details '+$entry.Name)
        } finally { if (Test-Path -LiteralPath $log) { Get-Content -LiteralPath $log } }
    }
} finally { Pop-Location }
