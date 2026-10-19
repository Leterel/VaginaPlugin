# Progress — 2026-09-25

## Linux build and editor fixes — 2026-10-12

- Built Linux x64 on Ubuntu 24.04 under WSL with the pinned official SDK.
- Fixed missing X11 frame/run-loop configuration and retained the XCB connection
  until module teardown, preventing crashes during opening and repeated reopening.
- Official validator: 47 passed, 0 failed. All three Linux CTest suites passed:
  audio, 353 lifecycle checks and 40 real X11 editor cycles (24.62 seconds total).
- The isolated Xvfb host completes XEmbed, captures only its synthetic window,
  verifies colored rendering, closed-editor polling stop and clean module unload.
- Added Linux packaging with complete bundle/license notices and checksum.
- No Linux DAW, native Wayland, CachyOS or macOS test is claimed.

## Requirements

- Open-source VST3 using current official SDK; Windows binary first.
- Unchanged audio, independent visualization in five specified frequency ranges.
- Abstract cylinder, flow, arcing jet, sparse drops and transparent particles.
- Preserve existing source and document cross-platform work honestly.

## Implemented

- Official VST3 3.8.1 + VSTGUI, separate processor/controller, stable class IDs.
- Allocation-free FFT analyzer with anti-phase-safe stereo energy, smoothing,
  meter output parameters, mono/stereo and float/double passthrough.
- 960 × 600 native editor, five simultaneous animated lanes, dBFS meters.
- Host bypass, state serialization and defensive handling of non-finite analyzer input.
- Correct SDK frame ownership and close lifecycle, verified using the actual DLL.
- Reproducible CMake project, dependency pin, build/contribution/license docs.
- Earlier JUCE source preserved unchanged in `legacy-original/` and in its original location.

## Verified locally

- Windows x64 Release build with MSVC and official SDK 3.8.1.
- Steinberg validator: **47 passed, 0 failed**.
- Audio and native editor lifecycle suites (see BUILD.md for exact coverage).
- Native offscreen visual render and inspection recorded with final artifacts.
- Final rebuild and all three CTest suites passed on 2026-09-20: 200 processor
  configurations, 20 frequency cases, five-tone/meter queue cases, 20 DLL editor
  attach/update/detach cycles, and both native graphics renders.
- Active render inspected: all five abstract effects, labels and meters are visible
  within the 960 × 600 canvas.

## Remaining

- Live soundcard playback, complete DAW editor stress testing and broader high-DPI checks.
- Linux DAW and native Wayland/CachyOS tests; verified macOS builds and signed release binaries. Linux Ubuntu/WSL build and Xvfb editor checks passed on 2026-10-12 (see above).

## Published

- Public MIT-licensed source and Windows x64 prototype release published on
  2026-09-23: https://github.com/Leterel/VaginaPlugin/releases/tag/v0.1.0.
- Release binaries remain those validated on 2026-09-20; subsequent README and
  publication-status edits do not change the compiled code.

## Additional host verification — 2026-09-23

- REAPER 7.80 Windows x64 instantiated the released plugin in an isolated profile.
- Three 48 kHz stereo, 24-bit offline renders: baseline, active VST3, host bypass.
  All 864,000 PCM bytes match exactly; the fixture includes five tones and silence.
- Added reproducible synthetic fixture, ReaScript project setup and batch-job tools.
- Expanded native DLL test: 20 new-view plus 20 reused-view editor cycles passed.
- REAPER GUI stress attempt: seven cycles verified, floating window not found on
  the eighth. This is recorded as incomplete, not a completed GUI stress pass.
- Released plugin code and ZIP are unchanged. See DAW-VERIFICATION.md.

## Version 0.1.1 — 2026-09-25

- Fixed stale custom-editor meters when processing stops or the host suspends
  audio callbacks. A callback counter keeps constant tones and stopped-transport
  live input visible; restart clears old FFT state.
- The open editor requests value-only snapshots through VST3 messages. Audio
  callbacks write fixed lock-free atomics; message allocation stays on the UI thread.
  Existing host meter parameters and state/class IDs remain compatible.
- Late host parameter updates cannot revive a stopped snapshot. Non-finite
  fallback meter values are sanitized before reaching the renderer.
- Windows x64 Release: official validator 47/47; all four CTest suites passed.
  New lifecycle suite: 353 checks, 55 allocation-audited realtime callbacks.
  Native DLL test: 40 editor cycles with timer start/stop assertions.
- REAPER 7.80: exact new DLL observed in the process module list during active and
  bypassed renders. All 8,640,000 PCM bytes of a 30-second fixture match baseline.
- Broader DAW GUI, high-DPI, physical soundcard, Linux and macOS verification remain open.
  The older incomplete REAPER GUI stress attempt is not counted as a 0.1.1 pass.

## Packaging maintenance and regression verification — 2026-10-07

- Packaging now starts from a fresh staging directory, excluding stale files
  left in a previous output bundle. Package version comes from CMakeLists.txt.
- Added a regression check that packages twice into the same output directory
  containing a stale sentinel; both archives exclude it, retain the complete
  VST3 bundle and license notices, and have correct SHA-256 sidecars.
- Existing Windows 0.1.1 binaries rechecked: all four CTest suites passed
  (15.49 seconds), and the official validator reported 47 passed, 0 failed.
  No plugin source or binary changed in this maintenance update.
- A Linux WSL availability probe returned Wsl/Service/E_ACCESSDENIED in this
  execution environment. Linux/macOS and additional DAW tests remain unverified;
  this update does not turn them into verified release targets.


## Documentation consistency — 2026-10-19

Updated README and remaining-work list to reflect the existing 2026-10-12 Linux
Ubuntu/WSL build and Xvfb verification. Linux DAW, native Wayland/CachyOS and macOS
remain unverified. No source, binary or new host-test claim in this documentation change.

## Host-driven content scaling (0.1.2) - 2026-10-19

- Implemented the standard VST3 content-scale interface using VSTGUI scene zoom.
  The host receives the new size, including calls before attachment; rejected
  host resizes restore the prior zoom and dimensions. Invalid and non-finite
  factors are rejected; supported range is 50-400%.
- Windows x64 Release rebuilt: official validator 47 passed, 0 failed. All four
  CTest suites passed (14.87 seconds), including 32 live scale changes, native
  child geometry, four reopenings, resize refusal and invalid-value regression
  checks in the loaded DLL. Existing 40 editor cycles and timer checks still pass.
- Repeated ZIP packaging regression passed, preserving bundle/licenses/checksum.
- A fresh REAPER offline attempt could not launch: Windows returned illegal
  system DLL relocation from Start-Process. No new DAW pass is claimed for 0.1.2.
  Prior 0.1.1 DAW evidence remains specific to that earlier binary. Physical
  audio, DAW high-DPI/multi-monitor use, Linux DAW/Wayland and macOS remain open.
