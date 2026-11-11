$ErrorActionPreference = 'Stop'
$root = $PSScriptRoot
. (Join-Path $root 'native-test-process.ps1')
$vs = & "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (-not $vs) { throw 'Visual Studio Build Tools C++ is required.' }
$vc = Join-Path $vs 'VC/Auxiliary/Build/vcvars64.bat'
$fixtureRoot = Join-Path $root 'work/new-task fixtures'
$fixtureCwd = Join-Path $fixtureRoot 'cwd spaces-я'
[void][IO.Directory]::CreateDirectory($fixtureCwd)
$exe = Join-Path $root 'work/new-task-tests.exe'
$fixtureExe = Join-Path $fixtureRoot 'fixture child-тест.exe'
$obj = Join-Path $root 'work/new-task-tests.obj'
$res = Join-Path $root 'work/new-task-tests.res'
$dialogExe = Join-Path $root 'work/new-task-dialog-tests.exe'
$libs = 'kernel32.lib user32.lib gdi32.lib comctl32.lib psapi.lib advapi32.lib shell32.lib wtsapi32.lib shlwapi.lib pdh.lib iphlpapi.lib dwmapi.lib dbghelp.lib d2d1.lib dwrite.lib ole32.lib oleaut32.lib uuid.lib uxtheme.lib'
$command = "call `"$vc`" >nul && ml64 /nologo /D TM_TESTS /I`"$root\src`" /c /Fo`"$obj`" `"$root\src\taskmanager.asm`" && rc /nologo /d PREVIEW /fo`"$res`" `"$root\taskmanager.rc`" && link /nologo /SUBSYSTEM:CONSOLE /ENTRY:NewTaskLaunchTest /OUT:`"$exe`" `"$obj`" `"$res`" $libs && link /nologo /SUBSYSTEM:CONSOLE /ENTRY:NewTaskFixtureEntry /OUT:`"$fixtureExe`" `"$obj`" $libs && link /nologo /SUBSYSTEM:CONSOLE /ENTRY:NewTaskDialogTest /OUT:`"$dialogExe`" `"$obj`" `"$res`" $libs"
Push-Location $root
try {
    & cmd.exe /d /s /c $command
    if ($LASTEXITCODE) { throw "New task test build failed: $LASTEXITCODE" }
} finally { Pop-Location }

function Invoke-LaunchCase {
    param([string]$Name,[string]$Executable=$fixtureExe,[string]$LiteralArguments='', [AllowNull()][string]$Directory=$fixtureCwd,[bool]$NullDirectory=$false,[uint32]$RunAs=0,[uint32]$Reserved=0,[uint32]$Expected=0,[uint32]$ElevationError=1223,[uint32]$ElevationCalls=0)
    foreach ($ownedDirectory in @($fixtureRoot,$fixtureCwd)) {
        $previousReport = Join-Path $ownedDirectory 'new-task-result.bin'
        if (Test-Path -LiteralPath $previousReport) { Remove-Item -LiteralPath $previousReport }
    }
    $inputPath = Join-Path $fixtureRoot 'new-task-request.bin'
    $stream = [IO.File]::Open($inputPath,[IO.FileMode]::Create,[IO.FileAccess]::Write,[IO.FileShare]::Read)
    $writer = New-Object IO.BinaryWriter($stream)
    try {
        foreach ($value in @($Expected,$RunAs,$Reserved,[uint32]$Executable.Length,[uint32]$LiteralArguments.Length)) { $writer.Write([uint32]$value) }
        if ($NullDirectory) { $writer.Write([uint32]::MaxValue) } else { $writer.Write([uint32]$Directory.Length) }
        $writer.Write($ElevationError)
        $writer.Write($ElevationCalls)
        foreach ($value in @($Executable,$LiteralArguments)) { $writer.Write([Text.Encoding]::Unicode.GetBytes($value + [char]0)) }
        if (-not $NullDirectory) { $writer.Write([Text.Encoding]::Unicode.GetBytes($Directory + [char]0)) }
    } finally { $writer.Dispose() }
    $log = Join-Path $fixtureRoot ($Name + '.log')
    try {
        $null = Invoke-NativeTestProcess -FilePath $exe -WorkingDirectory $fixtureRoot -TimeoutMilliseconds 10000 -StandardOutputPath $log -StandardErrorPath (Join-Path $fixtureRoot ($Name + '-errors.log')) -TestName "New task $Name"
    } finally { if (Test-Path -LiteralPath $log) { Get-Content -LiteralPath $log } }
}

