# SKELL SPECTRE — shipping checklist

The v1.6 source is a development candidate, **not a verified commercial-ready release**. Complete the following before shipping binaries or selling licenses.

## Must-pass build and host checks

- [ ] Build both **VST3** and **Standalone** in Windows MSVC / Visual Studio 2026 with JUCE 8.0.12 and run all Release/Debug configurations. Fix any warnings, resource problems, or plugin scans.
- [ ] Run `pluginval` at strictness 5 (and higher if practical) on the compiled VST3. Test 1/2-channel processing, unusual host block sizes, duplicate instances, state save/restore and host bypass.
- [ ] FL Studio: fresh scan, first load, project save/reopen, preset restore, undo/redo, freeze/render, automation playback, opening/closing UI repeatedly, closing project with UI open, deleting plugin, and 20+ simultaneous instances.
- [ ] DAW sessions: compare bypass latency to dry channels; check automation of input/output, mix, crossovers, modes, HQ and band mute/solo. Ensure no clicks/dropouts when switching modes and presets.
- [ ] Test 44.1 / 48 / 88.2 / 96 / 192 kHz and block sizes of 1, 16, 64, 256, 1024 and 8192 samples; also mono sources and non-stereo output bus layouts.
- [ ] Soak test multi-hour running and idle CPU usage, close/reopen crash handling, ASIO buffer sizes, and sleep/wake/resume with hardware audio interfaces.
- [ ] Compare reference mixes for phase, mono compatibility, transient overshoot, frequency response, output level drift and oversampling aliasing, preferably with null tests and listening panels.

## Potential follow-up engineering upgrades

- [x] Warm filter taps and smooth slope transitions when host automation changes the crossover mode (v1.5).
- [ ] Higher-order / selectable 4x and 8x true anti-aliasing with measured passband/stopband and latency.
- [ ] Calibrated true-peak lookahead limiter as a separate processing mode, including sample-rate dependent delay.
- [ ] State serialization of both A/B snapshots and reliable host undo/redo around snapshot changes.
- [ ] A custom user-preset library/browser with favorite/tag/search features and safe atomic save operations across platforms.
- [ ] Accessibility review (full focus traversal, UI contrast, accessible control descriptions).
- [ ] Build/release automation for versioning, installer packaging, plugin signing and crash-symbol distribution.
- [ ] In-host CPU benchmarks of HQ × instances and GUI open/closed render load.

## v1.5–v1.6 completed checks

- [x] Crossover mode transitions exercise initialized filter states; one-sample host automation test added.
- [x] Drive-character mode changes crossfade their nonlinear curves.
- [x] Stereo spread attack is sample-rate-correct.
- [x] Stereo mode changes morph across warmed M/S, Spread and Haas paths (v1.6).
- [x] Diffuse engagement fades in the phase network after idle periods (v1.6).
- [x] GUI meter redraws are localized and FFT display interpolates bins (v1.6).
- [x] Logarithmic input/output meters and sanitized waveform-data ingress.
- [ ] Rebuild VST3 with MSVC and check the JUCE UI after these changes.

## Known scope and metrics

- Signal path latency: **8 samples** (approx. 0.167 ms @ 48 kHz), reported to host and consistent across on/off HQ and bypass. This is not the 6 ms Haas echo interval, which is an intentional stereo effect.
- Saturator HQ: **2x halfband-filtered**, one 17-tap FIR for interpolation and another for decimation.
- Dynamics: sample-peak stereo-linked limiter, not a true-peak/lookahead implementation.
- Included tests are **DSP-only** because JUCE is not installed in this execution environment; successful tests do not imply an MSVC UI compilation result.
