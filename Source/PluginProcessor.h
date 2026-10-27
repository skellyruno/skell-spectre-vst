#pragma once
#include <JuceHeader.h>
#include "DSP.h"

class SkellSpectreAudioProcessor final : public juce::AudioProcessor
{
public:
    SkellSpectreAudioProcessor();
    ~SkellSpectreAudioProcessor() override = default;
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.1; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}
    bool isBusesLayoutSupported(const BusesLayout&) const override;
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;

    juce::AudioProcessorValueTreeState parameters;
    std::atomic<float> inputPeak { 0.0f }, outputPeak { 0.0f };
    static constexpr int spectrumSize = 1024;
    std::array<std::atomic<float>, spectrumSize> scopeInput {}, scopeOutput {};
    std::atomic<unsigned int> scopeWrite { 0 };
    float parameter(const juce::String& id) const noexcept;
    void loadPreset(int index);
    static const juce::StringArray& presetNames();
    int getPresetIndex() const;
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

private:
    // Cached APVTS atomics: zero String construction / heap allocation in processBlock.
    struct RawGlobal
    {
        std::atomic<float> *input{}, *output{}, *mix{}, *basic{}, *limiter{}, *autogain{}, *hq{};
        std::atomic<float> *slope{}, *driveType{}, *widthType{};
        std::atomic<float> *drive{}, *punch{}, *stereo{}, *diffuse{};
        std::array<std::atomic<float>*, 3> crossover {};
    } raw;
    struct RawBand
    {
        std::atomic<float> *drive{}, *punch{}, *stereo{}, *diffuse{}, *gain{}, *mix{}, *mute{}, *solo{};
    };
    std::array<RawBand, 4> rawBands {};
    skell::Engine engine;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SkellSpectreAudioProcessor)
};
