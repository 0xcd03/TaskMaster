# Uses existing development binaries; never builds or replaces outputs/ releases.
$ErrorActionPreference = 'Stop'
$root = $PSScriptRoot
Add-Type -AssemblyName System.IO.Compression.FileSystem

function Assert-Package([bool]$Condition, [string]$Message) {
    if (-not $Condition) { throw $Message }
}

function Get-DirectoryFingerprint([string]$Directory) {
    if (-not [IO.Directory]::Exists($Directory)) { return @() }
    @(Get-ChildItem -LiteralPath $Directory -File | Sort-Object Name | ForEach-Object {
        $_.Name + ':' + $_.Length + ':' + (Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash
    })
}

function Get-ZipEntryHash($Entry) {
    $stream = $Entry.Open()
    $sha = [Security.Cryptography.SHA256]::Create()
    try { [BitConverter]::ToString($sha.ComputeHash($stream)).Replace('-', '') }
    finally { $sha.Dispose(); $stream.Dispose() }
}

function Read-ZipEntryText($Entry) {
    $reader = New-Object IO.StreamReader($Entry.Open())
    try { $reader.ReadToEnd() } finally { $reader.Dispose() }
}

function Assert-Archives([string]$OutputDirectory, [string]$InputDirectory, [string]$SourceDirectory) {
    $zip = [IO.Compression.ZipFile]::OpenRead((Join-Path $OutputDirectory 'TaskManagerX64-release.zip'))
    try {
        $entries = @($zip.Entries.FullName)
        foreach ($name in @('TaskManagerX64.exe', 'TaskManagerX64-admin.exe', 'TaskManagerX64-standard.exe', 'CHANGELOG.txt', 'verification.txt', 'README.md', 'TaskManagerX64-icon.png', 'SHA256SUMS.txt', 'PACKAGING.json')) {
            Assert-Package (('TaskManagerX64/' + $name) -in $entries) "Release entry missing: $name"
        }
        $hashLines = (Read-ZipEntryText $zip.GetEntry('TaskManagerX64/SHA256SUMS.txt')) -split '\r?\n' | Where-Object { $_ }
        Assert-Package (@($hashLines).Count -eq 3) 'Expected exactly three executable hashes.'
        foreach ($name in @('TaskManagerX64.exe', 'TaskManagerX64-admin.exe', 'TaskManagerX64-standard.exe')) {
            $expectedHash = (Get-FileHash -LiteralPath (Join-Path $InputDirectory $name) -Algorithm SHA256).Hash
            Assert-Package ((Get-ZipEntryHash $zip.GetEntry(('TaskManagerX64/' + $name))) -eq $expectedHash) "Packaged binary differs from input: $name"
            Assert-Package (($expectedHash + '  ' + $name) -in $hashLines) "Checksum manifest is wrong: $name"
        }
        $metadata = Read-ZipEntryText $zip.GetEntry('TaskManagerX64/PACKAGING.json') | ConvertFrom-Json
        Assert-Package ($metadata.BuildPerformed -eq $false) 'Packaging metadata incorrectly claims a build.'
        Assert-Package ($metadata.VerificationSource -eq 'supplied verification.txt') 'Packaging must identify the supplied verification document.'
        Assert-Package ((Get-ZipEntryHash $zip.GetEntry('TaskManagerX64/README.md')) -eq (Get-FileHash -LiteralPath (Join-Path $SourceDirectory 'README.md')).Hash) 'README differs from the chosen source tree.'
    } finally { $zip.Dispose() }

    $zip = [IO.Compression.ZipFile]::OpenRead((Join-Path $OutputDirectory 'TaskManagerX64-source.zip'))
    try {
        $entries = @($zip.Entries.FullName)
        foreach ($name in @('src/taskmanager.asm', 'src/thread_metrics.inc', 'src/thread_metrics_tests.inc', 'src/service_enumeration_tests.inc', 'res/taskmanager.ico', 'DEVELOPMENT.md', 'docs/ROADMAP.md', 'native-test-process.ps1', 'test-native-test-process.ps1', 'test-services.ps1', 'test-thread-metrics.ps1', 'test-package-release.ps1', 'package-release.ps1')) {
            $entry = $zip.GetEntry(('TaskManagerX64-source/' + $name))
            Assert-Package ($null -ne $entry) "Source entry missing: $name"
            Assert-Package ((Get-ZipEntryHash $entry) -eq (Get-FileHash -LiteralPath (Join-Path $SourceDirectory $name)).Hash) "Source entry is stale: $name"
        }
        Assert-Package (@($entries | Where-Object { $_ -match '(^|/)(work|\.git|\.vscode|outputs)(/|$)' }).Count -eq 0) 'Workspace internals entered the archive.'
    } finally { $zip.Dispose() }
}

$originalOutputs = @(Get-DirectoryFingerprint (Join-Path $root 'outputs'))
$fixtureRoot = [IO.Path]::GetFullPath((Join-Path $root ('work/package-test-' + [Guid]::NewGuid().ToString('N'))))
$fixtureSource = Join-Path $fixtureRoot 'source'
$fixtureInput = Join-Path $fixtureSource 'outputs'
$fixtureOutput = Join-Path $fixtureRoot 'artifacts'
[void][IO.Directory]::CreateDirectory($fixtureInput)
foreach ($directory in @('src', 'res', 'docs')) {
    Copy-Item -LiteralPath (Join-Path $root $directory) -Destination $fixtureSource -Recurse
}
Get-ChildItem -LiteralPath $root -File | Where-Object {
    $_.Extension -in @('.ps1', '.manifest', '.rc') -or $_.Name -in @('README.md', '.gitignore', 'DEVELOPMENT.md')
} | ForEach-Object { Copy-Item -LiteralPath $_.FullName -Destination $fixtureSource }
Copy-Item -LiteralPath (Join-Path $root 'work/TaskManagerX64.exe') -Destination (Join-Path $fixtureInput 'TaskManagerX64.exe')
Copy-Item -LiteralPath (Join-Path $root 'work/TaskManagerX64.exe') -Destination (Join-Path $fixtureInput 'TaskManagerX64-admin.exe')
Copy-Item -LiteralPath (Join-Path $root 'work/TaskManagerX64-standard.exe') -Destination $fixtureInput
[IO.File]::WriteAllText((Join-Path $fixtureInput 'CHANGELOG.txt'), 'Local packaging fixture: existing development binaries, no rebuild by this test.')
[IO.File]::WriteAllText((Join-Path $fixtureInput 'verification.txt'), 'Packaging test inputs only. Native suite results are recorded separately in work/validation/summary.json.')
foreach ($directory in @('work', '.git', '.vscode')) {
    $junk = Join-Path $fixtureSource $directory
    [void][IO.Directory]::CreateDirectory($junk)
    [IO.File]::WriteAllText((Join-Path $junk 'must-not-package.txt'), 'workspace internals')
}
$packager = Join-Path $fixtureSource 'package-release.ps1'
$succeeded = $false
try {
    # This reproduces the old missing include/ failure using the exact default layout.
    $null = & $packager
    Assert-Archives -OutputDirectory $fixtureInput -InputDirectory $fixtureInput -SourceDirectory $fixtureSource
    'PASS: default layout packages current binaries/source without a root include directory.'

    $null = & $packager -SourceDirectory $fixtureSource -InputDirectory $fixtureInput -OutputDirectory $fixtureOutput
    Assert-Archives -OutputDirectory $fixtureOutput -InputDirectory $fixtureInput -SourceDirectory $fixtureSource
    'PASS: explicit input/output directories, ZIP contents and independent executable hashes.'

    foreach ($missingName in @('TaskManagerX64-standard.exe', 'verification.txt')) {
        $before = @(Get-DirectoryFingerprint $fixtureOutput)
        $missingPath = Join-Path $fixtureInput $missingName
        $savedPath = $missingPath + '.saved'
        Move-Item -LiteralPath $missingPath -Destination $savedPath
        try {
            $rejected = $false
            try { $null = & $packager -SourceDirectory $fixtureSource -InputDirectory $fixtureInput -OutputDirectory $fixtureOutput }
            catch {
                Assert-Package ($_.Exception.Message.Contains($missingName)) "Wrong missing-input failure: $($_.Exception.Message)"
                $rejected = $true
            }
            Assert-Package $rejected "Missing input accepted: $missingName"
            $after = @(Get-DirectoryFingerprint $fixtureOutput)
            Assert-Package (($before -join '|') -eq ($after -join '|')) "Missing input replaced previous artifacts: $missingName"
        } finally { Move-Item -LiteralPath $savedPath -Destination $missingPath }
    }
    'PASS: missing executable or verification input preserves every previous output byte.'
    $afterOutputs = @(Get-DirectoryFingerprint (Join-Path $root 'outputs'))
    Assert-Package (($originalOutputs -join '|') -eq ($afterOutputs -join '|')) 'Downloaded outputs/ changed during isolated packaging tests.'
    'PASS: downloaded outputs/ remain byte-for-byte unchanged.'
    $succeeded = $true
    'Local package artifacts: ' + $fixtureOutput
} finally {
    if (-not $succeeded) { 'Failed packaging fixture retained at ' + $fixtureRoot }
}
