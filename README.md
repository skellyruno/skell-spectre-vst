# SKELL SPECTRE v1.8.4 — Reactor Pro

**Source project:** JUCE 8 / C++17 / CMake. Builds VST3 and Standalone on Windows. Based on the supplied Skell VST template and the existing v1.2.1 Reactor UI project, retaining the **SkAu / Spct** VST identity and previous parameter IDs for project compatibility.

This is a working-source development release with an original skeleton illustration drawn by JUCE vector graphics. It is not a copy of Phantomic's binary, artwork or proprietary algorithms.

## v1.8.4 — permanent SkellCore visual review (November 10, 2026)

- Adds `SkellSpectreUIRenderer`: an **independent development executable** that
  renders the real JUCE editor through `SkellCore::UIRenderer`, pinned to
  SkellCore commit `438bc74a86a40bf54cfa8089832a477822c8ccda`.
- Screenshot workflow captures Character/Expert at 704×396, 880×495 and
  1440×810 plus bypass, rail/slot selection, preset, hover/disabled, Tape
  and muted-band states. Actual rendering inspection revealed that Expert's
  old vertical meter overlapped band D; Expert now uses compact sidebar meters.
  The compact mode-switch label is legible at minimum window size, and
  hover/disabled states have explicit visual contrast. Uploads individual PNG files, a contact sheet and
  `manifest.json` on every meaningful UI source change.
- Caches crossover filter exponentials when frequencies are unchanged,
  improving efficiency on very short host buffers without changing latency,
  plugin IDs, preset IDs or DSP filter coefficients. Portable regressions verify
  continuous versus 1-sample-block processing under crossover automation.
- See [docs/UI_RENDERER.md](docs/UI_RENDERER.md) for the permanent build–render–inspect
  workflow. Open JUCE popup menus and FL Studio usage still require manual host validation.

## Fix in v1.8.3 (November 10, 2026)

- Offscreen UI testing now calls the exact message-thread editor commands used by A/B and Expert-view buttons, removing the unsupported modal-loop dependency in the console-app test target.

## Previously added in v1.8.2 (November 10, 2026)

- The GUI smoke test target is now created with `juce_add_console_app`, which sets up JUCE-generated target directories correctly. This fixes the previous Windows CMake configuration failure; plugin identity and DSP remain unchanged.

## Previously added in v1.8.1 (November 10, 2026)

- Windows UI smoke target now generates `JuceHeader.h` for itself, fixing the build failure on the initial v1.8 push.
- Slow DSP silence tails now flush subnormal persistent filter states periodically and suppress output below −400 dBFS. This extends denormal safety beyond hosts that set FTZ/DAZ.
- Regression checks run 240,000-sample impulse tails at 44.1 and 192 kHz; the 8-sample latency, preset format and parameter IDs remain unchanged.

## Previously added in v1.8 (November 10, 2026)

- **Persistent A/B snapshots:** A/B states are serialized as non-recursive APVTS metadata, so closing/reopening the editor and saving/reopening a DAW session preserve both slots. Old sessions with no A/B metadata still load with the existing sound and initialize the unused slot safely.
- **Safer limiter switching:** disabling the sample-peak limiter fades from the limited to the unlimited path while enabling applies the configured ceiling immediately. This is not true-peak limiting.
- **HQ warm-up:** after oversampling has been idle, 2x HQ saturation fades in over 16 processed samples instead of exposing a cold FIR history abruptly; latency remains fixed at eight samples.
- **Offscreen JUCE GUI tests:** Windows CI now renders the Character and Expert editors at 704×396, 880×495, and 1440×810, verifies visible child bounds, and exercises A/B switching and host-state restoration. This does **not** replace FL Studio in-host testing.
- **Regression coverage:** sustained-signal limiter release, rapid HQ toggles, fixed sample-peak ceiling, and existing aliasing, silence, mono, block-size and saturation tests.

Compatibility: no parameter IDs, manufacturer/plugin IDs (`SkAu`/`Spct`), factory presets, or reported eight-sample latency have changed. CMake project version is 1.8.4.

## Previously added in v1.7 (November 7, 2026)

