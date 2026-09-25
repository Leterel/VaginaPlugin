# Building VaginaPlugin 0.1.1

## Requirements

- CMake 3.25+ (tested with 4.4.3), Git, a C++17 compiler.
- Windows: Visual Studio with **Desktop development with C++** and Windows SDK.
  Local verified build: Visual Studio 2026 Community, x64 Release, Windows SDK 10.0.26100.
- First configure downloads the official VST3 SDK and its required submodules.
  Alternatively pass `-DVST3_SDK_ROOT=/absolute/path/to/vst3sdk` for a complete checkout.
- Builds never install a plugin into your host folders automatically.

## Windows

Run from the repository root in PowerShell:

```powershell
cmake -S . -B build -G "Visual Studio 18 2026" -A x64
cmake --build build --config Release --target VaginaPlugin audio_tests meter_lifecycle editor_smoke visual_render --parallel
ctest --test-dir build -C Release --output-on-failure
& .\build\bin\Release\validator.exe .\build\VST3\Release\VaginaPlugin.vst3
```

For Visual Studio 2022 use `-G "Visual Studio 17 2022"` in a new build directory.
The MSVC runtime is statically linked consistently in the SDK and this plugin.

**Artifact:** `build/VST3/Release/VaginaPlugin.vst3/` — this is a bundle directory.
Do not distribute only its inner binary. Keep the folder structure.

After successful tests, package the full bundle together with installation
instructions and required license notices:

```powershell
.\scripts\package-windows.ps1 -BuildDirectory .\build -OutputDirectory .\dist
```

The script creates `VaginaPlugin-0.1.1-win-x64.zip` and its SHA-256 checksum.

If an unusual launcher supplies duplicate `PATH` / `Path` environment keys, Python 3
can normalize the child environment without modifying user/system settings:

```powershell
python scripts/run_clean.py cmake -S . -B build -G "Visual Studio 18 2026" -A x64
python scripts/run_clean.py cmake --build build --config Release --target VaginaPlugin audio_tests meter_lifecycle editor_smoke visual_render --parallel
```

## Linux — prepared, not locally verified

Install a C++17 compiler, CMake, Git and the VSTGUI development dependencies listed
in the [official setup guide](https://steinbergmedia.github.io/vst3_dev_portal/pages/Getting+Started/How+to+setup+my+system.html).
On Debian/Ubuntu this includes X11/XCB, xkbcommon, fontconfig, FreeType, Cairo, GTK3
and their development headers. Package names differ on CachyOS/Arch.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --target VaginaPlugin audio_tests meter_lifecycle --parallel
ctest --test-dir build --output-on-failure
```

Find the generated `.vst3` bundle under `build/VST3/`. Once verified in a host,
copy the entire bundle to `~/.vst3/`. Native Wayland hosting is not verified.

## macOS — prepared, not locally verified

Install Xcode command-line tools, CMake and Git. Configure with Xcode:

```sh
cmake -S . -B build -G Xcode
cmake --build build --config Release --target VaginaPlugin audio_tests meter_lifecycle --parallel
ctest --test-dir build -C Release --output-on-failure
```

The CMake project supplies a bundle identifier. Architecture selection, signing,
notarization and testing in a macOS VST3 host remain release work. No AU/AAX format
is offered or claimed.

## What the tests cover

`audio_tests`: 200 processor configurations spanning 22.05/44.1/48/96/192 kHz,
mono/stereo, float/double, in-place/separate buffers, 1/32/511/2048/8192-sample
blocks; bit-exact passthrough including negative zero, infinity and NaN; bypass,
state roundtrip, silence flags and empty flushes. Also 20 tone/RMS/band-isolation
cases with anti-phase stereo, silence release, simultaneous five-tone analysis,
non-finite graphics protection and host output-meter queues.

`editor_smoke` (Windows): loads the actual compiled VST3 DLL, creates its controller,
attaches/updates/removes 20 newly created editor views, then attaches/removes the
same view another 20 times in a hidden host window, and unloads cleanly.
The actual processor and controller are connected through SDK interfaces; the test
checks that editor timers exchange snapshots while open, stop polling on close,
and restart polling when reopened.
This test uses a small native host. The separate real REAPER test is documented in
[DAW-VERIFICATION.md](DAW-VERIFICATION.md).

`visual_render` (Windows): renders the same drawing code to silent/active PNGs using
VSTGUI's real offscreen graphics backend. Files are written in the test working folder.

`meter_lifecycle`: 353 checks using the production processor/controller and SDK
host messages: held meter values across short callbacks, live input with stopped
transport, stop with no following block, late host parameters, suspended callbacks,
restart, invalid messages, disconnect and standalone controller fallback. All
55 audited process/setProcessing calls perform zero C++ heap allocations and zero
host message allocations; stereo samples remain bit-identical. This allocation
audit counts C++ new/new[] (including aligned forms), not every possible OS or C allocation.

The official SDK validator runs automatically after plugin linking and can also
be called explicitly using the command above.
