param(
    [Parameter(Mandatory)][string]$TestDirectory,
    [Parameter(Mandatory)][string]$PluginBinary,
    [string]$ReaperExecutable = 'C:\Program Files\REAPER (x64)\reaper.exe',
    [string]$NodeExecutable = 'node'
)
$ErrorActionPreference = 'Stop'
$testRoot = (Resolve-Path -LiteralPath $TestDirectory).Path
$plugin = (Resolve-Path -LiteralPath $PluginBinary).Path
$profile = Join-Path $testRoot 'reaper.ini'
if (-not (Test-Path -LiteralPath $profile -PathType Leaf)) { throw 'Create an isolated REAPER profile in TestDirectory first; see DAW-VERIFICATION.md.' }
foreach ($case in @('baseline', 'active', 'bypassed')) {
    if (-not (Test-Path -LiteralPath (Join-Path $testRoot "$case-batch.txt"))) { throw "Missing $case-batch.txt; run reaper_batch.mjs first." }
}
$results = @()
foreach ($case in @('baseline', 'active', 'bypassed')) {
    $arguments = @('-newinst', '-nosplash', '-noactivate', '-cfgfile', ('"' + $profile + '"'), '-batchconvert', ('"' + $testRoot + '\' + $case + '-batch.txt"'))
    $started = [DateTime]::UtcNow
    $process = Start-Process -FilePath $ReaperExecutable -ArgumentList $arguments -WindowStyle Hidden -PassThru
    $seenPlugin = $false
    try {
        while (-not $process.HasExited -and [DateTime]::UtcNow -lt $started.AddSeconds(45)) {
            try {
                $process.Refresh()
                foreach ($module in $process.Modules) {
                    if ($module.FileName -ieq $plugin) { $seenPlugin = $true }
                }
            } catch {
                if (-not $process.HasExited) { Write-Verbose 'Module list temporarily unavailable.' }
            }
            Start-Sleep -Milliseconds 20
        }
        if (-not $process.HasExited) { throw "Isolated REAPER $case timed out. Check this test profile's startup dialogs before retrying." }
        $process.WaitForExit()
        if ($process.ExitCode -ne 0) { throw "REAPER $case failed: $($process.ExitCode)" }
        $output = Get-Item -LiteralPath (Join-Path $testRoot "$case.wav")
        $log = Get-Item -LiteralPath (Join-Path $testRoot "$case-batch.txt.log")
        if ($output.LastWriteTimeUtc -lt $started -or $log.LastWriteTimeUtc -lt $started) { throw "Stale $case output or log" }
        if (-not (Select-String -LiteralPath $log.FullName -Pattern '^OK\s*$' -Quiet)) { throw "REAPER $case log did not report OK" }
        $results += [pscustomobject]@{ case = $case; exitCode = $process.ExitCode; exactBuildModuleObserved = $seenPlugin }
    } finally {
        # Stop only the instance this script created, including on interruption.
        if (-not $process.HasExited) { $process.Kill(); $process.WaitForExit() }
        $process.Dispose()
    }
}
if (-not ($results | Where-Object { $_.case -eq 'active' -and $_.exactBuildModuleObserved })) {
    throw 'Exact active DLL was not observed: no DAW verification pass. Use a 30-second fixture and restrict this test profile to the new plugin folder.'
}
& $NodeExecutable (Join-Path $PSScriptRoot 'reaper_fixture.mjs') verify $testRoot
if ($LASTEXITCODE -ne 0) { throw 'PCM comparison failed' }
[pscustomobject]@{ pluginSha256 = (Get-FileHash -LiteralPath $plugin -Algorithm SHA256).Hash; results = $results } |
    ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $testRoot 'module-verification.json') -Encoding utf8
Write-Output 'PASS: exact active DLL observed; fresh REAPER renders and matching PCM verified.'