function Assert-LaunchValue {
    param($Actual,$Expected,[string]$Description)
    if ($Actual -cne $Expected) { throw "$Description expected=[$Expected] actual=[$Actual]" }
    $script:newTaskAssertions++
}

function Read-FixtureReport {
    param([string]$Directory)
    $path = Join-Path $Directory 'new-task-result.bin'
    $timer = [Diagnostics.Stopwatch]::StartNew()
    do {
        try {
            if (Test-Path -LiteralPath $path) {
                $reportStream = [IO.File]::Open($path,[IO.FileMode]::Open,[IO.FileAccess]::Read,[IO.FileShare]::None)
                try {
                    if ($reportStream.Length -eq 139280) {
                        $bytes = New-Object byte[] 139280
                        $offset = 0
                        while ($offset -lt $bytes.Length) {
                            $read = $reportStream.Read($bytes,$offset,$bytes.Length-$offset)
                            if ($read -eq 0) { throw 'Unexpected end of fixture report.' }
                            $offset += $read
                        }
                        return ,$bytes
                    }
                } finally { $reportStream.Dispose() }
            }
        } catch [IO.IOException] { }
        Start-Sleep -Milliseconds 20
    } while ($timer.ElapsedMilliseconds -lt 5000)
    throw "Owned fixture did not produce a complete report: $path"
}

function Read-FixtureString {
    param([byte[]]$Bytes,[int]$Offset,[int]$Capacity)
    for ($length=0; $length -lt $Capacity; $length++) {
        if ([BitConverter]::ToUInt16($Bytes,$Offset+$length*2) -eq 0) {
            return [Text.Encoding]::Unicode.GetString($Bytes,$Offset,$length*2)
        }
    }
    throw "Fixture string has no terminator at $Offset"
}

$script:newTaskAssertions = 0
$identity = [Security.Principal.WindowsIdentity]::GetCurrent()
try {
    $principal = New-Object Security.Principal.WindowsPrincipal($identity)
    $expectedElevated = [uint32]$principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
} finally { $identity.Dispose() }

