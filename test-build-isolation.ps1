# Exercise real production/test compilation in both orders. Never launch GUI entry.
$ErrorActionPreference = 'Stop'
$root = $PSScriptRoot
$production = Join-Path $root 'work/isolation-production.exe'
$productionMap = Join-Path $root 'work/isolation-production.map'
$testMap = Join-Path $root 'work/test-taskmanager.map'
$symbols = @('SelfTest','ModelTest','UiRegressionTest','TargetEntry','TaskMutationTest',
    'IntegrationTest','ServiceEnumerationTest','ThreadMetricsTest','NewTaskLaunchTest',
    'NewTaskDialogTest','ProcessPresentationTests','MenuThemeTests','TrimSchedulingTests',
    'ScrollPaintTests','IconQueueTests','ResetIconQueueFixtures','modelName',
    'uiRegressionClass','serviceTestReference','iconTestRow','scrollTestWindow')
function Assert-Symbols([string]$Path, [bool]$Present) {
    $map = [IO.File]::ReadAllText($Path)
    foreach ($symbol in $symbols) {
        $found = $map -match ('\s' + [regex]::Escape($symbol) + '\s')
        if ($found -ne $Present) { throw "Symbol $symbol present=$found (expected $Present): $Path" }
    }
}
function Build-Production {
    & (Join-Path $root 'build-taskmanager.ps1') -OutputPath $production
    Assert-Symbols $productionMap $false
    & (Join-Path $root 'verify-build.ps1') -Path $production
}
Push-Location $root
try {
    Build-Production
    $productionHash = (Get-FileHash -LiteralPath (Join-Path $root 'taskmanager.obj')).Hash
    & (Join-Path $root 'build-tests.ps1')
    Assert-Symbols $testMap $true
    if ((Get-FileHash -LiteralPath (Join-Path $root 'taskmanager.obj')).Hash -ne $productionHash) {
        throw 'Test build modified the production object'
    }
    $testHash = (Get-FileHash -LiteralPath (Join-Path $root 'work/test-taskmanager.obj')).Hash
    & (Join-Path $root 'test-ui-regression.ps1')
    Build-Production
    if ((Get-FileHash -LiteralPath (Join-Path $root 'work/test-taskmanager.obj')).Hash -ne $testHash) {
        throw 'Production build modified the test object'
    }
    # Verify each feature flag implies TM_TESTS without an explicit generic define.
    $vs = & "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
    if (-not $vs) { throw 'Visual Studio Build Tools C++ is required.' }
    $vc = Join-Path $vs 'VC/Auxiliary/Build/vcvars64.bat'
    $libs = 'kernel32.lib user32.lib gdi32.lib comctl32.lib psapi.lib advapi32.lib shell32.lib wtsapi32.lib shlwapi.lib pdh.lib iphlpapi.lib dwmapi.lib dbghelp.lib d2d1.lib dwrite.lib ole32.lib oleaut32.lib uuid.lib'
    foreach ($flag in @('PD_TESTS','PC_TESTS','PM_TESTS','GP_TESTS')) {
        $stem = "work/isolation-$flag"
        # Metadata suites already share the PD fixture/oracles, as in their runner.
        $fixtureDependencies = if ($flag -eq 'PM_TESTS') { '/D PD_TESTS' } else { '' }
        $command = "call `"$vc`" >nul && ml64 /nologo /D $flag $fixtureDependencies /I src /c /Fo`"$stem.obj`" src/taskmanager.asm && link /nologo /SUBSYSTEM:CONSOLE /ENTRY:SelfTest /MAP:`"$stem.map`" /OUT:`"$stem.exe`" `"$stem.obj`" $libs"
        & cmd.exe /d /s /c $command
        if ($LASTEXITCODE) { throw "Feature implication build failed: $flag ($LASTEXITCODE)" }
        Assert-Symbols (Join-Path $root "$stem.map") $true
    }
    'PASS: production/test build order, separate objects, absent production fixtures, shared test symbols, feature flag implication, SelfTest/ModelTest/UI execution.'
} finally { Pop-Location }
