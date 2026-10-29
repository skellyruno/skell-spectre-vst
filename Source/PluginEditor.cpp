#include "PluginEditor.h"
#include <algorithm>
#include <cmath>

namespace
{
const auto bg = juce::Colour::fromRGB(8, 9, 14);
const auto panel = juce::Colour::fromRGB(19, 20, 28);
const auto rim = juce::Colour::fromRGB(47, 45, 61);
const auto violet = juce::Colour::fromRGB(177, 128, 255);
const auto bone = juce::Colour::fromRGB(219, 217, 217);
const auto subtle = juce::Colour::fromRGB(126, 126, 148);
const auto active = juce::Colour::fromRGB(193, 165, 255);

juce::Rectangle<int> scaled(float x, float y, float w, float h, float s)
{
    return { juce::roundToInt(x*s), juce::roundToInt(y*s),
             juce::roundToInt(w*s), juce::roundToInt(h*s) };
}
void text(juce::Graphics& g, const juce::String& str, float x, float y, float w, float h,
          float size = 11, juce::Colour colour = subtle,
          juce::Justification how = juce::Justification::centredLeft)
{
    g.setColour(colour);
    g.setFont(juce::Font(juce::FontOptions(size, juce::Font::plain)));
    g.drawFittedText(str, { juce::roundToInt(x), juce::roundToInt(y),
                            juce::roundToInt(w), juce::roundToInt(h) }, how, 1);
}
void frame(juce::Graphics& g, juce::Rectangle<float> r, float corner = 7.0f)
{
    g.setColour(panel);
    g.fillRoundedRectangle(r, corner);
    g.setColour(rim.withAlpha(.85f));
    g.drawRoundedRectangle(r.reduced(.45f), corner, 1);
}
void boneStroke(juce::Graphics& g, juce::Point<float> a, juce::Point<float> b, float thick)
{
    g.setColour(juce::Colour::fromRGB(59, 57, 68));
    g.drawLine({a,b}, thick + 5.0f);
    g.setColour(juce::Colour::fromRGB(197, 193, 187));
    g.drawLine({a,b}, thick);
    g.setColour(juce::Colour::fromRGB(250, 237, 210).withAlpha(.55f));
    g.drawLine({a,b}, thick*.28f);
    g.setColour(violet.withAlpha(.20f));
    g.fillEllipse(b.x - thick*.5f, b.y - thick*.5f, thick, thick);
}
}

SkellLookAndFeel::SkellLookAndFeel()
{
    setColour(juce::Slider::thumbColourId, violet);
    setColour(juce::Slider::rotarySliderFillColourId, violet);
    setColour(juce::Slider::rotarySliderOutlineColourId, rim);
    setColour(juce::Slider::trackColourId, violet);
    setColour(juce::Slider::backgroundColourId, rim);
    setColour(juce::Slider::textBoxTextColourId, bone);
    setColour(juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour(juce::ComboBox::textColourId, bone);
    setColour(juce::ComboBox::backgroundColourId, panel);
    setColour(juce::ComboBox::outlineColourId, rim);
    setColour(juce::ComboBox::arrowColourId, violet);
    setColour(juce::PopupMenu::backgroundColourId, panel);
    setColour(juce::PopupMenu::textColourId, bone);
    setColour(juce::PopupMenu::highlightedBackgroundColourId, violet.withAlpha(.27f));
    setColour(juce::TextButton::textColourOffId, bone);
    setColour(juce::TextButton::textColourOnId, active);
    setColour(juce::Label::textColourId, bone);
}

void SkellLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int w, int h,
    float pos, float start, float end, juce::Slider& slider)
{
    const float size = static_cast<float>(juce::jmin(w, h));
    const float cx = x + w*.5f, cy = y + h*.5f;
    const bool compact = size < 62.0f;
    const float outer = size*(compact ? .46f : .475f);
    const float knob = outer*(compact ? .70f : .74f);
    const float a = start + pos*(end-start);
    if (!compact)
    {
        g.setColour(juce::Colours::black.withAlpha(.78f));
        g.fillEllipse(cx-outer-4,cy-outer-2,(outer+4)*2,(outer+4)*2);
        g.setColour(juce::Colour::fromRGB(55, 54, 64));
        for (int i=0;i<38;++i)
        {
            const float angle = juce::MathConstants<float>::twoPi * i / 38.0f;
            const float r0 = outer*.87f, r1 = outer*.95f;
            g.drawLine(cx+std::sin(angle)*r0,cy-std::cos(angle)*r0,
                       cx+std::sin(angle)*r1,cy-std::cos(angle)*r1,1.0f);
        }
    }
    juce::Path track, progress;
    track.addCentredArc(cx, cy, outer*.96f, outer*.96f, 0, start, end, true);
    progress.addCentredArc(cx, cy, outer*.96f, outer*.96f, 0, start, a, true);
    g.setColour(rim.brighter(.15f));
    g.strokePath(track,juce::PathStrokeType(compact?2.1f:3.0f));
    g.setColour(violet.withAlpha(.25f));
    g.strokePath(progress,juce::PathStrokeType(compact?5.0f:8.0f));
    g.setColour(active);
    g.strokePath(progress,juce::PathStrokeType(compact?2.2f:3.2f));
    juce::ColourGradient disk(juce::Colour::fromRGB(55,54,65),cx-knob,cy-knob,
                              juce::Colour::fromRGB(10,11,17),cx+knob,cy+knob,false);
    disk.addColour(.38,juce::Colour::fromRGB(24,25,32));
    g.setGradientFill(disk);
    g.fillEllipse(cx-knob,cy-knob,knob*2,knob*2);
    g.setColour(juce::Colour::fromRGB(80,78,89));
    g.drawEllipse(cx-knob,cy-knob,knob*2,knob*2,compact?1:2);
    g.setColour(juce::Colours::white.withAlpha(.065f));
    g.drawEllipse(cx-knob*.83f,cy-knob*.83f,knob*1.66f,knob*1.66f,1.0f);
    const auto lineLen = knob*.72f;
    g.setColour(juce::Colours::black.withAlpha(.7f));
    g.drawLine(cx+1,cy+1,cx+1+std::sin(a)*lineLen,cy+1-std::cos(a)*lineLen,compact?3:5);
    g.setColour(bone);
    g.drawLine(cx,cy,cx+std::sin(a)*lineLen,cy-std::cos(a)*lineLen,compact?1.5f:2.8f);
    if (!compact)
    {
        g.setColour(violet.withAlpha(.25f));
        g.fillEllipse(cx-3,cy-3,6,6);
    }
    juce::ignoreUnused(slider);
}

