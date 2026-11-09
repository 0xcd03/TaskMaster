$ErrorActionPreference = 'Stop'
$root = $PSScriptRoot
$strings = Get-Content -LiteralPath (Join-Path $root 'res/ui-strings.json') -Raw -Encoding utf8 | ConvertFrom-Json
$asm = [Text.StringBuilder]::new()
[void]$asm.AppendLine('; Generated from res/ui-strings.json. Resource buffers never contain counted strings.')
[void]$asm.AppendLine('.data?')
foreach($entry in $strings.PSObject.Properties) {
    [void]$asm.AppendLine(($entry.Name + ' dw 512 dup(?)'))
}
[void]$asm.AppendLine('.data')
[void]$asm.AppendLine('uiStringTable label qword')
foreach($entry in $strings.PSObject.Properties) { [void]$asm.AppendLine(('dq ' + $entry.Name)) }
[void]$asm.AppendLine('uiStringCount equ ' + @($strings.PSObject.Properties).Count)
[void]$asm.AppendLine(@'
.code
LoadUiStrings PROC
  push rbx
  push rsi
  sub rsp,28h
  lea rbx,uiStringTable
  xor esi,esi
lusNext:
  mov edx,1000
  cmp appLanguage,0
  je lusEnglish
  add edx,2000
lusEnglish:
  add edx,esi
  mov rcx,gInst
  mov r8,[rbx+rsi*8]
  mov r9d,512
  call LoadStringW
  inc esi
  cmp esi,uiStringCount
  jb lusNext
  add rsp,28h
  pop rsi
  pop rbx
  ret
LoadUiStrings ENDP
'@)
[IO.File]::WriteAllText((Join-Path $root 'src/ui_strings.inc'),$asm.ToString(),[Text.UTF8Encoding]::new($false))
$rc=[Text.StringBuilder]::new()
[void]$rc.AppendLine('#pragma code_page(65001)')
[void]$rc.AppendLine('#ifdef PREVIEW')
[void]$rc.AppendLine('1 24 "preview.manifest"')
[void]$rc.AppendLine('#else')
[void]$rc.AppendLine('1 24 "TaskManagerX64.manifest"')
[void]$rc.AppendLine('#endif')
[void]$rc.AppendLine('103 ICON "res\\taskmanager.ico"')
[void]$rc.AppendLine([IO.File]::ReadAllText((Join-Path $root 'res/menus.rc')))
[void]$rc.AppendLine('STRINGTABLE')
[void]$rc.AppendLine('BEGIN')
$index=0
foreach($entry in $strings.PSObject.Properties) {
    if($entry.Value.Count -ne 2){throw ('Missing EN/RU: '+$entry.Name)}
    for($lang=0;$lang -lt 2;$lang++) {
        $text=[string]$entry.Value[$lang]
        if(-not $text -or $text.Length -ge 512){throw ('Invalid string length: '+$entry.Name)}
        $id=1000+$index+2000*$lang
        $escaped=$text.Replace('"','""')
        [void]$rc.AppendLine((' {0} "{1}"' -f $id,$escaped))
    }
    $enArgs=([regex]::Matches($entry.Value[0],'%I64u|%[su]') | ForEach-Object Value) -join ','
    $ruArgs=([regex]::Matches($entry.Value[1],'%I64u|%[su]') | ForEach-Object Value) -join ','
    if($enArgs -cne $ruArgs){throw ('Format arguments differ: '+$entry.Name)}
    $index++
}
[void]$rc.AppendLine('END')
[IO.File]::WriteAllText((Join-Path $root 'taskmanager.rc'),$rc.ToString(),[Text.UTF8Encoding]::new($true))
