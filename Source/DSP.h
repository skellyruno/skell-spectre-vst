#pragma once
// SKELL SPECTRE — independently implemented multiband DSP.
// JUCE-free on purpose: regression tests compile without a plugin SDK.
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include "DriveOversampler.h"

namespace skell
{
constexpr int kBands = 4;
constexpr float kPi = 3.14159265358979323846f;

inline float clampf(float x, float lo, float hi) noexcept { return std::max(lo, std::min(x, hi)); }
inline float gain(float db) noexcept { return std::pow(10.0f, db / 20.0f); }

struct BandSettings
{
    float drive = 0.0f;   // 0..100
    float punch = 0.0f;   // -100..100
    float width = 0.0f;   // -100..100
    float diffuse = 0.0f; // 0..100
    float gainDb = 0.0f;
    float mix = 100.0f;
    bool mute = false, solo = false;
};

struct Settings
{
    std::array<BandSettings, kBands> bands {};
    std::array<float, 3> crossovers {{ 120.0f, 1000.0f, 5500.0f }};
    float inputDb = 0.0f, outputDb = 0.0f, mix = 100.0f;
    int slope = 1;        // 0..3 -> 12, 24, 36, 48 dB/oct nominal
    int driveType = 0;    // tube, tape, shred
    int widthType = 0;    // mid-side, spread, Haas
    bool basic = true, limiter = true, autoGain = false, hq = false, bypass = false;
};

class Engine
{
public:
    static constexpr int latencySamples() noexcept { return DriveOversampler::latency; }
    void prepare(double newSampleRate) noexcept
    {
        sampleRate = std::max(8000.0, newSampleRate);
        smoothing = 1.0f - std::exp(-1.0f / (0.018f * static_cast<float>(sampleRate)));
        bypassCoeff = 1.0f - std::exp(-1.0f / (0.010f * static_cast<float>(sampleRate)));
        limiterRelease = 1.0f - std::exp(-1.0f / (0.085f * static_cast<float>(sampleRate)));
        // Previously .017/sample: this now has the same stereo response at any host rate.
        spreadCoeff = 1.0f - std::exp(-2.0f * kPi * 131.0f / static_cast<float>(sampleRate));
        phaseAttackCoeff = 1.0f - std::exp(-1.0f / (0.0045f * static_cast<float>(sampleRate)));
        fastCoeff = 1.0f - std::exp(-1.0f / (0.0035f * static_cast<float>(sampleRate)));
        slowCoeff = 1.0f - std::exp(-1.0f / (0.075f * static_cast<float>(sampleRate)));
        rmsCoeff = 1.0f - std::exp(-1.0f / (0.35f * static_cast<float>(sampleRate)));
        delaySamples = static_cast<int>(clampf(static_cast<float>(sampleRate * 0.006), 1.0f, 8190.0f));
        // Compute phase-filter coefficients only when the sample rate changes.
        // Trigonometry is intentionally kept off the audio thread.
        for (int b = 0; b < kBands; ++b)
            for (int stage = 0; stage < 12; ++stage)
            {
                const float center = 70.0f * std::pow(1.28f, static_cast<float>(stage))
                                    * std::pow(1.33f, static_cast<float>(b));
                const float omega = clampf(center / static_cast<float>(sampleRate), 0.00005f, 0.44f);
                const float t = std::tan(kPi * omega);
                phaseCoefficient[b][stage] = (t - 1.0f) / (t + 1.0f);
            }
        reset();
    }

    void reset() noexcept
    {
        for (auto& channel : split) for (auto& x : channel) x = {};
        for (auto& channel : phase) for (auto& band : channel) for (auto& x : band) x = {};
        for (auto& x : envelope) x = {};
        for (auto& d : delays) d = {};
        sideLow.fill(0.0f);
        phaseReady.fill(0.0f);
        for (auto& channel : oversamplers) for (auto& os : channel) os.reset();
        for (auto& channel : dryInputDelay) channel.fill(0.0f);
        for (auto& channel : rawInputDelay) channel.fill(0.0f);
        inputDelayIndex = 0;
        limiterGain = 1.0f; bypassSmoothed = 0.0f; hqSmoothed = 0.0f;
        smoothedSlope = 3.0f; smoothedDriveType = 0.0f;
        smoothedWidthType = 0.0f;
        audibleSmoothed.fill(1.0f);
        delayIndex = 0;
        rmsIn = rmsOut = 0.001f;
        autoGainSmoothed = 1.0f;
        autoGainTarget = 1.0f;
        autoGainCountdown = 0;
        initialized = false;
    }