void SkellLookAndFeel::drawLinearSlider(juce::Graphics& g, int x, int y, int w, int h,
    float sliderPos, float, float, juce::Slider::SliderStyle, juce::Slider&)
{
    const float cy = y + h*.5f;
    g.setColour(juce::Colour::fromRGB(31,30,44));
    g.fillRoundedRectangle(static_cast<float>(x+3),cy-2,static_cast<float>(w-6),4,2);
    g.setColour(violet);
    g.fillRoundedRectangle(static_cast<float>(x+3),cy-2,
            std::max(0.0f,sliderPos-(x+3.0f)),4,2);
    g.setColour(bone);
    g.fillRoundedRectangle(sliderPos-3,cy-8,6,16,2);
}

void SkellLookAndFeel::drawButtonBackground(juce::Graphics& g, juce::Button& b,
    const juce::Colour&, bool highlighted, bool pressed)
{
    auto r = b.getLocalBounds().toFloat().reduced(.8f);
    if (b.getName() == "SKELL_POWER")
    {
        const float d = std::min(r.getWidth(),r.getHeight())-5.0f;
        const float cx=r.getCentreX(),cy=r.getCentreY();
        const auto colour = b.getToggleState() ? subtle : active; // toggle means bypass
        g.setColour(juce::Colours::black);g.fillEllipse(cx-d*.5f,cy-d*.5f,d,d);
        juce::Path ring;
        ring.addCentredArc(cx,cy,d*.36f,d*.36f,0,-2.48f,2.48f,true);
        g.setColour(colour.withAlpha(.27f));g.strokePath(ring,juce::PathStrokeType(6));
        g.setColour(colour);g.strokePath(ring,juce::PathStrokeType(2.2f));
        g.drawLine(cx,cy-d*.40f,cx,cy-d*.05f,2.2f);
        return;
    }
    const bool on=b.getToggleState();
    g.setColour(on ? juce::Colour::fromRGB(38,30,53) : (highlighted ? panel.brighter(.12f) : panel));
    g.fillRoundedRectangle(r,7);
    g.setColour(on ? violet : rim.brighter(.12f));
    g.drawRoundedRectangle(r,7,on?1.4f:1.0f);
    if (pressed) {g.setColour(violet.withAlpha(.12f));g.fillRoundedRectangle(r,7);}
}
void SkellLookAndFeel::drawComboBox(juce::Graphics& g, int w, int h, bool,
    int, int, int, int, juce::ComboBox&)
{
    frame(g,{.5f,.5f,static_cast<float>(w-1),static_cast<float>(h-1)},5);
    juce::Path a;
    const float cx=w-12.0f,cy=h*.5f;
    a.startNewSubPath(cx-3,cy-2);a.lineTo(cx,cy+1);a.lineTo(cx+3,cy-2);
    g.setColour(violet);g.strokePath(a,juce::PathStrokeType(1.2f));
}
juce::Font SkellLookAndFeel::getComboBoxFont(juce::ComboBox&)
{
    return juce::Font(juce::FontOptions(11.0f,juce::Font::plain));
}

void BandRail::paint(juce::Graphics& g)
{
    const auto step=getHeight()/5.0f;
    const char* labels[5]={"ALL","A","B","C","D"};
    for (int i=0;i<5;++i)
    {
        const float y=(i+.5f)*step;
        g.setColour(i==selection ? violet : subtle.withAlpha(.8f));
        g.drawEllipse(1,y-4.1f,8.2f,8.2f,1.15f);
        if (i==selection) g.fillEllipse(2.7f,y-2.4f,4.8f,4.8f);
        text(g, labels[i],12,y-7,getWidth()-12.0f,14,9.1f,
             i==selection ? bone : subtle);
    }
}
void BandRail::mouseUp(const juce::MouseEvent& e)
{
    if (!e.mouseWasClicked()) return;
    const int newIndex=juce::jlimit(0,4,static_cast<int>(e.position.y*5.0f/getHeight()));
    if (newIndex!=selection) {setSelection(newIndex);if(onSelect) onSelect(newIndex);}
}

void SplitStrip::paint(juce::Graphics& g)
{
    const float cell=getWidth()/4.0f;
    frame(g,{0,0,static_cast<float>(getWidth()),static_cast<float>(getHeight()-19)},9);
    const char* modes[]={"SMOOTH","FIRM","STEEP","BRICKWALL"};
    for (int i=0;i<4;++i)
    {
        const float x=i*cell;
        if (i==selected)
        {
            g.setColour(violet.withAlpha(.19f));
            g.fillRoundedRectangle(x+4,4,cell-8,getHeight()-27.0f,6);
            g.setColour(violet);
            g.drawRoundedRectangle(x+4,4,cell-8,getHeight()-27.0f,6,1.35f);
        }
        const float midY=(getHeight()-19)*.5f;
        juce::Path p;
        p.startNewSubPath(x+cell*.18f,midY-8);
        if (i==0) p.cubicTo(x+cell*.43f,midY-4,x+cell*.57f,midY+4,x+cell*.82f,midY+8);
        else if (i==1) {p.lineTo(x+cell*.48f,midY-8);p.lineTo(x+cell*.70f,midY+8);p.lineTo(x+cell*.82f,midY+8);}
        else if (i==2) {p.lineTo(x+cell*.57f,midY-8);p.lineTo(x+cell*.69f,midY+8);p.lineTo(x+cell*.82f,midY+8);}
        else {p.lineTo(x+cell*.69f,midY-8);p.lineTo(x+cell*.69f,midY+8);p.lineTo(x+cell*.82f,midY+8);}
        g.setColour(i==selected?active:subtle);
        g.strokePath(p,juce::PathStrokeType(1.2f));
        text(g,modes[i],x,getHeight()-18,cell,16,9,i==selected?bone:subtle,
             juce::Justification::centred);
    }
}
void SplitStrip::mouseUp(const juce::MouseEvent& e)
{
    if (!e.mouseWasClicked()) return;
    const int v=juce::jlimit(0,3,static_cast<int>(e.position.x*4/getWidth()));
    setSelection(v);if(onSelect) onSelect(v);
}

