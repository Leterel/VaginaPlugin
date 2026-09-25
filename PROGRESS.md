# Progress — 2026-09-25

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
- Verified Linux/macOS builds and signed release binaries.

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
