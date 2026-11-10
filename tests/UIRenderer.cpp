// Actual SKELL SPECTRE processor/editor, captured via immutable SkellCore renderer.
// This executable is a development tool, never packaged in the shipping VST3.
#include "../Source/PluginEditor.h"
#include "../Source/PluginProcessor.h"
#include <SkellCore/HeadlessUIRenderer.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <array>
#include <cstdio>
#include <memory>

namespace {
using namespace skellcore::ui::headless;

juce::TextButton* button(juce::Component& root, const juce::String& label)
{
    for (int i = 0; i < root.getNumChildComponents(); ++i)
    {
        auto* child = root.getChildComponent(i);
        if (auto* found = dynamic_cast<juce::TextButton*>(child))
            if (found->getButtonText() == label) return found;
        if (auto* found = button(*child, label)) return found;
    }
    return nullptr;
}

juce::ComboBox* combo(juce::Component& root, int index)
{
    int count = 0;
    for (int i = 0; i < root.getNumChildComponents(); ++i)
        if (auto* found = dynamic_cast<juce::ComboBox*>(root.getChildComponent(i)))
            if (count++ == index) return found;
    return nullptr;
}

void setParameter(SkellSpectreAudioProcessor& processor, const char* id, float value)
{
    if (auto* parameter = processor.parameters.getParameter(id))
        parameter->setValueNotifyingHost(parameter->convertTo0to1(value));
}

juce::StringArray inspectBounds(juce::Component& root)
{
    juce::StringArray issues;
    for (int i = 0; i < root.getNumChildComponents(); ++i)
    {
        auto* child = root.getChildComponent(i);
        if (!child->isVisible()) continue;
        const auto bounds = child->getBounds();
        if (bounds.getWidth() <= 0 || bounds.getHeight() <= 0)
            issues.add("Visible component has empty bounds: " + child->getName());
        if (bounds.getX() < 0 || bounds.getY() < 0
            || bounds.getRight() > root.getWidth() + 2
            || bounds.getBottom() > root.getHeight() + 2)
            issues.add("Component clips outside editor: " + child->getName()
                       + " " + bounds.toString());
    }
    return issues;
}

void add(std::vector<Scene>& scenes, const char* id, int width, int height, int mode = 0)
{
    scenes.push_back({id, width, height, 0, mode, false});
}

void createContactSheet(const juce::File& folder, const std::vector<Scene>& scenes, int& failures)
{
    constexpr int columns = 3;
    constexpr int cellWidth = 450, shotHeight = 254, captionHeight = 27;
    const int rows = (static_cast<int>(scenes.size()) + columns - 1) / columns;
    juce::Image sheet(juce::Image::RGB, columns * cellWidth,
                      rows * (shotHeight + captionHeight), true);
    juce::Graphics g(sheet);
    g.fillAll(juce::Colour::fromRGB(15, 16, 23));
    for (size_t i = 0; i < scenes.size(); ++i)
    {
        const auto& scene = scenes[i];
        auto image = juce::ImageFileFormat::loadFrom(folder.getChildFile(scene.id + ".png"));
        if (!image.isValid()) { ++failures; continue; }
        const int x = static_cast<int>(i % columns) * cellWidth;
        const int y = static_cast<int>(i / columns) * (shotHeight + captionHeight);
        g.drawImageWithin(image, x + 3, y + 2, cellWidth - 6, shotHeight - 4,
                          juce::RectanglePlacement::centred | juce::RectanglePlacement::onlyReduceInSize);
        g.setColour(juce::Colour::fromRGB(206, 205, 215));
        g.setFont(juce::Font(juce::FontOptions(14.0f)));
        g.drawFittedText(scene.id, x + 7, y + shotHeight, cellWidth - 14,
                         captionHeight - 3, juce::Justification::centredLeft, 1);
    }
    if (!writePng(sheet, folder.getChildFile("contact-sheet.png"))) ++failures;
}
} // namespace

