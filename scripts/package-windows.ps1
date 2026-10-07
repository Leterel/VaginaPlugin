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
$packageItems = @('VaginaPlugin.vst3', 'README.md', 'BUILD.md', 'LICENSE', 'THIRD_PARTY_NOTICES.md', 'licenses')
$projectDefinition = Get-Content -LiteralPath (Join-Path $projectDirectory 'CMakeLists.txt') -Raw
if ($projectDefinition -notmatch 'project\(VaginaPlugin VERSION ([0-9]+\.[0-9]+\.[0-9]+)') {
    throw 'Cannot determine the package version from CMakeLists.txt.'
}
$archive = Join-Path $outputDirectoryResolved "VaginaPlugin-$($Matches[1])-win-x64.zip"
# Stage each package from scratch. Reusing an old output bundle can silently
# include removed binaries or license files in a later release.
$stage = Join-Path $outputDirectoryResolved ('.package-' + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $stage | Out-Null
try {
    Copy-Item -LiteralPath $bundle -Destination $stage -Recurse
    foreach ($item in $packageItems | Select-Object -Skip 1) {
        Copy-Item -LiteralPath (Join-Path $projectDirectory $item) -Destination $stage -Recurse
    }
    $archiveItems = $packageItems | ForEach-Object { Join-Path $stage $_ }
    Compress-Archive -LiteralPath $archiveItems -DestinationPath $archive -CompressionLevel Optimal -Force
} finally {
    $stageResolved = (Resolve-Path -LiteralPath $stage).Path
    if ((Split-Path $stageResolved -Parent) -ne $outputDirectoryResolved -or
        (Split-Path $stageResolved -Leaf) -notmatch '^\.package-[0-9a-f]{32}$') {
        throw "Refusing to clean an unexpected staging path: $stageResolved"
    }
    Remove-Item -LiteralPath $stageResolved -Recurse -Force
}
$hash = (Get-FileHash -LiteralPath $archive -Algorithm SHA256).Hash.ToLowerInvariant()
"$hash  $(Split-Path $archive -Leaf)" | Set-Content -LiteralPath "$archive.sha256" -Encoding ascii
Get-Item -LiteralPath $archive | Select-Object FullName, Length
Write-Output "SHA256 $hash"
