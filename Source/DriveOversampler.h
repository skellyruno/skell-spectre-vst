#pragma once
// Fixed-latency 2x linear-phase half-band resampling for the nonlinear drive stage.
// Each stage has 17 taps / 8 oversampled-sample group delay: 8 base samples total.
// Coefficients are computed only at construction, never in process().
#include <array>
#include <cmath>
#include <algorithm>
#include <cstddef>

namespace skell
{
class DriveOversampler
{
public:
    static constexpr int taps = 17;
    static constexpr int latency = 8;

    DriveOversampler() noexcept
    {
        // Windowed halfband sinc, unity DC for downsampling. Zero coefficients
        // at all even offsets except the centre make the filter inexpensive.
        constexpr float pi = 3.14159265358979323846f;
        float sum = 0.0f;
        for (int i = 0; i < taps; ++i)
        {
            const int k = i - (taps - 1) / 2;
            const float ideal = k == 0 ? .5f :
                (k % 2 == 0 ? 0.0f : std::sin(.5f * pi * static_cast<float>(k)) / (pi * static_cast<float>(k)));
            const float window = .54f - .46f * std::cos(2.0f * pi * static_cast<float>(i) / static_cast<float>(taps - 1));
            coefficients[static_cast<std::size_t>(i)] = ideal * window;
            sum += coefficients[static_cast<std::size_t>(i)];
        }
        for (auto& x : coefficients) x /= sum;
        reset();
    }

    void reset() noexcept
    {
        up.fill(0.0f); down.fill(0.0f); direct.fill(0.0f);
        upPosition = downPosition = dryPosition = 0;
    }

    // Sample of the unprocessed signal, delayed by exactly the FIR's latency.
    float delayDry(float input) noexcept
    {
        const float delayed = direct[static_cast<std::size_t>(dryPosition)];
        direct[static_cast<std::size_t>(dryPosition)] = input;
        dryPosition = (dryPosition + 1) % latency;
        return delayed;
    }

    // Saturate at 2x base rate; the caller supplies a stateless shaping function.
    // This runs only while HQ is active/ramping. Other paths use delayDry().
    template <typename Shaper>
    float processHQ(float input, Shaper shape) noexcept
    {
        const float high0 = upsample(2.0f * input);
        const float high1 = upsample(0.0f);
        // Output on the even polyphase branch, after advancing the odd branch.
        // Both FIR stages contribute 8 oversampled samples of delay = 8 base.
        const float output = downsample(shape(high0));
        (void) downsample(shape(high1));
        return output;
    }

private:
    float upsample(float x) noexcept
    {
        up[static_cast<std::size_t>(upPosition)] = x;
        float y = 0.0f;
        int pos = upPosition;
        for (int i = 0; i < taps; ++i)
        {
            y += coefficients[static_cast<std::size_t>(i)] * up[static_cast<std::size_t>(pos)];
            pos = pos == 0 ? taps - 1 : pos - 1;
        }
        upPosition = (upPosition + 1) % taps;
        return y;
    }

    float downsample(float x) noexcept
    {
        down[static_cast<std::size_t>(downPosition)] = x;
        float y = 0.0f;
        int pos = downPosition;
        for (int i = 0; i < taps; ++i)
        {
            y += coefficients[static_cast<std::size_t>(i)] * down[static_cast<std::size_t>(pos)];
            pos = pos == 0 ? taps - 1 : pos - 1;
        }
        downPosition = (downPosition + 1) % taps;
        return y;
    }

    std::array<float, taps> coefficients {};
    std::array<float, taps> up {}, down {};
    std::array<float, latency> direct {};
    int upPosition = 0, downPosition = 0, dryPosition = 0;
};
}
