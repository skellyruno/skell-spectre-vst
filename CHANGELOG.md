# Change log — SKELL SPECTRE

## v1.6.0 — 2026-11-07

- Smooth width processing mode switching between continuously warmed M/S, Spread, and Haas paths.
- Add a short phase-warmup attenuation for Diffuse activation, preserving zero-allocation audio processing.
- Reduce GUI repaint area during metering and lower skeleton animation refresh rate.
- Draw focused selections, higher-contrast band options and accurate FFT interpolation; add input/output plot legend and fixed-delay readout.
- Add first-sample stereo/Diffuse automation checks and rapid stereo mode stress.
- Preserve manufacturer ID, plugin ID, all parameter IDs, and fixed 8-sample latency.


## v1.5.0 — 2026-11-07

- Migrate v1.4 Reactor Pro DSP, 2x filtered oversampling and 8-sample latency contract into the GitHub repository.
- Initialize all split-filter states continuously and smoothly interpolate split-slope curves under host automation.
- Morph Drive character choices with a 10 ms smoothing interval.
- Make stereo spread smoothing independent of sample rate.
- Prevent non-finite host samples from reaching the editor FFT ring buffer.
- Change input/output metering to a logarithmic -60 to 0 dBFS display.
- Add aggressive real-time automation regression coverage, plus corrected Windows VST3 ZIP packaging.
- Preserve VST3 identity (`SkAu` / `Spct`) and all previously published parameter IDs.

**Verification status:** GNU Release DSP tests passed. Windows JUCE/VST3 build and FL Studio testing not completed locally.
