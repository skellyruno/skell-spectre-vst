# SKELL SPECTRE — actual JUCE UI renderer

The separate `SkellSpectreUIRenderer` development executable uses **the real
SkellSpectreAudioProcessor and SkellSpectreAudioProcessorEditor** and the
`SkellCore::UIRenderer` CMake target from the pinned SkellCore commit
`438bc74a86a40bf54cfa8089832a477822c8ccda`.

The VST3 target does not link the renderer. Its identity (`SkAu`/`Spct`), DSP,
parameter IDs and host state format are unchanged.

## Windows render cycle

```powershell
cmake -S . -B build-ui -A x64 -DSKELL_BUILD_UI_RENDERER=ON -DSKELL_BUILD_TESTS=OFF
cmake --build build-ui --config Release --target SkellSpectreUIRenderer
.\build-ui\Release\SkellSpectreUIRenderer.exe --output ui-screenshots
```

Find the executable under `build-ui` if your Visual Studio generator emits a
different layout. On every Source/CMake/renderer change, `.github/workflows/ui-screenshots.yml`
repeats the **BUILD -> RENDER -> VALIDATE -> SAVE PNGs** cycle, uploads individual
screenshots, `contact-sheet.png`, and `manifest.json`.

Scenes cover Character and Expert at all three supported sizes (704x396,
880x495, 1440x810), bypass, band rail selection, A/B slot, HQ hover and
disabled, Expert Tape mode, band mute and preset selection. There is exactly
one UI theme in this plugin; do not claim other skins exist.

`HeadlessUIRenderer.h` checks image validity, dimensions, sampled colors and
expected visual differences; the plugin's capture checks visible component
bounds. The existing `SkellUISmoke` validates actual editor A/B and host-state
restoration, and portable DSP tests remain independently built.

**Limitations:** JUCE popup menus draw in a separate peer; an editor component
snapshot cannot certify the *open* popup's geometry. Verify native popup
appearance and mouse/keyboard interactions in a running Windows VST3 host.
Offscreen rendering does not prove host stability or audio glitch freedom.
