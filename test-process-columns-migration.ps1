param()
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
$entries = @(@{Entry='ProcessColumnsMigrationPathsTest'; Name='paths'},@{Entry='ProcessColumnsMigrationStartupTest'; Name='startup'},@{Entry='ProcessColumnsMigrationMissingDirectoryTest'; Name='missing-dir'})
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
Write-Output "Migration adapter evidence: $work"
Push-Location $root
try {
    & cmd.exe /d /s /c $command
    if ($LASTEXITCODE) { throw "Adapter build failed: $LASTEXITCODE" }
    function Bytes($words) {
        $out = New-Object byte[] ($words.Count*4)
        for($i=0;$i -lt $words.Count;$i++){[Array]::Copy([BitConverter]::GetBytes([uint32]$words[$i]),0,$out,$i*4,4)}
        return ,$out
    }
    $def = Bytes (@(544,3,7,0,0,0)+@(0..6)+@(1..57 | ForEach-Object {4294967295})+@(240,130,76,120,125,90,68,90,150,190,140,140,180,360,480,90)+@(1..48 | ForEach-Object {90})+@(0,0))
    $legacy = Bytes @(128,1,5,10,1,0,0,6,10,3,8,4294967295,4294967295,4294967295,4294967295,4294967295,4294967295,4294967295,333,135,80,121,130,95,72,100,160,200,149,151,0,0)
    $v2 = Bytes @(160,2,7,0,0,0,0,1,2,3,4,5,6,4294967295,4294967295,4294967295,4294967295,4294967295,4294967295,4294967295,4294967295,4294967295,240,130,76,120,125,90,68,90,150,190,140,140,180,360,480,90,0,0)
    $migrated = Bytes (@(544,3,5,10,1,0)+@(0,6,10,3,8)+@(1..59 | ForEach-Object {4294967295})+@(333,135,80,121,130,95,72,100,160,200,149,151,180,360,480,90)+@(1..48 | ForEach-Object {90})+@(0,0))
    $sentinel = [byte[]]$def.Clone()
    [Array]::Copy([BitConverter]::GetBytes([uint32]777),0,$sentinel,280,4)
    foreach($case in @('fallback','v3-wins','corrupt-v3','locked-v3','v2-wins','corrupt-v2','locked-v2','both-missing','corrupt-v1','locked-v1','directory-v3')) {
        $dir=Join-Path $work $case
        [void][IO.Directory]::CreateDirectory($dir)
        $new=Join-Path $dir 'process-columns-v3.bin'
        $middle=Join-Path $dir 'process-columns-v2.bin'
        $old=Join-Path $dir 'process-columns-v1.bin'
        $code=0; $wanted=$migrated; $lock=$null
        if($case -ne 'both-missing') {[IO.File]::WriteAllBytes($old,$legacy)}
        switch($case) {
            'v3-wins' {[IO.File]::WriteAllBytes($new,$def);[IO.File]::WriteAllBytes($middle,[byte[]]@(7));$wanted=$def}
            'v2-wins' {[IO.File]::WriteAllBytes($middle,$v2);$wanted=$def}
            'corrupt-v3' {[IO.File]::WriteAllBytes($new,[byte[]]@(9));[IO.File]::WriteAllBytes($middle,$v2);$code=13}
            'corrupt-v2' {[IO.File]::WriteAllBytes($middle,[byte[]]@(9));$code=13}
            'locked-v3' {[IO.File]::WriteAllBytes($new,$def);$lock=[IO.File]::Open($new,'Open','ReadWrite','None');$code=32}
            'locked-v2' {[IO.File]::WriteAllBytes($middle,$v2);$lock=[IO.File]::Open($middle,'Open','ReadWrite','None');$code=32}
            'both-missing' {$code=2}
            'directory-v3' {[void][IO.Directory]::CreateDirectory($new);$code=5}
            'corrupt-v1' {[IO.File]::WriteAllBytes($old,[byte[]]@(8));$code=13}
            'locked-v1' {$lock=[IO.File]::Open($old,'Open','ReadWrite','None');$code=32}
        }
        try {
            foreach($index in 0,1) {
                $expected = if($code -eq 0){$wanted}elseif($index -eq 0){$sentinel}else{$def}
                [IO.File]::WriteAllBytes((Join-Path $dir 'expected.bin'),([byte[]]([BitConverter]::GetBytes([uint32]$code)+$expected)))
                Run-ColumnsEntry @{Exe=$entries[$index].Exe;WorkingDirectory=$dir} ($case+'-'+$index)
            }
        } finally {if($lock){$lock.Dispose()}}
        if(Test-Path -LiteralPath $middle) {
            $middleExpected = if($case -eq 'v3-wins'){[byte[]]@(7)}elseif($case -eq 'corrupt-v2'){[byte[]]@(9)}else{$v2}
            if([Convert]::ToBase64String([IO.File]::ReadAllBytes($middle)) -cne [Convert]::ToBase64String($middleExpected)){throw 'V2 legacy file mutated'}
        }
        if(Test-Path $old){
            $oldExpected=if($case -eq 'corrupt-v1'){[byte[]]@(8)}else{$legacy}
            if([Convert]::ToBase64String([IO.File]::ReadAllBytes($old)) -cne [Convert]::ToBase64String($oldExpected)){throw 'Legacy file mutated'}
        }
        if($case -eq 'fallback' -and (Test-Path -LiteralPath $new)){throw 'Migration wrote V3 during load'}
    }
    $dir=Join-Path $work 'missing-parent'
    [void][IO.Directory]::CreateDirectory($dir)
    [IO.File]::WriteAllBytes((Join-Path $dir 'process-columns-v1.bin'),$legacy)
    [IO.File]::WriteAllBytes((Join-Path $dir 'expected.bin'),([byte[]]([BitConverter]::GetBytes([uint32]3)+$sentinel)))
    Run-ColumnsEntry @{Exe=$entries[2].Exe;WorkingDirectory=$dir} 'missing-parent-no-fallback'
    Write-Output "PASS migration adapter helper/startup 23 cases and unchanged legacy evidence: $work"
} finally {Pop-Location}
