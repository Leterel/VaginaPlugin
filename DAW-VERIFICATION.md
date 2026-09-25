# Actual DAW verification — 2026-09-25

## Result

REAPER **7.80 Windows x64** loaded the new VaginaPlugin 0.1.1 VST3 using
a disposable profile. No user project or physical audio recording was used.

| Check | Result |
| --- | --- |
| Exact 0.1.1 DLL observed in the REAPER process module list | Passed for active and bypassed render; absent from baseline |
| Offline render with active VST3 compared with no effect | Bit-identical PCM |
| Offline render with REAPER host bypass compared with no effect | Bit-identical PCM |
| Render format | Stereo, 48,000 Hz, 24-bit PCM; 8,640,000 audio bytes |
| Fixture | 30 seconds, five simultaneous tones (60/250/1,000/3,500/10,000 Hz) plus silence at both ends |
| Native DLL host editor test | 20 new-view and 20 reused-view cycles; real processor snapshots start/stop with the window |
| REAPER floating editor stress | Not verified for 0.1.1; the earlier 0.1.0 attempt stopped after seven verified cycles |

SHA-256 of the exact inner DLL observed in REAPER:
`d95a4dbd48eb0ae875800acde039c3dda9e5ff20c18f191628c638cade8108f3`.
The proof archive includes `module-verification.json`, `audio-results.json` and
the three successful batch logs. REAPER startup initially timed out with a new
profile; the final runs completed using an initialized disposable profile. A
shorter early probe did not observe the DLL and was not counted as a pass.

The WAV comparison parses RIFF chunks and compares audio data and format, rather
than file metadata. The bypass render explicitly uses REAPER's host bypass flag.
It does **not** establish that a controller parameter changed while processing is
idle has been flushed to processor state; processor bypass/state tests are separate.

## Reproduce on Windows

Requires Node.js, an existing REAPER installation and the built/extracted VST3
bundle. Use an empty disposable folder, for example `C:\temp\vagina-daw-test`.
The Lua script refuses to work if the REAPER resource path does not match this
folder or the project already contains tracks.

From the repository root:

```powershell
$testRoot = 'C:\temp\vagina-daw-test'
New-Item -ItemType Directory -Force -Path $testRoot | Out-Null
$env:VAGINA_TEST_OUTPUT = $testRoot
$env:VAGINA_TEST_EDITOR = '0'
node tests/reaper_fixture.mjs create $testRoot 30
# Point ONLY this disposable profile at the folder containing VaginaPlugin.vst3.
"[REAPER]`nvstpath64=C:\your-plugin-build\VST3\Release`n" |
  Set-Content (Join-Path $testRoot 'reaper.ini')
$reaper = 'C:\Program Files\REAPER (x64)\reaper.exe'
$script = (Resolve-Path tests/reaper_host.lua).Path
Start-Process $reaper -ArgumentList @('-newinst', '-nosplash', '-cfgfile',
  ('"' + $testRoot + '\reaper.ini"'), ('"' + $script + '"')) -Wait
```

A fresh REAPER profile may show its normal license/startup dialog. Handle it
only in that disposable instance. Wait for `host-results.txt` to report success
and for the instance to exit before continuing. The setup script creates three
synthetic `.rpp` projects and does not start playback.

```powershell
$plugin = 'C:\your-plugin-build\VST3\Release\VaginaPlugin.vst3\Contents\x86_64-win\VaginaPlugin.vst3'
node tests/reaper_batch.mjs $testRoot $plugin
.\tests\reaper_verify_windows.ps1 -TestDirectory $testRoot -PluginBinary $plugin -ReaperExecutable $reaper
```

Each batch log must end with `OK`. `audio-results.json` records the comparison.
`module-verification.json` records the observed DLL and its hash. The runner
rejects stale output files, failed or missing logs and unobserved active binaries.
It inspects only its own REAPER processes and stops its own instance on timeout.
Restrict the test profile's VST search path to the new bundle folder; a successful
passthrough comparison by itself cannot prove that the intended effect was loaded.
The optional `VAGINA_TEST_EDITOR=1` path opens floating plugin windows and logs
ten cycles; it is **not** required for the offline audio test. The local attempt
did not complete all ten cycles, and the missing-window cause is unresolved.

ReaScript calls follow the
[official REAPER API reference](https://www.reaper.fm/sdk/reascript/reascripthelp.html).

## Limits

This tests a real DAW's offline audio path, not live soundcard performance or
musical listening quality. High-DPI behavior, other DAWs, Linux and macOS remain
unverified. No claim of a complete REAPER GUI stress pass is made. Stop/resume
behavior is verified in the SDK/native test hosts; this REAPER test covers the
offline audio path, not the DAW's interactive transport or GUI lifecycle.

Historical 0.1.0 verification (2026-09-23): the three-second fixture matched
864,000 PCM bytes, native host passed 40 editor cycles, and a REAPER GUI attempt
verified seven cycles before failing to find the eighth floating window. Those
older release files and evidence are retained separately.
