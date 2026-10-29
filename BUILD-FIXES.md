# SKELL SPECTRE v1.2.1 — Windows Build Fixes

These two edits address the compiler errors in the provided Visual Studio 2026 / JUCE 8 log.

## MSVC C2666 — `juce::Graphics::fillRect` overload (PluginEditor.cpp, around line 295)

```cpp
// Previous code: x is float, the remaining values are int.
g.fillRect(x,0,5,23+i*3);

// Updated code: all four dimensions use the floating-point overload.
g.fillRect(x, 0.0f, 5.0f, 23.0f + static_cast<float>(i) * 3.0f);
```

## MSVC C2039 — `setTooltip` on `BandRail` (PluginEditor.h)

```cpp
// Previous:
class BandRail final : public juce::Component

// Updated: opt in to JUCE's settable tooltip API.
class BandRail final : public juce::Component, public juce::SettableTooltipClient
```

`rails[k].setTooltip(...)` can now be compiled because the derived class provides the tooltip API.

## Validation

- Source-level checks confirm the two problematic constructs have been replaced.
- `cmake -S . -B build-dsp -DSKELL_BUILD_PLUGIN=OFF -DSKELL_BUILD_TESTS=ON -DCMAKE_BUILD_TYPE=Release`
- `cmake --build build-dsp -j 2`
- `ctest --test-dir build-dsp --output-on-failure` — **1/1 passed**.

The full JUCE VST3 build cannot be performed in this container because the JUCE dependency is not present and GitHub network access is unavailable. Please use the included workflow or Windows build script to validate the complete plugin. If your workflow still fails, provide the next compiler error from the log.
