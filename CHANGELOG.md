# Change log — SKELL SPECTRE

## v1.8.1 — 2026-11-10

- Fix the Windows `SkellUISmoke` build target's missing JUCE-generated header by generating it per target and linking the GUI dependencies.
- Flush inaudible IIR states periodically and zero subnormal output, so long silence tails stay denormal-free even outside JUCE's FTZ/DAZ guard.
- Add 44.1/192 kHz long-tail regression tests and confirm no audible-range change or measurable synthetic CPU regression.

## v1.8.0 — 2026-11-10

- Serialize A/B comparison snapshots inside the host-restored plugin state without recursive metadata.
- Keep A/B selections through plugin editor closure, reopening and DAW session restoration.
- Warm inactive HQ oversampler filters before fully fading in driven output; preserve fixed eight-sample latency.
- Smooth release of the sample-peak limiter when switched off; maintain immediate ceiling when switched on.
- Add Windows-only JUCE UI regression tests for renderer/layout at three sizes, A/B controls and persistence, plus DSP automation checks.
- Clean up FIR indexing and conversion warnings without changing filter coefficients.

## v1.7.0 — 2026-11-07

- Reduce Auto Gain square-root target updates to once per 16 audio samples; keep the 350 ms RMS smoothing and block-partition consistency.
- Suppress analyzer snapshot and meter work when no plugin editor is open.
- Increase audio spectrum resolution to 4,096 samples, with correctly scaled Hann normalization and bin interpolation.
- Make the four main Reactor knobs numerically editable while retaining right-click/drag, automation, and default-reset behavior.
- Clarify selected band in the rail, label FFT size, and update displayed version.
- Expand DSP tests for variable buffer sizes and Auto Gain toggling without adding parameters or changing reported latency.

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
