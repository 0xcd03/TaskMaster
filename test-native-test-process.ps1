param(
    [string]$ProcessHelperPath = (Join-Path $PSScriptRoot 'native-test-process.ps1'),
    [ValidateSet('', 'Flood', 'Sleep', 'State', 'Failure', 'Environment', 'Hash')]
    [string]$FixtureMode = '',
    [string]$FixtureMarkerPath,
    [string]$FixtureDirectory
)

$ErrorActionPreference = 'Stop'
if ($FixtureMode -eq 'Flood') {
    [Console]::Out.WriteLine('stdout-start')
    [Console]::Error.WriteLine('stderr-start')
    for ($i = 0; $i -lt 1024; $i++) {
        [Console]::Out.WriteLine(('o' * 1024))
        [Console]::Error.WriteLine(('e' * 1024))
    }
    [Console]::Out.WriteLine('stdout-end')
    [Console]::Error.WriteLine('stderr-end')
    exit 0
}
if ($FixtureMode -eq 'Sleep') {
    [IO.File]::WriteAllText($FixtureMarkerPath, [string]$PID)
    [Console]::Out.WriteLine('sleep-start')
    [Console]::Error.WriteLine('sleep-error')
    [Threading.Thread]::Sleep(15000)
    exit 0
}
if ($FixtureMode -eq 'State') {
    Add-Type -MemberDefinition '[DllImport("kernel32.dll")] public static extern IntPtr GetConsoleWindow();' -Name NativeProcess -Namespace TaskMasterFixture
    $state = [PSCustomObject]@{
        Directory = [Environment]::CurrentDirectory
        Console = [TaskMasterFixture.NativeProcess]::GetConsoleWindow().ToInt64()
    }
    [IO.File]::WriteAllText($FixtureMarkerPath, ($state | ConvertTo-Json))
    exit 0
}
if ($FixtureMode -eq 'Environment') {
    $state = [PSCustomObject]@{
        Override = [Environment]::GetEnvironmentVariable('TASKMASTER_RUNNER_OVERRIDE', 'Process')
        Removed = [Environment]::GetEnvironmentVariable('TASKMASTER_RUNNER_REMOVE', 'Process')
        Inherited = [Environment]::GetEnvironmentVariable('TASKMASTER_RUNNER_INHERITED', 'Process')
    }
    [IO.File]::WriteAllText($FixtureMarkerPath, ($state | ConvertTo-Json))
    exit 0
}
if ($FixtureMode -eq 'Hash') {
    # Exercise real command auto-loading in Windows PowerShell, not a mocked hash.
    $hash = Get-FileHash -LiteralPath (Join-Path $FixtureDirectory 'hash-input.bin') -Algorithm SHA256
    $command = Get-Command Get-FileHash
    $state = [PSCustomObject]@{
        Hash = $hash.Hash
        Edition = $PSVersionTable.PSEdition
        ModulePath = $command.Module.Path
    }
    [IO.File]::WriteAllText($FixtureMarkerPath, ($state | ConvertTo-Json))
    exit 0
}

. $ProcessHelperPath
if ($FixtureMode -eq 'Failure') {
    $null = Invoke-NativeTestProcess -FilePath "$env:SystemRoot\System32\cmd.exe" -Arguments '/d /c exit 135' -WorkingDirectory $FixtureDirectory -TimeoutMilliseconds 5000 -StandardOutputPath (Join-Path $FixtureDirectory 'nested.log') -StandardErrorPath (Join-Path $FixtureDirectory 'nested-errors.log') -TestName 'Nested fixture'
    exit 0
}

function Assert-True([bool]$Condition, [string]$Message) {
    if (-not $Condition) { throw $Message }
}

function Assert-Fails([scriptblock]$Action, [string]$ExpectedMessage) {
    try { $null = & $Action } catch {
        Assert-True ($_.Exception.Message -match $ExpectedMessage) "Wrong failure: $($_.Exception.Message)"
        return
    }
    throw "Expected failure matching '$ExpectedMessage', but the native test was accepted."
}

$fixtureRoot = Join-Path ([IO.Path]::GetTempPath()) ('TaskMaster-runner-' + [Guid]::NewGuid().ToString('N'))
$fixtureRoot = [IO.Path]::GetFullPath($fixtureRoot)
[void][IO.Directory]::CreateDirectory($fixtureRoot)
$fixtureShell = if ($PSVersionTable.PSEdition -eq 'Core') { Join-Path $PSHOME 'pwsh.exe' } else { Join-Path $PSHOME 'powershell.exe' }
$fixtureArguments = '-NoProfile -ExecutionPolicy Bypass -File "' + $PSCommandPath + '"'
$failures = New-Object 'Collections.Generic.List[string]'

