# SKELL SPECTRE — Skell VST Template Edition

**SKELL SPECTRE** is an original four-band skeleton-themed texture effect written in C++17/JUCE 8. This version is **ported onto your supplied `skell-vst-template-main(1).zip` build template**, replacing its demo gain effect while retaining its JUCE pin, GitHub Actions packaging conventions, and Skell Audio plugin manufacturer code.

![Skell vector mark](docs/logo.svg)

## What was imported from the template

- CMake project layout (`Source/PluginProcessor.*`, `Source/PluginEditor.*`), `JUCE_PATH` override, and JUCE **8.0.12** dependency.
- `SkAu` manufacturer ID and unique `Spct` four-character product ID, with `audio.skell.spectre` bundle ID to avoid collision with the template's `Sk01` quickstart.
- The **`.github/workflows/build.yml`** Windows Actions build with automatic `.vst3` ZIP upload.
- State save/restore through JUCE AudioProcessorValueTreeState, and mono/stereo effect bus layout.

## SKELL SPECTRE features

- 4 frequency bands with independent drive, punch, stereo width, and phase dispersion, plus per-band mix/gain/mute/solo.
- **BASIC** (4 macro controls) / **EXPERT** (full bands and crossover controls) mode.
- Original vector-drawn reactive skull, lime-green / charcoal styling, analyzer, and resizable interface.
- 30 original factory presets for bass, leads, drums, vocals, mixes, and effects.
- Input/output trim, mix, 3 editable crossovers, saturation choices (Tube/Tape/Shred), width choices, safety clip, optional autogain, save/restore of automatable parameters.
- JUCE-free regression tests and deterministic synthetic DSP benchmark.

### Build without installing anything in a DAW

Upload the **contents** of this folder to a new GitHub repository (not the outer folder). Open **Actions → Build SKELL SPECTRE Windows VST3 → Run workflow**. When successful, download the `SKELL-SPECTRE-Windows-VST3` artifact and extract its `.vst3` folder.

### Windows local build

Requirements: Windows, Git, CMake 3.22+, Visual Studio with Desktop development with C++, and internet access for the first JUCE fetch (or JUCE 8.0.12 at a local path).

```powershell
.\BUILD-WINDOWS.ps1
# OR Windows Visual Studio 2026:
.\BUILD-WINDOWS.ps1 -Generator 'Visual Studio 18 2026'
# OR use a local JUCE checkout:
.\BUILD-WINDOWS.ps1 -JucePath 'C:\JUCE'
```

The VST3 is generated inside `build/SkellSpectre_artefacts/Release/VST3/`; the standalone app is in `build/SkellSpectre_artefacts/Release/Standalone/`. Install the VST3 at `C:\Program Files\Common Files\VST3` or add its build folder to the scan path in your DAW.

### JUCE-free local DSP verification

```bash
cmake -S . -B build-dsp -DSKELL_BUILD_PLUGIN=OFF -DSKELL_BUILD_TESTS=ON
cmake --build build-dsp
ctest --test-dir build-dsp --output-on-failure
./build-dsp/SkellDSPBenchmark
```

**Important:** The regression tests exercise the DSP, not JUCE GUI or VST3 hosting. The source port is not proof of a successful Windows plug-in build. Compile on Windows/Actions and test loading, preset/state recall, automation, and mono/stereo routing in your DAW.

### Technical notes

- The four bands sum to dry input at neutral processing settings. Crossovers use nested one-pole cascades with nominal 12/24/36/48 dB/oct slopes, *not* true brickwall isolation.
- Diffuse is an original all-pass dispersion implementation. Drive HQ is midpoint interpolation rather than true band-limited oversampling. "Limiter" is a sample-peak safety clip, **not** a look-ahead true-peak limiter.
- Preset controls are set on the UI thread; core DSP processes without heap allocations or locks. Mode, mix, and other important controls are automatable.
- No code, artwork, preset banks, trademarks, activation system, or proprietary signal-processing algorithms were copied from Phantomic. No affiliation with the original developer.
- Observe JUCE license obligations when distributing plugin binaries.