int main(int argc, char** argv)
{
    const auto directory = argc >= 3 && juce::String(argv[1]) == "--output"
        ? juce::String::fromUTF8(argv[2]) : juce::String("ui-screenshots");
    juce::ScopedJuceInitialiser_GUI gui;
    SkellSpectreAudioProcessor processor;
    processor.prepareToPlay(48000.0, 512);
    int failures = 0;
    {
        SkellSpectreAudioProcessorEditor editor(processor);
        Options options;
        options.project = "SKELL SPECTRE / real JUCE Reactor editor";
        add(options.scenes, "character-small-704", 704, 396);
        add(options.scenes, "character-default-880", 880, 495);
        add(options.scenes, "character-large-1440", 1440, 810);
        add(options.scenes, "expert-small-704", 704, 396, 1);
        add(options.scenes, "expert-default-880", 880, 495, 1);
        add(options.scenes, "expert-large-1440", 1440, 810, 1);
        add(options.scenes, "character-bypass-880", 880, 495);
        add(options.scenes, "character-band-b-880", 880, 495);
        add(options.scenes, "character-slot-b-880", 880, 495);
        add(options.scenes, "character-hover-hq-880", 880, 495);
        add(options.scenes, "character-disabled-hq-880", 880, 495);
        add(options.scenes, "expert-tape-880", 880, 495, 1);
        add(options.scenes, "expert-band-muted-880", 880, 495, 1);
        add(options.scenes, "character-factory-preset-880", 880, 495);
        options.configure = [&](const Scene& scene)
        {
            // Restore deterministic, actual parameter and component state.
            editor.selectComparisonSlot(0);
            setParameter(processor, "bypass", 0.0f);
            setParameter(processor, "hq", 1.0f);
            setParameter(processor, "driveType", 0.0f);
            editor.setDetailedView(scene.mode == 1);
            if (auto* hq = button(editor, "HQ"))
            {
                hq->setEnabled(true);
                hq->setState(juce::Button::buttonNormal);
                if (scene.id.contains("hover")) hq->setState(juce::Button::buttonOver);
                if (scene.id.contains("disabled")) hq->setEnabled(false);
            }
            if (auto* power = button(editor, "")) power->setToggleState(false, juce::dontSendNotification);
            for (int i = 0; i < editor.getNumChildComponents(); ++i)
                if (auto* rail = dynamic_cast<BandRail*>(editor.getChildComponent(i)))
                    rail->setSelection(0);
            if (scene.id.contains("bypass"))
            {
                setParameter(processor, "bypass", 1.0f);
                if (auto* power = button(editor, "")) power->setToggleState(true, juce::dontSendNotification);
            }
            if (scene.id.contains("band-b"))
                for (int i = 0; i < editor.getNumChildComponents(); ++i)
                    if (auto* rail = dynamic_cast<BandRail*>(editor.getChildComponent(i)))
                        rail->setSelection(2);
            if (scene.id.contains("slot-b")) editor.selectComparisonSlot(1);
            if (scene.id.contains("tape"))
                if (auto* drive = combo(editor, 1)) drive->setSelectedId(2, juce::sendNotificationSync);
            if (scene.id.contains("muted"))
                if (auto* mute = button(editor, "MUTE")) mute->setToggleState(true, juce::sendNotificationSync);
            if (scene.id.contains("preset"))
                if (auto* presets = combo(editor, 0)) presets->setSelectedId(2, juce::sendNotificationSync);
            editor.repaint();
        };
        options.inspect = [&] { return inspectBounds(editor); };
        options.mustDiffer = {
            {"character-default-880", "expert-default-880"},
            {"character-default-880", "character-bypass-880"},
            {"character-default-880", "character-band-b-880"},
            {"character-default-880", "character-slot-b-880"},
            {"character-default-880", "character-hover-hq-880"},
            {"expert-default-880", "expert-tape-880"}
        };
        auto output = juce::File::getCurrentWorkingDirectory().getChildFile(directory);
        const auto report = render(editor, output, options);
        for (const auto& error : report.errors)
        {
            std::fprintf(stderr, "UI capture failed: %s\n", error.toRawUTF8());
            ++failures;
        }
        if (report.captured != static_cast<int>(options.scenes.size())) ++failures;
        createContactSheet(output, options.scenes, failures);
        std::printf("SKELL SPECTRE real-UI renderer: %d scenes, %d failures\n",
                    report.captured, failures);
    }
    processor.releaseResources();
    return failures == 0 ? 0 : 1;
}
