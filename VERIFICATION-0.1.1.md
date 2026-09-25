# VaginaPlugin 0.1.1 verification

Tested on Windows x64, 25 September 2026. Public MIT project:
https://github.com/Leterel/VaginaPlugin

## Changes

The custom visualizer clears after processing stops. A callback counter handles
suspended processing without mistaking a steady tone for inactivity. Restart
discards old FFT state. Editor open/close starts and stops UI-thread snapshot
requests; audio callbacks write only fixed, lock-free atomics. Existing class IDs,
state format and host meter parameters are preserved. Non-finite fallback meter
values cannot enter the drawing code.

## Verified

| Check | Result |
| --- | --- |
| MSVC x64 Release, official VST3 SDK 3.8.1 | Built |
| Official Steinberg validator | 47 passed, 0 failed |
| Audio suite | 200 configurations, 20 frequency/RMS cases, simultaneous bands, silence, bypass, state and host queues |
| New meter lifecycle suite | 353 checks passed; 55 audited realtime callbacks with no C++ new/new[] or host message allocation |
| Held level / live input with stopped transport | Visible for over 850 ms with fresh short callbacks and unchanged FFT result |
| Stop, late parameter messages, suspend, resume | Passed against actual processor/controller code and SDK host messages |
| Compiled DLL editor lifecycle | 20 new-view and 20 reused-view cycles; snapshot timers stop on close and restart on open |
| Graphics | Native VSTGUI silent/active PNG renders; active image inspected |
| REAPER 7.80 | Exact new DLL observed; active and host-bypassed 30-second offline renders bit-identical to baseline |

All four CTest suites passed. After adding the factory source required for the
Linux test target, the affected meter suite was rebuilt and passed again. This
source-level Linux correction does not constitute a tested Linux build.

The inner Windows DLL SHA-256 is
`d95a4dbd48eb0ae875800acde039c3dda9e5ff20c18f191628c638cade8108f3`.
The REAPER comparison covers 8,640,000 PCM bytes at 48 kHz, stereo, 24-bit.
Test audio is synthesized; no physical soundcard, microphone or camera was used.
See [BUILD.md](BUILD.md) and [DAW-VERIFICATION.md](DAW-VERIFICATION.md) to reproduce.

## Limits

No verified Linux/macOS artifacts, live soundcard run, high-DPI check or complete
interactive DAW editor stress test. The old REAPER GUI attempt belongs to 0.1.0,
not this version. Generic host meters use host behavior; the custom editor's stop
detection requires standard VST3 processor/controller message forwarding.
The C++ allocation audit does not intercept all OS/C allocation APIs.
Windows binaries are unsigned.