    void process(float* const* channels, int numChannels, int numSamples, const Settings& settings) noexcept
    {
        if (channels == nullptr || numChannels < 1 || numChannels > 2 || numSamples <= 0) return;
        for (int c = 0; c < numChannels; ++c) if (channels[c] == nullptr) return;
        const float nyquist = static_cast<float>(sampleRate * 0.5);
        // Order crossover targets to guarantee that all four bands have valid regions.
        const float f1 = clampf(settings.crossovers[0], 25.0f, nyquist * 0.19f);
        const float f2 = clampf(settings.crossovers[1], f1 * 1.2f, nyquist * 0.60f);
        const float f3 = clampf(settings.crossovers[2], f2 * 1.2f, nyquist * 0.90f);
        const std::array<float, 3> crossTarget {{ f1, f2, f3 }};
        const float targetBypass = settings.bypass ? 1.0f : 0.0f;
        const float targetHQ = settings.hq ? 1.0f : 0.0f;
        std::array<float, 3> alphaTarget {};
        for (int j = 0; j < 3; ++j)
            alphaTarget[j] = -std::expm1(-2.0f * kPi * crossTarget[j] / static_cast<float>(sampleRate));
        const float inTarget = gain(settings.inputDb), outTarget = gain(settings.outputDb);
        const float mixTarget = clampf(settings.mix * 0.01f, 0.0f, 1.0f);
        // Keep all crossover filter stages running even in Smooth mode. This
        // prevents stale stage states when automating the split slope.
        const float targetSlope = static_cast<float>(std::max(0, std::min(3, settings.slope)));
        const float targetDriveType = static_cast<float>(std::max(0, std::min(2, settings.driveType)));
        const float targetWidthType = static_cast<float>(std::max(0, std::min(2, settings.widthType)));
        bool anySolo = false;
        std::array<std::array<float, 6>, kBands> bandTargets {};
        for (int b=0; b<kBands; ++b)
        {
            const auto& p=settings.bands[b];
            anySolo |= p.solo;
            bandTargets[b] = {{p.drive, p.punch, p.width, p.diffuse,
                gain(p.gainDb), clampf(p.mix * .01f, 0.0f, 1.0f)}};
        }

        if (!initialized)
        {
            smoothedAlpha = alphaTarget;
            smoothedSlope = targetSlope;
            smoothedDriveType = targetDriveType;
            smoothedWidthType = targetWidthType;
            bypassSmoothed = targetBypass;
            hqSmoothed = targetHQ;
            smoothedIn = inTarget;
            smoothedOut = outTarget;
            smoothedMix = mixTarget;
            for (int b = 0; b < kBands; ++b)
            {
                audibleSmoothed[b] = (!settings.bands[b].mute &&
                    (!anySolo || settings.bands[b].solo)) ? 1.0f : 0.0f;
                smoothBand[b][0] = settings.bands[b].drive;
                smoothBand[b][1] = settings.bands[b].punch;
                smoothBand[b][2] = settings.bands[b].width;
                smoothBand[b][3] = settings.bands[b].diffuse;
                smoothBand[b][4] = gain(settings.bands[b].gainDb);
                smoothBand[b][5] = clampf(settings.bands[b].mix * .01f, 0.0f, 1.0f);
            }
            initialized = true;
        }

        for (int i = 0; i < numSamples; ++i)
        {
            smoothedIn += smoothing * (inTarget - smoothedIn);
            smoothedOut += smoothing * (outTarget - smoothedOut);
            smoothedMix += smoothing * (mixTarget - smoothedMix);
            bypassSmoothed += bypassCoeff * (targetBypass - bypassSmoothed);
            hqSmoothed += bypassCoeff * (targetHQ - hqSmoothed);
            smoothedSlope += smoothing * (targetSlope - smoothedSlope);
            smoothedDriveType += bypassCoeff * (targetDriveType - smoothedDriveType);
            // The Spread / Haas / Mid-Side selector now morphs over ~10 ms;
            // delay and side-filter memories are continuously maintained.
            smoothedWidthType += bypassCoeff * (targetWidthType - smoothedWidthType);
            const int slopeLow = std::min(3, static_cast<int>(smoothedSlope));
            const int slopeHigh = std::min(3, slopeLow + 1);
            const float slopeBlend = smoothedSlope - static_cast<float>(slopeLow);
            for (int j = 0; j < 3; ++j) smoothedAlpha[j] += smoothing * (alphaTarget[j] - smoothedAlpha[j]);
            for (int b = 0; b < kBands; ++b)
            {
                for (int k = 0; k < 6; ++k)
                    smoothBand[b][k] += smoothing * (bandTargets[b][k] - smoothBand[b][k]);
            }

            float original[2] = {}, input[2] = {};
            float rawDelayed[2] = {}, inputDelayed[2] = {};
            float bands[2][kBands] = {};
            for (int c = 0; c < numChannels; ++c)
            {
                original[c] = channels[c][i];
                // Guard downstream filters against non-finite host/sample input.
                // Huge invalid host values must not poison persistent IIR states.
                const float safe = std::isfinite(original[c]) ?
                    clampf(original[c], -64.0f, 64.0f) : 0.0f;
                input[c] = safe * smoothedIn;
                rawDelayed[c] = rawInputDelay[c][inputDelayIndex];
                inputDelayed[c] = dryInputDelay[c][inputDelayIndex];
                rawInputDelay[c][inputDelayIndex] = safe;
                dryInputDelay[c][inputDelayIndex] = input[c];
                float low[3] = {};
                for (int j = 0; j < 3; ++j)
                {
                    float s = input[c];
                    float slopeTaps[4] {};
                    for (int k = 0; k < 8; ++k)
                    {
                        auto& z = split[c][j].states[k];
                        z += smoothedAlpha[j] * (s - z);
                        s = z;
                        if ((k & 1) != 0) slopeTaps[k / 2] = s;
                    }
                    low[j] = slopeTaps[slopeLow] +
                        slopeBlend * (slopeTaps[slopeHigh] - slopeTaps[slopeLow]);
                }
                bands[c][0] = low[0];
                bands[c][1] = low[1] - low[0];
                bands[c][2] = low[2] - low[1];
                bands[c][3] = input[c] - low[2];
            }

            for (int b = 0; b < kBands; ++b)
            {
                const auto& p = settings.bands[b];
                const float drive = clampf(smoothBand[b][0] * .01f, 0.0f, 1.0f);
                const float punch = clampf(smoothBand[b][1] * .01f, -1.0f, 1.0f);
                float width = clampf(smoothBand[b][2] * .01f, -1.0f, 1.0f);
                const float diffuse = clampf(smoothBand[b][3] * .01f, 0.0f, 1.0f);
                const float bandGain = smoothBand[b][4];
                const float bandMix = smoothBand[b][5];
                const float audibleTarget = (!p.mute && (!anySolo || p.solo)) ? 1.0f : 0.0f;
                audibleSmoothed[b] += bypassCoeff * (audibleTarget - audibleSmoothed[b]);
                // Prime all-pass stages before bringing in the phase-rotated path.
                // Avoids a cold-start transient on Diffuse activation while keeping
                // the expensive stages off when diffusion is fully inactive.
                phaseReady[b] += phaseAttackCoeff *
                    ((diffuse > 0.0001f ? 1.0f : 0.0f) - phaseReady[b]);
                // Analyze both channels together to avoid transient-induced image movement.
                // Drive, per-band dry/wet, and the master dry path share fixed
                // 8-sample latency whether HQ is enabled or not.
                float delayedBand[2] = {};
                float driven[2] = {};
                for (int c = 0; c < numChannels; ++c)
                {
                    auto& os = oversamplers[c][b];
                    delayedBand[c] = os.delayDry(bands[c][b]);
                    if (drive <= 0.00001f)
                        driven[c] = delayedBand[c];
                    else
                    {
                        const float native = saturateMorph(delayedBand[c], drive, smoothedDriveType);
                        float highQuality = native;
                        if (targetHQ > 0.0f || hqSmoothed > 0.001f)
                        {
                            const float oversampled = os.processHQ(bands[c][b],
                                [drive, mode = smoothedDriveType](float v) noexcept
                                { return shapeMorph(v, drive, mode); });
                            // Crossfade the filtered nonlinear path, not the direct
                            // passband, to preserve exact unity when Drive is zero.
                            highQuality = delayedBand[c] + drive * (oversampled - delayedBand[c]);
                        }
                        driven[c] = native + hqSmoothed * (highQuality - native);
                    }
                }
                const float absBand = numChannels == 2 ?
                    0.5f * (std::abs(delayedBand[0]) + std::abs(delayedBand[1])) : std::abs(delayedBand[0]);
                auto& e = envelope[b];
                e.fast += fastCoeff * (absBand - e.fast);
                e.slow += slowCoeff * (absBand - e.slow);
                const float transient = clampf((e.fast - e.slow) / (e.slow + 0.08f), -1.0f, 1.6f);
                const float transientGain = clampf(1.0f + punch * transient * 0.72f, 0.20f, 2.20f);
                float shaped[2] = {};
                for (int c = 0; c < numChannels; ++c)
                {
                    const float dry = delayedBand[c];
                    float wet = driven[c];
                    wet *= transientGain;
                    // Phase rotation uses a proprietary-independent first-order all-pass network.
                    // Band-specific warped frequencies keep rotations smooth and causally stable.
                    if (diffuse > 0.0001f)
                    {
                        float phased = wet;
                        for (int stage = 0; stage < 12; ++stage)
                        {
                            const float a = phaseCoefficient[b][stage];
                            auto& ap = phase[c][b][stage];
                            const float output = a * phased + ap.z;
                            ap.z = phased - a * output;
                            phased = output;
                        }
                        wet += (diffuse * phaseReady[b]) * (phased - wet);
                    }
                    shaped[c] = dry + bandMix * ((wet * bandGain) - dry);
                }

                // Stereo modes: all three candidate paths are warm and the
                // selection morph is continuous. Side emphasis is mid-safe,
                // whereas Haas can change mono compatibility by design.
                if (numChannels == 2)
                {
                    if (settings.basic && b == 0 && width > 0.0f) width = 0.0f;
                    const float left = shaped[0], right = shaped[1];
                    const float mid = 0.5f * (left + right);
                    const float side = 0.5f * (left - right);
                    sideLow[b] += spreadCoeff * (side - sideLow[b]);
                    const float spread = (side - sideLow[b]) * std::max(0.0f, width) * 0.40f;
                    const float multiplier = width < 0.0f ? (1.0f + width) : (1.0f + 1.15f * width);
                    const float msL = mid + side * multiplier;
                    const float msR = mid - side * multiplier;
                    const float spreadL = mid + (side + spread) * multiplier;
                    const float spreadR = mid - (side + spread) * multiplier;
                    auto& delay = delays[b];
                    const int read = (delayIndex - delaySamples + 8192) % 8192;
                    const float haasRight = right + std::max(0.0f, width) * 0.35f *
                        (delay.samples[read] - right);
                    const float haasMid = 0.5f * (left + haasRight);
                    const float haasSide = 0.5f * (left - haasRight);
                    const float haasL = haasMid + haasSide * multiplier;
                    const float haasR = haasMid - haasSide * multiplier;
                    const int modeLow = std::max(0, std::min(2, static_cast<int>(smoothedWidthType)));
                    const float modeBlend = clampf(smoothedWidthType - static_cast<float>(modeLow), 0.0f, 1.0f);
                    const float leftCandidates[] = { msL, spreadL, haasL };
                    const float rightCandidates[] = { msR, spreadR, haasR };
                    const int modeHigh = std::min(2, modeLow + 1);
                    shaped[0] = leftCandidates[modeLow] + modeBlend *
                        (leftCandidates[modeHigh] - leftCandidates[modeLow]);
                    shaped[1] = rightCandidates[modeLow] + modeBlend *
                        (rightCandidates[modeHigh] - rightCandidates[modeLow]);
                    delay.samples[delayIndex] = right;
                }
                for (int c = 0; c < numChannels; ++c)
                {
                    bands[c][b] = shaped[c] * audibleSmoothed[b];
                }
            }
            float wet[2] = {};
            float inEnergy = 0.0f, outEnergy = 0.0f;
            for (int c = 0; c < numChannels; ++c)
            {
                for (int b = 0; b < kBands; ++b) wet[c] += bands[c][b];
                inEnergy += inputDelayed[c] * inputDelayed[c];
                outEnergy += wet[c] * wet[c];
            }
            inEnergy /= static_cast<float>(numChannels);
            outEnergy /= static_cast<float>(numChannels);
            rmsIn += rmsCoeff * (inEnergy - rmsIn);
            rmsOut += rmsCoeff * (outEnergy - rmsOut);
            // 350 ms RMS envelopes cannot perceptibly change over 16 samples,
            // so recompute the expensive square-root target at ~3 kHz rather
            // than once per audio sample. A persistent countdown guarantees
            // identical behaviour across arbitrary host buffer partitions.
            if (settings.autoGain)
            {
                if (autoGainCountdown == 0)
                {
                    autoGainTarget = clampf(std::sqrt((rmsIn + .0001f) / (rmsOut + .0001f)), .35f, 2.82f);
                    autoGainCountdown = 16;
                }
                --autoGainCountdown;
            }
            else
            {
                autoGainTarget = 1.0f;
                autoGainCountdown = 0;
            }
            autoGainSmoothed += rmsCoeff * (autoGainTarget - autoGainSmoothed);
            float preLimit[2] = {};
            float maxPeak = 0.0f;
            for (int c = 0; c < numChannels; ++c)
            {
                preLimit[c] = ((1.0f - smoothedMix) * inputDelayed[c] +
                    smoothedMix * wet[c] * autoGainSmoothed) * smoothedOut;
                preLimit[c] = std::isfinite(preLimit[c]) ? preLimit[c] : 0.0f;
                maxPeak = std::max(maxPeak, std::abs(preLimit[c]));
            }
            // Instant stereo-linked gain reduction, with a slower recovery.
            // This is a zero-lookahead sample-peak limiter, not true-peak protection.
            const float needed = std::min(1.0f, .985f / std::max(.985f, maxPeak));
            limiterGain = needed < limiterGain ? needed :
                limiterGain + limiterRelease * (needed - limiterGain);
            for (int c = 0; c < numChannels; ++c)
            {
                const float limited = settings.limiter ?
                    clampf(preLimit[c] * limiterGain, -.985f, .985f) : preLimit[c];
                const float out = limited + bypassSmoothed * (rawDelayed[c] - limited);
                channels[c][i] = std::isfinite(out) ? out : 0.0f;
            }
            delayIndex = (delayIndex + 1) % 8192;
            inputDelayIndex = (inputDelayIndex + 1) % DriveOversampler::latency;
        }
    }

private:
    struct Filter { std::array<float, 8> states {}; };
    struct Allpass { float z = 0.0f; };
    struct Envelope { float fast = 0.0f, slow = 0.0f; };
    struct Delay { std::array<float, 8192> samples {}; };
    static float shape(float x, float amount, int mode) noexcept
    {
        const float pre = 1.0f + 9.0f * amount;
        if (mode == 0)
            return std::tanh(x * pre) / std::max(0.1f, std::tanh(pre));
        if (mode == 1)
            return std::atan(x * pre) / std::max(0.1f, std::atan(pre));
        const float denom = std::sin(1.0f + 3.5f * amount);
        return clampf(std::sin(x * (1.0f + 3.5f * amount)) /
            (std::abs(denom) < .1f ? (denom < 0 ? -.1f : .1f) : denom), -2.5f, 2.5f);
    }
    // Morph between characters rather than switching transfer functions mid-wave.
    // The host parameter ID and discrete choices remain exactly as in v1.4.
    static float shapeMorph(float x, float amount, float mode) noexcept
    {
        const int lo = std::max(0, std::min(2, static_cast<int>(mode)));
        const float blend = clampf(mode - static_cast<float>(lo), 0.0f, 1.0f);
        const float a = shape(x, amount, lo);
        if (lo == 2 || blend < 0.00001f) return a;
        return a + blend * (shape(x, amount, lo + 1) - a);
    }
    static float saturateMorph(float x, float amount, float mode) noexcept
    {
        return x + amount * (shapeMorph(x, amount, mode) - x);
    }

