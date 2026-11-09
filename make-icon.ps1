Add-Type -AssemblyName System.Drawing
$root=$PSScriptRoot
$res=Join-Path $root 'res'
New-Item -ItemType Directory -Force -Path $res | Out-Null
$bmp=New-Object Drawing.Bitmap 256,256,([Drawing.Imaging.PixelFormat]::Format32bppArgb)
$g=[Drawing.Graphics]::FromImage($bmp)
$g.SmoothingMode=[Drawing.Drawing2D.SmoothingMode]::AntiAlias
$g.Clear([Drawing.Color]::FromArgb(14,24,42))
$bg=[Drawing.Drawing2D.LinearGradientBrush]::new(([Drawing.Rectangle]::new(0,0,256,256)),[Drawing.Color]::FromArgb(24,82,156),[Drawing.Color]::FromArgb(15,181,170),45)
$g.FillRectangle($bg,0,0,256,256)
$bg.Dispose()
$white=[Drawing.SolidBrush]::new([Drawing.Color]::FromArgb(245,250,255))
$cyan=[Drawing.SolidBrush]::new([Drawing.Color]::FromArgb(111,232,220))
$blue=[Drawing.SolidBrush]::new([Drawing.Color]::FromArgb(73,156,255))
$pen=[Drawing.Pen]::new([Drawing.Color]::FromArgb(205,235,255),9)
$pen.StartCap=[Drawing.Drawing2D.LineCap]::Round;$pen.EndCap=[Drawing.Drawing2D.LineCap]::Round
$g.DrawArc($pen,37,32,182,182,215,110)
$g.FillRectangle($white,55,158,25,38);$g.FillRectangle($cyan,92,128,25,68);$g.FillRectangle($blue,129,94,25,102);$g.FillRectangle($white,166,62,25,134)
$g.DrawLine($pen,128,128,176,88)
$g.FillEllipse($cyan,115,115,26,26)
$pen.Dispose();$white.Dispose();$cyan.Dispose();$blue.Dispose();$g.Dispose()
$ico=Join-Path $res 'taskmanager.ico'
$bmp.Save((Join-Path $res 'taskmanager.png'),[Drawing.Imaging.ImageFormat]::Png)
$sizes=@(16,20,24,32,40,48,64,128,256)
$frames=@()
foreach($size in $sizes){
  $scaled=[Drawing.Bitmap]::new($size,$size)
  $graphics=[Drawing.Graphics]::FromImage($scaled)
  $graphics.InterpolationMode=[Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
  $graphics.DrawImage($bmp,0,0,$size,$size)
  $graphics.Dispose()
  $stream=[IO.MemoryStream]::new()
  $scaled.Save($stream,[Drawing.Imaging.ImageFormat]::Png)
  $frames+=,@{Size=$size;Bytes=$stream.ToArray()}
  $stream.Dispose();$scaled.Dispose()
}
$stream=[IO.File]::Create($ico)
$writer=[IO.BinaryWriter]::new($stream)
$writer.Write([uint16]0);$writer.Write([uint16]1);$writer.Write([uint16]$sizes.Count)
$offset=6+16*$sizes.Count
foreach($frame in $frames){
  $dimension=if($frame.Size -eq 256){0}else{$frame.Size}
  $writer.Write([byte]$dimension);$writer.Write([byte]$dimension)
  $writer.Write([byte]0);$writer.Write([byte]0)
  $writer.Write([uint16]1);$writer.Write([uint16]32)
  $writer.Write([uint32]$frame.Bytes.Length);$writer.Write([uint32]$offset)
  $offset+=$frame.Bytes.Length
}
foreach($frame in $frames){$writer.Write([byte[]]$frame.Bytes)}
$writer.Dispose()
$bmp.Dispose()
Write-Output $ico