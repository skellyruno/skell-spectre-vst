#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include <array>
#include <functional>
#include <memory>
#include <vector>

class SkellLookAndFeel : public juce::LookAndFeel_V4
{
public:
    SkellLookAndFeel();
    void drawRotarySlider(juce::Graphics&, int, int, int, int, float, float, float, juce::Slider&) override;
    void drawLinearSlider(juce::Graphics&, int, int, int, int, float, float, float,
                          juce::Slider::SliderStyle, juce::Slider&) override;
    void drawButtonBackground(juce::Graphics&, juce::Button&, const juce::Colour&, bool, bool) override;
    void drawComboBox(juce::Graphics&, int, int, bool, int, int, int, int, juce::ComboBox&) override;
    juce::Font getComboBoxFont(juce::ComboBox&) override;
};

class SkullDisplay final : public juce::Component, private juce::Timer
{
public:
    explicit SkullDisplay(SkellSpectreAudioProcessor&);
    void paint(juce::Graphics&) override;
private:
    void timerCallback() override { if (isShowing()) repaint(); }
    SkellSpectreAudioProcessor& processor;
    float shownPeak = 0.0f;
};

class SpectrumDisplay final : public juce::Component, private juce::Timer
{
public:
    explicit SpectrumDisplay(SkellSpectreAudioProcessor&);
    void paint(juce::Graphics&) override;
private:
    void timerCallback() override;
    SkellSpectreAudioProcessor& processor;
    juce::dsp::FFT fft { 10 };
    std::array<float, 2048> fftData {};
    std::array<float, 512> magsInput {}, magsOutput {};
};

// On-screen band picker: sigma means all bands; A-D address four independent bands.
class BandRail final : public juce::Component
{
public:
    std::function<void(int)> onSelect;
    void setSelection(int s) { selection = juce::jlimit(0, 4, s); repaint(); }
    int getSelection() const noexcept { return selection; }
    void paint(juce::Graphics&) override;
    void mouseUp(const juce::MouseEvent&) override;
private:
    int selection = 0;
};

class SplitStrip final : public juce::Component
{
public:
    std::function<void(int)> onSelect;
    void setSelection(int s) { selected = juce::jlimit(0, 3, s); repaint(); }
    void paint(juce::Graphics&) override;
    void mouseUp(const juce::MouseEvent&) override;
private:
    int selected = 3;
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
    Control* addControl(const juce::String&, const juce::String&, bool linear = false, bool attach = true);
    void place(Control*, float x, float y, float w, float h, bool show, bool header = false);
    void bindMacro(int index, int band);
    void changeMode(bool useBasic);
    void showExpert(bool show);
    void setParam(const juce::String&, float);
    void timerCallback() override;
    void setModeStyle();
    void changeAB(int);
    void copyAToB();

    SkellSpectreAudioProcessor& processor;
    SkellLookAndFeel look;
    SkullDisplay skull;
    SpectrumDisplay spectrum;
    SplitStrip splitStrip;
    std::array<BandRail, 4> rails;
    std::vector<std::unique_ptr<Control>> controls;
    std::vector<std::unique_ptr<SliderAttachment>> sliderAttachments;
    std::array<std::unique_ptr<SliderAttachment>, 4> macroAttachments;
    std::vector<std::unique_ptr<ButtonAttachment>> buttonAttachments;
    std::vector<std::unique_ptr<ComboAttachment>> comboAttachments;
    std::array<Control*, 4> macro {};
    std::array<Control*, 3> masters {}, crossover {};
    std::array<std::array<Control*, 6>, 4> strips {};
    std::array<std::array<std::unique_ptr<juce::TextButton>, 2>, 4> bandSwitches {};
    juce::TextButton modeToggle { "MULTIBAND" };
    juce::TextButton prev { "<" }, next { ">" };
    juce::TextButton limiter { "LIMITER" }, autoGain { "AUTOGAIN" }, hq { "HQ" };
    juce::TextButton power { "" }, slotA { "A" }, slotB { "B" }, copyAB { "A > B" };
    juce::ComboBox preset, driveType, widthType;
    juce::Label footer;
    std::array<juce::ValueTree, 2> snapshots;
    int currentAB = 0;
    bool showingExpert = false, previousBasic = true, rebindingMacro = false;
    float uiScale = 1.0f;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SkellSpectreAudioProcessorEditor)
};
