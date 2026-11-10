// Real-editor visual regression runner. Development-only, not linked into VST3.
// Captures the production AudioProcessorEditor, not a UI imitation.
#include "../Source/PluginEditor.h"
#include "../Source/PluginProcessor.h"
#include <SkellCore/HeadlessUIRenderer.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <array>
#include <cstdio>
#include <memory>

namespace {
void setBool(SkellSpectreAudioProcessor& processor, const juce::String& id, bool state) {
    if (auto* p = processor.parameters.getParameter(id))
        p->setValueNotifyingHost(state ? 1.0f : 0.0f);
}
juce::TextButton* findButton(juce::Component& component, const juce::String& text) {
    for (int i = 0; i < component.getNumChildComponents(); ++i) {
        auto* child = component.getChildComponent(i);
        if (auto* button = dynamic_cast<juce::TextButton*>(child))
            if (button->getButtonText() == text) return button;
        if (auto* nested = findButton(*child, text)) return nested;
    }
    return nullptr;
}
juce::StringArray inspectLayout(juce::Component& root) {
    juce::StringArray issues;
    for (int i = 0; i < root.getNumChildComponents(); ++i) {
        auto* child = root.getChildComponent(i);
        if (!child->isVisible()) continue;
        const auto b = child->getBounds();
        if (b.getWidth() <= 0 || b.getHeight() <= 0)
            issues.add("Visible child " + juce::String(i) + " has empty bounds");
        if (b.getX() < -2 || b.getY() < -2 ||
            b.getRight() > root.getWidth() + 2 || b.getBottom() > root.getHeight() + 2)
            issues.add("Visible child " + juce::String(i) + " exceeds editor bounds");
    }
    return issues;
}
}

int main(int argc, char** argv) {
    const juce::String path = argc >= 3 && juce::String(argv[1]) == "--output"
        ? juce::String::fromUTF8(argv[2]) : juce::String("ui-screenshots");
    juce::ScopedJuceInitialiser_GUI gui;
    SkellSpectreAudioProcessor processor;
    processor.prepareToPlay(48000.0, 512);
    int failures = 0;
    {
        SkellSpectreAudioProcessorEditor editor(processor);
        skellcore::ui::headless::Options options;
        options.project = "SKELL SPECTRE production JUCE editor";
        options.minimumColours = 12;
        const std::array<std::pair<int,int>, 3> sizes {{{704,396},{880,495},{1440,810}}};
        for (const auto& size : sizes) {
            const juce::String tag = juce::String(size.first);
            options.scenes.push_back({"basic-" + tag,size.first,size.second,0,0,false});
            options.scenes.push_back({"expert-" + tag,size.first,size.second,0,1,false});
        }
        options.scenes.push_back({"bypass-880",880,495,0,0,false});
        options.scenes.push_back({"slot-b-880",880,495,0,0,false});
        options.scenes.push_back({"selected-880",880,495,0,0,false});
        options.scenes.push_back({"expert-bypass-704",704,396,0,1,false});
        options.configure = [&](const skellcore::ui::headless::Scene& scene) {
            const auto id = scene.id;
            const bool expert = id.startsWith("expert-");
            editor.setDetailedView(expert);
            editor.selectComparisonSlot(id == "slot-b-880" ? 1 : 0);
            const bool bypass = id.contains("bypass");
            setBool(processor, "bypass", bypass);
            if (auto* power = findButton(editor, ""))
                power->setToggleState(bypass, juce::dontSendNotification);
            // A/B switching is synchronous. A visibly distinct B state also
            // exercises the UI binding rather than only a selected tab label.
            if (id == "slot-b-880")
                if (auto* drive = processor.parameters.getParameter("drive"))
                    drive->setValueNotifyingHost(drive->convertTo0to1(45.0f));
            for (int i=0; i<editor.getNumChildComponents(); ++i)
                if (auto* rail = dynamic_cast<BandRail*>(editor.getChildComponent(i)))
                    rail->setSelection(id == "selected-880" ? 2 : 0);
        };
        options.inspect = [&] { return inspectLayout(editor); };
        options.mustDiffer = {
            {"basic-880","expert-880"},
            {"basic-880","bypass-880"},
            {"basic-880","slot-b-880"},
            {"basic-880","selected-880"}
        };
        const juce::File folder = juce::File::getCurrentWorkingDirectory().getChildFile(path);
        const auto report = skellcore::ui::headless::render(editor, folder, options);
        for (const auto& error : report.errors) {
            std::fprintf(stderr,"Render: %s\n",error.toRawUTF8());
            ++failures;
        }
        if (report.captured != 10) {
            std::fprintf(stderr,"Expected 10 scenes, received %d\n",report.captured);
            ++failures;
        }
        // Contact sheet: 3 sizes, Basic and Expert. Preserve native pixels of small view.
        juce::Image sheet(juce::Image::RGB, 1760, 1485, true);
        juce::Graphics g(sheet);
        for (int row=0; row<3; ++row) {
            const auto sz = sizes[static_cast<size_t>(row)];
            for (int col=0; col<2; ++col) {
                editor.setDetailedView(col == 1);
                editor.selectComparisonSlot(0);
                setBool(processor,"bypass",false);
                auto capture = skellcore::ui::headless::capture(editor,sz.first,sz.second);
                if (!capture.image.isValid() || capture.distinctColours < 12) {
                    std::fprintf(stderr,"Contact sheet capture invalid\n");
                    ++failures;
                } else {
                    const juce::Rectangle<int> target(
                        col * 880, row * 495, 880, 495);
                    g.drawImageWithin(capture.image,target.getX(),target.getY(),
                                      target.getWidth(),target.getHeight(),
                                      juce::RectanglePlacement::centred);
                }
            }
        }
        if (!skellcore::ui::headless::writePng(sheet,folder.getChildFile("contact-sheet.png")))
            ++failures;
        std::printf("SKELL SPECTRE SkellCore renderer: %d scenes; %d failures\n",
                    report.captured,failures);
    }
    processor.releaseResources();
    return failures == 0 ? 0 : 1;
}