# These expected argv values are independent of the engine's quote builder.
$literal = 'plain "Привет мир" "quote\"value" "C:\tail\\" ""'
Invoke-LaunchCase -Name 'literal-arguments' -LiteralArguments $literal
$report = Read-FixtureReport -Directory $fixtureCwd
Assert-LaunchValue ([BitConverter]::ToUInt32($report,0)) ([uint32]0x31544B4E) 'Fixture magic'
Assert-LaunchValue ([BitConverter]::ToUInt32($report,4)) ([uint32]6) 'Argument count'
Assert-LaunchValue ([BitConverter]::ToUInt32($report,8)) $expectedElevated 'Inherited token elevation'
Assert-LaunchValue ([BitConverter]::ToUInt32($report,12)) ([uint32]0) 'Inheritable event excluded'
Assert-LaunchValue (Read-FixtureString $report 16 32768) $fixtureCwd 'Working directory'
Assert-LaunchValue (Read-FixtureString $report 65552 32768) ('"'+$fixtureExe+'" '+$literal) 'Exact raw command line'
$expectedArgv = @($fixtureExe,'plain','Привет мир','quote"value','C:\tail\','')
for ($index=0; $index -lt $expectedArgv.Count; $index++) {
    Assert-LaunchValue (Read-FixtureString $report (131088+$index*1024) 512) $expectedArgv[$index] "Parsed argv[$index]"
}

Invoke-LaunchCase -Name 'empty-arguments-inherited-cwd' -NullDirectory $true
$report = Read-FixtureReport -Directory $fixtureRoot
Assert-LaunchValue ([BitConverter]::ToUInt32($report,4)) ([uint32]1) 'Empty arguments do not add an argument'
Assert-LaunchValue (Read-FixtureString $report 16 32768) $fixtureRoot 'Null cwd inherits caller directory'
Assert-LaunchValue (Read-FixtureString $report 65552 32768) ('"'+$fixtureExe+'"') 'Empty-argument command line'

$maximumArguments = 'x' * (32767 - $fixtureExe.Length - 4)
Invoke-LaunchCase -Name 'exact-command-limit' -LiteralArguments $maximumArguments
$report = Read-FixtureReport -Directory $fixtureCwd
$boundaryCommand = Read-FixtureString $report 65552 32768
Assert-LaunchValue $boundaryCommand.Length 32766 'Maximum command length without NUL'
Assert-LaunchValue $boundaryCommand ('"'+$fixtureExe+'" '+$maximumArguments) 'Exact boundary command read-back'
Invoke-LaunchCase -Name 'command-over-limit' -LiteralArguments ($maximumArguments+'x') -Expected 206
Invoke-LaunchCase -Name 'arguments-over-limit' -LiteralArguments ('x'*32767) -Expected 206
Invoke-LaunchCase -Name 'executable-over-limit' -Executable ('C:\'+('x'*32764)) -Expected 206
Invoke-LaunchCase -Name 'directory-over-limit' -Directory ('C:\'+('x'*32764)) -Expected 206
Invoke-LaunchCase -Name 'empty-executable' -Executable '' -Expected 87
Invoke-LaunchCase -Name 'relative-executable' -Executable 'fixture.exe' -Expected 87
Invoke-LaunchCase -Name 'drive-relative-executable' -Executable 'C:fixture.exe' -Expected 87
Invoke-LaunchCase -Name 'root-relative-executable' -Executable '\fixture.exe' -Expected 87
Invoke-LaunchCase -Name 'malformed-unc' -Executable '\\server' -Expected 87
Invoke-LaunchCase -Name 'quoted-executable' -Executable ('"'+$fixtureExe+'"') -Expected 87
Invoke-LaunchCase -Name 'relative-directory' -Directory 'relative-folder' -Expected 87
Invoke-LaunchCase -Name 'empty-supplied-directory' -Directory '' -Expected 87
Invoke-LaunchCase -Name 'missing-executable' -Executable (Join-Path $fixtureRoot 'not-present.exe') -Expected 2
Invoke-LaunchCase -Name 'directory-as-executable' -Executable $fixtureCwd -Expected 193
Invoke-LaunchCase -Name 'missing-directory' -Directory (Join-Path $fixtureRoot 'not-present') -Expected 3
Invoke-LaunchCase -Name 'file-as-directory' -Directory $fixtureExe -Expected 267
Invoke-LaunchCase -Name 'invalid-runas' -RunAs 2 -Expected 87
Invoke-LaunchCase -Name 'reserved-not-zero' -Reserved 1 -Expected 87
$notExecutable = Join-Path $fixtureRoot 'not-an-executable.txt'
[IO.File]::WriteAllText($notExecutable,'Owned non-executable fixture.')
Invoke-LaunchCase -Name 'os-invalid-image' -Executable $notExecutable -Expected 193

# No real UAC: only ShellExecuteExW is replaced, after real request validation.
Invoke-LaunchCase -Name 'uac-cancel' -RunAs 1 -Expected 1223 -ElevationError 1223 -ElevationCalls 1
Invoke-LaunchCase -Name 'elevation-error' -RunAs 1 -Expected 5 -ElevationError 5 -ElevationCalls 1
Invoke-LaunchCase -Name 'elevation-success-handle-closed' -RunAs 1 -Expected 0 -ElevationError 0 -ElevationCalls 1
Invoke-LaunchCase -Name 'elevated-validation-before-boundary' -RunAs 1 -Executable 'relative.exe' -Expected 87 -ElevationCalls 0
if (Test-Path -LiteralPath (Join-Path $fixtureCwd 'new-task-result.bin')) { throw 'Elevation boundary fixture unexpectedly launched a child.' }
$dialogLog = Join-Path $root 'work/new-task-dialog-tests.log'
try {
    $null = Invoke-NativeTestProcess -FilePath $dialogExe -WorkingDirectory $root -TimeoutMilliseconds 10000 -StandardOutputPath $dialogLog -StandardErrorPath (Join-Path $root 'work/new-task-dialog-tests-errors.log') -TestName 'New task native dialog'
} finally { if (Test-Path -LiteralPath $dialogLog) { Get-Content -LiteralPath $dialogLog } }
"PASS: native launch cases; $script:newTaskAssertions independent fixture assertions; no real UAC."
