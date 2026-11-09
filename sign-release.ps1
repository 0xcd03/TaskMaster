param(
  [Parameter(Mandatory=$true)][ValidatePattern('^[A-Fa-f0-9]{40}$')][string]$CertificateThumbprint,
  [switch]$LocalMachine,
  [uri]$TimestampUrl='http://timestamp.digicert.com',
  [string[]]$Files=(Join-Path $PSScriptRoot 'outputs\TaskManagerX64.exe')
)
$ErrorActionPreference='Stop'
$signtool=Get-ChildItem 'C:\Program Files (x86)\Windows Kits\10\bin' -Recurse -Filter signtool.exe -ErrorAction Stop | Where-Object FullName -match '\\x64\\signtool.exe$' | Sort-Object FullName -Descending | Select-Object -First 1 -ExpandProperty FullName
if(-not $signtool){throw 'Windows SDK signtool.exe was not found.'}
$store=if($LocalMachine){'Cert:\LocalMachine\My'}else{'Cert:\CurrentUser\My'}
$certificate=Get-Item -LiteralPath (Join-Path $store $CertificateThumbprint) -ErrorAction Stop
if(-not $certificate.HasPrivateKey){throw 'The signing certificate has no accessible private key.'}
if($certificate.NotAfter -lt (Get-Date)){throw 'The signing certificate has expired.'}
$eku=@($certificate.EnhancedKeyUsageList | ForEach-Object {$_.ObjectId.Value})
if('1.3.6.1.5.5.7.3.3' -notin $eku){throw 'A Code Signing certificate is required.'}
foreach($file in $Files){
  if(-not (Test-Path -LiteralPath $file)){throw "File was not found: $file"}
  $signArgs=@('sign','/fd','SHA256','/sha1',$CertificateThumbprint,'/s','My','/tr',$TimestampUrl.AbsoluteUri,'/td','SHA256','/d','TaskManagerX64','/du','https://github.com/0xcd03/TaskMaster')
  if($LocalMachine){$signArgs+='/sm'}
  & $signtool @signArgs $file
  if($LASTEXITCODE){throw "Signing failed: $file"}
  & $signtool verify /pa /all /v $file
  if($LASTEXITCODE){throw "Signature verification failed: $file"}
}
Write-Output 'Signed and verified. Repackage ZIP assets after signing.'