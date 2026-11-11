# Shared by native test runners; compatible with Windows PowerShell 5.1 and PowerShell 7.
function Invoke-NativeTestProcess {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory = $true)][string]$FilePath,
        [string]$Arguments = '',
        [Parameter(Mandatory = $true)][string]$WorkingDirectory,
        [Parameter(Mandatory = $true)][ValidateRange(1, 2147483647)][int]$TimeoutMilliseconds,
        [Parameter(Mandatory = $true)][string]$StandardOutputPath,
        [Parameter(Mandatory = $true)][string]$StandardErrorPath,
        [Parameter(Mandatory = $true)][string]$TestName,
        [hashtable]$EnvironmentOverrides
    )

    $process = New-Object Diagnostics.Process
    $outputStream = $null
    $errorStream = $null
    $captureTasks = @()
    $started = $false
    try {
        $outputStream = [IO.File]::Open($StandardOutputPath, [IO.FileMode]::Create, [IO.FileAccess]::Write, [IO.FileShare]::Read)
        $errorStream = [IO.File]::Open($StandardErrorPath, [IO.FileMode]::Create, [IO.FileAccess]::Write, [IO.FileShare]::Read)
        $process.StartInfo.FileName = $FilePath
        $process.StartInfo.Arguments = $Arguments
        $process.StartInfo.WorkingDirectory = $WorkingDirectory
        $process.StartInfo.UseShellExecute = $false
        $process.StartInfo.CreateNoWindow = $true
        $process.StartInfo.WindowStyle = [Diagnostics.ProcessWindowStyle]::Hidden
        $process.StartInfo.RedirectStandardOutput = $true
        $process.StartInfo.RedirectStandardError = $true
        if ($null -ne $EnvironmentOverrides) {
            # Process.Start inherits the raw parent environment. Override only this
            # child's copy; null removes a variable so the child can initialize it.
            foreach ($key in $EnvironmentOverrides.Keys) {
                if ($null -eq $EnvironmentOverrides[$key]) {
                    [void]$process.StartInfo.EnvironmentVariables.Remove([string]$key)
                } else {
                    $process.StartInfo.EnvironmentVariables[[string]$key] = [string]$EnvironmentOverrides[$key]
                }
            }
        }
        $timer = [Diagnostics.Stopwatch]::StartNew()
        try {
            # Process.Start retains the native handle. Start-Process with redirection on
            # Windows PowerShell 5.1 can return an object whose ExitCode getter fails.
            $started = $process.Start()
            if (-not $started) { throw 'Process.Start returned false.' }
        } catch {
            throw "$TestName could not start '$FilePath': $($_.Exception.Message)"
        }

        # Drain both pipes concurrently, including output larger than a pipe buffer.
        # Copy directly to files so output and timeout diagnostics stay bounded in memory.
        $captureTasks = [Threading.Tasks.Task[]]@(
            $process.StandardOutput.BaseStream.CopyToAsync($outputStream)
            $process.StandardError.BaseStream.CopyToAsync($errorStream)
        )
        $remaining = [int][Math]::Max(0, $TimeoutMilliseconds - $timer.ElapsedMilliseconds)
        $completed = $process.WaitForExit($remaining)
        if ($completed) {
            $remaining = [int][Math]::Max(0, $TimeoutMilliseconds - $timer.ElapsedMilliseconds)
            $completed = [Threading.Tasks.Task]::WaitAll($captureTasks, $remaining)
        }
        if (-not $completed) {
            throw "$TestName timed out after $TimeoutMilliseconds ms; cleanup is limited to its own child process (PID=$($process.Id)). See $StandardOutputPath and $StandardErrorPath"
        }

        $outputStream.Flush()
        $errorStream.Flush()
        # Invoke the getter explicitly: PowerShell property access can silently turn a
        # throwing getter into $null, which would otherwise be mistaken for success.
        $exitCode = $process.get_ExitCode()
        if ($exitCode -ne 0) {
            throw "$TestName failed: exit=$exitCode. See $StandardOutputPath and $StandardErrorPath"
        }
        [PSCustomObject]@{
            ExitCode = $exitCode
            ProcessId = $process.Id
            StandardOutputPath = $StandardOutputPath
            StandardErrorPath = $StandardErrorPath
        }
    } finally {
        try {
            if ($started) {
                # Never kill by image name or enumerate other processes. Kill() targets
                # only the exact Process.Start handle, not the process tree.
                if (-not $process.HasExited) {
                    $process.Kill()
                    if (-not $process.WaitForExit(5000)) {
                        throw "$TestName child process could not be stopped within 5 seconds (PID=$($process.Id))."
                    }
                }
                if ($captureTasks.Count -gt 0) {
                    if (-not [Threading.Tasks.Task]::WaitAll($captureTasks, 2000)) {
                        # A descendant could inherit a pipe after the direct child exits.
                        # Close our read ends instead of waiting on that other process.
                        $process.StandardOutput.Close()
                        $process.StandardError.Close()
                    }
                }
            }
        } finally {
            $process.Dispose()
            if ($null -ne $outputStream) { $outputStream.Dispose() }
            if ($null -ne $errorStream) { $errorStream.Dispose() }
        }
    }
}
