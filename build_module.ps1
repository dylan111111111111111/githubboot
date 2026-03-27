param([string]$Dir="module_template",[string]$Out="my_module.zip")
if (Test-Path $Out) { Remove-Item $Out }
Add-Type -AssemblyName System.IO.Compression.FileSystem
[System.IO.Compression.ZipFile]::CreateFromDirectory($Dir,$Out)
Write-Host "Module ZIP: $Out"