function Test-Case([string]$Name, [scriptblock]$Action) {
    try {
        $null = & $Action
        "PASS: $Name"
    } catch {
        $failures.Add("${Name}: $($_.Exception.Message)")
        "FAIL: ${Name}: $($_.Exception.Message)"
    }
}

try {
    Test-Case 'zero exit succeeds and creates both logs' {
        $result = Invoke-NativeTestProcess -FilePath "$env:SystemRoot\System32\cmd.exe" -Arguments '/d /c exit 0' -WorkingDirectory $fixtureRoot -TimeoutMilliseconds 5000 -StandardOutputPath (Join-Path $fixtureRoot 'success.log') -StandardErrorPath (Join-Path $fixtureRoot 'success-errors.log') -TestName 'Success fixture'
        Assert-True ($result.ExitCode -eq 0) 'Success did not return exit 0.'
        Assert-True ([IO.File]::Exists((Join-Path $fixtureRoot 'success.log'))) 'Stdout log missing.'
        Assert-True ([IO.File]::Exists((Join-Path $fixtureRoot 'success-errors.log'))) 'Stderr log missing.'
    }

    Test-Case 'native exit 135 fails instead of being accepted as null' {
        Assert-Fails {
            Invoke-NativeTestProcess -FilePath "$env:SystemRoot\System32\cmd.exe" -Arguments '/d /c exit 135' -WorkingDirectory $fixtureRoot -TimeoutMilliseconds 5000 -StandardOutputPath (Join-Path $fixtureRoot 'failure.log') -StandardErrorPath (Join-Path $fixtureRoot 'failure-errors.log') -TestName 'Failure fixture'
        } 'exit=135'
    }

    Test-Case 'both large output streams drain fully without deadlock' {
        $outPath = Join-Path $fixtureRoot 'flood.log'
        $errPath = Join-Path $fixtureRoot 'flood-errors.log'
        $null = Invoke-NativeTestProcess -FilePath $fixtureShell -Arguments ($fixtureArguments + ' -FixtureMode Flood') -WorkingDirectory $fixtureRoot -TimeoutMilliseconds 10000 -StandardOutputPath $outPath -StandardErrorPath $errPath -TestName 'Flood fixture'
        $stdout = [IO.File]::ReadAllText($outPath)
        $stderr = [IO.File]::ReadAllText($errPath)
        Assert-True ($stdout.Length -gt 1048576 -and $stdout.Contains('stdout-start') -and $stdout.TrimEnd().EndsWith('stdout-end')) 'Stdout was truncated.'
        Assert-True ($stderr.Length -gt 1048576 -and $stderr.Contains('stderr-start') -and $stderr.TrimEnd().EndsWith('stderr-end')) 'Stderr was truncated.'
    }

    Test-Case 'child has requested working directory and no console window' {
        $marker = Join-Path $fixtureRoot 'state.json'
        $null = Invoke-NativeTestProcess -FilePath $fixtureShell -Arguments ($fixtureArguments + ' -FixtureMode State -FixtureMarkerPath "' + $marker + '"') -WorkingDirectory $fixtureRoot -TimeoutMilliseconds 10000 -StandardOutputPath (Join-Path $fixtureRoot 'state.log') -StandardErrorPath (Join-Path $fixtureRoot 'state-errors.log') -TestName 'State fixture'
        $state = [IO.File]::ReadAllText($marker) | ConvertFrom-Json
        Assert-True ($state.Directory -eq $fixtureRoot) 'Working directory was not applied.'
        Assert-True ($state.Console -eq 0) 'The fixture received a console window.'
    }

    Test-Case 'timeout kills only its own child and preserves partial logs' {
        $unrelatedMarker = Join-Path $fixtureRoot 'unrelated.pid'
        $ownedMarker = Join-Path $fixtureRoot 'owned.pid'
        $info = New-Object Diagnostics.ProcessStartInfo
        $info.FileName = $fixtureShell
        $info.Arguments = $fixtureArguments + ' -FixtureMode Sleep -FixtureMarkerPath "' + $unrelatedMarker + '"'
        $info.UseShellExecute = $false
        $info.CreateNoWindow = $true
        $unrelated = [Diagnostics.Process]::Start($info)
        try {
            $timer = [Diagnostics.Stopwatch]::StartNew()
            Assert-Fails {
                Invoke-NativeTestProcess -FilePath $fixtureShell -Arguments ($fixtureArguments + ' -FixtureMode Sleep -FixtureMarkerPath "' + $ownedMarker + '"') -WorkingDirectory $fixtureRoot -TimeoutMilliseconds 2500 -StandardOutputPath (Join-Path $fixtureRoot 'timeout.log') -StandardErrorPath (Join-Path $fixtureRoot 'timeout-errors.log') -TestName 'Timeout fixture'
            } 'timed out'
            Assert-True ($timer.ElapsedMilliseconds -lt 10000) 'Timeout cleanup was not bounded.'
            Assert-True (-not $unrelated.HasExited) 'Timeout stopped an unrelated process.'
            Assert-True ([IO.File]::Exists($ownedMarker)) 'Timed out before the fixture began; timeout coverage is inconclusive.'
            $ownedPid = [int][IO.File]::ReadAllText($ownedMarker)
            $owned = Get-Process -Id $ownedPid -ErrorAction SilentlyContinue
            Assert-True ($null -eq $owned) 'Timed-out child is still running.'
            Assert-True ([IO.File]::ReadAllText((Join-Path $fixtureRoot 'timeout.log')).Contains('sleep-start')) 'Timeout lost stdout.'
            Assert-True ([IO.File]::ReadAllText((Join-Path $fixtureRoot 'timeout-errors.log')).Contains('sleep-error')) 'Timeout lost stderr.'
        } finally {
            if (-not $unrelated.HasExited) { $unrelated.Kill(); $null = $unrelated.WaitForExit(5000) }
            $unrelated.Dispose()
        }
    }

    Test-Case 'missing executable fails to start' {
        Assert-Fails {
            Invoke-NativeTestProcess -FilePath (Join-Path $fixtureRoot 'missing.exe') -WorkingDirectory $fixtureRoot -TimeoutMilliseconds 1000 -StandardOutputPath (Join-Path $fixtureRoot 'missing.log') -StandardErrorPath (Join-Path $fixtureRoot 'missing-errors.log') -TestName 'Missing fixture'
        } 'could not start'
    }

    Test-Case 'PowerShell -File reports native test failure as nonzero' {
        Assert-Fails {
            Invoke-NativeTestProcess -FilePath $fixtureShell -Arguments ($fixtureArguments + ' -FixtureMode Failure -FixtureDirectory "' + $fixtureRoot + '" -ProcessHelperPath "' + $ProcessHelperPath + '"') -WorkingDirectory $fixtureRoot -TimeoutMilliseconds 10000 -StandardOutputPath (Join-Path $fixtureRoot 'host.log') -StandardErrorPath (Join-Path $fixtureRoot 'host-errors.log') -TestName 'Host fixture'
        } 'exit=1\b'
        Assert-True ([IO.File]::ReadAllText((Join-Path $fixtureRoot 'host-errors.log')).Contains('exit=135')) 'Host failed for a reason other than the native exit code.'
    }

    Test-Case 'child environment overrides and removal preserve parent and default inheritance' {
        $keys = @('TASKMASTER_RUNNER_OVERRIDE', 'TASKMASTER_RUNNER_REMOVE', 'TASKMASTER_RUNNER_INHERITED')
        $original = @{}
        foreach ($key in $keys) { $original[$key] = [Environment]::GetEnvironmentVariable($key, 'Process') }
        $parentValue = 'parent-' + [Guid]::NewGuid().ToString('N')
        try {
            foreach ($key in $keys) { [Environment]::SetEnvironmentVariable($key, $parentValue, 'Process') }
            $defaultMarker = Join-Path $fixtureRoot 'environment-default.json'
            $null = Invoke-NativeTestProcess -FilePath $fixtureShell -Arguments ($fixtureArguments + ' -FixtureMode Environment -FixtureMarkerPath "' + $defaultMarker + '"') -WorkingDirectory $fixtureRoot -TimeoutMilliseconds 10000 -StandardOutputPath (Join-Path $fixtureRoot 'environment-default.log') -StandardErrorPath (Join-Path $fixtureRoot 'environment-default-errors.log') -TestName 'Inherited environment fixture'
            $defaultState = [IO.File]::ReadAllText($defaultMarker) | ConvertFrom-Json
            Assert-True ($defaultState.Override -ceq $parentValue -and $defaultState.Removed -ceq $parentValue -and $defaultState.Inherited -ceq $parentValue) 'Default invocation changed inherited variables.'

            $childValue = 'child value with spaces = ' + [Guid]::NewGuid().ToString('N')
            $overrides = @{ TASKMASTER_RUNNER_OVERRIDE = $childValue; TASKMASTER_RUNNER_REMOVE = $null }
            $overrideMarker = Join-Path $fixtureRoot 'environment-override.json'
            $null = Invoke-NativeTestProcess -FilePath $fixtureShell -Arguments ($fixtureArguments + ' -FixtureMode Environment -FixtureMarkerPath "' + $overrideMarker + '"') -WorkingDirectory $fixtureRoot -TimeoutMilliseconds 10000 -StandardOutputPath (Join-Path $fixtureRoot 'environment-override.log') -StandardErrorPath (Join-Path $fixtureRoot 'environment-override-errors.log') -TestName 'Overridden environment fixture' -EnvironmentOverrides $overrides
            $state = [IO.File]::ReadAllText($overrideMarker) | ConvertFrom-Json
            Assert-True ($state.Override -ceq $childValue) 'Child did not receive the exact override value.'
            Assert-True ($null -eq $state.Removed) 'The removed child variable is still present.'
            Assert-True ($state.Inherited -ceq $parentValue) 'An unspecified child variable changed.'
            foreach ($key in $keys) {
                Assert-True ([Environment]::GetEnvironmentVariable($key, 'Process') -ceq $parentValue) "Parent variable changed: $key"
            }
            Assert-True ($overrides.TASKMASTER_RUNNER_OVERRIDE -ceq $childValue -and $null -eq $overrides.TASKMASTER_RUNNER_REMOVE) 'The caller override table was changed.'
        } finally {
            foreach ($key in $keys) { [Environment]::SetEnvironmentVariable($key, $original[$key], 'Process') }
        }
    }

    Test-Case 'Windows PowerShell hash auto-loading works with child PSModulePath removed' {
        $windowsPowerShell = Join-Path $env:SystemRoot 'System32/WindowsPowerShell/v1.0/powershell.exe'
        $originalModulePath = [Environment]::GetEnvironmentVariable('PSModulePath', 'Process')
        [IO.File]::WriteAllBytes((Join-Path $fixtureRoot 'hash-input.bin'), [byte[]]@(97, 98, 99))
        $marker = Join-Path $fixtureRoot 'hash.json'
        $null = Invoke-NativeTestProcess -FilePath $windowsPowerShell -Arguments ($fixtureArguments + ' -FixtureMode Hash -FixtureMarkerPath "' + $marker + '" -FixtureDirectory "' + $fixtureRoot + '"') -WorkingDirectory $fixtureRoot -TimeoutMilliseconds 10000 -StandardOutputPath (Join-Path $fixtureRoot 'hash.log') -StandardErrorPath (Join-Path $fixtureRoot 'hash-errors.log') -TestName 'Windows PowerShell hash fixture' -EnvironmentOverrides @{ PSModulePath = $null }
        $state = [IO.File]::ReadAllText($marker) | ConvertFrom-Json
        Assert-True ($state.Hash -ceq 'BA7816BF8F01CFEA414140DE5DAE2223B00361A396177A9CB410FF61F20015AD') 'Windows PowerShell returned the wrong SHA256 for abc.'
        Assert-True ($state.Edition -eq 'Desktop') 'The hash fixture did not run in Windows PowerShell.'
        $windowsModuleRoot = Join-Path (Split-Path $windowsPowerShell -Parent) 'Modules'
        Assert-True ($state.ModulePath.StartsWith($windowsModuleRoot + [IO.Path]::DirectorySeparatorChar, [StringComparison]::OrdinalIgnoreCase)) 'The hash command loaded from a foreign PowerShell module directory.'
        Assert-True ([Environment]::GetEnvironmentVariable('PSModulePath', 'Process') -ceq $originalModulePath) 'Child hashing changed parent PSModulePath.'
    }

    if ($failures.Count -gt 0) { throw ($failures -join [Environment]::NewLine) }
    'PASS: 9 native-process runner cases on PowerShell ' + $PSVersionTable.PSVersion.ToString()
} finally {
    if ($failures.Count -eq 0) {
        $tempRoot = [IO.Path]::GetFullPath([IO.Path]::GetTempPath())
        if (-not $fixtureRoot.StartsWith($tempRoot, [StringComparison]::OrdinalIgnoreCase)) { throw 'Fixture cleanup path escaped the temporary directory.' }
        Remove-Item -LiteralPath $fixtureRoot -Recurse -Force
    } else {
        'Fixture logs retained at ' + $fixtureRoot
    }
}
