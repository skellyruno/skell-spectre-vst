# SKELL SPECTRE v1.2.1 — Reactor UI Build Fix

An original skeleton-themed, four-band texture-shaping VST3 effect based on the supplied **Skell VST JUCE template**. v1.2 redesigns the GUI around the user's 880 × 493 reference: compact horizontal format, purple electromechanical knobs, split modes, and a full-body reactive skeleton in a bespoke laboratory chamber. **The GUI is all JUCE vector graphics, not a borrowed screenshot or a bitmap imitation of the referenced character.**

## v1.2.1 — MSVC / JUCE 8 build fixes

- Corrected `Graphics::fillRect` arguments in `Source/PluginEditor.cpp` near line 295 to explicitly select the float overload (resolves MSVC C2666).
- Made `BandRail` inherit from `juce::SettableTooltipClient`, enabling its tooltip text without requiring an unavailable `Component::setTooltip` member (resolves MSVC C2039).
- Preserved v1.2 UI, parameters, DSP, factory presets, and GitHub Actions build system.
- **Validation:** JUCE-free DSP regression tests run locally; a Windows JUCE VST3 build still needs confirmation in GitHub Actions or Visual Studio.

## What changed in v1.2

- Reworked the default window from 1080 × 740 to **880 × 495** (about 40% smaller in area), with fixed 16:9 aspect ratio and resizable UI down to 704 × 396.
- Built a tall, original full-body skeletal character with an open purple jacket, ribs, cracked skull, electrical coils, pipes, and a reactor platform. Eye intensity/electric arcs respond to the processed signal.
- Redesigned four full-size controls (DRIVE, PUNCH, STEREO, DIFFUSE) as metallic dials; values are shown below.
- Added four independent **ALL/A/B/C/D** selector rails. **ALL** adjusts the global macro in Basic mode, or simultaneously updates all four expert-band values while Expert DSP is enabled. Selecting A–D switches to Expert DSP and edits the chosen band's value for that effect. Returning every selector to ALL restores Basic processing.
- Split-slope switch is a functional four-position custom graphic selector (**Smooth / Firm / Steep / Brickwall-ish**). The DSP uses 12/24/36/48 dB/oct nominal cascades, *not an ideal brickwall filter*.
- New **BANDS** button on the right toggles full Expert editor containing independent band controls, crossover frequency sliders and readouts, gain/mix, mute/solo, saturation mode, stereo mode and FFT.
- Added **automatable BYPASS** controlled by the power icon. When bypassed, audio passes through unchanged.
- Added session-local **A/B** controls with copy A → B for evaluating settings. A/B slots are not serialized in DAW state; the active parameter state is.
- Kept presets, HQ, limiter, autogain, mono/stereo, nonblocking DSP and lock-free editor meters.

## Build — Windows/GitHub Actions

The project retains JUCE 8.0.12, the template's `SkAu` manufacturer code and `Spct` plugin code, the GitHub Windows VST3 build workflow, and `BUILD-WINDOWS.ps1`. The ZIP's **root contents** belong at the root of your repository.

Windows command:

```powershell
.\BUILD-WINDOWS.ps1 -Generator 'Visual Studio 18 2026'
```

Or upload the root source files to GitHub and run **Actions → Build SKELL SPECTRE Windows VST3 → Run workflow**. Download the resulting `SKELL-SPECTRE-Windows-VST3` artifact when the build succeeds.

For an offline DSP-only build on a host with CMake and C++17 (no JUCE required):

```bash
cmake -S . -B build-dsp -DSKELL_BUILD_PLUGIN=OFF -DSKELL_BUILD_TESTS=ON
cmake --build build-dsp --config Release
ctest --test-dir build-dsp --output-on-failure
```

## Engine and scope

Independent four-band Drive, Punch, Stereo, and Diffuse, plus per-band gain, dry/wet blend, and mute/solo; three adjustable crossovers; 30 factory presets; input/output trim; master mix; basic/expert processing. Drive HQ is midpoint interpolation rather than true filtered oversampling. The limiter is a sample-peak safety clip, not look-ahead true-peak limiting. Diffuse is independent all-pass phase dispersion, not the proprietary Phantomic algorithm.

**Validation limits:** DSP-only CMake regression tests run in this environment. A full JUCE Windows plugin build and DAW UI/automation validation must still be carried out on Windows or GitHub Actions. Not affiliated with the referenced plugin developer. Do not distribute binaries without respecting JUCE license requirements.