SkullDisplay::SkullDisplay(SkellSpectreAudioProcessor& p) : processor(p)
{
    setInterceptsMouseClicks(false,false);
    startTimerHz(30);
}

void SkullDisplay::paint(juce::Graphics& g)
{
    const float energy=juce::jlimit(0.0f,1.0f,
        processor.outputPeak.load(std::memory_order_relaxed));
    shownPeak+=.16f*(energy-shownPeak);
    juce::Graphics::ScopedSaveState save(g);
    g.addTransform(juce::AffineTransform::scale(getWidth()/400.0f,getHeight()/478.0f));
    juce::ColourGradient wall(juce::Colour::fromRGB(35,34,49),200,0,
                              juce::Colour::fromRGB(6,9,18),200,478,false);
    wall.addColour(.55,juce::Colour::fromRGB(23,25,35));
    g.setGradientFill(wall);
    g.fillRect(0,0,400,478);
    g.setColour(juce::Colours::black.withAlpha(.5f));
    g.fillRect(5,5,390,10);
    g.setColour(violet.withAlpha(.15f));
    for (int i=0;i<13;++i) g.drawHorizontalLine(34+i*31,9,393);
    for (int i=0;i<21;++i)
    {
        const float x=11.0f+(i*137%376),y=27.0f+(i*67%427);
        g.setColour(i%3==0?violet.withAlpha(.17f):bone.withAlpha(.075f));
        g.drawLine(x,y,x+5+(i%4)*6,y+2,1);
    }
    // Laboratory electrical columns, with ceramic insulators.
    for (int side=0;side<2;++side)
    {
        const float x=side==0?33.0f:367.0f;
        const float flip=side==0?1.0f:-1.0f;
        g.setColour(juce::Colour::fromRGB(57,47,42));
        g.fillRoundedRectangle(x-10,58,20,362,4);
        g.setColour(juce::Colour::fromRGB(116,70,46));
        g.drawLine(x-7,61,x-7,410,3);
        g.setColour(juce::Colour::fromRGB(104,90,83));
        g.fillRoundedRectangle(x-15,54,30,13,3);
        for (int j=0;j<9;++j)
        {
            const float yy=84.0f+j*31.0f;
            g.setColour(juce::Colour::fromRGB(188,176,154));
            g.fillEllipse(x-16,yy,32,16);
            g.setColour(juce::Colour::fromRGB(97,92,89));
            g.drawEllipse(x-16,yy,32,16,1);
            g.setColour(juce::Colour::fromRGB(227,216,184).withAlpha(.65f));
            g.drawLine(x-9,yy+4,x+6,yy+4,1.5f);
        }
        g.setColour(juce::Colour::fromRGB(104,62,41));
        g.fillRoundedRectangle(x-18,411,36,34,3);
        g.setColour(violet.withAlpha(.20f+shownPeak*.45f));
        g.fillRoundedRectangle(x-10,414,20,7,2);
        juce::Path cable;
        cable.startNewSubPath(x,62);
        cable.cubicTo(x+flip*21,12,x+flip*75,37,x+flip*91,19);
        g.setColour(juce::Colour::fromRGB(160,91,54));
        g.strokePath(cable,juce::PathStrokeType(4.5f));
    }
    // Hanging electrode housings.
    for (int i=0;i<3;++i)
    {
        const float x=154.0f+i*45.0f;
        g.setColour(juce::Colour::fromRGB(114,94,91));
        g.fillRect(x, 0.0f, 5.0f, 23.0f + static_cast<float>(i) * 3.0f);
        g.setColour(violet.withAlpha(.4f));
        g.drawLine(x-2,22,x+7,22,1);
    }
    // Floor turntable and cast shadow.
    g.setColour(juce::Colours::black.withAlpha(.75f));
    g.fillEllipse(93,426,221,47);
    g.setColour(juce::Colour::fromRGB(56,51,61));
    g.fillEllipse(108,429,184,29);
    g.setColour(violet.withAlpha(.40f+shownPeak*.20f));
    g.drawEllipse(110,428,180,25,1.5f);
    for (int j=0;j<12;++j)
    {
        const float a=j*juce::MathConstants<float>::twoPi/12;
        g.setColour(juce::Colour::fromRGB(80,65,77));
        g.fillEllipse(200+87*std::cos(a)-2,440+11*std::sin(a)-2,4,4);
    }
    // Bone legs, feet and dark tattered coat tails.
    juce::Path coat;
    coat.startNewSubPath(152,179);coat.lineTo(164,302);coat.lineTo(144,378);
    coat.lineTo(165,370);coat.lineTo(196,320);coat.lineTo(204,320);
    coat.lineTo(235,370);coat.lineTo(256,376);coat.lineTo(238,303);coat.lineTo(250,179);
    coat.closeSubPath();
    g.setColour(juce::Colour::fromRGB(51,35,62));g.fillPath(coat);
    g.setColour(violet.withAlpha(.40f));g.strokePath(coat,juce::PathStrokeType(1.5f));
    boneStroke(g,{179,315},{171,370},13);boneStroke(g,{171,370},{167,429},11);
    boneStroke(g,{221,315},{229,370},13);boneStroke(g,{229,370},{233,429},11);
    g.setColour(juce::Colour::fromRGB(35,33,41));
    g.fillRoundedRectangle(140,423,39,17,5);
    g.fillRoundedRectangle(221,423,39,17,5);
    g.setColour(bone.withAlpha(.72f));g.drawLine(150,424,169,424,2);g.drawLine(231,424,250,424,2);
    // Spinal column, pelvis and ribs.
    boneStroke(g,{200,184},{200,302},8);
    juce::Path hip;
    hip.startNewSubPath(161,300);hip.quadraticTo(177,286,200,302);
    hip.quadraticTo(222,285,240,300);hip.lineTo(219,323);
    hip.quadraticTo(200,336,181,323);hip.closeSubPath();
    g.setColour(bone);g.fillPath(hip);
    g.setColour(bg);g.fillEllipse(186,306,28,17);
    for (int k=0;k<7;++k)
    {
        const float y=205.0f+k*13.5f,wide=49.0f-k*3.8f;
        juce::Path ribs;
        ribs.startNewSubPath(199,y);
        ribs.cubicTo(199-wide*.85f,y-10,199-wide*1.07f,y+9,199-wide*.66f,y+25);
        ribs.startNewSubPath(201,y);
        ribs.cubicTo(201+wide*.85f,y-10,201+wide*1.07f,y+9,201+wide*.66f,y+25);
        g.setColour(bone);
        g.strokePath(ribs,juce::PathStrokeType(5,juce::PathStrokeType::curved,juce::PathStrokeType::rounded));
        g.setColour(juce::Colour::fromRGB(109,90,116));
        g.strokePath(ribs,juce::PathStrokeType(.9f));
        g.setColour(bone);
        g.fillRoundedRectangle(193,y+3,14,5,2);
    }
    // Exposed bony arms, joint details and hand phalanges.
    const float motion=shownPeak*4.0f;
    boneStroke(g,{155,197},{125-motion,257},13);
    boneStroke(g,{125-motion,257},{118-motion,338},10);
    boneStroke(g,{245,197},{275+motion,257},13);
    boneStroke(g,{275+motion,257},{282+motion,338},10);
    for (int sgn : {-1,1})
    {
        const float x=200+sgn*(82+motion);
        boneStroke(g,{x,334},{x+sgn*3,352},5);
        for (int f=0;f<4;++f)
        {
            const float finger=x+sgn*(f-1.5f)*6;
            boneStroke(g,{finger,351},{finger+sgn*2,366+f*1.4f},2.2f);
        }
    }
    // Jacket is skeletal and open so ribs stay visible.
    juce::Path lapelL,lapelR;
    lapelL.startNewSubPath(154,183);lapelL.lineTo(183,178);lapelL.lineTo(190,202);
    lapelL.lineTo(177,228);lapelL.lineTo(183,295);lapelL.lineTo(159,290);lapelL.closeSubPath();
    lapelR.startNewSubPath(246,183);lapelR.lineTo(217,178);lapelR.lineTo(210,202);
    lapelR.lineTo(223,228);lapelR.lineTo(217,295);lapelR.lineTo(241,290);lapelR.closeSubPath();
    g.setColour(juce::Colour::fromRGB(81,48,94));g.fillPath(lapelL);g.fillPath(lapelR);
    g.setColour(violet.withAlpha(.36f));
    g.strokePath(lapelL,juce::PathStrokeType(1.3f));
    g.strokePath(lapelR,juce::PathStrokeType(1.3f));
    // Neck vertebrae.
    boneStroke(g,{200,169},{200,190},12);
    // A distinctive long, cracked skull (not a copy of the reference character).
    juce::Path skull;
    skull.startNewSubPath(172,115);
    skull.cubicTo(164,78,181,61,201,63);
    skull.cubicTo(228,60,241,91,228,127);
    skull.lineTo(224,150);skull.lineTo(212,174);skull.lineTo(188,174);
    skull.lineTo(176,150);skull.closeSubPath();
    g.setColour(violet.withAlpha(.12f+shownPeak*.12f));
    g.strokePath(skull,juce::PathStrokeType(14));
    juce::ColourGradient cranium(juce::Colour::fromRGB(242,236,216),172,77,
                                 juce::Colour::fromRGB(112,116,123),226,170,false);
    g.setGradientFill(cranium);g.fillPath(skull);
    g.setColour(juce::Colour::fromRGB(78,74,81));
    g.strokePath(skull,juce::PathStrokeType(2));
    g.setColour(bg);g.fillEllipse(181,119,19,20);g.fillEllipse(207,119,19,20);
    g.setColour(violet.withAlpha(.34f+shownPeak*.66f));
    g.fillEllipse(187,128,8,5);g.fillEllipse(213,128,8,5);
    juce::Path nose;
    nose.startNewSubPath(201,138);nose.lineTo(194,151);nose.lineTo(208,151);nose.closeSubPath();
    g.setColour(juce::Colour::fromRGB(28,27,37));g.fillPath(nose);
    g.fillRoundedRectangle(182,155,37,9,2);
    g.setColour(bone);
    for (int t=0;t<5;++t) g.fillRoundedRectangle(183.5f+t*7,155.5f,5,8,1);
    juce::Path crack;
    crack.startNewSubPath(206,64);crack.lineTo(200,87);crack.lineTo(210,102);
    crack.lineTo(198,113);
    g.setColour(juce::Colour::fromRGB(75,65,77));
    g.strokePath(crack,juce::PathStrokeType(2));
    // Reactive short electrical arcs, visually tied to processed output peak.
    if (shownPeak > .025f)
    {
        const float e=shownPeak;
        juce::Path arc;
        arc.startNewSubPath(85,221);arc.lineTo(100,210-e*10);arc.lineTo(93,240);
        arc.lineTo(112,230);arc.lineTo(125,247);
        arc.startNewSubPath(279,212);arc.lineTo(292,227);arc.lineTo(284,245);
        arc.lineTo(303,235);arc.lineTo(318,250);
        g.setColour(violet.withAlpha(juce::jmin(.75f,.20f+e*.6f)));
        g.strokePath(arc,juce::PathStrokeType(1.5f+e*1.8f));
    }
    // Frame and scene title.
    g.setColour(juce::Colour::fromRGB(58,53,75));
    g.drawRect(2,2,396,474,3);
    g.setColour(violet.withAlpha(.46f));g.drawLine(6,3,394,3,1);
    text(g,"S K E L L   //   R E A C T O R",17,450,366,18,9.0f,
         bone.withAlpha(.58f),juce::Justification::centred);
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
            text(g,juce::String(hz>=1000.0f?hz/1000.0f:hz,0)+(hz>=1000?"k":""),
                 x+2,r.getBottom()-14,42,13,9.5f,subtle);
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
    draw(magsOutput, violet);
}