- **Lower Audio Thread overhead:** Auto Gain RMS ratios are recomputed once every 16 samples instead of every sample, with continuous 350 ms level smoothing. The phase of the update is persistent across host audio blocks, so output stays block-size-independent.
- **Zero FFT/peak-capture work while the editor is closed:** the processor reference-counts active editor visualizations. When there are none, it processes identical audio without FFT ring-buffer writes, magnitude scans, or UI-meter updates. This helps sessions with many plugin instances.
- **Sharper low-end spectrum:** 4,096-point FFT (previously 1,024), updated ring buffer and Hann-window calibration. At 48 kHz, bin resolution improves from ~46.9 Hz to ~11.7 Hz. The analyzer runs only in the visible Expert view.
- **Precise knob editing:** the four main Reactor macro knobs now display editable numeric boxes; click a number to type a value or double-click the knob to reset. Expert controls keep their compact popup values.
- **Cleaner selection affordances:** improved ALL/A/B/C/D rail contrast, an active selection stripe, explicit spectrum-size label, and preserved keyboard focus.
- **Added regression test:** Auto Gain under variable host block sizes with fast mode toggles. Normal, strict-warning and sanitizer tests are required before the Windows VST3 build.

The plugin ID (`SkAu` / `Spct`), all parameter identifiers, 30 factory presets and the fixed eight-sample delay are unchanged. The limiter remains a sample-peak limiter; no true-peak certification is claimed.

## Previously added in v1.6 (November 7, 2026)

- **Click-reduced stereo type automation:** Mid/Side, Spread, and Haas no longer switch abruptly. The DSP maintains the spread filter and Haas delay continuously, morphing between their outputs over approximately 10 ms. This also avoids unwarmed delay lines on mode activation.
- **Diffuse engagement protection:** the 12-stage independently designed phase-rotation network now fades into the wet path over a short (~4.5 ms time constant) warm-up window, reducing the first-sample phase pop after an idle period while retaining CPU savings when fully inactive.
- **Lighter live UI:** active meters and macro readouts repaint only their changing regions instead of repainting the entire editor 20 times per second. The skeleton animation updates at 24 Hz rather than 30 Hz, without modifying DSP timing.
- **More legible controls:** keyboard-focus outlines on band and slope selectors; more visible band selection; improved analyzer with fractional FFT-bin interpolation and clearly labeled input/output traces; high-contrast fixed-delay status.
- **New regression coverage:** initial-sample behavior of stereo mode and Diffuse automation, plus fast mode toggling inside the single-sample stress tests. The installed plugin's IDs and the 8-sample latency contract are unchanged.

The prior v1.5 Windows GitHub Actions build passed. This version requires its own Windows JUCE/VST3 validation before being considered a release candidate.

## Previously added in v1.5 (November 7, 2026)

- **Click-reduced crossover slope automation:** all four filter tap depths stay active, with an 18 ms crossfade between Smooth, Firm, Steep and Brickwall-style. This avoids the previous cold-state transients during slope changes, while preserving four-band complementary recombination.
- **Drive character morph:** Tube, Tape and Shred now transition over approximately 10 ms rather than switching saturator curves at a sample boundary. Host automation keeps the same parameter choices and IDs.
- **Sample-rate-correct stereo spread:** the side-channel high-pass enhancement uses a time-based coefficient at 44.1–192 kHz, instead of a fixed per-sample constant.
- **More readable Reactor meters:** -60 to 0 dBFS logarithmic scale, -36/-18/-6 dB grid markers, and an over-0 indication instead of an uninformative linear bar.
- **Safer input FFT:** non-finite host input and enormous outliers are sanitized before writing analyzer ring-buffer samples.
- **More automation regression tests:** mode transitions under a sustained tone, aggressive single-sample-block host automation across two sample rates, plus earlier latency, limiter and anti-alias tests.
- **Windows artifact packaging fix:** the zipped artifact now includes the complete `.vst3` directory for installation.

### Scope and compatibility

This iteration is built on the previously supplied v1.4 **source**, not on any unverified newer binary. Manufacturer/plugin IDs (`SkAu`/`Spct`), APVTS parameter identifiers, source preset names and **8-sample** reported host latency remain unchanged. A full JUCE/MSVC VST3 compile and FL Studio integration tests have not yet run in this environment.

## Previously added in v1.4

### DSP and host stability

