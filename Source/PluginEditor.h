#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include <array>
#include <memory>
#include <vector>

class SkellLookAndFeel : public juce::LookAndFeel_V4
{
public:
    SkellLookAndFeel();
    void drawRotarySlider(juce::Graphics&, int x, int y, int width, int height,
                          float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                          juce::Slider&) override;
    void drawButtonBackground(juce::Graphics&, juce::Button&, const juce::Colour&,
                              bool isHighlighted, bool isDown) override;
    void drawComboBox(juce::Graphics&, int width, int height, bool isButtonDown,
                      int bx, int by, int bw, int bh, juce::ComboBox&) override;
    juce::Font getComboBoxFont(juce::ComboBox&) override;
};

class SkullDisplay final : public juce::Component, private juce::Timer
{
public:
    explicit SkullDisplay(SkellSpectreAudioProcessor& processor);
    void paint(juce::Graphics&) override;
private:
    void timerCallback() override { repaint(); }
    SkellSpectreAudioProcessor& processor;
    float shownPeak = 0.0f;
};

class SpectrumDisplay final : public juce::Component, private juce::Timer
{
public:
    explicit SpectrumDisplay(SkellSpectreAudioProcessor& processor);
    void paint(juce::Graphics&) override;
private:
    void timerCallback() override;
    SkellSpectreAudioProcessor& processor;
    juce::dsp::FFT fft { 10 };
    std::array<float, 2048> fftData {};
    std::array<float, 512> magsInput {}, magsOutput {};
};

class SkellSpectreAudioProcessorEditor final : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit SkellSpectreAudioProcessorEditor(SkellSpectreAudioProcessor&);
    ~SkellSpectreAudioProcessorEditor() override;
    void paint(juce::Graphics&) override;
    void resized() override;

private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using ComboAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    struct Control
    {
        std::unique_ptr<juce::Slider> slider;
        std::unique_ptr<juce::Label> name;
        bool linear = false;
    };
    Control* addControl(const juce::String& id, const juce::String& title, bool linear = false);
    void place(Control* control, float x, float y, float w, float h, bool show);
    void changeMode(bool useBasic);
    void setParam(const juce::String& id, float value);
    void timerCallback() override;
    void setModeStyle();
    SkellSpectreAudioProcessor& processor;
    SkellLookAndFeel look;
    SkullDisplay skull;
    SpectrumDisplay spectrum;
    std::vector<std::unique_ptr<Control>> controls;
    std::vector<std::unique_ptr<SliderAttachment>> sliderAttachments;
    std::vector<std::unique_ptr<ButtonAttachment>> buttonAttachments;
    std::vector<std::unique_ptr<ComboAttachment>> comboAttachments;
    std::array<Control*, 4> macro {};
    std::array<Control*, 3> masters {}, crossover {};
    std::array<std::array<Control*, 6>, 4> strips {};
    std::array<std::array<std::unique_ptr<juce::TextButton>, 2>, 4> bandSwitches {};
    juce::TextButton basicButton { "BASIC" }, expertButton { "EXPERT" };
    juce::TextButton prev { "<" }, next { ">" };
    juce::TextButton limiter { "LIMITER" }, autoGain { "AUTO GAIN" }, hq { "HQ" };
    juce::ComboBox preset, slope, driveType, widthType;
    juce::Label footer;
    bool previousBasic = true;
    float uiScale = 1.0f;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SkellSpectreAudioProcessorEditor)
};
