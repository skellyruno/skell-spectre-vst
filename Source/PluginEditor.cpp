#include "PluginEditor.h"
#include <cmath>

namespace
{
const juce::Colour bg = juce::Colour::fromRGB(9, 13, 14);
const juce::Colour panel = juce::Colour::fromRGB(18, 24, 25);
const juce::Colour rim = juce::Colour::fromRGB(55, 70, 67);
const juce::Colour neon = juce::Colour::fromRGB(175, 255, 65);
const juce::Colour bone = juce::Colour::fromRGB(221, 229, 206);
const juce::Colour subtle = juce::Colour::fromRGB(114, 131, 120);

void smallText(juce::Graphics& g, juce::String str, juce::Rectangle<float> rect,
               float size = 11.0f, juce::Colour colour = subtle, juce::Justification j = juce::Justification::centredLeft)
{
    g.setColour(colour);
    g.setFont(juce::Font(size, juce::Font::plain));
    g.drawFittedText(str, rect.toNearestInt(), j, 1);
}
void smallText(juce::Graphics& g, juce::String str, float x, float y, float w, float h,
               float size = 11.0f, juce::Colour colour = subtle,
               juce::Justification j = juce::Justification::centredLeft)
{
    smallText(g, str, juce::Rectangle<float>(x, y, w, h), size, colour, j);
}
void outlinedBox(juce::Graphics& g, juce::Rectangle<float> r, float corner = 13.0f)
{
    g.setColour(panel);
    g.fillRoundedRectangle(r, corner);
    g.setColour(rim.withAlpha(.75f));
    g.drawRoundedRectangle(r.reduced(.6f), corner, 1.15f);
}
juce::Rectangle<int> scaled(float x, float y, float w, float h, float s)
{
    return { juce::roundToInt(x*s), juce::roundToInt(y*s), juce::roundToInt(w*s), juce::roundToInt(h*s) };
}
}

