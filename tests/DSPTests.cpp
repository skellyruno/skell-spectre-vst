#include "../Source/DSP.h"
#include <cstdlib>
#include <sstream>
#include <stdexcept>
#include <cmath>
#include <iostream>
#include <random>
#include <vector>
#include <array>
#include <algorithm>
#include <complex>
#include <limits>

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
        const float expectedLeft = i >= skell::Engine::latencySamples() ?
            cleanLeft[i - skell::Engine::latencySamples()] : 0.0f;
        const float expectedRight = i >= skell::Engine::latencySamples() ?
            cleanRight[i - skell::Engine::latencySamples()] : 0.0f;
        maxError = std::max(maxError, std::abs(left[i] - expectedLeft));
        maxError = std::max(maxError, std::abs(right[i] - expectedRight));
    }
    std::cout << "Neutral latency-aligned maximum error: " << maxError << '\n';
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

    // The linear phase FIR pair must preserve low-frequency identity when the
    // saturation shaper is linear, and agree with the exact dry latency.
    {
        skell::DriveOversampler os;
        float error = 0.0f;
        std::array<float, 2048> inputs {};
        for (int i=0;i<2048;++i)
            inputs[i] = .4f * std::sin(2.0f*skell::kPi*180.0f*i/48000.0f);
        for (int i=0;i<2048;++i)
        {
            const float dry = os.delayDry(inputs[i]);
            const float hq = os.processHQ(inputs[i], [](float x) noexcept { return x; });
            if (i>80) error = std::max(error, std::abs(hq-dry));
        }
        std::cout << "HQ FIR unity low-frequency max error: " << error << '\n';
        SKELL_CHECK(error < .012f);
    }

    // Block partitions may not alter the output of the same parameter state.
    {
        skell::Engine whole, chunked;
        whole.prepare(48000.0); chunked.prepare(48000.0);
        skell::Settings p;
        p.bands[0].drive=70; p.bands[1].punch=24;
        p.bands[2].diffuse=48; p.bands[3].width=38;
        p.hq=true; p.limiter=true;
        std::vector<float> a(4096), b(4096), c(4096), d(4096);
        for (int i=0;i<4096;++i)
        {
            const float v=distribution(rng);
            a[i]=c[i]=v;
            b[i]=d[i]=distribution(rng);
        }
        float* all[2]={a.data(),b.data()};
        whole.process(all,2,4096,p);
        for (int offset=0;offset<4096;)
        {
            const int block=std::min(4096-offset,37+(offset%271));
            float* piece[2]={c.data()+offset,d.data()+offset};
            chunked.process(piece,2,block,p);
            offset+=block;
        }
        float maxDiff=0.0f;
        for (int i=0;i<4096;++i)
        {
            maxDiff=std::max(maxDiff,std::abs(a[i]-c[i]));
            maxDiff=std::max(maxDiff,std::abs(b[i]-d[i]));
        }
        std::cout << "Block partition max error: " << maxDiff << '\n';
        SKELL_CHECK(maxDiff < 0.000005f);
    }

    // Synthetic nonlinear spectral test (7 kHz sine at 48 kHz): the 5th
    // harmonic folds to 13 kHz without oversampling. HQ must suppress it.
    {
        constexpr int count=24000, begin=6000;
        auto ratio=[&](bool hq)
        {
            skell::Engine oversampled;
            oversampled.prepare(48000);
            skell::Settings p;
            p.hq=hq; p.limiter=false;
            for (auto& b : p.bands) b.drive=100.0f;
            std::vector<float> audio(count);
            for (int i=0;i<count;++i)
                audio[i]=.8f*std::sin(2.0*skell::kPi*7000.0*i/48000.0);
            float* channel[]={audio.data()};
            oversampled.process(channel,1,count,p);
            const auto measure=[&](double hz)
            {
                std::complex<double> total{};
                for (int i=begin;i<count;++i)
                {
                    // std::polar(radius, phase) requires radius >= 0 (Clang 21
                    // and libstdc++ assert on negative audio samples).
                    const double angle = -2.0 * static_cast<double>(skell::kPi) * hz * i / 48000.0;
                    total += static_cast<double>(audio[i]) *
                        std::complex<double>(std::cos(angle), std::sin(angle));
                }
                return 2.0*std::abs(total)/(count-begin);
            };
            return measure(13000.0)/std::max(1e-8,measure(7000.0));
        };
        const auto native=ratio(false), hq=ratio(true);
        std::cout << "5th-harmonic folded alias relative to 7 kHz: native "
            << native << ", HQ " << hq << '\n';
        SKELL_CHECK(hq < native * 0.25);
    }

    // Host block sizes, including a single sample and a long offline render,
    // must produce finite output and respect the limiter when enabled.
    {
        skell::Engine hostBlocks;
        hostBlocks.prepare(48000.0);
        skell::Settings p;
        p.limiter = true; p.hq = true;
        for (auto& band : p.bands) {band.drive=80;band.diffuse=42;}
        for (int n : { 1, 16, 64, 256, 1024, 8192 })
        {
            std::vector<float> samples(static_cast<size_t>(n));
            for (int i=0;i<n;++i) samples[static_cast<size_t>(i)]=distribution(rng);
            float* channel[]={samples.data()};
            hostBlocks.process(channel,1,n,p);
            for (float v : samples)
            {
                SKELL_CHECK(std::isfinite(v));
                SKELL_CHECK(std::abs(v)<=.985001f);
            }
        }
    }

    // Invalid floating-point samples cannot permanently corrupt filter state.
    {
        skell::Engine guard;
        guard.prepare(44100);
        skell::Settings p;
        p.bands[0].drive=91;
        p.hq=true; p.limiter=true;
        std::vector<float> audio(1024,.25f);
        audio[4]=std::numeric_limits<float>::quiet_NaN();
        audio[5]=std::numeric_limits<float>::infinity();
        audio[6]=std::numeric_limits<float>::max();
        float* channel[]={audio.data()};
        guard.process(channel,1,static_cast<int>(audio.size()),p);
        for (auto v:audio) SKELL_CHECK(std::isfinite(v));
        SKELL_CHECK(std::isfinite(audio.back()));
    }

    // Real-time split-slope automation must not expose cold/inactive filter
    // stages: each of the four candidate tap depths is always warmed up.
    {
        skell::Engine changing, reference;
        changing.prepare(48000); reference.prepare(48000);
        skell::Settings before, after;
        before.slope=0; before.limiter=false;
        before.bands[1].gainDb=14.0f;
        after=before; after.slope=3;
        std::array<float,4096> a {}, b {};
        for (int i=0;i<4096;++i)
            a[i]=b[i]=.4f*std::sin(2.0*skell::kPi*520.0*i/48000.0);
        float* ap[]={a.data()}, *bp[]={b.data()};
        changing.process(ap,1,4096,before);
        reference.process(bp,1,4096,before);
        std::array<float,512> switched {}, unchanged {};
        for (int i=0;i<512;++i)
            switched[i]=unchanged[i]=.4f*std::sin(
                2.0*skell::kPi*520.0*(4096.0+i)/48000.0);
        float* pa[]={switched.data()}, *pb[]={unchanged.data()};
        changing.process(pa,1,512,after);
        reference.process(pb,1,512,before);
        float beginning=0.0f, settled=0.0f;
        for (int i=0;i<512;++i)
        {
            const float diff=std::abs(switched[i]-unchanged[i]);
            if(i<32) beginning=std::max(beginning,diff);
            if(i>400) settled=std::max(settled,diff);
        }
        std::cout << "Slope morph initial/later max difference: "
                  << beginning << " / " << settled << '\n';
        SKELL_CHECK(beginning < .10f);
        SKELL_CHECK(settled > beginning * 4.0f);
    }

    // Switching the discrete drive character should be a 10 ms morph,
    // rather than an instantaneous transfer-function discontinuity.
    {
        skell::Engine changing, reference;
        changing.prepare(48000); reference.prepare(48000);
        skell::Settings before, after;
        before.bands[2].drive=100.0f;
        before.limiter=false;
        after=before; after.driveType=2;
        std::array<float,4096> a {}, b {};
        for (int i=0;i<4096;++i)
            a[i]=b[i]=.54f*std::sin(2.0*skell::kPi*2700.0*i/48000.0);
        float* ap[]={a.data()}, *bp[]={b.data()};
        changing.process(ap,1,4096,before);
        reference.process(bp,1,4096,before);
        float firstA=.54f*std::sin(2.0*skell::kPi*2700.0*4096.0/48000.0);
        float firstB=firstA;
        float* pa[]={&firstA}, *pb[]={&firstB};
        changing.process(pa,1,1,after);
        reference.process(pb,1,1,before);
        std::cout << "One-sample drive-character automation difference: "
                  << std::abs(firstA-firstB) << '\n';
        SKELL_CHECK(std::abs(firstA-firstB) < .025f);
    }

    // Repeated 1-sample automation of slope and character must remain
    // stable for mono and stereo at both minimum and high sample rates.
    {
        skell::Engine automated;
        skell::Settings p;
        p.hq=true;
        p.limiter=true;
        for (auto& band:p.bands)
        {
            band.drive=95; band.punch=64;
            band.diffuse=68; band.width=70;
        }
        for (double sr:{44100., 192000.})
        {
            automated.prepare(sr);
            for (int i=0;i<8192;++i)
            {
                p.slope=(i/73)%4;
                p.driveType=(i/79)%3;
                const int channels=1+(i/2048)%2;
                float l=distribution(rng), r=distribution(rng);
                float* samples[]={&l,&r};
                automated.process(samples,channels,1,p);
                SKELL_CHECK(std::isfinite(l) && std::abs(l)<=.985001f);
                if(channels==2) SKELL_CHECK(std::isfinite(r) && std::abs(r)<=.985001f);
            }
        }
        std::cout << "Fast automation and 1-sample host blocks: passed.\n";
    }

    // Bypass must return the time-aligned raw input, ignoring output trim and
    // limiter after the short smoothing transition; no unreported timing jump.
    {
        skell::Engine bypassEngine;
        bypassEngine.prepare(48000);
        skell::Settings p;
        p.bypass=true; p.inputDb=13; p.outputDb=-6;
        p.bands[2].drive=93; p.limiter=true;
        std::array<float,1024> signal {};
        for (int i=0;i<1024;++i) signal[i] = std::sin(.076f*i)*.98f;
        const auto original=signal;
        float* channel[]={ signal.data() };
        bypassEngine.process(channel,1,1024,p);
        float err=0.0f;
        for (int i=0;i<1024;++i)
            err=std::max(err,std::abs(signal[i]-(i>=8 ? original[i-8] : 0.0f)));
        SKELL_CHECK(err < 0.000005f);
        std::cout << "Bypass dry alignment: passed.\n";
    }
}

