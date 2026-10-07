param([string]$BuildDirectory = (Join-Path $PSScriptRoot '..\build'))
$ErrorActionPreference = 'Stop'
$testRoot = Join-Path $PSScriptRoot ('..\build-package-test-' + [guid]::NewGuid().ToString('N'))
$output = Join-Path $testRoot 'output'
$staleBundle = Join-Path $output 'VaginaPlugin.vst3'
New-Item -ItemType Directory -Path $staleBundle -Force | Out-Null
Set-Content -LiteralPath (Join-Path $staleBundle 'stale-sentinel.txt') -Value 'Must not enter a release.'
try {
    # Reuse the output directory twice, as a release maintainer would.
    1..2 | ForEach-Object {
        & (Join-Path $PSScriptRoot 'package-windows.ps1') -BuildDirectory $BuildDirectory -OutputDirectory $output | Out-Null
        $archive = @(Get-ChildItem -LiteralPath $output -Filter '*-win-x64.zip')
        if ($archive.Count -ne 1) { throw 'Expected exactly one release archive.' }
        $zip = [System.IO.Compression.ZipFile]::OpenRead($archive[0].FullName)
        try {
            $entries = @($zip.Entries | ForEach-Object { $_.FullName.Replace('\', '/') })
            foreach ($required in @('VaginaPlugin.vst3/Contents/x86_64-win/VaginaPlugin.vst3', 'LICENSE', 'THIRD_PARTY_NOTICES.md')) {
                if ($required -notin $entries) { throw "Missing release entry: $required" }
            }
            if ($entries -match 'stale-sentinel|\.package-') { throw 'Stale or staging content leaked into archive.' }
            if (-not ($entries -match '^licenses/')) { throw 'Third-party licenses missing.' }
        } finally { $zip.Dispose() }
        $expected = (Get-FileHash -LiteralPath $archive[0].FullName -Algorithm SHA256).Hash.ToLowerInvariant()
        $checksum = Get-Content -LiteralPath ($archive[0].FullName + '.sha256') -Raw
        if (-not $checksum.StartsWith($expected + '  ')) { throw 'Checksum mismatch.' }
        if (Get-ChildItem -LiteralPath $output -Directory -Force -Filter '.package-*') { throw 'Staging directory was not cleaned.' }
    }
    Write-Output 'PASS: repeated packaging excludes stale files, preserves bundle/licenses and verifies checksum.'
} finally {
    $resolved = (Resolve-Path -LiteralPath $testRoot).Path
    $project = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
    if ((Split-Path $resolved -Parent) -ne $project -or
        (Split-Path $resolved -Leaf) -notmatch '^build-package-test-[0-9a-f]{32}$') {
        throw "Refusing to clean unexpected test path: $resolved"
    }
    Remove-Item -LiteralPath $resolved -Recurse -Force
}
