# vajayjay

Formerly VaginaPlugin. Version 0.1.3 uses the name **vajayjay** in the host,
editor and packages. The repository URL and VST3 class IDs remain unchanged.

**Five bands. Zero sound changes.** An abstract, open-source VST3 visualizer.

Insert it on an audio track or master bus, open the editor, and play audio. Five
independent frequency bands animate simultaneously. There are no explicit visuals.

| Band | Animation |
| --- | --- |
| Below 100 Hz | Brown extrusion growing from a fixed abstract slot |
| 100–500 Hz | Red flowing ribbon |
| 500–2,000 Hz | Yellow arcing jet |
| 2,000–6,000 Hz | Occasional white droplets above a higher threshold |
| 6,000–20,000 Hz | Transparent outlined particles |

## Install on Windows

The current package is `vajayjay-0.1.3-win-x64.zip`. The existing
[historical 0.1.1 download](https://github.com/Leterel/VaginaPlugin/releases/tag/v0.1.1)
still has the old name and predates the new animation and editor scaling.

1. Unzip the Windows x64 download.
2. Copy the **entire `vajayjay.vst3` folder** to a VST3 location scanned by your DAW,
   for example `C:\Program Files\Common Files\VST3` (administrator rights needed),
   or your DAW's configured custom VST3 folder.
3. Rescan plugins and add **vajayjay** as an **audio effect**, not an instrument.
4. Open its editor and play a track. The host's bypass control suppresses the animation;
   audio still passes through. Remove the folder to uninstall.

The ZIP is portable; it makes no system changes. The plugin is not a standalone app.
Windows x64 is the tested binary target. A compatible 64-bit VST3 host is required.
This early build is unsigned.

When upgrading, back up the old `VaginaPlugin.vst3` outside the scanned plugin
folders before installing the new bundle. Both names use the same class IDs;
keeping both scanned can make the host choose the old binary. Existing host
projects still need a rescan; file-name-based host caches may require relinking.

## Audio behavior

- Mono/stereo, 32-bit and 64-bit floating-point audio; no added latency or tail.
- Active, non-silent audio is copied bit for bit, including special float values.
- Host-marked silent channels are cleared as required by the VST3 contract.
- Analysis uses a separate 4,096-sample Hann-window FFT with a 2,048-sample hop.
  Stereo channel energies are averaged, so anti-phase audio is still detected.
- Fixed storage, no heap allocations, locks, network calls or file access inside
  our audio-processing callback. Fixed, lock-free atomics supply meter snapshots;
  the editor exchanges VST3 messages on the UI thread. Host output-meter parameters
  remain available for generic editors and as a compatibility fallback.
- The display maps approximately −60 to 0 dBFS to visual strength. Attack/release
  smoothing affects graphics only. Animation refresh is about 30 fps.
- Version 0.1.1 clears meters when processing stops, and discards old analysis on
  restart. If a host suspends callbacks without a stop notification, meters expire
  after 750 ms or three maximum audio blocks, whichever is longer. Constant tones
  and live input with stopped transport remain visible while callbacks continue.

Version 0.1.2 added host-driven editor scaling. Version 0.1.3 renames the
product and anchors the brown shape to an abstract emitter: bass energy changes
its length and animation speed. The processor, IDs and saved state stay compatible.
See [PROGRESS.md](PROGRESS.md) for verification.

## Build / contribute

See [BUILD.md](BUILD.md), [CONTRIBUTING.md](CONTRIBUTING.md) and [PROGRESS.md](PROGRESS.md).
The current implementation uses official **Steinberg VST3 SDK 3.8.1**, pinned to
commit `3cdf9ca5d1f5b1b21e0a86832aa4abe55607bd96`.
[Official version notes](https://steinbergmedia.github.io/vst3_dev_portal/pages/Versions/Version%2B3.8.1.html).

New code is MIT licensed; dependency notices are in [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md)
and `licenses/`. `legacy-original/` preserves the user's earlier JUCE synth source;
it is not built, linked, or included in the binary package.

## Prototype limits

- REAPER 7.80 on Windows loaded the exact 0.1.1 binary and rendered a 30-second synthetic stereo fixture
  bit-identically with the effect active and host-bypassed. See
  [DAW-VERIFICATION.md](DAW-VERIFICATION.md) for the reproducible test and its limits.
- No live soundcard test. The REAPER editor stress attempt completed seven cycles;
  the eighth could not be verified. The separate native host passed 40 editor cycles.
- Linux x64 was built on Ubuntu 24.04 under WSL and passed the official validator and Xvfb editor tests. No Linux DAW, CachyOS or native Wayland test is claimed. macOS remains unverified; see PROGRESS.md.
- Fixed 960 x 600 logical canvas with host-requested 50-400% content scaling.
  Native Windows host tests cover scaling and reopening; real DAW and multi-monitor
  DPI testing remains open.
- Band edges follow FFT bins; they are not ideal brick-wall filters. The resolution
  is sample-rate dependent, and frequencies above Nyquist cannot be analyzed.
- Stop detection in the custom editor requires the host to route standard VST3
  processor/controller messages. Generic host meter displays retain host behavior.