SkellSpectreAudioProcessorEditor::Control* SkellSpectreAudioProcessorEditor::addControl(
    const juce::String& id, const juce::String& caption, bool linear, bool attach)
{
    auto c=std::make_unique<Control>();
    c->slider=std::make_unique<juce::Slider>();
    c->name=std::make_unique<juce::Label>();
    c->linear=linear;
    c->slider->setSliderStyle(linear ? juce::Slider::LinearHorizontal
                                     : juce::Slider::RotaryHorizontalVerticalDrag);
    c->slider->setTextBoxStyle(juce::Slider::NoTextBox,false,0,0);
    c->slider->setRotaryParameters(juce::degreesToRadians(225.0f),
                                   juce::degreesToRadians(495.0f),true);
    double def=0.0;
    if (id=="mix"||id.endsWith("mix")) def=100;
    if (id=="cross1") def=120;
    if (id=="cross2") def=1000;
    if (id=="cross3") def=5500;
    c->slider->setDoubleClickReturnValue(true,def);
    c->name->setText(caption,juce::dontSendNotification);
    c->name->setJustificationType(juce::Justification::centred);
    c->name->setColour(juce::Label::textColourId,bone);
    c->slider->setTooltip(caption+" — drag to change; double-click to reset");
    addAndMakeVisible(*c->slider);
    addAndMakeVisible(*c->name);
    auto* p=c.get();
    controls.push_back(std::move(c));
    if (attach)
        sliderAttachments.push_back(std::make_unique<SliderAttachment>(processor.parameters,id,*p->slider));
    return p;
}

