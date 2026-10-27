#include "../Source/DSP.h"
#include <chrono>
#include <cmath>
#include <iostream>
#include <vector>

int main()
{
    skell::Engine engine;
    engine.prepare(48000);
    skell::Settings p;
    p.hq = true;
    p.limiter = true;
    for (auto& b : p.bands)
    {
        b.drive = 60;
        b.punch = 30;
        b.width = 45;
        b.diffuse = 65;
    }
    std::vector<float> left(512), right(512);
    float* channels[2] = { left.data(), right.data() };
    constexpr int blocks = 1000;
    auto before = std::chrono::steady_clock::now();
    for (int k = 0; k < blocks; ++k)
    {
        for (int i = 0; i < 512; ++i)
        {
            left[i] = std::sin(.13f*static_cast<float>(i+k));
            right[i] = std::sin(.15f*static_cast<float>(i+k));
        }
        engine.process(channels, 2, 512, p);
    }
    const auto elapsed = std::chrono::duration<double>(std::chrono::steady_clock::now()-before).count();
    std::cout << "Full stereo DSP workload, 10.67 sec at 48kHz in " << elapsed << " wall seconds (" <<
        (10.6667/elapsed) << "x realtime). Platform-dependent synthetic benchmark; excludes GUI/host.\n";
}
