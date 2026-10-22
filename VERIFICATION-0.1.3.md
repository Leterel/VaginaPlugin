# vajayjay 0.1.3 — 22 October 2026

## Product and visuals

The product, factory classes, native editor title, VST3 bundle and archives now
use **vajayjay**. The existing repository remains `Leterel/VaginaPlugin`.
The internal CMake target remains `VaginaPlugin`; the official SDK's
`PACKAGE_NAME` selects the output bundle/binary name.

The brown low-frequency shape now emerges from a fixed abstract mechanical
opening. Its root stays attached to the slot; bass energy controls thickness,
maximum length and movement speed. A smooth extension/retraction cycle changes
the exposed length. Silence shows an empty opening. No anatomical imagery.

Four deterministic native VSTGUI renders were generated and visually inspected:
silent, high-energy extended, high-energy contracted, and low-energy extended.
The active shape is connected to the opening and stays inside its own panel.
All five independent effects, labels and meters remain visible.

## Compatibility

No changes to `Processor.cpp`, state format, parameter IDs or meter IPC. Original
processor class ID `8A9DC5A1D7EF4AE5A134380C1E02D811` and controller class ID
`7C35FD930DE942D4BA431EA83DF1511B` remain. The Windows loaded-DLL test checks both
against independent fixed values and checks factory name/version. Generated
module metadata also contains the new name and the original class IDs.

Back up the old bundle outside the host's scanned directories, install the
complete new bundle and rescan. Do not leave both bundles scanned: their IDs are
intentionally identical. Filename-based host caches may need relinking. Loading
old projects in individual DAWs was not retested for this rename.

## Windows x64

- MSVC Release build passed. The official SDK validator passed **47/47**.
- All **4/4 CTest suites** passed in 26.07 seconds.
- 200 processor configurations: float/double, mono/stereo, in-place/separate,
  bypass/state, silence/flush; 20 frequency/RMS/anti-phase/release cases;
  simultaneous five-tone and non-finite-input cases.
- 353 meter-lifecycle checks, including 55 allocation-audited realtime calls
  with zero C++ allocation or host-message calls; audio remained bit-identical.
- Loaded DLL: 20 new-view and 20 reused-view editor cycles, 32 live scale changes
  from 50–400%, native child sizing, reopen, invalid factor and host-refusal cases.
- Four actual offscreen native renders, not mockups or image-generation output.
- Repeated Windows packaging checks the renamed complete bundle, module metadata,
  notices, verification document, checksum and absence of stale/old-name bundles.

## Linux x64

- Existing Ubuntu 24.04/WSL compiler and pinned SDK used; no system installation.
- Release build and official validator **47/47** passed.
- All **3/3 CTest suites** passed in 24.70 seconds: audio, 353 lifecycle checks and
  real X11 graphics in 20 new-view plus 20 reused-view cycles under isolated Xvfb.
- The new complete `vajayjay.vst3` bundle is packaged with licenses and checksum.

## Limits

No physical soundcard, microphone, camera or personal-desktop capture. No new
REAPER/other DAW test, Linux DAW, CachyOS, native Wayland, macOS, signing or
notarization claim. Older DAW evidence remains specific to the earlier named
binary. These are native test-host checks, not universal platform certification.
Previous build outputs and release artifacts were retained. No GitHub repository
rename or new hosted service. Local deliverables include exact hashes and logs.