    double sampleRate = 48000.0;
    float smoothing = .001f, fastCoeff = .01f, slowCoeff = .001f, rmsCoeff = .0001f;
    float bypassCoeff = .001f, limiterRelease = .001f, spreadCoeff = .017f;
    float phaseAttackCoeff = .003f;
    float smoothedIn = 1.0f, smoothedOut = 1.0f, smoothedMix = 1.0f;
    std::array<float, 3> smoothedAlpha {{ .015f, .12f, .5f }};
    std::array<std::array<float, 6>, kBands> smoothBand {};
    std::array<std::array<Filter, 3>, 2> split {};
    std::array<std::array<std::array<Allpass, 12>, kBands>, 2> phase {};
    std::array<Envelope, kBands> envelope {};
    std::array<std::array<float, 12>, kBands> phaseCoefficient {};
    std::array<Delay, kBands> delays {};
    std::array<float, kBands> sideLow {}, phaseReady {};
    std::array<float, kBands> audibleSmoothed {{ 1, 1, 1, 1 }};
    std::array<std::array<DriveOversampler, kBands>, 2> oversamplers {};
    std::array<std::array<float, DriveOversampler::latency>, 2> dryInputDelay {}, rawInputDelay {};
    int inputDelayIndex = 0;
    float bypassSmoothed = 0.0f, hqSmoothed = 0.0f, limiterGain = 1.0f;
    float smoothedSlope = 3.0f, smoothedDriveType = 0.0f, smoothedWidthType = 0.0f;
    int delaySamples = 288, delayIndex = 0;
    float rmsIn = .001f, rmsOut = .001f, autoGainSmoothed = 1.0f;
    float autoGainTarget = 1.0f;
    int autoGainCountdown = 0;
    bool initialized = false;
};
}
