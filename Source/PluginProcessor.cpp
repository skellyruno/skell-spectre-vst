#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <array>
#include <cmath>

namespace
{
using ParamID = juce::ParameterID;
using Layout = juce::AudioProcessorValueTreeState::ParameterLayout;
void addFloat(Layout& layout, const juce::String& id, const juce::String& name,
              float lo, float hi, float start, float step = .01f, float skew = 1.0f)
{
    juce::NormalisableRange<float> range(lo, hi, step);
    range.setSkewForCentre(skew == 1.0f ? (lo + hi) * .5f : skew);
    layout.add(std::make_unique<juce::AudioParameterFloat>(ParamID(id, 1), name, range, start));
}
void addBool(Layout& layout, const juce::String& id, const juce::String& label, bool def)
{
    layout.add(std::make_unique<juce::AudioParameterBool>(ParamID(id, 1), label, def));
}
void addChoice(Layout& layout, const juce::String& id, const juce::String& label,
               const juce::StringArray& choices, int def = 0)
{
    layout.add(std::make_unique<juce::AudioParameterChoice>(ParamID(id, 1), label, choices, def));
}

struct Preset
{
    const char* name;
    float drive, punch, width, diffuse;
    int driveType = 0;
};
// Factory snapshots are original; zero-valued per-band parameters are set to macro
// levels on selection so Basic -> Expert preserves the sound exactly.
const std::array<Preset, 30> presets {{
    { "INIT / Skeleton", 0, 0, 0, 0 },
    { "BASS / Rib Crusher", 48, 29, -8, 22 },
    { "BASS / Sub Spine", 20, 12, -48, 7 },
    { "BASS / Bone Shaker", 69, 37, 11, 15 },
    { "BASS / Head Rattler", 82, 45, 12, 34, 2 },
    { "BASS / Night Stalker", 43, -15, 35, 47 },
    { "BASS / Low Crypt", 35, 8, -60, 28 },
    { "LEAD / Toxic Bones", 57, 26, 51, 52 },
    { "LEAD / Neon Bite", 73, 54, 24, 12, 2 },
    { "LEAD / Phantom Wire", 29, 6, 75, 75 },
    { "LEAD / Razor Teeth", 91, 70, 22, 8, 2 },
    { "LEAD / Afterlife", 42, -32, 54, 92 },
    { "DRUM / Coffin Knock", 31, 66, 4, 14 },
    { "DRUM / Skull Crack", 66, 80, 7, 8 },
    { "DRUM / Ancient Dust", 21, -46, 30, 63, 1 },
    { "DRUM / Hard Bones", 78, 45, -10, 22, 2 },
    { "VOCAL / Hollow Voice", 19, -20, 36, 67 },
    { "VOCAL / Haunted Double", 24, 10, 89, 72 },
    { "VOCAL / Teeth & Breath", 28, 40, 14, 34 },
    { "VOCAL / Grave Radio", 82, -19, -20, 53, 1 },
    { "MIX / Skeleton Glue", 16, 13, 14, 10 },
    { "MIX / Crypt Air", 11, -10, 38, 31 },
    { "MIX / Bone Density", 38, 22, -12, 19 },
    { "MIX / Twilight", 34, -18, 42, 57 },
    { "FX / Phase Apparition", 12, -35, 99, 100 },
    { "FX / Radioactive", 87, 36, 75, 81, 2 },
    { "FX / Bone Melt", 65, -60, 47, 100 },
    { "FX / Teeth Grinder", 98, 85, 17, 33, 2 },
    { "FX / Dark Tunnel", 23, -80, 65, 86 },
    { "FX / Full Possession", 100, 100, 100, 100, 1 }
}};
}

