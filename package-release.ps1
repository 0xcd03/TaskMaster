$ErrorActionPreference = 'Stop'
$root = $PSScriptRoot
$out = Join-Path $root 'outputs'
$stage = Join-Path $root ('work/package-' + [Guid]::NewGuid().ToString('N'))
$release = Join-Path $stage 'release/TaskManagerX64'
$source = Join-Path $stage 'source/TaskManagerX64-source'
[void][IO.Directory]::CreateDirectory($release)
[void][IO.Directory]::CreateDirectory($source)

$releaseFiles = @('TaskManagerX64.exe', 'TaskManagerX64-admin.exe', 'TaskManagerX64-standard.exe', 'CHANGELOG.txt', 'verification.txt')
foreach ($name in $releaseFiles) {
    Copy-Item -LiteralPath (Join-Path $out $name) -Destination $release
}
Copy-Item -LiteralPath (Join-Path $root 'README.md') -Destination (Join-Path $out 'README.md') -Force
Copy-Item -LiteralPath (Join-Path $root 'README.md') -Destination $release
Copy-Item -LiteralPath (Join-Path $root 'res/taskmanager.png') -Destination (Join-Path $out 'TaskManagerX64-icon.png') -Force
Copy-Item -LiteralPath (Join-Path $root 'res/taskmanager.png') -Destination (Join-Path $release 'TaskManagerX64-icon.png')
$hashes = foreach ($name in $releaseFiles | Where-Object { $_ -like '*.exe' }) {
    $hash = (Get-FileHash -LiteralPath (Join-Path $release $name) -Algorithm SHA256).Hash
    "$hash  $name"
}
[IO.File]::WriteAllLines((Join-Path $release 'SHA256SUMS.txt'), [string[]]$hashes)
Copy-Item -LiteralPath (Join-Path $release 'SHA256SUMS.txt') -Destination $out -Force

foreach ($directory in @('src', 'res', 'include')) {
    Copy-Item -LiteralPath (Join-Path $root $directory) -Destination $source -Recurse
}
Get-ChildItem -LiteralPath $root -File | Where-Object {
    $_.Extension -in @('.ps1', '.manifest', '.rc') -or $_.Name -in @('README.md', '.gitignore')
} | ForEach-Object { Copy-Item -LiteralPath $_.FullName -Destination $source }
Compress-Archive -LiteralPath $release -DestinationPath (Join-Path $out 'TaskManagerX64-release.zip') -CompressionLevel Optimal -Force
Compress-Archive -LiteralPath $source -DestinationPath (Join-Path $out 'TaskManagerX64-source.zip') -CompressionLevel Optimal -Force

Add-Type -AssemblyName System.IO.Compression.FileSystem
foreach ($name in @('TaskManagerX64-release.zip', 'TaskManagerX64-source.zip')) {
    $path = Join-Path $out $name
    $zip = [IO.Compression.ZipFile]::OpenRead($path)
    try {
        $entries = @($zip.Entries.FullName)
        $required = if ($name -like '*-release.zip') {
            @('TaskManagerX64/TaskManagerX64.exe', 'TaskManagerX64/README.md', 'TaskManagerX64/SHA256SUMS.txt')
        } else {
            @('TaskManagerX64-source/src/taskmanager.asm', 'TaskManagerX64-source/res/taskmanager.ico', 'TaskManagerX64-source/build-taskmanager.ps1')
        }
        foreach ($entry in $required) {
            if ($entry -notin $entries) { throw "Archive is missing $entry" }
        }
        if ($entries | Where-Object { $_ -match '(^|/)(work|\.git)(/|$)' }) { throw 'Scratch/Git files entered the package.' }
        [PSCustomObject]@{ Archive = $name; Entries = $entries.Count; Bytes = (Get-Item -LiteralPath $path).Length; SHA256 = (Get-FileHash -LiteralPath $path).Hash }
    } finally { $zip.Dispose() }
}
