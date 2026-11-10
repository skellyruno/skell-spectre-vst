// Windows build/interaction smoke test for the actual JUCE editor.
// Runs offscreen: no user desktop, FL Studio or audio device is required.
#include "../Source/PluginProcessor.h"
#include "../Source/PluginEditor.h"
#include <cmath>
#include <iostream>
#include <memory>
#include <stdexcept>

static void check(bool condition, const char* reason)
{
    if (!condition) throw std::runtime_error(reason);
}

static juce::TextButton* findButton(juce::Component& root, const juce::String& label)
{
    for (int i = 0; i < root.getNumChildComponents(); ++i)
    {
        auto* c = root.getChildComponent(i);
        if (auto* b = dynamic_cast<juce::TextButton*>(c))
            if (b->getButtonText() == label) return b;
        if (auto* nested = findButton(*c, label)) return nested;
    }
    return nullptr;
}

static void setDrive(SkellSpectreAudioProcessor& processor, float value)
{
    auto* parameter = processor.parameters.getParameter("drive");
    check(parameter != nullptr, "missing Drive parameter");
    parameter->setValueNotifyingHost(parameter->convertTo0to1(value));
}

static void click(juce::Component& editor, const char* label)
{
    auto* b = findButton(editor, label);
    check(b != nullptr, "missing A/B button");
    b->triggerClick();
    juce::MessageManager::getInstance()->runDispatchLoopUntil(80);
}

static void screenshot(juce::Component& editor, int width, int height)
{
    editor.setSize(width, height);
    const auto image = editor.createComponentSnapshot(editor.getLocalBounds());
    check(image.isValid(), "cannot render offscreen plugin UI");
    check(image.getWidth() == width && image.getHeight() == height, "wrong snapshot size");
    const auto background = image.getPixelAt(0, 0);
    int different = 0;
    for (int y = 20; y < height; y += 19)
        for (int x = 20; x < width; x += 19)
            different += image.getPixelAt(x, y) != background;
    check(different >= 25, "editor rendered blank or mostly uniform");
    for (int i=0;i<editor.getNumChildComponents();++i)
    {
        auto* child=editor.getChildComponent(i);
        if (!child->isVisible()) continue;
        const auto r=child->getBounds();
        check(r.getWidth() > 0 && r.getHeight() > 0, "visible child has empty bounds");
        check(r.getX() >= 0 && r.getY() >= 0 && r.getRight() <= width+2 && r.getBottom() <= height+2,
              "visible child falls outside resizable editor");
    }
}

int main()
{
    try
    {
        juce::ScopedJuceInitialiser_GUI gui;
        SkellSpectreAudioProcessor processor;
        processor.prepareToPlay(48000, 512);
        {
            std::unique_ptr<juce::AudioProcessorEditor> editor(processor.createEditor());
            screenshot(*editor, 704, 396);
            screenshot(*editor, 880, 495);
            screenshot(*editor, 1440, 810);
            setDrive(processor, 22.5f);
            click(*editor, "B");
            check(std::abs(processor.parameter("drive")) < .2f, "initial B slot did not load");
            setDrive(processor, 75.0f);
        }
        {
            std::unique_ptr<juce::AudioProcessorEditor> editor(processor.createEditor());
            check(std::abs(processor.parameter("drive") - 75.0f) < .2f, "active B lost on editor reopen");
            click(*editor, "A");
            check(std::abs(processor.parameter("drive") - 22.5f) < .2f, "A slot lost on editor reopen");
            click(*editor, "B");
            check(std::abs(processor.parameter("drive") - 75.0f) < .2f, "B slot lost on editor reopen");
            for (int i=0;i<8;++i)
            {
                click(*editor, "A");
                click(*editor, "B");
            }
            juce::MemoryBlock state;
            processor.getStateInformation(state);
            check(state.getSize() < 128000, "recursive A/B snapshot growth detected");
        }
        juce::MemoryBlock data;
        processor.getStateInformation(data);
        SkellSpectreAudioProcessor restored;
        restored.setStateInformation(data.getData(), static_cast<int>(data.getSize()));
        {
            std::unique_ptr<juce::AudioProcessorEditor> editor(restored.createEditor());
            screenshot(*editor, 880, 495);
            check(std::abs(restored.parameter("drive") - 75.0f) < .2f, "host state did not restore B");
            click(*editor, "A");
            check(std::abs(restored.parameter("drive") - 22.5f) < .2f, "host state did not restore A");
            click(*editor, "BANDS");
            screenshot(*editor, 704, 396);
            screenshot(*editor, 880, 495);
            screenshot(*editor, 1440, 810);
        }
        std::cout << "UI render / resize / A-B / session state passed\n";
        return 0;
    }
    catch (const std::exception& ex)
    {
        std::cerr << ex.what() << '\n';
        return 1;
    }
}