Layout SkellSpectreAudioProcessor::createParameterLayout()
{
    Layout layout;
    addFloat(layout, "input", "Input dB", -24, 24, 0, .1f);
    addFloat(layout, "output", "Output dB", -24, 24, 0, .1f);
    addFloat(layout, "mix", "Master Mix", 0, 100, 100, .1f);
    addBool(layout, "basic", "Basic Mode", true);
    addBool(layout, "limiter", "Output Limiter", false);
    addBool(layout, "autogain", "Auto Gain", false);
    addBool(layout, "hq", "HQ Drive", false);
    addChoice(layout, "slope", "Split Slope", { "Smooth 12", "Firm 24", "Steep 36", "Brickwall-ish 48" }, 1);
    addChoice(layout, "driveType", "Drive Character", { "Tube", "Tape", "Shred" });
    addChoice(layout, "widthType", "Stereo Character", { "Mid Side", "Spread", "Haas" });
    addFloat(layout, "drive", "Global Drive", 0, 100, 0, .1f);
    addFloat(layout, "punch", "Global Punch", -100, 100, 0, .1f);
    addFloat(layout, "stereo", "Global Stereo", -100, 100, 0, .1f);
    addFloat(layout, "diffuse", "Global Diffuse", 0, 100, 0, .1f);
    addFloat(layout, "cross1", "Low Crossover Hz", 40, 300, 120, 1, 110);
    addFloat(layout, "cross2", "Mid Crossover Hz", 301, 2500, 1000, 1, 1000);
    addFloat(layout, "cross3", "High Crossover Hz", 2501, 12000, 5500, 1, 5500);
    for (int b = 0; b < 4; ++b)
    {
        const auto prefix = "b" + juce::String(b + 1);
        const auto nice = "Band " + juce::String(b + 1) + " ";
        addFloat(layout, prefix + "drive", nice + "Drive", 0, 100, 0, .1f);
        addFloat(layout, prefix + "punch", nice + "Punch", -100, 100, 0, .1f);
        addFloat(layout, prefix + "stereo", nice + "Stereo", -100, 100, 0, .1f);
        addFloat(layout, prefix + "diffuse", nice + "Diffuse", 0, 100, 0, .1f);
        addFloat(layout, prefix + "gain", nice + "Gain dB", -18, 18, 0, .1f);
        addFloat(layout, prefix + "mix", nice + "Mix", 0, 100, 100, .1f);
        addBool(layout, prefix + "mute", nice + "Mute", false);
        addBool(layout, prefix + "solo", nice + "Solo", false);
    }
    return layout;
}

SkellSpectreAudioProcessor::SkellSpectreAudioProcessor()
    : AudioProcessor(BusesProperties().withInput("Input", juce::AudioChannelSet::stereo(), true)
                                      .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      parameters(*this, nullptr, "SKELL_STATE", createParameterLayout())
{
    const auto ptr = [this](const juce::String& id) { return parameters.getRawParameterValue(id); };
    raw.input=ptr("input"); raw.output=ptr("output"); raw.mix=ptr("mix");
    raw.basic=ptr("basic"); raw.limiter=ptr("limiter"); raw.autogain=ptr("autogain"); raw.hq=ptr("hq");
    raw.slope=ptr("slope"); raw.driveType=ptr("driveType"); raw.widthType=ptr("widthType");
    raw.drive=ptr("drive"); raw.punch=ptr("punch"); raw.stereo=ptr("stereo"); raw.diffuse=ptr("diffuse");
    for (int i=0; i<3; ++i) raw.crossover[i]=ptr("cross"+juce::String(i+1));
    for (int b=0; b<4; ++b)
    {
        auto prefix="b"+juce::String(b+1);
        auto& r=rawBands[b];
        r.drive=ptr(prefix+"drive"); r.punch=ptr(prefix+"punch");
        r.stereo=ptr(prefix+"stereo"); r.diffuse=ptr(prefix+"diffuse");
        r.gain=ptr(prefix+"gain"); r.mix=ptr(prefix+"mix");
        r.mute=ptr(prefix+"mute"); r.solo=ptr(prefix+"solo");
    }
}

void SkellSpectreAudioProcessor::prepareToPlay(double sampleRate, int /*samplesPerBlock*/)
{
    engine.prepare(sampleRate);
    setLatencySamples(0);
    inputPeak.store(0.0f);
    outputPeak.store(0.0f);
    scopeWrite.store(0);
}

void SkellSpectreAudioProcessor::releaseResources() { engine.reset(); }

bool SkellSpectreAudioProcessor::isBusesLayoutSupported(const BusesLayout& layout) const
{
    const auto& in = layout.getMainInputChannelSet();
    const auto& out = layout.getMainOutputChannelSet();
    return (in == juce::AudioChannelSet::mono() || in == juce::AudioChannelSet::stereo()) && in == out;
}

float SkellSpectreAudioProcessor::parameter(const juce::String& id) const noexcept
{
    if (auto* p = parameters.getRawParameterValue(id)) return p->load(std::memory_order_relaxed);
    return 0.0f;
}

void SkellSpectreAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const auto activeChannels = std::min(2, getTotalNumInputChannels());
    const int n = buffer.getNumSamples();
    for (int c = activeChannels; c < buffer.getNumChannels(); ++c) buffer.clear(c, 0, n);
    if (activeChannels < 1 || n < 1) return;

    // All parameter reads are pre-cached atomic loads: realtime-safe and cheap.
    const auto get=[](const std::atomic<float>* v) noexcept { return v->load(std::memory_order_relaxed); };
    skell::Settings s;
    s.basic = get(raw.basic) > 0.5f;
    s.limiter = get(raw.limiter) > 0.5f;
    s.autoGain = get(raw.autogain) > 0.5f;
    s.hq = get(raw.hq) > 0.5f;
    s.slope = static_cast<int>(get(raw.slope));
    s.driveType = static_cast<int>(get(raw.driveType));
    s.widthType = static_cast<int>(get(raw.widthType));
    s.inputDb = get(raw.input);
    s.outputDb = get(raw.output);
    s.mix = get(raw.mix);
    for (int j = 0; j < 3; ++j) s.crossovers[j] = get(raw.crossover[j]);
    const float globalDrive = get(raw.drive), globalPunch = get(raw.punch);
    const float globalStereo = get(raw.stereo), globalDiffuse = get(raw.diffuse);
    for (int b = 0; b < 4; ++b)
    {
        const auto& r = rawBands[b];
        auto& p = s.bands[b];
        p.drive = s.basic ? globalDrive : get(r.drive);
        p.punch = s.basic ? globalPunch : get(r.punch);
        p.width = s.basic ? globalStereo : get(r.stereo);
        p.diffuse = s.basic ? globalDiffuse : get(r.diffuse);
        p.gainDb = s.basic ? 0.0f : get(r.gain);
        p.mix = s.basic ? 100.0f : get(r.mix);
        p.mute = !s.basic && get(r.mute) > 0.5f;
        p.solo = !s.basic && get(r.solo) > 0.5f;
    }
    float before = 0.0f;
    for (int c = 0; c < activeChannels; ++c) before = std::max(before, buffer.getMagnitude(c, 0, n));
    float* ptrs[2] = { buffer.getWritePointer(0), activeChannels > 1 ? buffer.getWritePointer(1) : nullptr };
    // Store a small, lock-free waveform snapshot for the editor FFT; no heap work.
    const auto writeStart = scopeWrite.load(std::memory_order_relaxed);
    for (int i = 0; i < n; ++i)
        scopeInput[(writeStart + static_cast<unsigned int>(i)) % spectrumSize]
            .store(ptrs[0][i], std::memory_order_relaxed);
    engine.process(ptrs, activeChannels, n, s);
    for (int i = 0; i < n; ++i)
        scopeOutput[(writeStart + static_cast<unsigned int>(i)) % spectrumSize]
            .store(ptrs[0][i], std::memory_order_relaxed);
    scopeWrite.store(writeStart + static_cast<unsigned int>(n), std::memory_order_release);
    float after = 0.0f;
    for (int c = 0; c < activeChannels; ++c) after = std::max(after, buffer.getMagnitude(c, 0, n));
    // Peak hold decay is independent of GUI frame rate.
    inputPeak.store(std::max(before, inputPeak.load(std::memory_order_relaxed) * .88f), std::memory_order_relaxed);
    outputPeak.store(std::max(after, outputPeak.load(std::memory_order_relaxed) * .88f), std::memory_order_relaxed);
}

