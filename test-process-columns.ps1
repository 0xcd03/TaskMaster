param([switch]$ModelOnly)
$ErrorActionPreference = 'Stop'
$root = $PSScriptRoot
. (Join-Path $root 'native-test-process.ps1')
$vs = & "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (-not $vs) { throw 'Visual Studio Build Tools C++ is required.' }
$vc = Join-Path $vs 'VC/Auxiliary/Build/vcvars64.bat'
# Every invocation owns all of its fixtures, logs and object files.
$work = Join-Path $root ('work/process-columns-'+[Guid]::NewGuid().ToString('N'))
[void][IO.Directory]::CreateDirectory($work)
$obj = Join-Path $work 'columns.obj'
$res = Join-Path $work 'columns.res'
$entries = @(
    @{Entry='ProcessColumnsModelTest'; Name='model'},
    @{Entry='ProcessColumnsLoadTest'; Name='load'},
    @{Entry='ProcessColumnsBadLoadTest'; Name='bad-load'},
    @{Entry='ProcessColumnsMigrationTest'; Name='migration'}
)
if (-not $ModelOnly) {
    $entries += @{Entry='ProcessColumnsNativeTest'; Name='native'}
    $entries += @{Entry='ProcessColumnsUiTest'; Name='ui'}
    $entries += @{Entry='UiRegressionTest'; Name='routing'; WorkingDirectory=$root}
}
$libs = 'kernel32.lib user32.lib gdi32.lib comctl32.lib psapi.lib advapi32.lib shell32.lib wtsapi32.lib shlwapi.lib pdh.lib iphlpapi.lib dwmapi.lib dbghelp.lib d2d1.lib dwrite.lib ole32.lib oleaut32.lib uuid.lib uxtheme.lib'
$command = "call `"$vc`" >nul && ml64 /nologo /D TM_TESTS /D PC_TESTS /I src /c /Fo`"$obj`" src\taskmanager.asm && rc /nologo /d PREVIEW /fo`"$res`" taskmanager.rc"
foreach ($entry in $entries) {
    $entry.Exe = Join-Path $work ($entry.Name+'.exe')
    $command += " && link /nologo /SUBSYSTEM:CONSOLE /ENTRY:$($entry.Entry) /OUT:`"$($entry.Exe)`" `"$obj`" `"$res`" $libs"
}
function Run-ColumnsEntry($entry, [string]$label) {
    $log = Join-Path $work ($label+'.log')
    try {
        $entryDirectory = if ($entry.WorkingDirectory) { $entry.WorkingDirectory } else { $work }
        $null = Invoke-NativeTestProcess -FilePath $entry.Exe -WorkingDirectory $entryDirectory -TimeoutMilliseconds 30000 -StandardOutputPath $log -StandardErrorPath (Join-Path $work ($label+'-errors.log')) -TestName ('Process columns '+$label)
    } finally { if (Test-Path -LiteralPath $log) { Get-Content -LiteralPath $log } }
}
Write-Output "Process columns owned evidence: $work"
Push-Location $root
try {
    & cmd.exe /d /s /c $command
    if ($LASTEXITCODE) { throw "Process columns actual-main build failed: $LASTEXITCODE" }
    $unownedTemp = Join-Path $work 'unowned.tmp'
    [IO.File]::WriteAllBytes($unownedTemp,[byte[]]@(71,83,97))
    Run-ColumnsEntry $entries[0] 'model'
    # Independent little-endian byte oracle; never calls model serialization.
    $words = [uint32[]](@(544,3,7,0,0,0)+@(0..6)+@(1..57 | ForEach-Object {4294967295})+@(240,130,76,120,125,90,68,90,150,190,140,140,180,360,480,90)+@(1..48 | ForEach-Object {90})+@(0,0))
    $expected = New-Object byte[] 544
    for ($i=0; $i -lt 136; $i++) { [Array]::Copy([BitConverter]::GetBytes($words[$i]),0,$expected,$i*4,4) }
    $actual = [IO.File]::ReadAllBytes((Join-Path $work 'layout.bin'))
    if ([Convert]::ToBase64String($actual) -cne [Convert]::ToBase64String($expected)) { throw 'Independent exact-544-byte oracle failed.' }
    $maximal = [byte[]]$expected.Clone()
    foreach($pair in @(@(8,20),@(12,19),@(16,1))) { [Array]::Copy([BitConverter]::GetBytes([uint32]$pair[1]),0,$maximal,[int]$pair[0],4) }
    for($i=0;$i -lt 20;$i++){[Array]::Copy([BitConverter]::GetBytes([uint32]$i),0,$maximal,24+$i*4,4)}
    if ([Convert]::ToBase64String([IO.File]::ReadAllBytes((Join-Path $work 'maximal.bin'))) -cne [Convert]::ToBase64String($maximal)) { throw 'Independent all-20-column roundtrip oracle failed.' }
    if ([Convert]::ToBase64String([IO.File]::ReadAllBytes($unownedTemp)) -cne 'R1Nh') { throw 'An unowned temporary file was changed/deleted.' }
    if (@(Get-ChildItem -LiteralPath $work -Filter '*.pc-*.tmp').Count -ne 0) { throw 'Owned temporary file leaked after replacement failure.' }
    # This is a separate executable/process loading the first process output.
    Run-ColumnsEntry $entries[1] 'second-process-load'
    $legacyWords = [uint32[]]@(128,1,5,10,1,0,0,6,10,3,8,4294967295,4294967295,4294967295,4294967295,4294967295,4294967295,4294967295,333,135,80,121,130,95,72,100,160,200,149,151,0,0)
    $legacy = New-Object byte[] 128
    for ($i=0; $i -lt 32; $i++) { [Array]::Copy([BitConverter]::GetBytes($legacyWords[$i]),0,$legacy,$i*4,4) }
    [IO.File]::WriteAllBytes((Join-Path $work 'legacy.bin'),$legacy)
    Run-ColumnsEntry @{Exe=(Join-Path $work 'migration.exe')} 'v1-migration'
    $legacyExpectedWords = [uint32[]](@(544,3,5,10,1,0)+@(0,6,10,3,8)+@(1..59 | ForEach-Object {4294967295})+@(333,135,80,121,130,95,72,100,160,200,149,151,180,360,480,90)+@(1..48 | ForEach-Object {90})+@(0,0))
    $legacyExpected = New-Object byte[] 544
    for ($i=0; $i -lt 136; $i++) { [Array]::Copy([BitConverter]::GetBytes($legacyExpectedWords[$i]),0,$legacyExpected,$i*4,4) }
    if ([Convert]::ToBase64String([IO.File]::ReadAllBytes((Join-Path $work 'converted.bin'))) -cne [Convert]::ToBase64String($legacyExpected)) { throw 'Independent nondefault V1 migration oracle failed.' }
    $v2Words = [uint32[]]@(160,2,5,15,1,0,0,6,15,3,8,4294967295,4294967295,4294967295,4294967295,4294967295,4294967295,4294967295,4294967295,4294967295,4294967295,4294967295,333,135,80,121,130,95,72,100,160,200,149,151,181,361,481,91,0,0)
    $v2 = New-Object byte[] 160
    for ($i=0; $i -lt 40; $i++) { [Array]::Copy([BitConverter]::GetBytes($v2Words[$i]),0,$v2,$i*4,4) }
    [IO.File]::WriteAllBytes((Join-Path $work 'legacy.bin'),$v2)
    Run-ColumnsEntry @{Exe=(Join-Path $work 'migration.exe')} 'v2-migration'
    $v2Expected = [byte[]]$legacyExpected.Clone()
    foreach($pair in @(@(12,15),@(32,15),@(328,181),@(332,361),@(336,481),@(340,91))) {
        [Array]::Copy([BitConverter]::GetBytes([uint32]$pair[1]),0,$v2Expected,[int]$pair[0],4)
    }
    if ([Convert]::ToBase64String([IO.File]::ReadAllBytes((Join-Path $work 'converted.bin'))) -cne [Convert]::ToBase64String($v2Expected)) { throw 'Independent V2 migration oracle failed.' }
    $badPath = Join-Path $work 'invalid.bin'
    $cases = @()
    $cases += ,([byte[]]@())
    $cases += ,([byte[]]$expected[0..542])
    $cases += ,([byte[]]($expected + [byte]0))
    foreach ($pair in @(@(0,543),@(4,2),@(8,1),@(8,21),@(12,7),@(16,2),@(20,1),@(24,1),@(28,0),@(28,20),@(48,7),@(52,0),@(276,0),@(280,31),@(532,4097),@(536,1),@(540,1))) {
        $bad = [byte[]]$expected.Clone()
        [Array]::Copy([BitConverter]::GetBytes([uint32]$pair[1]),0,$bad,[int]$pair[0],4)
        $cases += ,$bad
    }
    $cases += ,([byte[]]$legacy[0..126])
    $cases += ,([byte[]]($legacy + [byte]0))
    foreach ($pair in @(@(0,127),@(4,2),@(8,1),@(8,13),@(12,11),@(16,2),@(20,1),@(24,6),@(28,0),@(28,12),@(40,6),@(44,0),@(72,31),@(116,4097),@(120,1),@(124,1))) {
        $bad = [byte[]]$legacy.Clone()
        [Array]::Copy([BitConverter]::GetBytes([uint32]$pair[1]),0,$bad,[int]$pair[0],4)
        $cases += ,$bad
    }
    $cases += ,([byte[]]$v2[0..158])
    $cases += ,([byte[]]($v2 + [byte]0))
    foreach ($pair in @(@(0,159),@(4,1),@(8,1),@(8,17),@(12,14),@(16,2),@(20,1),@(24,6),@(28,0),@(28,16),@(40,6),@(44,0),@(88,31),@(148,4097),@(152,1),@(156,1))) {
        $bad = [byte[]]$v2.Clone()
        [Array]::Copy([BitConverter]::GetBytes([uint32]$pair[1]),0,$bad,[int]$pair[0],4)
        $cases += ,$bad
    }
    $caseIndex=0
    foreach ($bad in $cases) {
        [IO.File]::WriteAllBytes($badPath,$bad)
        Run-ColumnsEntry $entries[2] ('malformed-'+$caseIndex)
        $caseIndex++
    }
    [IO.File]::Delete($badPath)
    Run-ColumnsEntry $entries[2] 'missing-file'
    if (-not $ModelOnly) {
        Run-ColumnsEntry $entries[4] 'native'
        Run-ColumnsEntry $entries[5] 'ui'
        Run-ColumnsEntry $entries[6] 'routing'
    } else { Write-Output 'SKIP: native/UI entries requested out by -ModelOnly; this is not the full feature gate.' }
    & (Join-Path $root 'test-process-columns-migration.ps1')
    Write-Output "PASS: independent bytes, second process, $caseIndex malformed files, missing file, atomic failure cleanup. Evidence: $work"
} finally { Pop-Location }