void SkellSpectreAudioProcessorEditor::setParam(const juce::String& id,float v)
{
    if (auto* p=processor.parameters.getParameter(id))
    {
        p->beginChangeGesture();
        p->setValueNotifyingHost(p->convertTo0to1(v));
        p->endChangeGesture();
    }
}

void SkellSpectreAudioProcessorEditor::bindMacro(int k,int band)
{
    static const juce::String ids[] = {"drive","punch","stereo","diffuse"};
    rebindingMacro=true;
    macroAttachments[k].reset();
    rails[k].setSelection(band);
    const juce::String id=band==0 ? ids[k] : "b"+juce::String(band)+ids[k];
    macroAttachments[k]=std::make_unique<SliderAttachment>(processor.parameters,id,*macro[k]->slider);
    rebindingMacro=false;
    // ALL edits update every band in Expert processing, preserving the auditory result
    // when other effect knobs are targeting only a particular band.
    macro[k]->slider->onValueChange=[this,k]
    {
        if (rebindingMacro || rails[k].getSelection()!=0 ||
            processor.parameter("basic")>.5f) return;
        static const juce::String ids[]={"drive","punch","stereo","diffuse"};
        const float value=static_cast<float>(macro[k]->slider->getValue());
        for (int b=1;b<=4;++b)
            setParam("b"+juce::String(b)+ids[k],value);
    };
}

