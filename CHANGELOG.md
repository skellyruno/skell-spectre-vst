# Change log — SKELL SPECTRE

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