const juce::StringArray& SkellSpectreAudioProcessor::presetNames()
{
    static const juce::StringArray names = []
    {
        juce::StringArray out;
        for (const auto& p : presets) out.add(p.name);
        return out;
    }();
    return names;
}

int SkellSpectreAudioProcessor::getPresetIndex() const
{
    return juce::jlimit(0, static_cast<int>(presets.size()) - 1,
        static_cast<int>(parameters.state.getProperty("presetIndex", 0)));
}

void SkellSpectreAudioProcessor::loadPreset(int index)
{
    if (index < 0 || index >= static_cast<int>(presets.size())) return;
    const auto& p = presets[static_cast<size_t>(index)];
    const auto setParam = [this](const juce::String& id, float value)
    {
        if (auto* param = parameters.getParameter(id))
        {
            param->beginChangeGesture();
            param->setValueNotifyingHost(param->convertTo0to1(value));
            param->endChangeGesture();
        }
    };
    setParam("drive", p.drive); setParam("punch", p.punch);
    setParam("stereo", p.width); setParam("diffuse", p.diffuse);
    setParam("driveType", static_cast<float>(p.driveType));
    setParam("widthType", 0.0f);
    setParam("basic", 1.0f);
    setParam("mix", 100.0f);
    for (int b = 0; b < 4; ++b)
    {
        const auto prefix = "b" + juce::String(b + 1);
        setParam(prefix + "drive", p.drive);
        setParam(prefix + "punch", p.punch);
        setParam(prefix + "stereo", p.width);
        setParam(prefix + "diffuse", p.diffuse);
        setParam(prefix + "gain", 0.0f);
        setParam(prefix + "mix", 100.0f);
        setParam(prefix + "solo", 0.0f);
        setParam(prefix + "mute", 0.0f);
    }
    parameters.state.setProperty("presetIndex", index, nullptr);
}

void SkellSpectreAudioProcessor::getStateInformation(juce::MemoryBlock& destination)
{
    auto xml = parameters.copyState().createXml();
    copyXmlToBinary(*xml, destination);
}

void SkellSpectreAudioProcessor::setStateInformation(const void* data, int length)
{
    const auto xml = getXmlFromBinary(data, length);
    if (xml != nullptr && xml->hasTagName(parameters.state.getType()))
        parameters.replaceState(juce::ValueTree::fromXml(*xml));
}

juce::AudioProcessorEditor* SkellSpectreAudioProcessor::createEditor()
{
    return new SkellSpectreAudioProcessorEditor(*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new SkellSpectreAudioProcessor(); }
