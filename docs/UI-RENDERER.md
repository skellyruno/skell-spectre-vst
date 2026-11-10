# SKELL SPECTRE real-editor visual regression

This development-only harness uses **SkellCore::UIRenderer** pinned to `438bc74a86a40bf54cfa8089832a477822c8ccda` and renders the **real production** JUCE AudioProcessorEditor and Processor. Neither SkellCore nor the renderer is part of the shipping VST3.

Windows: `cmake -S . -B build-ui -A x64 -DSKELL_BUILD_UI_RENDERER=ON -DSKELL_BUILD_TESTS=ON` then `cmake --build build-ui --config Release --target SkellSpectreUIRenderer`, then run `build-ui/Release/SkellSpectreUIRenderer.exe --output ui-screenshots` (the executable location is generator-dependent).

Screens: Basic and Expert at 704x396, 880x495, and 1440x810, plus bypass, A/B slot, band-selected and Expert-bypass. The contact sheet is generated from real editor frames. Each push that changes the editor reruns the standalone screenshot workflow. Checks include output dimensions, nonflat pixels, scene differences and direct visible child bounds. Existing `SkellUISmoke` continues covering A/B session roundtrip.

**Limitations:** One built-in visual theme is supported; no extra themes are invented. JUCE native popup menus/OS hover are not yet captured. Automated image checks cannot replace a manual overlap/typography review. DSP is not modified in this integration.