- **Real 2x HQ saturation** with 17-tap linear-phase halfband interpolation and decimation. Unlike v1.2's midpoint method, HQ now filters high-frequency nonlinear images.
- **Constant 8-sample latency**, on/off HQ, wet/dry, normal bypass and DAW-host bypass. Reported using `setLatencySamples()` so the host can compensate.
- **Smoothed bypass (10 ms)** and **smoothed band mute/solo (10 ms)** to reduce switching artifacts.
- **Stereo-linked sample-peak limiter** with instantaneous peak gain reduction and approximately 85 ms release. This is **not** an oversampled intersample/true-peak limiter.
- **Faster control smoothing**: crossover frequency exponentials and dB-to-gain conversions are evaluated once per audio block rather than repeatedly for every sample.
- Mono and stereo compatibility, constant-latency dry paths, input sanitization for non-finite or pathological samples, and time-based meter decay.
- The Haas spread mid/side calculation now uses the updated right-channel sample, so the output does not add an unintended centre-channel error.

### Interface and workflow

- Preserves the compact, scalable Reactor UI with full-body vector skeleton and original purple analog controls, plus **reactive signal HUD** and secondary mint-coloured meters.
- Faster live spectrum drawing via a cached Hann window.
- **HQ 2X** label and clear processing-limit tooltips; popup value readouts on drag/hover and units in the popups.
- **SAVE / LOAD** custom `.skellpreset` files (binary JUCE XML state), using non-blocking file chooser dialogs. Imported files are validated for type and maximum size.
- Band selector rails and split selector support keyboard arrows; tooltip descriptions explain their functions.
- Switching all four band rails to ALL no longer silently discards Expert mode; an Expert session restored by a DAW opens its Expert view.
- Edited settings display **CUSTOM** in the preset menu after manual drag or custom command; preset navigation works from CUSTOM.

### Tests and build

Run DSP tests without JUCE:

```bash
cmake -S . -B build-dsp -DSKELL_BUILD_PLUGIN=OFF -DSKELL_BUILD_TESTS=ON -DCMAKE_BUILD_TYPE=Release
cmake --build build-dsp --config Release
ctest --test-dir build-dsp -C Release --output-on-failure
```

Windows native build (requires Visual Studio 2026 and network access for JUCE 8.0.12 unless `-JucePath` is supplied):

```powershell
.\BUILD-WINDOWS.ps1 -Generator 'Visual Studio 18 2026'
```

The connected GitHub repository `skellyruno/skell-spectre-vst` is the source of truth. Run its build workflow at **Actions → Build SKELL SPECTRE Windows VST3 → Run workflow** after updating source. The workflow builds the VST3 plus Standalone, runs the DSP regressions, and packages the compiled bundle.

**Plugin paths:** CMake produces the release VST3 inside `build/SkellSpectre_artefacts/Release/VST3/`. In FL Studio, scan your installed plugin folder after copying the VST3 bundle there.

## Validation and build requirements

- GNU C++17 Release DSP build and regression suite: passed.
- Clang 17 AddressSanitizer and UndefinedBehaviorSanitizer DSP build/tests: passed.
- Compile with Clang `-Wall -Wextra -Wpedantic`: passed with no warnings.
- Neutral, latency-aligned processing; mono silence; 44.1/48/96/192 kHz processing; all four slopes; all three saturation modes; block-partition equivalence; bypass alignment; non-finite safety; HQ filter response; anti-aliasing spectral test: passed.
- Representative 7 kHz / 48 kHz / full-drive synthetic test: 13 kHz folded harmonic measured approximately **54 dB lower** in HQ than normal processing. This is a specific fixture, not a guaranteed worst-case specification.

**Still required for v1.6:** the full JUCE 8 MSVC/VST3 Windows compile; FL Studio / Ableton / Reaper plugin loading and session restore; long-session host automation; pluginval; hardware-real-time CPU profiling; code signing, installers, and release qualification. **Do not label the binary production-certified until these are complete.** See [RELEASE-CHECKLIST.md](RELEASE-CHECKLIST.md).

## Audio DSP caveats

- HQ is **2x**, not 4x/8x. Aliasing at extreme drive/high source frequencies is not eliminated.
- Split modes are complementary cascaded-IIR divisions with nominal 12/24/36/48 dB/octave rolls; "Brickwall" is an artistic UI label, **not an ideal brickwall or linear-phase crossover**.
- Limiter guarantees only a bounded **sample peak** in active processing, not an intersample true peak. The bypass deliberately preserves original sample levels.
- Haas stereo mode can reduce mono compatibility; audition mono before printing a mix.
- A/B slots are session-local to the editor and not serialized, while the current parameter state is saved in the DAW project. Imported user presets save just the current state.
- DSP operates at floating-point sample rate / block size independently; editor and plugin wrappers still require end-to-end Windows validation.

SKELL SPECTRE is independently implemented and unaffiliated with the Phantomic developer. JUCE redistribution requires compliance with your JUCE license.
