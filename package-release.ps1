param(
    [string]$SourceDirectory = $PSScriptRoot,
    [string]$InputDirectory,
    [string]$OutputDirectory
)

$ErrorActionPreference = 'Stop'
$root = [IO.Path]::GetFullPath($SourceDirectory)
$inputPath = if ($InputDirectory) { [IO.Path]::GetFullPath($InputDirectory) } else { Join-Path $root 'outputs' }
$out = if ($OutputDirectory) { [IO.Path]::GetFullPath($OutputDirectory) } else { $inputPath }
$releaseFiles = @('TaskManagerX64.exe', 'TaskManagerX64-admin.exe', 'TaskManagerX64-standard.exe', 'CHANGELOG.txt', 'verification.txt')

# Validate the complete input set before creating or replacing any output file.
foreach ($name in $releaseFiles) {
    if (-not (Test-Path -LiteralPath (Join-Path $inputPath $name) -PathType Leaf)) {
        throw "Required release input is missing: $(Join-Path $inputPath $name)"
    }
}
foreach ($directory in @('src', 'res')) {
    if (-not (Test-Path -LiteralPath (Join-Path $root $directory) -PathType Container)) {
        throw "Required source directory is missing: $(Join-Path $root $directory)"
    }
}
foreach ($name in @('README.md', 'src/taskmanager.asm', 'res/taskmanager.ico', 'res/taskmanager.png', 'build-taskmanager.ps1', 'native-test-process.ps1')) {
    if (-not (Test-Path -LiteralPath (Join-Path $root $name) -PathType Leaf)) {
        throw "Required source input is missing: $(Join-Path $root $name)"
    }
}

$stageParent = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot 'work'))
$stage = Join-Path $stageParent ('package-' + [Guid]::NewGuid().ToString('N'))
$release = Join-Path $stage 'release/TaskManagerX64'
$source = Join-Path $stage 'source/TaskManagerX64-source'
$reports = New-Object 'Collections.Generic.List[object]'
try {
    [void][IO.Directory]::CreateDirectory($release)
    [void][IO.Directory]::CreateDirectory($source)
    foreach ($name in $releaseFiles) {
        Copy-Item -LiteralPath (Join-Path $inputPath $name) -Destination $release
    }
    Copy-Item -LiteralPath (Join-Path $root 'README.md') -Destination $release
    Copy-Item -LiteralPath (Join-Path $root 'res/taskmanager.png') -Destination (Join-Path $release 'TaskManagerX64-icon.png')
    $binaries = @(foreach ($name in $releaseFiles | Where-Object { $_ -like '*.exe' }) {
        $file = Get-Item -LiteralPath (Join-Path $release $name)
        [PSCustomObject]@{ File = $name; Bytes = $file.Length; SHA256 = (Get-FileHash -LiteralPath $file.FullName -Algorithm SHA256).Hash }
    })
    $hashes = @($binaries | ForEach-Object { $_.SHA256 + '  ' + $_.File })
    [IO.File]::WriteAllLines((Join-Path $release 'SHA256SUMS.txt'), [string[]]$hashes)
    # Packaging records its inputs; it neither builds binaries nor refreshes the
    # supplied verification document, including when the default outputs/ is used.
    $metadata = [PSCustomObject]@{
        PackagedAtUtc = [DateTime]::UtcNow.ToString('o')
        BuildPerformed = $false
        VerificationSource = 'supplied verification.txt'
        BinarySourceCorrespondence = 'not checked by packaging'
        Binaries = $binaries
    } | ConvertTo-Json -Depth 4
    [IO.File]::WriteAllText((Join-Path $release 'PACKAGING.json'), $metadata, [Text.Encoding]::UTF8)

    # All MASM includes live under src/. There is no required root include/.
    foreach ($directory in @('src', 'res', 'docs')) {
        $path = Join-Path $root $directory
        if (Test-Path -LiteralPath $path -PathType Container) {
            Copy-Item -LiteralPath $path -Destination $source -Recurse
        }
    }
    Get-ChildItem -LiteralPath $root -File -Force | Where-Object {
        $_.Extension -in @('.ps1', '.manifest', '.rc') -or $_.Name -in @('README.md', '.gitignore', 'DEVELOPMENT.md')
    } | ForEach-Object { Copy-Item -LiteralPath $_.FullName -Destination $source }
    Copy-Item -LiteralPath (Join-Path $release 'PACKAGING.json') -Destination $source

    Add-Type -AssemblyName System.IO.Compression
    Add-Type -AssemblyName System.IO.Compression.FileSystem
    foreach ($name in @('TaskManagerX64-release.zip', 'TaskManagerX64-source.zip')) {
        $content = if ($name -like '*-release.zip') { $release } else { $source }
        $prefix = [IO.Path]::GetFileName($content) + '/'
        $contentFiles = @(Get-ChildItem -LiteralPath $content -Recurse -File -Force)
        $required = @($contentFiles | ForEach-Object {
            $prefix + $_.FullName.Substring($content.Length + 1).Replace('\', '/')
        })
        if ($required | Where-Object { $_ -match '(^|/)(work|\.git|\.vscode|outputs)(/|$)' }) {
            throw 'Workspace internals entered the staged package.'
        }
        $path = Join-Path $stage $name
        # Windows PowerShell 5.1 Compress-Archive writes backslash entry names.
        # Explicit ZIP entries use portable forward slashes and include dotfiles.
        $archive = [IO.Compression.ZipFile]::Open($path, [IO.Compression.ZipArchiveMode]::Create)
        try {
            foreach ($file in $contentFiles) {
                $entryName = $prefix + $file.FullName.Substring($content.Length + 1).Replace('\', '/')
                [void][IO.Compression.ZipFileExtensions]::CreateEntryFromFile($archive, $file.FullName, $entryName, [IO.Compression.CompressionLevel]::Optimal)
            }
        } finally { $archive.Dispose() }
        $zip = [IO.Compression.ZipFile]::OpenRead($path)
        try {
            $entries = @($zip.Entries.FullName)
            foreach ($entry in $required) {
                if ($entry -notin $entries) { throw "Archive is missing $entry" }
            }
            if ($entries | Where-Object { $_ -match '(^|/)(work|\.git|\.vscode|outputs)(/|$)' }) {
                throw 'Workspace internals entered the archive.'
            }
            $reports.Add([PSCustomObject]@{ Archive = $name; Entries = $entries.Count; Bytes = (Get-Item -LiteralPath $path).Length; SHA256 = (Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash })
        } finally { $zip.Dispose() }
    }

    # Both archives are complete and validated before previous artifacts change.
    [void][IO.Directory]::CreateDirectory($out)
    foreach ($name in @('README.md', 'TaskManagerX64-icon.png', 'SHA256SUMS.txt', 'PACKAGING.json')) {
        Copy-Item -LiteralPath (Join-Path $release $name) -Destination (Join-Path $out $name) -Force
    }
    foreach ($name in @('TaskManagerX64-release.zip', 'TaskManagerX64-source.zip')) {
        Copy-Item -LiteralPath (Join-Path $stage $name) -Destination (Join-Path $out $name) -Force
    }
    $reports.ToArray()
} finally {
    if (Test-Path -LiteralPath $stage) {
        if ([IO.Path]::GetDirectoryName([IO.Path]::GetFullPath($stage)) -ne $stageParent) { throw 'Package cleanup escaped its work directory.' }
        Remove-Item -LiteralPath $stage -Recurse -Force
    }
}