SkellSpectreAudioProcessorEditor::SkellSpectreAudioProcessorEditor(SkellSpectreAudioProcessor& p)
    : juce::AudioProcessorEditor(&p),processor(p),skull(p),spectrum(p)
{
    setLookAndFeel(&look);
    setOpaque(true);
    addAndMakeVisible(skull);
    addAndMakeVisible(spectrum);
    addAndMakeVisible(splitStrip);
    const juce::String ids[]={"drive","punch","stereo","diffuse"};
    const juce::String names[]={"DRIVE","PUNCH","STEREO","DIFFUSE"};
    for (int k=0;k<4;++k)
    {
        macro[k]=addControl(ids[k],names[k],false,false);
        addAndMakeVisible(rails[k]);
        rails[k].setTooltip("ALL = all bands, A-D = a single frequency band");
        rails[k].onSelect=[this,k](int index)
        {
            if (index>0 && processor.parameter("basic")>.5f)
                changeMode(false);
            bindMacro(k,index);
            bool all=true;
            for (const auto& rail:rails) if (rail.getSelection()!=0) all=false;
            if (all && processor.parameter("basic")<.5f)
                changeMode(true);
            repaint();
        };
    }
    masters[0]=addControl("mix","MIX");
    masters[1]=addControl("input","IN");
    masters[2]=addControl("output","OUT");
    for (int i=0;i<3;++i)
        crossover[i]=addControl("cross"+juce::String(i+1),"CROSS "+juce::String(i+1),true);
    for (int b=0;b<4;++b)
    {
        const auto prefix="b"+juce::String(b+1);
        for (int k=0;k<4;++k)
            strips[b][k]=addControl(prefix+ids[k],names[k]);
        strips[b][4]=addControl(prefix+"gain","GAIN",true);
        strips[b][5]=addControl(prefix+"mix","MIX",true);
        for (int j=0;j<2;++j)
        {
            auto btn=std::make_unique<juce::TextButton>(j==0?"MUTE":"SOLO");
            btn->setClickingTogglesState(true);
            addAndMakeVisible(*btn);
            buttonAttachments.push_back(std::make_unique<ButtonAttachment>(processor.parameters,
                prefix+(j==0?"mute":"solo"),*btn));
            bandSwitches[b][j]=std::move(btn);
        }
    }
    for (auto* b:{&modeToggle,&prev,&next,&limiter,&autoGain,&hq,&power,&slotA,&slotB,&copyAB})
        addAndMakeVisible(*b);
    for (auto* b:{&limiter,&autoGain,&hq,&power}) b->setClickingTogglesState(true);
    power.setName("SKELL_POWER");
    power.setTooltip("Bypass plugin (automatable)");
    buttonAttachments.push_back(std::make_unique<ButtonAttachment>(processor.parameters,"limiter",limiter));
    buttonAttachments.push_back(std::make_unique<ButtonAttachment>(processor.parameters,"autogain",autoGain));
    buttonAttachments.push_back(std::make_unique<ButtonAttachment>(processor.parameters,"hq",hq));
    buttonAttachments.push_back(std::make_unique<ButtonAttachment>(processor.parameters,"bypass",power));
    for (auto* box:{&preset,&driveType,&widthType}) addAndMakeVisible(*box);
    for (int i=0;i<SkellSpectreAudioProcessor::presetNames().size();++i)
        preset.addItem(SkellSpectreAudioProcessor::presetNames()[i],i+1);
    preset.setSelectedId(processor.getPresetIndex()+1,juce::dontSendNotification);
    preset.onChange=[this]
    {
        if (preset.getSelectedId()>0)
        {
            processor.loadPreset(preset.getSelectedId()-1);
            for (int k=0;k<4;++k) bindMacro(k,0);
            showingExpert=false;
            resized();repaint();
        }
    };
    prev.onClick=[this]
    {
        const int n=preset.getNumItems();
        if (n>0) preset.setSelectedId((preset.getSelectedId()+n-2)%n+1);
    };
    next.onClick=[this]
    {
        const int n=preset.getNumItems();
        if (n>0) preset.setSelectedId(preset.getSelectedId()%n+1);
    };
    driveType.addItemList({"TUBE","TAPE","SHRED"},1);
    widthType.addItemList({"MID / SIDE","SPREAD","HAAS"},1);
    comboAttachments.push_back(std::make_unique<ComboAttachment>(processor.parameters,"driveType",driveType));
    comboAttachments.push_back(std::make_unique<ComboAttachment>(processor.parameters,"widthType",widthType));
    splitStrip.onSelect=[this](int index) {setParam("slope",static_cast<float>(index));};
    modeToggle.onClick=[this] {showExpert(!showingExpert);};
    modeToggle.setTooltip("Toggle multiband Expert editing / character view");
    slotA.onClick=[this] {changeAB(0);};
    slotB.onClick=[this] {changeAB(1);};
    copyAB.onClick=[this] {copyAToB();};
    copyAB.setTooltip("Copy slot A to slot B");
    footer.setText("SKELL SPECTRE // ORIGINAL FOUR-BAND REACTOR",juce::dontSendNotification);
    footer.setJustificationType(juce::Justification::centred);
    footer.setColour(juce::Label::textColourId,subtle);
    addAndMakeVisible(footer);
    previousBasic=processor.parameter("basic")>.5f;
    for (int k=0;k<4;++k) bindMacro(k,0);
    snapshots[0]=processor.parameters.copyState();
    snapshots[1]=snapshots[0].createCopy();
    setModeStyle();
    setResizable(true,true);
    getConstrainer()->setFixedAspectRatio(960.0/540.0);
    setResizeLimits(704,396,1440,810);
    setSize(880,495); // matches the supplied screenshot's compact default scale
    startTimerHz(20);
}

SkellSpectreAudioProcessorEditor::~SkellSpectreAudioProcessorEditor()
{
    stopTimer();
    for (auto& attachment:macroAttachments) attachment.reset();
    comboAttachments.clear();
    buttonAttachments.clear();
    sliderAttachments.clear();
    setLookAndFeel(nullptr);
}

void SkellSpectreAudioProcessorEditor::changeMode(bool useBasic)
{
    const bool old=processor.parameter("basic")>.5f;
    if (old==useBasic) return;
    if (!useBasic)
    {
        // Copy macro sound into the four band parameters BEFORE enabling Expert DSP.
        for (int b=1;b<=4;++b)
        {
            const auto prefix="b"+juce::String(b);
            for (const auto* id:{"drive","punch","stereo","diffuse"})
            {
                float value=processor.parameter(id);
                if(b==1 && juce::String(id)=="stereo") value=std::min(0.0f,value);
                setParam(prefix+id,value);
            }
            setParam(prefix+"mix",100.0f);
            setParam(prefix+"gain",0.0f);
            setParam(prefix+"solo",0.0f);
            setParam(prefix+"mute",0.0f);
        }
    }
    setParam("basic",useBasic?1.0f:0.0f);
    previousBasic=useBasic;
}

void SkellSpectreAudioProcessorEditor::showExpert(bool show)
{
    if(show && processor.parameter("basic")>.5f) changeMode(false);
    showingExpert=show;
    setModeStyle();resized();repaint();
}
void SkellSpectreAudioProcessorEditor::setModeStyle()
{
    modeToggle.setToggleState(showingExpert,juce::dontSendNotification);
    slotA.setToggleState(currentAB==0,juce::dontSendNotification);
    slotB.setToggleState(currentAB==1,juce::dontSendNotification);
}
void SkellSpectreAudioProcessorEditor::changeAB(int index)
{
    if(index==currentAB) return;
    snapshots[currentAB]=processor.parameters.copyState();
    currentAB=index;
    processor.parameters.replaceState(snapshots[index].createCopy());
    const bool basic=processor.parameter("basic")>.5f;
    for (int k=0;k<4;++k)
        if(basic) bindMacro(k,0);
    if(basic) showingExpert=false;
    previousBasic=basic;
    setModeStyle();resized();repaint();
}
void SkellSpectreAudioProcessorEditor::copyAToB()
{
    if(currentAB==0) snapshots[0]=processor.parameters.copyState();
    snapshots[1]=snapshots[0].createCopy();
    if(currentAB==1) processor.parameters.replaceState(snapshots[1].createCopy());
    repaint();
}

