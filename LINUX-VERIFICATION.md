# Linux x64 verification — 2026-10-12

Ubuntu 24.04 under WSL, GCC 13, Release build and the pinned official SDK.
The validator accepts the compiled Linux bundle: 47 tests passed, 0 failed.
All three CTest suites pass (24.62 seconds total): audio, 353 meter lifecycle
checks and 40 editor cycles with actual X11 rendering under an isolated Xvfb server.

The first editor run exposed missing X11 frame configuration, followed by a
stale Cairo/XCB connection on reopening. The editor now supplies the SDK host
run loop and retains the XCB connection until orderly module teardown.
The test host completes XEmbed, checks colored pixels, verifies snapshot timers
while open and no polling after close, and requires every timer/file descriptor
to be unregistered after module exit. It opens no DAW or physical audio device.

Windows rebuilt from the same source: official validator 47/47, all four CTest
suites passed (12.93 seconds). Previously published Windows downloads are preserved.

Package: VaginaPlugin-0.1.1-linux-x64.tar.gz; keep the whole VaginaPlugin.vst3
directory together. Includes README, build instructions, MIT license and third-party
notices/licenses. Packaging refuses to replace an existing archive.

Not tested: a real Linux DAW, native Wayland embedding, CachyOS, macOS, hardware
audio or broader distribution compatibility. Xvfb is a graphics test server.
Local build logs and test output are under build-linux/ (ignored by Git).
