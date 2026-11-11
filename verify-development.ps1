# Rebuild and validate development binaries without replacing downloaded releases.
$ErrorActionPreference = 'Stop'
$root = $PSScriptRoot
. (Join-Path $root 'native-test-process.ps1')
$validationDir = Join-Path $root 'work/validation'
[void][IO.Directory]::CreateDirectory($validationDir)
$shell = Join-Path $env:WINDIR 'System32/WindowsPowerShell/v1.0/powershell.exe'
# Diagnostics.Process does not perform PowerShell's native-invocation module-path
# cleanup. Let the Windows PowerShell child build its own Desktop defaults.
$childEnvironment = @{}
if ($PSVersionTable.PSVersion.Major -ge 6) { $childEnvironment['PSModulePath'] = $null }
$results = New-Object 'System.Collections.Generic.List[object]'
$steps = @(
    @{ Name = 'build-admin'; Script = 'build-taskmanager.ps1'; Arguments = '-OutputPath "./work/TaskManagerX64.exe"' },
    @{ Name = 'build-standard'; Script = 'build-taskmanager.ps1'; Arguments = '-Preview -OutputPath "./work/TaskManagerX64-standard.exe"' },
    @{ Name = 'verify-admin'; Script = 'verify-build.ps1'; Arguments = '-Path "./work/TaskManagerX64.exe"' },
    @{ Name = 'verify-standard'; Script = 'verify-build.ps1'; Arguments = '-Path "./work/TaskManagerX64-standard.exe"' },
    @{ Name = 'test-build-isolation'; Script = 'test-build-isolation.ps1'; Arguments = '' },
    @{ Name = 'native-runner'; Script = 'test-native-test-process.ps1'; Arguments = '' },
    @{ Name = 'models-actions-scheduler'; Script = 'build-tests.ps1'; Arguments = '' },
    @{ Name = 'ui-regression'; Script = 'test-ui-regression.ps1'; Arguments = '' },
    @{ Name = 'integration'; Script = 'test-integration.ps1'; Arguments = '' },
    @{ Name = 'services'; Script = 'test-services.ps1'; Arguments = '' },
    @{ Name = 'thread-metrics'; Script = 'test-thread-metrics.ps1'; Arguments = '' },
    @{ Name = 'new-task'; Script = 'test-new-task.ps1'; Arguments = '' },
    @{ Name = 'process-details'; Script = 'test-process-details.ps1'; Arguments = '' },
    @{ Name = 'process-details-optional'; Script = 'test-process-details-optional.ps1'; Arguments = '' },
    @{ Name = 'process-columns'; Script = 'test-process-columns.ps1'; Arguments = '' },
    @{ Name = 'process-metadata'; Script = 'test-process-metadata.ps1'; Arguments = '' },
    @{ Name = 'gpu'; Script = 'test-gpu.ps1'; Arguments = '' },
    @{ Name = 'packaging'; Script = 'test-package-release.ps1'; Arguments = '' }
)
try {
    foreach ($step in $steps) {
        $outputLog = Join-Path $validationDir ($step.Name + '.log')
        $errorLog = Join-Path $validationDir ($step.Name + '-errors.log')
        $scriptPath = Join-Path $root $step.Script
        Write-Host "Running $($step.Name)..."
        try {
            $null = Invoke-NativeTestProcess -FilePath $shell -Arguments "-NoProfile -ExecutionPolicy Bypass -File `"$scriptPath`" $($step.Arguments)" -WorkingDirectory $root -TimeoutMilliseconds 120000 -StandardOutputPath $outputLog -StandardErrorPath $errorLog -TestName $step.Name -EnvironmentOverrides $childEnvironment
            $skippedCoverage = @(Get-Content -LiteralPath $outputLog | Where-Object { $_ -match '^\s*SKIP:' })
            $resultName = if ($skippedCoverage.Count) { 'PARTIAL' } else { 'PASS' }
            $results.Add([PSCustomObject]@{ Step = $step.Name; Result = $resultName; SkippedCoverage = $skippedCoverage; Output = $outputLog; Errors = $errorLog })
            Write-Host "${resultName}: $($step.Name)"
            foreach ($skipped in $skippedCoverage) { Write-Warning $skipped }
        } catch {
            $results.Add([PSCustomObject]@{ Step = $step.Name; Result = 'FAIL'; Output = $outputLog; Errors = $errorLog })
            if (Test-Path -LiteralPath $outputLog) { Get-Content -LiteralPath $outputLog -Tail 20 | Out-Host }
            if (Test-Path -LiteralPath $errorLog) { Get-Content -LiteralPath $errorLog -Tail 20 | Out-Host }
            throw
        }
    }
} finally {
    [PSCustomObject]@{
        TimestampUtc = [DateTime]::UtcNow.ToString('o')
        Results = @($results.ToArray())
        ExecutedAllSteps = ($results.Count -eq $steps.Count)
        Complete = ($results.Count -eq $steps.Count -and @($results | Where-Object { $_.Result -ne 'PASS' }).Count -eq 0)
    } | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $validationDir 'summary.json') -Encoding UTF8
}
$partialResults = @($results | Where-Object { $_.Result -eq 'PARTIAL' })
if ($partialResults.Count) {
    Write-Warning "Development checks finished with skipped coverage in $($partialResults.Count) step(s); summary.json Complete=false."
} else {
    Write-Host 'All development checks passed. This does not establish complete Task Manager or PC Manager parity.'
}
