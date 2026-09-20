param(
    [string]$BuildDirectory = (Join-Path $PSScriptRoot '..\build'),
    [string]$OutputDirectory = (Join-Path $PSScriptRoot '..\dist')
)
$ErrorActionPreference = 'Stop'
$projectDirectory = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$buildDirectoryResolved = (Resolve-Path -LiteralPath $BuildDirectory).Path
$bundle = Join-Path $buildDirectoryResolved 'VST3\Release\VaginaPlugin.vst3'
$binary = Join-Path $bundle 'Contents\x86_64-win\VaginaPlugin.vst3'
if (-not (Test-Path -LiteralPath $binary -PathType Leaf)) {
    throw "Build the Windows x64 Release plugin first: missing $binary"
}
New-Item -ItemType Directory -Path $OutputDirectory -Force | Out-Null
$outputDirectoryResolved = (Resolve-Path -LiteralPath $OutputDirectory).Path
Copy-Item -LiteralPath $bundle -Destination $outputDirectoryResolved -Recurse -Force
$packageItems = @('VaginaPlugin.vst3', 'README.md', 'BUILD.md', 'LICENSE', 'THIRD_PARTY_NOTICES.md', 'licenses')
foreach ($item in $packageItems | Select-Object -Skip 1) {
    Copy-Item -LiteralPath (Join-Path $projectDirectory $item) -Destination $outputDirectoryResolved -Recurse -Force
}
$archive = Join-Path $outputDirectoryResolved 'VaginaPlugin-0.1.0-win-x64.zip'
$archiveItems = $packageItems | ForEach-Object { Join-Path $outputDirectoryResolved $_ }
Compress-Archive -LiteralPath $archiveItems -DestinationPath $archive -CompressionLevel Optimal -Force
$hash = (Get-FileHash -LiteralPath $archive -Algorithm SHA256).Hash.ToLowerInvariant()
"$hash  $(Split-Path $archive -Leaf)" | Set-Content -LiteralPath "$archive.sha256" -Encoding ascii
Get-Item -LiteralPath $archive | Select-Object FullName, Length
Write-Output "SHA256 $hash"