void SkellSpectreAudioProcessorEditor::timerCallback()
{
    const bool basic=processor.parameter("basic")>.5f;
    if(basic!=previousBasic)
    {
        previousBasic=basic;
        if(basic)
        {
            for (int k=0;k<4;++k) bindMacro(k,0);
            showingExpert=false;
        }
        setModeStyle();resized();
    }
    const int index=processor.getPresetIndex()+1;
    if(preset.getSelectedId()!=index) preset.setSelectedId(index,juce::dontSendNotification);
    splitStrip.setSelection(static_cast<int>(processor.parameter("slope")));
    repaint();
}

void SkellSpectreAudioProcessorEditor::place(Control* c,float x,float y,float w,float h,
                                              bool show,bool header)
{
    if(c==nullptr) return;
    c->slider->setVisible(show);
    c->name->setVisible(show);
    if(!show) return;
    if(header)
    {
        c->name->setBounds(scaled(x,y,w,12,uiScale));
        c->name->setFont(juce::Font(juce::FontOptions(std::max(8.0f,9.8f*uiScale))));
        c->slider->setBounds(scaled(x+2,y+11,w-4,h-11,uiScale));
    }
    else if(c->linear)
    {
        c->name->setBounds(scaled(x,y,w*.40f,15,uiScale));
        c->name->setFont(juce::Font(juce::FontOptions(std::max(8.0f,9.2f*uiScale))));
        c->slider->setBounds(scaled(x+w*.35f,y,w*.65f,h,uiScale));
    }
    else
    {
        c->slider->setBounds(scaled(x,y,w,h-21,uiScale));
        c->name->setBounds(scaled(x,y+h-23,w,21,uiScale));
        c->name->setFont(juce::Font(juce::FontOptions(std::max(8.0f,10.2f*uiScale),juce::Font::bold)));
    }
}

void SkellSpectreAudioProcessorEditor::resized()
{
    uiScale=getWidth()/960.0f;
    prev.setBounds(scaled(365,11,22,24,uiScale));
    preset.setBounds(scaled(147,10,214,27,uiScale));
    next.setBounds(scaled(389,11,22,24,uiScale));
    slotA.setBounds(scaled(419,9,27,21,uiScale));
    slotB.setBounds(scaled(447,9,27,21,uiScale));
    copyAB.setBounds(scaled(419,33,55,18,uiScale));
    hq.setBounds(scaled(537,15,49,27,uiScale));
    limiter.setBounds(scaled(590,15,77,27,uiScale));
    autoGain.setBounds(scaled(671,15,92,27,uiScale));
    for(int k=0;k<3;++k)
        place(masters[k],768+k*39,2,37,54,true,true);
    power.setBounds(scaled(918,7,35,42,uiScale));
    modeToggle.setBounds(scaled(877,279,76,34,uiScale));
    modeToggle.setButtonText(showingExpert?"CHARACTER":"BANDS");
    footer.setVisible(!showingExpert);
    footer.setBounds(scaled(408,520,454,17,uiScale));
    footer.setFont(juce::Font(juce::FontOptions(9.1f*uiScale)));
    if(!showingExpert)
    {
        skull.setVisible(true);
        skull.setBounds(scaled(408,62,396,455,uiScale));
        spectrum.setVisible(false);
        splitStrip.setVisible(true);
        splitStrip.setBounds(scaled(57,103,298,67,uiScale));
        for (int k=0;k<4;++k)
        {
            const float x=k%2==0?77.0f:231.0f;
            const float y=k<2?178.0f:347.0f;
            place(macro[k],x,y,112,144,true);
            rails[k].setVisible(true);
            rails[k].setBounds(scaled(x-32,y+16,31,83,uiScale));
        }
        driveType.setVisible(false);
        widthType.setVisible(false);
        for (auto* c:crossover) place(c,0,0,0,0,false);
        for (auto& band:strips)
            for (auto* c:band) place(c,0,0,0,0,false);
        for (auto& bs:bandSwitches)
            for (auto& b:bs) b->setVisible(false);
    }
    else
    {
        skull.setVisible(false);
        spectrum.setVisible(true);
        spectrum.setBounds(scaled(171,81,686,110,uiScale));
        splitStrip.setVisible(true);
        splitStrip.setBounds(scaled(20,205,223,55,uiScale));
        for (int k=0;k<4;++k)
        {
            place(macro[k],0,0,0,0,false);
            rails[k].setVisible(false);
        }
        for (int k=0;k<3;++k)
            place(crossover[k],251+k*203,210,193,34,true);
        for (int b=0;b<4;++b)
        {
            const float x=16+b*211;
            for (int k=0;k<4;++k)
            {
                const float bx=x+10+(k%2)*98;
                const float by=300+(k/2)*77;
                place(strips[b][k],bx,by,90,75,true);
            }
            place(strips[b][4],x+13,455,184,22,true);
            place(strips[b][5],x+13,480,184,22,true);
            for (int j=0;j<2;++j)
            {
                bandSwitches[b][j]->setVisible(true);
                bandSwitches[b][j]->setBounds(scaled(x+15+j*96,506,88,25,uiScale));
            }
        }
        driveType.setVisible(true);
        widthType.setVisible(true);
        driveType.setBounds(scaled(18,132,131,27,uiScale));
        widthType.setBounds(scaled(18,166,131,27,uiScale));
    }
}

void SkellSpectreAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(bg);
    juce::Graphics::ScopedSaveState save(g);
    g.addTransform(juce::AffineTransform::scale(getWidth()/960.0f));
    // Header is the fixed anchor; no bitmap artwork or image assets.
    juce::ColourGradient head(juce::Colour::fromRGB(23,23,33),0,0,bg,0,59,false);
    g.setGradientFill(head);g.fillRect(0,0,960,59);
    g.setColour(juce::Colour::fromRGB(64,54,83));g.drawHorizontalLine(58,0,960);
    g.setColour(juce::Colour::fromRGB(19,18,25));g.fillRect(0,60,408,480);
    // Subtle grain lines keep the dark panel from appearing empty.
    for (int i=0;i<35;++i)
    {
        const float y=74.0f+i*13.0f;
        g.setColour(juce::Colours::white.withAlpha(i%3==0?.025f:.012f));
        g.drawLine(5,y,401,y+1,1);
    }
    g.setColour(juce::Colour::fromRGB(62,55,72));
    g.drawVerticalLine(407,60,540);
    g.drawVerticalLine(805,60,540);
    g.drawVerticalLine(864,60,540);
    // Original Skell skull badge in the top left.
    g.setColour(juce::Colours::black);g.fillEllipse(53,3,41,49);
    g.setColour(violet.withAlpha(.42f));g.drawEllipse(54,4,39,47,1.5f);
    g.setColour(bone);g.fillRoundedRectangle(65,13,18,22,7);
    g.fillRoundedRectangle(68,32,13,7,2);
    g.setColour(bg);g.fillEllipse(68,23,6,6);g.fillEllipse(76,23,6,6);
    g.setColour(violet);g.drawLine(74,10,79,20,1);g.drawLine(79,20,74,29,1);
    text(g,"SKELL",99,13,44,16,10,bone);
    text(g,"SPECTRE",99,29,44,14,7.7f,violet);
    // Header legend above A/B and macro meters.
    text(g,"PRESETS",148,0,85,9,7.3f,subtle);
    g.setColour(rim);g.drawVerticalLine(520,9,49);
    g.setColour(rim);g.drawVerticalLine(910,9,49);
    const bool expert=showingExpert;
    if(!expert)
    {
        text(g,"SPLIT",50,76,307,19,11.0f,bone,juce::Justification::centred);
        g.setColour(rim.withAlpha(.6f));g.drawHorizontalLine(173,19,390);
        for (int k=0;k<4;++k)
        {
            const float x=(k%2==0?133.0f:287.0f);
            const float y=(k<2?236.0f:405.0f);
            g.setColour(violet.withAlpha(.03f));g.fillEllipse(x-51,y-51,102,102);
            g.setColour(juce::Colour::fromRGB(67,60,81));
            for(int j=0;j<4;++j)
            {
                const float a=j*juce::MathConstants<float>::halfPi+.48f;
                g.fillEllipse(x+55*std::sin(a)-2,y+55*std::cos(a)-2,4,4);
            }
            const float v=static_cast<float>(macro[k]->slider->getValue());
            text(g,juce::String(v,1),x-34,y+88,68,17,9.3f,subtle,
                 juce::Justification::centred);
        }
        text(g,"BAND SELECT  //  ALL  A  B  C  D",45,520,342,15,8.5f,subtle);
        g.setColour(violet.withAlpha(.15f));g.drawHorizontalLine(530,11,395);
    }
    else
    {
        text(g,"MULTIBAND",22,75,132,20,12,bone);
        text(g,"EXPERT  /  FFT",22,97,135,15,9,violet);
        text(g,"SATURATION",22,116,131,14,8,subtle);
        text(g,"STEREO MODE",22,150,131,14,8,subtle);
        frame(g,{159,76,707,118},6);
        text(g,"SPLIT CHARACTER",20,190,193,15,10,violet);
        text(g,"CROSSOVER  FREQUENCY",255,193,481,15,9,subtle);
        for (int i=0;i<3;++i)
            text(g,juce::String(juce::roundToInt(processor.parameter("cross"+juce::String(i+1))))+" Hz",
                 264+i*203,248,177,12,8.4f,active,juce::Justification::centred);
        for (int b=0;b<4;++b)
        {
            const float x=16.0f+b*211.0f;
            frame(g,{x,267,201,267},9);
            g.setColour(violet.withAlpha(.10f));g.fillRoundedRectangle(x+4,271,193,25,6);
            const char* bands[]={"A / SUB","B / LOW MID","C / HIGH MID","D / AIR"};
            text(g,bands[b],x+11,274,180,18,11,bone);
            g.setColour(rim);g.drawHorizontalLine(448, x+10,x+191);
        }
    }
    // Thin meter/breakout panel adjacent to the central reactor scene.
    g.setColour(juce::Colour::fromRGB(13,14,22));g.fillRect(806,60,58,480);
    text(g,"IN",814,82,40,15,9,bone,juce::Justification::centred);
    text(g,"OUT",814,229,40,15,9,bone,juce::Justification::centred);
    const float in=juce::jlimit(0.0f,1.0f,processor.inputPeak.load(std::memory_order_relaxed));
    const float out=juce::jlimit(0.0f,1.0f,processor.outputPeak.load(std::memory_order_relaxed));
    for (int j=0;j<2;++j)
    {
        const float yy=j==0?104.0f:251.0f;
        const float level=j==0?in:out;
        g.setColour(juce::Colour::fromRGB(39,36,53));
        g.fillRoundedRectangle(831,yy,8,105,3);
        g.setColour(j==0?violet:active);
        const float filled=105.0f*level;
        if(filled>0.01f)g.fillRoundedRectangle(831,yy+105-filled,8,filled,2);
        text(g,juce::String(juce::Decibels::gainToDecibels(std::max(.00001f,level)),0)+" dB",
             806,yy+110,57,15,8,subtle,juce::Justification::centred);
    }
    // Right-mode rail: same proportions as the reference, fully independent artwork.
    g.setColour(juce::Colour::fromRGB(13,14,21));g.fillRect(865,60,95,480);
    g.setColour(rim);g.drawVerticalLine(865,61,539);
    g.setColour(violet.withAlpha(expert?.8f:.55f));
    g.drawRoundedRectangle(886,232,56,29,7,1.2f);
    g.drawLine(892,246,902,239,1.2f);
    g.drawLine(902,239,911,250,1.2f);
    g.drawLine(911,250,921,239,1.2f);
    g.drawLine(921,239,931,246,1.2f);
    text(g,expert?"SKELETON":"MULTIBAND",871,318,84,16,10,bone,
         juce::Justification::centred);
    text(g,expert?"VIEW":"MODE",871,333,84,16,10,bone,
         juce::Justification::centred);
    text(g,"AUDIO REACTIVE",870,482,86,13,8.3f,subtle,juce::Justification::centred);
    text(g,"SKELL AUDIO  1.2",866,515,94,17,8.3f,subtle,
         juce::Justification::centred);
}
