Add-Type -AssemblyName System.Drawing
$root=$PSScriptRoot
$res=Join-Path $root 'res'
New-Item -ItemType Directory -Force -Path $res | Out-Null
$bmp=New-Object Drawing.Bitmap 256,256,([Drawing.Imaging.PixelFormat]::Format32bppArgb)
$g=[Drawing.Graphics]::FromImage($bmp)
$g.SmoothingMode=[Drawing.Drawing2D.SmoothingMode]::AntiAlias
$g.Clear([Drawing.Color]::Transparent)
# Original TaskMaster-inspired shield mark: a blue/graphite round shield with
# an orange angular T. It is a new app emblem, not a copied character asset.
$outer=[Drawing.SolidBrush]::new([Drawing.Color]::FromArgb(232,236,242))
$g.FillEllipse($outer,8,8,240,240)
$outer.Dispose()
$ring=[Drawing.SolidBrush]::new([Drawing.Color]::FromArgb(31,42,62))
$g.FillEllipse($ring,20,20,216,216)
$ring.Dispose()
$inner=[Drawing.Drawing2D.LinearGradientBrush]::new(([Drawing.Rectangle]::new(28,28,200,200)),[Drawing.Color]::FromArgb(78,97,122),[Drawing.Color]::FromArgb(22,28,43),45)
$g.FillEllipse($inner,28,28,200,200)
$inner.Dispose()
$edge=[Drawing.Pen]::new([Drawing.Color]::FromArgb(203,211,224),3)
$g.DrawEllipse($edge,28,28,200,200)
$edge.Dispose()
# The orange crest uses a shield-like chevron and central stem, legible at 16px.
$orange=[Drawing.SolidBrush]::new([Drawing.Color]::FromArgb(238,108,24))
$crest=[Drawing.Drawing2D.GraphicsPath]::new()
$crest.AddPolygon([Drawing.Point[]]@(
  [Drawing.Point]::new(42,76), [Drawing.Point]::new(82,88),
  [Drawing.Point]::new(128,72), [Drawing.Point]::new(174,88),
  [Drawing.Point]::new(214,76), [Drawing.Point]::new(202,122),
  [Drawing.Point]::new(166,108), [Drawing.Point]::new(151,124),
  [Drawing.Point]::new(151,184), [Drawing.Point]::new(128,204),
  [Drawing.Point]::new(105,184), [Drawing.Point]::new(105,124),
  [Drawing.Point]::new(90,108), [Drawing.Point]::new(54,122)
))
$g.FillPath($orange,$crest)
$crest.Dispose();$orange.Dispose()
$highlight=[Drawing.Pen]::new([Drawing.Color]::FromArgb(255,211,130),2)
$highlight.StartCap=[Drawing.Drawing2D.LineCap]::Round;$highlight.EndCap=[Drawing.Drawing2D.LineCap]::Round
$g.DrawArc($highlight,53,44,150,150,205,130)
$highlight.Dispose()
$g.Dispose()
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
