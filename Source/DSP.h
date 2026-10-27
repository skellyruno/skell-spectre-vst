#pragma once
// SKELL SPECTRE — independently implemented multiband DSP.
// JUCE-free on purpose: regression tests compile without a plugin SDK.
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>

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
    bool basic = true, limiter = true, autoGain = false, hq = false;
};

class Engine
{
public:
    void prepare(double newSampleRate) noexcept
    {
        sampleRate = std::max(8000.0, newSampleRate);
        smoothing = 1.0f - std::exp(-1.0f / (0.018f * static_cast<float>(sampleRate)));
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
        lastDrive = {};
        delayIndex = 0;
        rmsIn = rmsOut = 0.001f;
        autoGainSmoothed = 1.0f;
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
        const float inTarget = gain(settings.inputDb), outTarget = gain(settings.outputDb);
        const float mixTarget = clampf(settings.mix * 0.01f, 0.0f, 1.0f);
        const int stages = (std::max(0, std::min(3, settings.slope)) + 1) * 2;
        const int driveMode = std::max(0, std::min(2, settings.driveType));
        const int widthMode = std::max(0, std::min(2, settings.widthType));
        bool anySolo = false;
        for (auto& b : settings.bands) anySolo |= b.solo;

        if (!initialized)
        {
            smoothedCross = crossTarget;
            smoothedIn = inTarget;
            smoothedOut = outTarget;
            smoothedMix = mixTarget;
            for (int b = 0; b < kBands; ++b)
            {
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
            for (int j = 0; j < 3; ++j) smoothedCross[j] += smoothing * (crossTarget[j] - smoothedCross[j]);
            std::array<float, 3> alpha {};
            for (int j = 0; j < 3; ++j)
                alpha[j] = 1.0f - std::exp(-2.0f * kPi * smoothedCross[j] / static_cast<float>(sampleRate));
            for (int b = 0; b < kBands; ++b)
            {
                const auto& p = settings.bands[b];
                const float targets[6] = { p.drive, p.punch, p.width, p.diffuse, gain(p.gainDb), clampf(p.mix * .01f, 0.0f, 1.0f) };
                for (int k = 0; k < 6; ++k)
                    smoothBand[b][k] += smoothing * (targets[k] - smoothBand[b][k]);
            }

            float original[2] = {}, input[2] = {};
            float bands[2][kBands] = {};
            for (int c = 0; c < numChannels; ++c)
            {
                original[c] = channels[c][i];
                // Guard downstream filters against non-finite host/sample input.
                input[c] = (std::isfinite(original[c]) ? original[c] : 0.0f) * smoothedIn;
                float low[3] = {};
                for (int j = 0; j < 3; ++j)
                {
                    float s = input[c];
                    for (int k = 0; k < stages; ++k)
                    {
                        auto& z = split[c][j].states[k];
                        z += alpha[j] * (s - z);
                        s = z;
                    }
                    low[j] = s;
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
                // Analyze both channels together to avoid transient-induced image movement.
                const float absBand = numChannels == 2 ?
                    0.5f * (std::abs(bands[0][b]) + std::abs(bands[1][b])) : std::abs(bands[0][b]);
                auto& e = envelope[b];
                e.fast += fastCoeff * (absBand - e.fast);
                e.slow += slowCoeff * (absBand - e.slow);
                const float transient = clampf((e.fast - e.slow) / (e.slow + 0.08f), -1.0f, 1.6f);
                const float transientGain = clampf(1.0f + punch * transient * 0.72f, 0.20f, 2.20f);
                float shaped[2] = {};
                for (int c = 0; c < numChannels; ++c)
                {
                    const float dry = bands[c][b];
                    float wet = dry;
                    if (drive > 0.00001f)
                    {
                        if (settings.hq)
                        {
                            const float halfway = 0.5f * (lastDrive[c][b] + dry);
                            wet = 0.5f * (saturate(halfway, drive, driveMode) + saturate(dry, drive, driveMode));
                        }
                        else wet = saturate(dry, drive, driveMode);
                    }
                    lastDrive[c][b] = dry;
                    wet *= transientGain;
                    // Phase rotation uses a proprietary-independent first-order all-pass network.
                    // Band-specific warped frequencies keep rotations smooth and causally stable.
                    float phased = wet;
                    for (int stage = 0; stage < 12; ++stage)
                    {
                        const float a = phaseCoefficient[b][stage];
                        auto& ap = phase[c][b][stage];
                        const float output = a * phased + ap.z;
                        ap.z = phased - a * output;
                        phased = output;
                    }
                    wet += diffuse * (phased - wet);
                    shaped[c] = dry + bandMix * ((wet * bandGain) - dry);
                }

                // Stereo modes: M/S, controlled side emphasis, or subtle Haas spread.
                const float rightBeforeWidth = shaped[1];
                if (numChannels == 2)
                {
                    if (settings.basic && b == 0 && width > 0.0f) width = 0.0f;
                    const float mid = 0.5f * (shaped[0] + shaped[1]);
                    float side = 0.5f * (shaped[0] - shaped[1]);
                    if (widthMode == 1)
                    {
                        sideLow[b] += .017f * (side - sideLow[b]);
                        side += (side - sideLow[b]) * std::max(0.0f, width) * .40f;
                    }
                    if (widthMode == 2 && width > 0.0f)
                    {
                        auto& d = delays[b];
                        const int read = (delayIndex - delaySamples + 8192) % 8192;
                        const float delayedRight = d.samples[read];
                        const float mixDelay = width * 0.35f;
                        shaped[1] += mixDelay * (delayedRight - shaped[1]);
                        side = 0.5f * (shaped[0] - shaped[1]);
                    }
                    const float multiplier = width < 0.0f ? (1.0f + width) : (1.0f + 1.15f * width);
                    shaped[0] = mid + side * multiplier;
                    shaped[1] = mid - side * multiplier;
                }
                if (numChannels == 2) delays[b].samples[delayIndex] = rightBeforeWidth;
                for (int c = 0; c < numChannels; ++c)
                {
                    const bool audible = !p.mute && (!anySolo || p.solo);
                    bands[c][b] = audible ? shaped[c] : 0.0f;
                }
            }
            float wet[2] = {};
            float inEnergy = 0.0f, outEnergy = 0.0f;
            for (int c = 0; c < numChannels; ++c)
            {
                for (int b = 0; b < kBands; ++b) wet[c] += bands[c][b];
                inEnergy += input[c] * input[c];
                outEnergy += wet[c] * wet[c];
            }
            inEnergy /= static_cast<float>(numChannels);
            outEnergy /= static_cast<float>(numChannels);
            rmsIn += rmsCoeff * (inEnergy - rmsIn);
            rmsOut += rmsCoeff * (outEnergy - rmsOut);
            const float ag = settings.autoGain ? clampf(std::sqrt((rmsIn + .0001f) / (rmsOut + .0001f)), .35f, 2.82f) : 1.0f;
            autoGainSmoothed += rmsCoeff * (ag - autoGainSmoothed);
            for (int c = 0; c < numChannels; ++c)
            {
                // True dry is the original input: output still applies the output trim.
                const float out = ((1.0f - smoothedMix) * input[c] + smoothedMix * wet[c] * autoGainSmoothed) * smoothedOut;
                const float limited = settings.limiter ? clampf(out, -0.985f, 0.985f) : out;
                channels[c][i] = std::isfinite(limited) ? limited : 0.0f;
            }
            delayIndex = (delayIndex + 1) % 8192;
        }
    }

private:
    struct Filter { std::array<float, 8> states {}; };
    struct Allpass { float z = 0.0f; };
    struct Envelope { float fast = 0.0f, slow = 0.0f; };
    struct Delay { std::array<float, 8192> samples {}; };
    static float saturate(float x, float amount, int mode) noexcept
    {
        const float pre = 1.0f + 9.0f * amount;
        float y = x;
        if (mode == 0)
            y = std::tanh(x * pre) / std::max(0.1f, std::tanh(pre));
        else if (mode == 1)
            y = std::atan(x * pre) / std::max(0.1f, std::atan(pre));
        else
        {
            // Soft fold at higher amounts; bounded output, smoothly zero at zero input.
            y = std::sin(x * (1.0f + 3.5f * amount)) / std::max(0.1f, std::sin(1.0f + 3.5f * amount));
            y = clampf(y, -2.5f, 2.5f);
        }
        // Linear blend avoids a sudden change from 0% Drive to ~0% Drive.
        return x + amount * (y - x);
    }

    double sampleRate = 48000.0;
    float smoothing = .001f, fastCoeff = .01f, slowCoeff = .001f, rmsCoeff = .0001f;
    float smoothedIn = 1.0f, smoothedOut = 1.0f, smoothedMix = 1.0f;
    std::array<float, 3> smoothedCross {{ 120.0f, 1000.0f, 5500.0f }};
    std::array<std::array<float, 6>, kBands> smoothBand {};
    std::array<std::array<Filter, 3>, 2> split {};
    std::array<std::array<std::array<Allpass, 12>, kBands>, 2> phase {};
    std::array<Envelope, kBands> envelope {};
    std::array<std::array<float, 12>, kBands> phaseCoefficient {};
    std::array<Delay, kBands> delays {};
    std::array<float, kBands> sideLow {};
    std::array<std::array<float, kBands>, 2> lastDrive {};
    int delaySamples = 288, delayIndex = 0;
    float rmsIn = .001f, rmsOut = .001f, autoGainSmoothed = 1.0f;
    bool initialized = false;
};
}
