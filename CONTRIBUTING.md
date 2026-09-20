# Contributing

Fork the repository, make a focused change on a branch, run the documented build
and tests, then open a pull request explaining the behavior and platforms tested.
Contributions to the new visualizer are under the MIT license in `LICENSE`.

## Core constraints

- Keep audio bit-identical for active input; respect the host's silence flags.
- Keep realtime processing free from heap allocation, locks, I/O and drawing.
- Keep graphics and audio data exchange through the VST3 meter parameters.
- Preserve stable plugin class IDs and state compatibility.
- Keep the visuals abstract; this project has no anatomical imagery.
- Do not modify `legacy-original/` as part of the new implementation.

## Useful next contributions

1. Confirm installation and playback in several real Windows DAWs.
2. Verify and automate Linux/macOS builds and host compatibility.
3. Add scalable/high-DPI editor layouts and accessibility options.
4. Improve transport-stop meter decay across hosts without audio-thread timers.

Include relevant regression coverage for DSP or lifecycle fixes. Visual-only
changes should include before/after renders rather than tests that mirror shapes.

