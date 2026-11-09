param([switch]$Preview,[string]$OutputPath)
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $MyInvocation.MyCommand.Path
$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/vswhere.exe'
$vs = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (-not $vs) { throw 'Visual Studio Build Tools C++ is required.' }
$vcvars = Join-Path $vs 'VC/Auxiliary/Build/vcvars64.bat'
& (Join-Path $root 'build-resources.ps1')
& (Join-Path $root 'make-icon.ps1')
$src = Join-Path $root 'src/taskmanager.asm'
$out = if($OutputPath){[IO.Path]::GetFullPath($OutputPath)}else{Join-Path $root 'TaskManagerX64.exe'}
[void][IO.Directory]::CreateDirectory((Split-Path -Parent $out))
[void][IO.Directory]::CreateDirectory((Join-Path $root 'work'))
$map = Join-Path $root ('work/'+[IO.Path]::GetFileNameWithoutExtension($out)+'.map')
$obj = Join-Path $root 'taskmanager.obj'
$manifest = Join-Path $root 'TaskManagerX64.manifest'
$rcOptions = if($Preview){'/d PREVIEW'}else{''}
$cmd = "call `"$vcvars`" >nul && ml64 /nologo /I`"$root\src`" /c /Fo`"$obj`" `"$src`" && rc /nologo $rcOptions /fo`"$root\taskmanager.res`" `"$root\taskmanager.rc`" && link /nologo /MANIFEST:NO /SUBSYSTEM:WINDOWS /DYNAMICBASE /HIGHENTROPYVA /NXCOMPAT /CETCOMPAT /MAP:`"$map`" /ENTRY:SimpleEntry /OUT:`"$out`" `"$obj`" `"$root\taskmanager.res`" kernel32.lib user32.lib gdi32.lib comctl32.lib psapi.lib advapi32.lib shell32.lib wtsapi32.lib shlwapi.lib pdh.lib iphlpapi.lib dwmapi.lib dbghelp.lib d2d1.lib dwrite.lib ole32.lib oleaut32.lib uuid.lib"
Push-Location $root
try { & cmd.exe /d /s /c $cmd; if ($LASTEXITCODE) { throw "Build failed: $LASTEXITCODE" } }
finally { Pop-Location }
Write-Output $out






