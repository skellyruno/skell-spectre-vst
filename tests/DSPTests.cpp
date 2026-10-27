#include "../Source/DSP.h"
#include <cstdlib>
#include <sstream>
#include <stdexcept>
#include <cmath>
#include <iostream>
#include <random>
#include <vector>

// Keep checks active in Release builds (where standard assert() is disabled).
void require(bool ok, const char* expression, int line)
{
    if (!ok)
    {
        std::ostringstream message;
        message << "DSP regression failed at line " << line << ": " << expression;
        throw std::runtime_error(message.str());
    }
}
#define SKELL_CHECK(expr) require(static_cast<bool>(expr), #expr, __LINE__)

int main()
{
    skell::Engine e;
    e.prepare(48000.0);
    skell::Settings settings;
    settings.limiter = false;
    std::vector<float> left(4096), right(4096);
    std::mt19937 rng(87);
    std::uniform_real_distribution<float> distribution(-0.4f, 0.4f);
    for (int i = 0; i < 4096; ++i) { left[i] = distribution(rng); right[i] = distribution(rng); }
    const auto cleanLeft = left, cleanRight = right;
    float* pointers[] = { left.data(), right.data() };
    e.process(pointers, 2, 4096, settings);
    float maxError = 0.0f;
    for (int i = 0; i < 4096; ++i)
    {
        maxError = std::max(maxError, std::abs(left[i] - cleanLeft[i]));
        maxError = std::max(maxError, std::abs(right[i] - cleanRight[i]));
    }
    std::cout << "Neutral bypass maximum error: " << maxError << '\n';
    SKELL_CHECK(maxError < 0.000005f);

    settings.bands[1].drive = 83.0f;
    settings.bands[2].diffuse = 88.0f;
    settings.bands[3].width = 60.0f;
    settings.bands[0].punch = 35.0f;
    settings.bands[2].gainDb = 3.0f;
    settings.hq = true;
    settings.autoGain = true;
    settings.limiter = true;
    for (int n = 0; n < 12; ++n)
    {
        for (int i = 0; i < 4096; ++i) { left[i] = distribution(rng); right[i] = distribution(rng); }
        e.process(pointers, 2, 4096, settings);
        for (int i = 0; i < 4096; ++i)
        {
            SKELL_CHECK(std::isfinite(left[i]) && std::isfinite(right[i]));
            SKELL_CHECK(std::abs(left[i]) <= .985001f && std::abs(right[i]) <= .985001f);
        }
    }
    std::cout << "Stereo processed samples finite, output ceiling held.\n";
    e.prepare(96000.0);
    settings.limiter = false;
    for (int i = 0; i < 4096; ++i) left[i] = 0;
    e.process(pointers, 1, 4096, settings);
    for (float s : left) SKELL_CHECK(std::isfinite(s) && s == 0.0f);
    std::cout << "Mono silence and reset: passed.\n";

    // Assert ceiling only while limiter is actually enabled.
    settings.limiter = true;
    // Sweep extreme but valid settings across crossover orders, processing types,
    // mono/stereo layouts and samplerates. Catch state overflow or unstable poles.
    for (double sr : { 44100.0, 48000.0, 96000.0, 192000.0 })
    {
        e.prepare(sr);
        for (int slope = 0; slope < 4; ++slope)
        {
            settings.slope = slope;
            for (int mode = 0; mode < 3; ++mode)
            {
                settings.driveType = settings.widthType = mode;
                settings.crossovers = { 40.0f + 60.0f * mode,
                                         700.0f + 350.0f * mode,
                                         4200.0f + 2300.0f * mode };
                for (auto& b : settings.bands)
                {
                    b.drive=100.0f; b.punch=100.0f; b.diffuse=100.0f;
                    b.width=100.0f; b.mix=100.0f; b.gainDb=8.0f;
                }
                for (int i = 0; i < 4096; ++i)
                {
                    left[i] = distribution(rng); right[i] = distribution(rng);
                }
                e.process(pointers, (mode%2)+1, 4096, settings);
                for (int i=0; i<4096; ++i)
                {
                    SKELL_CHECK(std::isfinite(left[i]));
                    SKELL_CHECK(std::abs(left[i]) <= .985001f);
                }
            }
        }
    }
    std::cout << "Samplerate / slope / shaper / channel stress matrix: passed.\n";
}
