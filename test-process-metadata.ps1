param()
$ErrorActionPreference='Stop'
$root=$PSScriptRoot
. (Join-Path $root 'native-test-process.ps1')
$vs = & "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if(-not $vs){throw 'Visual Studio Build Tools C++ is required.'}
$vc=Join-Path $vs 'VC/Auxiliary/Build/vcvars64.bat'
$work=Join-Path $root ('work/process-metadata-'+[Guid]::NewGuid().ToString('N'))
[void][IO.Directory]::CreateDirectory($work)
$obj=Join-Path $work 'metadata.obj'
$res=Join-Path $work 'metadata.res'
$entries=@(
 @{Entry='ProcessMetadataCacheTest';Name='cache'},
 @{Entry='PmHandlesTest';Name='handles'},
 @{Entry='ProcessMetadataPresentationTest';Name='presentation'}
 @{Entry='ProcessDetailsFixtureEntry';Name='process-details-fixture';Fixture=$true},
 @{Entry='PmCollectorTest';Name='collector'},
 @{Entry='PmWorkerTest';Name='worker'},
 @{Entry='PMUiTest';Name='ui'}
)
$libs='kernel32.lib user32.lib gdi32.lib comctl32.lib psapi.lib advapi32.lib shell32.lib wtsapi32.lib shlwapi.lib pdh.lib iphlpapi.lib dwmapi.lib dbghelp.lib d2d1.lib dwrite.lib ole32.lib oleaut32.lib uuid.lib uxtheme.lib'
$command="call `"$vc`" >nul && ml64 /nologo /D TM_TESTS /D PM_TESTS /D PD_TESTS /I src /c /Fo`"$obj`" src\taskmanager.asm && rc /nologo /d PREVIEW /fo`"$res`" taskmanager.rc"
foreach($entry in $entries){
 $entry.Exe=Join-Path $work ($entry.Name+'.exe')
 $command+=" && link /nologo /SUBSYSTEM:CONSOLE /ENTRY:$($entry.Entry) /OUT:`"$($entry.Exe)`" `"$obj`" `"$res`" $libs"
}
Write-Output "Process metadata owned evidence: $work"
Push-Location $root
try {
 & cmd.exe /d /s /c $command
 if($LASTEXITCODE){throw "Metadata actual-main build failed: $LASTEXITCODE"}
 foreach($entry in $entries){
  if($entry.Fixture){continue}
  $log=Join-Path $work ($entry.Name+'.log')
  try {
   $null=Invoke-NativeTestProcess -FilePath $entry.Exe -WorkingDirectory $work -TimeoutMilliseconds 30000 -StandardOutputPath $log -StandardErrorPath (Join-Path $work ($entry.Name+'-errors.log')) -TestName ('Process metadata '+$entry.Name)
   Write-Output ('PASS: '+$entry.Name)
  } finally {if(Test-Path -LiteralPath $log){Get-Content -LiteralPath $log}}
 }
 Write-Output "PASS: process metadata mandatory gates. Evidence: $work"
} finally {Pop-Location}
