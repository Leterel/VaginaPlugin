# VaginaPlugin

**Five bands. Zero sound changes.** An abstract, open-source VST3 visualizer.

Insert it on an audio track or master bus, open the editor, and play audio. Five
independent frequency bands animate simultaneously. There are no explicit visuals.

| Band | Animation |
| --- | --- |
| Below 100 Hz | Brown cylinder |
| 100–500 Hz | Red flowing ribbon |
| 500–2,000 Hz | Yellow arcing jet |
| 2,000–6,000 Hz | Occasional white droplets above a higher threshold |
| 6,000–20,000 Hz | Transparent outlined particles |

## Install on Windows

1. Unzip the Windows x64 download.
2. Copy the **entire `VaginaPlugin.vst3` folder** to a VST3 location scanned by your DAW,
   for example `C:\Program Files\Common Files\VST3` (administrator rights needed),
   or your DAW's configured custom VST3 folder.
3. Rescan plugins and add **VaginaPlugin** as an **audio effect**, not an instrument.
4. Open its editor and play a track. The host's bypass control suppresses the animation;
   audio still passes through. Remove the folder to uninstall.

The ZIP is portable; it makes no system changes. The plugin is not a standalone app.
Windows x64 is the tested binary target. A compatible 64-bit VST3 host is required.
This early build is unsigned.

## Audio behavior

- Mono/stereo, 32-bit and 64-bit floating-point audio; no added latency or tail.
- Active, non-silent audio is copied bit for bit, including special float values.
- Host-marked silent channels are cleared as required by the VST3 contract.
- Analysis uses a separate 4,096-sample Hann-window FFT with a 2,048-sample hop.
  Stereo channel energies are averaged, so anti-phase audio is still detected.
- Fixed storage, no heap allocations, locks, network calls or file access inside
  our audio-processing callback. Host-provided meter queues communicate with the UI.
- The display maps approximately −60 to 0 dBFS to visual strength. Attack/release
  smoothing affects graphics only. Animation refresh is about 30 fps.

## Build / contribute

See [BUILD.md](BUILD.md), [CONTRIBUTING.md](CONTRIBUTING.md) and [PROGRESS.md](PROGRESS.md).
The current implementation uses official **Steinberg VST3 SDK 3.8.1**, pinned to
commit `3cdf9ca5d1f5b1b21e0a86832aa4abe55607bd96`.
[Official version notes](https://steinbergmedia.github.io/vst3_dev_portal/pages/Versions/Version%2B3.8.1.html).

New code is MIT licensed; dependency notices are in [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md)
and `licenses/`. `legacy-original/` preserves the user's earlier JUCE synth source;
it is not built, linked, or included in the binary package.

## Prototype limits

- No commercial DAW session or live soundcard was exercised here. The SDK validator,
  direct processor tests, DLL editor lifecycle and native offscreen drawing were tested.
- Linux and macOS have build instructions but no verified artifacts yet.
- Fixed 960 × 600 editor; high-DPI behavior needs DAW testing.
- Band edges follow FFT bins; they are not ideal brick-wall filters. The resolution
  is sample-rate dependent, and frequencies above Nyquist cannot be analyzed.
- If a host stops processing completely, the last meter reading can remain visible.
  Sending silent blocks releases the meters normally.
