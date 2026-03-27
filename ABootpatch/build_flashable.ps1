$out = "ABootpatch-beta.zip"
if (Test-Path $out) { Remove-Item $out }
Add-Type -AssemblyName System.IO.Compression.FileSystem
[System.IO.Compression.ZipFile]::CreateFromDirectory("flashable", $out)
Write-Host "Created: $out"
