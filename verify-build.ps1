param([string]$Path=(Join-Path $PSScriptRoot 'outputs/TaskManagerX64.exe'))
$ErrorActionPreference='Stop'
$bytes=[IO.File]::ReadAllBytes($Path)
function U16([int]$offset){[BitConverter]::ToUInt16($bytes,$offset)}
function U32([int]$offset){[BitConverter]::ToUInt32($bytes,$offset)}
if((U16 0) -ne 0x5a4d){throw 'Missing DOS signature'}
$pe=U32 0x3c
if((U32 $pe) -ne 0x4550){throw 'Missing PE signature'}
if((U16 ($pe+4)) -ne 0x8664){throw 'Not AMD64'}
$optional=$pe+24
if((U16 $optional) -ne 0x20b){throw 'Not PE32+'}
if((U16 ($optional+68)) -ne 2){throw 'Not the Windows GUI subsystem'}
$entry=U32 ($optional+16)
$sectionCount=U16 ($pe+6)
$sectionStart=$optional+(U16 ($pe+20))
$entryIsCode=$false
for($i=0;$i -lt $sectionCount;$i++){
    $section=$sectionStart+40*$i
    $address=U32 ($section+12)
    $size=[Math]::Max((U32 ($section+8)),(U32 ($section+16)))
    if($entry -ge $address -and $entry -lt ($address+$size)){
        $entryIsCode=((U32 ($section+36)) -band 0x20000000) -ne 0
    }
}
if(-not $entryIsCode){throw 'Entry point is not executable'}
$mapPath=Join-Path $PSScriptRoot ('work/'+[IO.Path]::GetFileNameWithoutExtension($Path)+'.map')
$map=[IO.File]::ReadAllText($mapPath)
$line=($map -split '\r?\n' | Where-Object {$_ -match '\sSimpleEntry\s+([0-9a-fA-F]+)\s'})
if(-not $line -or $line.Count -gt 1){throw 'Missing or ambiguous SimpleEntry in linker map'}
$null=$line -match '\sSimpleEntry\s+([0-9a-fA-F]+)\s'
$entryAddress=[Convert]::ToUInt64($Matches[1],16)
$imageBase=[BitConverter]::ToUInt64($bytes,$optional+24)
if(($entryAddress-$imageBase) -ne $entry){throw 'PE entry differs from linker SimpleEntry'}
$security=U16 ($optional+70)
if(($security -band 0x160) -ne 0x160){throw 'ASLR/high entropy/NX flags missing'}
if($bytes.Length -gt 262144){throw 'Unexpected EXE size regression (limit 256 KiB)'}
[PSCustomObject]@{File=[IO.Path]::GetFileName($Path);Architecture='AMD64';Entry=('SimpleEntry RVA 0x{0:X}' -f $entry);Bytes=$bytes.Length;SHA256=(Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash}