SkellLookAndFeel::SkellLookAndFeel()
{
    setColour(juce::Slider::thumbColourId, neon);
    setColour(juce::Slider::rotarySliderFillColourId, neon);
    setColour(juce::Slider::rotarySliderOutlineColourId, rim);
    setColour(juce::Slider::trackColourId, neon);
    setColour(juce::Slider::backgroundColourId, rim);
    setColour(juce::Slider::textBoxTextColourId, bone);
    setColour(juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour(juce::ComboBox::textColourId, bone);
    setColour(juce::ComboBox::backgroundColourId, panel);
    setColour(juce::ComboBox::outlineColourId, rim);
    setColour(juce::ComboBox::arrowColourId, neon);
    setColour(juce::PopupMenu::backgroundColourId, panel);
    setColour(juce::PopupMenu::textColourId, bone);
    setColour(juce::PopupMenu::highlightedBackgroundColourId, neon.withAlpha(.25f));
    setColour(juce::TextButton::textColourOffId, bone);
    setColour(juce::TextButton::textColourOnId, bg);
}

void SkellLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int w, int h,
    float pos, float start, float end, juce::Slider&)
{
    const float d = static_cast<float>(juce::jmin(w, h));
    const float radius = d * .34f;
    const float cx = x + .5f * w, cy = y + .5f * h;
    const float angle = start + pos * (end - start);
    g.setColour(juce::Colours::black.withAlpha(.6f));
    g.fillEllipse(cx - radius - 5, cy - radius - 3, radius*2+10, radius*2+10);
    g.setColour(juce::Colour::fromRGB(28, 38, 38));
    g.fillEllipse(cx - radius, cy - radius, radius*2, radius*2);
    g.setColour(rim.brighter(.15f));
    g.drawEllipse(cx - radius, cy - radius, radius*2, radius*2, 1.5f);
    juce::Path track, progress;
    track.addCentredArc(cx, cy, radius*1.21f, radius*1.21f, 0.0f, start, end, true);
    progress.addCentredArc(cx, cy, radius*1.21f, radius*1.21f, 0.0f, start, angle, true);
    g.setColour(rim);
    g.strokePath(track, juce::PathStrokeType(4.3f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour(neon.withAlpha(.17f));
    g.strokePath(progress, juce::PathStrokeType(8.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour(neon);
    g.strokePath(progress, juce::PathStrokeType(3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    const float x1 = cx + std::sin(angle)*radius*.28f;
    const float y1 = cy - std::cos(angle)*radius*.28f;
    const float x2 = cx + std::sin(angle)*radius*.77f;
    const float y2 = cy - std::cos(angle)*radius*.77f;
    g.setColour(bone);
    g.drawLine(x1, y1, x2, y2, 3.1f);
    g.setColour(neon.withAlpha(.6f));
    g.fillEllipse(cx-2.0f, cy-2.0f, 4.0f, 4.0f);
}

void SkellLookAndFeel::drawButtonBackground(juce::Graphics& g, juce::Button& btn,
    const juce::Colour&, bool highlighted, bool down)
{
    auto rect = btn.getLocalBounds().toFloat().reduced(1.0f);
    const bool selected = btn.getToggleState();
    g.setColour(selected ? neon : (highlighted ? panel.brighter(.12f) : panel));
    g.fillRoundedRectangle(rect, 8.0f);
    g.setColour(selected ? neon : rim);
    g.drawRoundedRectangle(rect, 8.0f, 1.2f);
    if (down) { g.setColour(neon.withAlpha(.13f)); g.fillRoundedRectangle(rect, 8.0f); }
}

void SkellLookAndFeel::drawComboBox(juce::Graphics& g, int width, int height, bool,
    int, int, int, int, juce::ComboBox&)
{
    outlinedBox(g, { 0.5f, 0.5f, static_cast<float>(width - 1), static_cast<float>(height - 1) }, 7.0f);
    juce::Path arrow;
    const float cx = width - 15.0f, cy = height * .5f;
    arrow.startNewSubPath(cx-4.0f, cy-2.0f); arrow.lineTo(cx,cy+2.0f); arrow.lineTo(cx+4.0f,cy-2.0f);
    g.setColour(neon); g.strokePath(arrow, juce::PathStrokeType(1.5f));
}
juce::Font SkellLookAndFeel::getComboBoxFont(juce::ComboBox&) { return juce::Font(13.0f, juce::Font::bold); }

SkullDisplay::SkullDisplay(SkellSpectreAudioProcessor& p) : processor(p)
{
    setInterceptsMouseClicks(false, false);
    startTimerHz(30);
}

void SkullDisplay::paint(juce::Graphics& g)
{
    const float energy = juce::jlimit(0.0f, 1.0f, processor.outputPeak.load(std::memory_order_relaxed));
    shownPeak += .16f * (energy - shownPeak);
    juce::Graphics::ScopedSaveState save(g);
    g.addTransform(juce::AffineTransform::scale(getWidth()/500.0f, getHeight()/320.0f));
    const float glow = juce::jlimit(0.2f, 1.0f, .3f + shownPeak*.9f);
    // Neon field: all vectors, no copied character assets.
    for (int a = 0; a < 4; ++a)
    {
        g.setColour(neon.withAlpha(.035f + .02f * glow));
        g.drawEllipse(250.0f - 135.0f - a * 14.0f, 152.0f - 117.0f - a * 9.0f,
                      270.0f + a * 28.0f, 234.0f + a * 18.0f, 1.4f);
    }
    g.setColour(neon.withAlpha(.2f));
    for (int i = 0; i < 7; ++i)
    {
        const float y = 50.0f + i*36.0f;
        g.drawLine(60.0f, y, 100.0f, y, 1.0f);
        g.drawLine(400.0f, y, 440.0f, y, 1.0f);
    }
    // Skeletal shoulders/arms: the arm span responds to the stereo macro.
    const float width = processor.parameter("stereo") / 100.0f;
    const float widen = width*22.0f;
    auto boneLine = [&g](juce::Point<float> a, juce::Point<float> b, float thickness)
    {
        g.setColour(juce::Colour::fromRGB(105, 122, 108));
        g.drawLine({a,b}, thickness+6.0f);
        g.setColour(bone);
        g.drawLine({a,b}, thickness);
        g.setColour(neon.withAlpha(.22f));
        g.drawLine({a,b}, 2.0f);
    };
    boneLine({204,195}, {142-widen,221}, 10);
    boneLine({142-widen,221}, {114-widen,272}, 8);
    boneLine({296,195}, {358+widen,221}, 10);
    boneLine({358+widen,221}, {386+widen,272}, 8);
    // Vertebral column and hips.
    boneLine({250,168},{250,283},12);
    for (int i=0; i<6; ++i)
    {
        const float y = 208.0f + i*14.0f;
        g.setColour(bone);
        g.fillRoundedRectangle(241.0f,y,18.0f,6.0f,2.0f);
    }
    boneLine({250,280},{205,307},12); boneLine({250,280},{295,307},12);
    // Ribs: thin, symmetric quadratic arcs.
    for (int i=0; i<5; ++i)
    {
        const float ry = 209.0f + i*14.0f;
        const float spread = 79.0f - i*8.0f;
        juce::Path rib;
        rib.startNewSubPath(250.0f, ry);
        rib.quadraticTo(250.0f-spread*1.1f,ry+2,250.0f-spread,ry+22);
        rib.startNewSubPath(250.0f, ry);
        rib.quadraticTo(250.0f+spread*1.1f,ry+2,250.0f+spread,ry+22);
        g.setColour(bone.withAlpha(.92f));
        g.strokePath(rib, juce::PathStrokeType(5.5f,juce::PathStrokeType::curved,juce::PathStrokeType::rounded));
        g.setColour(neon.withAlpha(.27f));
        g.strokePath(rib, juce::PathStrokeType(1.1f));
    }
    // Skull housing: uneven bone silhouette instead of a borrowed mascot.
    juce::Path skull;
    skull.startNewSubPath(184.0f,90.0f);
    skull.cubicTo(180.0f,28.0f,215.0f,10.0f,253.0f,16.0f);
    skull.cubicTo(300.0f,11.0f,324.0f,43.0f,316.0f,94.0f);
    skull.cubicTo(326.0f,132.0f,306.0f,150.0f,291.0f,155.0f);
    skull.lineTo(286.0f,177.0f); skull.lineTo(217.0f,177.0f); skull.lineTo(211.0f,153.0f);
    skull.cubicTo(184.0f,142.0f,176.0f,120.0f,184.0f,90.0f);
    skull.closeSubPath();
    g.setColour(neon.withAlpha(.12f));
    g.strokePath(skull,juce::PathStrokeType(14.0f));
    g.setColour(bone);
    g.fillPath(skull);
    g.setColour(juce::Colour::fromRGB(101, 122, 105));
    g.strokePath(skull,juce::PathStrokeType(3.2f));
    // Brow, hollow eye sockets, nose cavity, teeth.
    const float recoil = processor.parameter("punch") * .02f;
    g.setColour(bg);
    g.fillEllipse(203,75+recoil,41,39);
    g.fillEllipse(260,75+recoil,41,39);
    juce::Path nose;
    nose.startNewSubPath(250,109); nose.lineTo(240,133); nose.lineTo(261,133); nose.closeSubPath();
    g.fillPath(nose);
    g.fillRoundedRectangle(215,145,73,17,4);
    g.setColour(neon.withAlpha(glow));
    g.fillEllipse(215,86+recoil,16,9); g.fillEllipse(273,86+recoil,16,9);
    g.setColour(bone);
    for (int t=0;t<6;++t) g.fillRoundedRectangle(218.0f+t*11.5f,145.0f,6,17,1);
    // Split cranium crack, Skell signature.
    juce::Path crack;
    crack.startNewSubPath(260,20); crack.lineTo(252,44); crack.lineTo(262,58);
    crack.lineTo(248,76);
    g.setColour(juce::Colour::fromRGB(77, 95, 81));
    g.strokePath(crack, juce::PathStrokeType(3.0f));
    // Signal electricity highlights, increasing with Drive.
    const float drive = processor.parameter("drive") / 100.0f;
    if (drive > .01f)
    {
        juce::Path spark;
        spark.startNewSubPath(326,62); spark.lineTo(350,40); spark.lineTo(346,72);
        spark.lineTo(373,55); spark.lineTo(355,99);
        g.setColour(neon.withAlpha(.28f + drive*.65f));
        g.strokePath(spark,juce::PathStrokeType(1.0f+drive*3.0f));
    }
    smallText(g, "SKELL // ACTIVE", { 13,287,137,18 }, 10.0f, neon.withAlpha(.8f));
    smallText(g, "BONE-DRIVEN DSP", { 351,287,139,18 }, 10.0f, subtle, juce::Justification::centredRight);
}

SpectrumDisplay::SpectrumDisplay(SkellSpectreAudioProcessor& p) : processor(p)
{
    setInterceptsMouseClicks(false, false);
    startTimerHz(20);
}

void SpectrumDisplay::timerCallback()
{
    if (!isShowing()) return;
    const unsigned int end = processor.scopeWrite.load(std::memory_order_acquire);
    for (int stage = 0; stage < 2; ++stage)
    {
        fftData.fill(0.0f);
        const auto& scope = stage == 0 ? processor.scopeInput : processor.scopeOutput;
        for (int i = 0; i < 1024; ++i)
        {
            const auto index = (end - 1024u + static_cast<unsigned int>(i)) % 1024u;
            const float wave = scope[index].load(std::memory_order_relaxed);
            // Hann window
            const float hann = .5f - .5f*std::cos(2.0f*skell::kPi*static_cast<float>(i)/1023.0f);
            fftData[i] = wave * hann;
        }
        fft.performFrequencyOnlyForwardTransform(fftData.data());
        auto& dest = stage == 0 ? magsInput : magsOutput;
        for (int k=0; k<512; ++k)
        {
            const float magnitude = fftData[k] / 256.0f;
            const float level = juce::jlimit(0.0f,1.0f,(20.0f*std::log10(magnitude+0.00001f)+82.0f)/82.0f);
            dest[k] = .72f*dest[k] + .28f*level;
        }
    }
    repaint();
}

void SpectrumDisplay::paint(juce::Graphics& g)
{
    const auto r = getLocalBounds().toFloat().reduced(11);
    const double rate = std::max(8000.0, processor.getSampleRate());
    g.setColour(rim.withAlpha(.5f));
    for (float hz : { 50.0f, 100.0f, 250.0f, 500.0f, 1000.0f, 2500.0f, 5000.0f, 10000.0f, 20000.0f })
    {
        const float p = juce::jlimit(0.0f,1.0f,std::log(hz/20.0f)/std::log(20000.0f/20.0f));
        const float x = r.getX()+r.getWidth()*p;
        g.drawVerticalLine(static_cast<int>(x),r.getY(),r.getBottom());
        if (hz==100.0f || hz==1000.0f || hz==10000.0f)
            smallText(g,juce::String(hz>=1000.0f?hz/1000.0f:hz,0)+(hz>=1000?"k":""),
                      {x+2,r.getBottom()-14,42,13},9.5f,subtle);
    }
    for (int t=1;t<4;++t) g.drawHorizontalLine(static_cast<int>(r.getY()+t*r.getHeight()/4),r.getX(),r.getRight());
    auto draw = [&](const std::array<float,512>& mags, juce::Colour colour)
    {
        juce::Path p;
        bool first = true;
        for (int pixel=0; pixel<=static_cast<int>(r.getWidth()); pixel+=2)
        {
            const float f = 20.0f*std::pow(1000.0f,static_cast<float>(pixel)/r.getWidth());
            const int bin = juce::jlimit(1,511,static_cast<int>(f*1024.0/rate));
            const float level = mags[bin];
            const float x = r.getX()+pixel, y = r.getBottom()-level*r.getHeight();
            if (first) {p.startNewSubPath(x,y); first=false;} else p.lineTo(x,y);
        }
        g.setColour(colour.withAlpha(.18f));g.strokePath(p,juce::PathStrokeType(4.0f));
        g.setColour(colour);g.strokePath(p,juce::PathStrokeType(1.7f));
    };
    draw(magsInput, subtle);
    draw(magsOutput, neon);
}

SkellSpectreAudioProcessorEditor::Control* SkellSpectreAudioProcessorEditor::addControl(
    const juce::String& id, const juce::String& title, bool linear)
{
    auto c = std::make_unique<Control>();
    c->slider = std::make_unique<juce::Slider>();
    c->name = std::make_unique<juce::Label>();
    c->linear = linear;
    c->slider->setSliderStyle(linear ? juce::Slider::LinearHorizontal : juce::Slider::RotaryHorizontalVerticalDrag);
    c->slider->setTextBoxStyle(linear ? juce::Slider::TextBoxRight : juce::Slider::TextBoxBelow, false,
                               linear ? 55 : 80, linear ? 20 : 27);
    c->slider->setRotaryParameters(juce::degreesToRadians(220.0f),
                                   juce::degreesToRadians(500.0f), true);
    double defaultValue = 0.0;
    if (id == "mix" || id.endsWith("mix")) defaultValue = 100.0;
    if (id == "cross1") defaultValue = 120.0;
    if (id == "cross2") defaultValue = 1000.0;
    if (id == "cross3") defaultValue = 5500.0;
    c->slider->setDoubleClickReturnValue(true, defaultValue);
    c->name->setText(title, juce::dontSendNotification);
    c->name->setJustificationType(juce::Justification::centred);
    c->name->setColour(juce::Label::textColourId, subtle);
    addAndMakeVisible(*c->slider);
    addAndMakeVisible(*c->name);
    auto* p = c.get();
    controls.push_back(std::move(c));
    sliderAttachments.push_back(std::make_unique<SliderAttachment>(processor.parameters, id, *p->slider));
    return p;
}

void SkellSpectreAudioProcessorEditor::setParam(const juce::String& id, float v)
{
    if (auto* p = processor.parameters.getParameter(id))
    {
        p->beginChangeGesture();
        p->setValueNotifyingHost(p->convertTo0to1(v));
        p->endChangeGesture();
    }
}

SkellSpectreAudioProcessorEditor::SkellSpectreAudioProcessorEditor(SkellSpectreAudioProcessor& p)
    : AudioProcessorEditor(&p), processor(p), skull(p), spectrum(p)
{
    setLookAndFeel(&look);
    setOpaque(true);
    addAndMakeVisible(skull);
    addAndMakeVisible(spectrum);
    const juce::String labels[] = { "DRIVE", "PUNCH", "STEREO", "DIFFUSE" };
    const juce::String ids[] = { "drive", "punch", "stereo", "diffuse" };
    for (int b=0;b<4;++b) macro[b] = addControl(ids[b],labels[b]);
    masters[0] = addControl("mix","MIX",true);
    masters[1] = addControl("input","INPUT",true);
    masters[2] = addControl("output","OUTPUT",true);
    for (int c=0;c<3;++c)
        crossover[c] = addControl("cross"+juce::String(c+1), "CROSSOVER "+juce::String(c+1),true);
    for (int b=0;b<4;++b)
    {
        const auto prefix = "b"+juce::String(b+1);
        for (int k=0;k<4;++k) strips[b][k] = addControl(prefix+ids[k],labels[k]);
        strips[b][4] = addControl(prefix+"gain","BAND GAIN",true);
        strips[b][5] = addControl(prefix+"mix","PARALLEL MIX",true);
        for (int j=0;j<2;++j)
        {
            auto btn = std::make_unique<juce::TextButton>(j==0?"MUTE":"SOLO");
            btn->setClickingTogglesState(true);
            addAndMakeVisible(*btn);
            buttonAttachments.push_back(std::make_unique<ButtonAttachment>(processor.parameters,
                prefix + (j==0?"mute":"solo"),*btn));
            bandSwitches[b][j] = std::move(btn);
        }
    }
    for (auto* btn : { &basicButton, &expertButton, &prev, &next, &limiter, &autoGain, &hq })
        addAndMakeVisible(*btn);
    for (auto* btn : { &limiter, &autoGain, &hq }) btn->setClickingTogglesState(true);
    buttonAttachments.push_back(std::make_unique<ButtonAttachment>(processor.parameters,"limiter",limiter));
    buttonAttachments.push_back(std::make_unique<ButtonAttachment>(processor.parameters,"autogain",autoGain));
    buttonAttachments.push_back(std::make_unique<ButtonAttachment>(processor.parameters,"hq",hq));

    for (auto* cb : { &preset, &slope, &driveType, &widthType }) addAndMakeVisible(*cb);
    for (int i=0;i<SkellSpectreAudioProcessor::presetNames().size();++i)
        preset.addItem(SkellSpectreAudioProcessor::presetNames()[i],i+1);
    preset.setSelectedId(processor.getPresetIndex()+1,juce::dontSendNotification);
    preset.onChange = [this] { if(preset.getSelectedId()>0) processor.loadPreset(preset.getSelectedId()-1); };
    prev.onClick = [this]
    {
        const int n = preset.getNumItems();
        preset.setSelectedId((preset.getSelectedId()+n-2)%n+1);
    };
    next.onClick = [this]
    {
        const int n = preset.getNumItems();
        preset.setSelectedId(preset.getSelectedId()%n+1);
    };
    slope.addItemList({"SMOOTH", "FIRM", "STEEP", "BRICKWALL"},1);
    driveType.addItemList({"TUBE", "TAPE", "SHRED"},1);
    widthType.addItemList({"M/S", "SPREAD", "HAAS"},1);
    comboAttachments.push_back(std::make_unique<ComboAttachment>(processor.parameters,"slope",slope));
    comboAttachments.push_back(std::make_unique<ComboAttachment>(processor.parameters,"driveType",driveType));
    comboAttachments.push_back(std::make_unique<ComboAttachment>(processor.parameters,"widthType",widthType));
    basicButton.onClick = [this] { changeMode(true); };
    expertButton.onClick = [this] { changeMode(false); };
    footer.setText("SPECTRE 1.0  //  FOUR BAND TEXTURE ENGINE  //  ORIGINAL SKELL AUDIO DSP",juce::dontSendNotification);
    footer.setJustificationType(juce::Justification::centred);
    footer.setColour(juce::Label::textColourId,subtle);
    addAndMakeVisible(footer);
    previousBasic = processor.parameter("basic")>.5f;
    setModeStyle();
    setResizable(true,true);
    getConstrainer()->setFixedAspectRatio(1080.0/740.0);
    setResizeLimits(810,555,1728,1184);
    setSize(1080,740);
    startTimerHz(12);
}

SkellSpectreAudioProcessorEditor::~SkellSpectreAudioProcessorEditor()
{
    stopTimer();
    comboAttachments.clear();
    buttonAttachments.clear();
    sliderAttachments.clear();
    setLookAndFeel(nullptr);
}

void SkellSpectreAudioProcessorEditor::setModeStyle()
{
    const bool basic = processor.parameter("basic")>.5f;
    basicButton.setToggleState(basic,juce::dontSendNotification);
    expertButton.setToggleState(!basic,juce::dontSendNotification);
}

void SkellSpectreAudioProcessorEditor::changeMode(bool useBasic)
{
    if ((processor.parameter("basic") > .5f) == useBasic) return;
    if (!useBasic)
    {
        // Transfer Basic sound into independent Expert values, including current automation values.
        for (int b=0;b<4;++b)
        {
            const auto prefix="b"+juce::String(b+1);
            for (auto& id : { "drive", "punch", "stereo", "diffuse" })
            {
                float value = processor.parameter(id);
                if (b == 0 && juce::String(id) == "stereo") value = std::min(0.0f, value);
                setParam(prefix+id, value);
            }
            setParam(prefix+"mix",100.0f);
            setParam(prefix+"gain",0.0f);
            setParam(prefix+"solo",0.0f);
            setParam(prefix+"mute",0.0f);
        }
    }
    setParam("basic",useBasic?1.0f:0.0f);
    previousBasic=useBasic;
    setModeStyle();
    resized();
    repaint();
}

void SkellSpectreAudioProcessorEditor::timerCallback()
{
    const bool basic = processor.parameter("basic")>.5f;
    if (basic != previousBasic)
    {
        previousBasic=basic;
        setModeStyle();
        resized();
        repaint();
    }
    const int i=processor.getPresetIndex()+1;
    if (preset.getSelectedId()!=i) preset.setSelectedId(i,juce::dontSendNotification);
    // Meters and live crossover readings are painted by the parent editor.
    // Without this, the spectrum animates but the BASIC meters remain frozen.
    if (basic) repaint();
}

void SkellSpectreAudioProcessorEditor::place(Control* c, float x, float y, float w, float h, bool show)
{
    if (c==nullptr) return;
    c->name->setVisible(show); c->slider->setVisible(show);
    if (!show) return;
    c->name->setBounds(scaled(x,y,w,18,uiScale));
    c->name->setFont(juce::Font(juce::jmax(9.0f,12.0f*uiScale),juce::Font::bold));
    c->slider->setBounds(scaled(x,y+20,w,h-20,uiScale));
}

void SkellSpectreAudioProcessorEditor::resized()
{
    uiScale = static_cast<float>(getWidth())/1080.0f;
    basicButton.setBounds(scaled(850,20,90,32,uiScale));
    expertButton.setBounds(scaled(944,20,105,32,uiScale));
    prev.setBounds(scaled(322,20,30,32,uiScale));
    preset.setBounds(scaled(355,20,291,32,uiScale));
    next.setBounds(scaled(650,20,30,32,uiScale));
    for (int k=0;k<3;++k) place(masters[k],28+k*233,88,220,49,true);
    limiter.setBounds(scaled(746,90,90,35,uiScale));
    autoGain.setBounds(scaled(840,90,118,35,uiScale));
    hq.setBounds(scaled(962,90,86,35,uiScale));
    const bool basic = processor.parameter("basic") > .5f;
    skull.setVisible(basic);
    spectrum.setVisible(!basic);
    slope.setBounds(scaled(basic?51:41,basic?382:155,basic?210:154,34,uiScale));
    driveType.setBounds(scaled(51,436,210,34,uiScale));
    widthType.setBounds(scaled(820,436,208,34,uiScale));
    driveType.setVisible(basic);
    widthType.setVisible(basic);
    if (basic)
    {
        skull.setBounds(scaled(300,165,480,320,uiScale));
        for (int k=0;k<4;++k) place(macro[k],54+k*259,522,195,187,true);
    }
    else
    {
        spectrum.setBounds(scaled(225,147,825,150,uiScale));
        for (int k=0;k<4;++k) place(macro[k],0,0,0,0,false);
    }
    for (int k=0;k<3;++k) place(crossover[k], 207+k*279,300,259,45,!basic);
    for (int b=0;b<4;++b)
    {
        const float x=32+b*260;
        for (int k=0;k<4;++k)
            place(strips[b][k],x+(k%2)*112,365+(k/2)*104,106,100,!basic);
        place(strips[b][4],x+8,579,224,43,!basic);
        place(strips[b][5],x+8,627,224,43,!basic);
        for (int j=0;j<2;++j)
        {
            bandSwitches[b][j]->setBounds(scaled(x+15+j*110,683,94,30,uiScale));
            bandSwitches[b][j]->setVisible(!basic);
        }
    }
    footer.setBounds(scaled(25,717,1030,18,uiScale));
    footer.setFont(juce::Font(10.0f*uiScale));
}

void SkellSpectreAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(bg);
    const float s=static_cast<float>(getWidth())/1080.0f;
    juce::Graphics::ScopedSaveState save(g);
    g.addTransform(juce::AffineTransform::scale(s));
    g.setColour(panel);
    g.fillRect(0,0,1080,72);
    g.setColour(rim);
    g.drawHorizontalLine(72,20,1060);
    // Original SKELL monogram: vector skull + diagonal slash.
    g.setColour(neon.withAlpha(.20f));g.fillEllipse(28,12,46,46);
    g.setColour(bone);g.fillEllipse(35,16,32,28);
    g.fillRoundedRectangle(40,38,22,13,3);
    g.setColour(bg);g.fillEllipse(39,24,10,11);g.fillEllipse(53,24,10,11);
    g.setColour(neon);g.drawLine(57,15,46,46,3);
    g.setFont(juce::Font(28.0f,juce::Font::bold));g.drawText("SKELL",83,12,174,31,juce::Justification::centredLeft);
    smallText(g,"SPECTRE // MULTIBAND TEXTURE",85+0,43,234,16,10,bone.withAlpha(.78f));
    smallText(g,"PRESET",354,5,180,15,10,subtle);
    outlinedBox(g,{20,83,1040,57},12);
    g.setColour(neon.withAlpha(.09f));
    g.fillRect(21,141,1038,2);
    const bool basic = processor.parameter("basic")>.5f;
    if (basic)
    {
        outlinedBox(g,{21,154,1038,343},15);
        outlinedBox(g,{37,168,238,314},12);
        outlinedBox(g,{800,168,244,314},12);
        smallText(g,"CROSSOVER CHARACTER",52,183,200,19,11,neon);
        smallText(g,"SPLIT",51,346,105,20,11,bone);
        const char* names[] = { "SUB", "LOW MID", "HIGH MID", "AIR" };
        const float frequencies[] = {
            processor.parameter("cross1"), processor.parameter("cross2"), processor.parameter("cross3")
        };
        for (int i=0;i<4;++i)
        {
            g.setColour(neon.withAlpha(.9f));g.fillRoundedRectangle(52,218+i*27,5,11,2);
            smallText(g,names[i],65,212+i*27,140,23,11,bone);
            if(i<3) smallText(g,juce::String(static_cast<int>(frequencies[i]))+" Hz",180,212+i*27,65,23,10,subtle,juce::Justification::centredRight);
        }
        smallText(g,"SATURATION TYPE",52,411,180,18,11,neon);
        smallText(g,"STEREO ENGINE",819,411,180,18,11,neon);
        smallText(g,"SKULL REACTOR",822,187,182,20,12,neon);
        const float in=processor.inputPeak.load(std::memory_order_relaxed), out=processor.outputPeak.load(std::memory_order_relaxed);
        auto meter=[&g](int y,const juce::String& name,float peak)
        {
            smallText(g,name,819, y,95,20,11,bone);
            g.setColour(rim);g.fillRoundedRectangle(820.0f,static_cast<float>(y+27),203,9,4);
            g.setColour(neon);g.fillRoundedRectangle(821.0f,static_cast<float>(y+28),
                201.0f*juce::jlimit(0.0f,1.0f,peak),7,3);
            smallText(g, juce::String(juce::Decibels::gainToDecibels(std::max(peak,.00001f)),-1)+" dB",
                      925, y,98,20,11,subtle,juce::Justification::centredRight);
        };
        meter(230,"INPUT PEAK",in);
        meter(304,"OUTPUT PEAK",out);
        smallText(g,"PHASE // BONES // MOTION",818,371,220,16,10,subtle);
        g.setColour(neon.withAlpha(.1f));g.fillRoundedRectangle(21,504,1038,205,14);
        outlinedBox(g,{21,504,1038,205},14);
        smallText(g,"FOUR SHAPERS  //  BASIC MACRO",37,509,340,13,11,neon);
    }
    else
    {
        outlinedBox(g,{22,151,1035,146},13);
        smallText(g,"EXPERT",38,166,163,20,13,neon);
        smallText(g,"INPUT / OUTPUT FFT",38,188,177,20,10,bone);
        smallText(g,"PRE",41,251,45,20,10,subtle);
        smallText(g,"POST",88,251,70,20,10,neon);
        outlinedBox(g,{21,299,1038,49},9);
        smallText(g,"CROSSOVER // Hz",26,308,130,22,10,subtle);
        for (int b=0;b<4;++b)
        {
            const int x=21+b*260;
            outlinedBox(g,{static_cast<float>(x),356,248,355},13);
            const char* bandNames[] = {"01 / SUB","02 / LOW MID","03 / HIGH MID","04 / AIR"};
            smallText(g,bandNames[b],x+14,360,215,19,12,neon);
            g.setColour(neon.withAlpha(.10f));g.fillRect(x+12,387,224,1);
        }
    }
    smallText(g,"SKELL AUDIO",24,717,145,18,10,subtle);
}
