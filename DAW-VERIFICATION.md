# Actual DAW verification — 2026-09-23

## Result

REAPER **7.80 Windows x64** loaded the released VaginaPlugin 0.1.0 VST3 using
a disposable profile. No user project or physical audio recording was used.

| Check | Result |
| --- | --- |
| Instantiate VST3, identify effect, expose visualizer bypass parameter | Passed |
| Offline render with active VST3 compared with no effect | Bit-identical PCM |
| Offline render with REAPER host bypass compared with no effect | Bit-identical PCM |
| Render format | Stereo, 48,000 Hz, 24-bit PCM; 864,000 audio bytes |
| Fixture | Three seconds, five simultaneous tones (60/250/1,000/3,500/10,000 Hz) plus silence |
| Native DLL host editor test | 20 new-view and 20 reused-view attach/detach cycles passed |
| REAPER floating editor stress attempt | Seven cycles verified; no window found on cycle eight; incomplete |

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
node tests/reaper_fixture.mjs create $testRoot
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
node tests/reaper_batch.mjs $testRoot
foreach ($case in @('baseline', 'active', 'bypassed')) {
  Start-Process $reaper -ArgumentList @('-newinst', '-nosplash', '-cfgfile',
    ('"' + $testRoot + '\reaper.ini"'), '-batchconvert',
    ('"' + $testRoot + '\' + $case + '-batch.txt"')) -WindowStyle Hidden -Wait
}
node tests/reaper_fixture.mjs verify $testRoot
```

Each batch log must end with `OK`. `audio-results.json` records the comparison.
The optional `VAGINA_TEST_EDITOR=1` path opens floating plugin windows and logs
ten cycles; it is **not** required for the offline audio test. The local attempt
did not complete all ten cycles, and the missing-window cause is unresolved.

ReaScript calls follow the
[official REAPER API reference](https://www.reaper.fm/sdk/reascript/reascripthelp.html).

## Limits

This tests a real DAW's offline audio path, not live soundcard performance or
musical listening quality. High-DPI behavior, other DAWs, Linux and macOS remain
unverified. No claim of a complete REAPER GUI stress pass is made. The released
plugin binary is unchanged; only test tools and documentation were added.
