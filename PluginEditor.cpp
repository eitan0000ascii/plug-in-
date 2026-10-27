#include "PluginEditor.h"
#include "Presets.h"

namespace
{
    const juce::Colour kBg    { 0xff12141c };
    const juce::Colour kPanel { 0xff1b1e2b };
    const juce::Colour kView  { 0xff151826 };
    const juce::Colour kLine  { 0xff2c3144 };
    const juce::Colour kAcc   { 0xff8b7cf6 };
    const juce::Colour kDot   { 0xffffb454 };
    const juce::Colour kMute  { 0xff8c92a8 };

    void setParam (juce::AudioProcessorValueTreeState& apvts, const juce::String& id, float value)
    {
        if (auto* p = apvts.getParameter (id))
        {
            p->beginChangeGesture();
            p->setValueNotifyingHost (p->convertTo0to1 (value));
            p->endChangeGesture();
        }
    }
}

//==============================================================================
HeadView::HeadView (juce::AudioProcessorValueTreeState& s) : apvts (s)
{
    startTimerHz (30);
}

void HeadView::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    g.setColour (kView);
    g.fillRoundedRectangle (b, 14.0f);

    const auto centre = b.getCentre();
    const float R = juce::jmin (b.getWidth(), b.getHeight()) * 0.46f;
    const float k = b.getWidth() / 420.0f;

    g.setColour (kLine);
    for (float f : { 0.25f, 0.5f, 0.75f, 1.0f })
        g.drawEllipse (centre.x - R * f, centre.y - R * f, 2 * R * f, 2 * R * f, 1.0f);
    g.drawLine (centre.x, centre.y - R, centre.x, centre.y + R);
    g.drawLine (centre.x - R, centre.y, centre.x + R, centre.y);

    // Head
    g.setColour (juce::Colour (0xff2a2f45));
    g.fillEllipse (centre.x - 32 * k, centre.y - 38 * k, 64 * k, 76 * k);
    g.setColour (kAcc);
    g.drawEllipse (centre.x - 32 * k, centre.y - 38 * k, 64 * k, 76 * k, 2.0f);

    // Nose
    juce::Path nose;
    nose.startNewSubPath (centre.x - 8 * k, centre.y - 36 * k);
    nose.lineTo (centre.x, centre.y - 50 * k);
    nose.lineTo (centre.x + 8 * k, centre.y - 36 * k);
    g.strokePath (nose, juce::PathStrokeType (2.0f));

    // Ears
    for (float side : { -1.0f, 1.0f })
    {
        const float ex = centre.x + side * 35 * k - 7 * k;
        g.setColour (juce::Colour (0xff3a4060));
        g.fillEllipse (ex, centre.y - 13 * k, 14 * k, 26 * k);
        g.setColour (kAcc);
        g.drawEllipse (ex, centre.y - 13 * k, 14 * k, 26 * k, 2.0f);
    }

    g.setColour (kMute);
    g.setFont (juce::FontOptions (11.0f * k));
    g.drawText ("Front", (int) (centre.x - 40), (int) (centre.y - R - 16 * k), 80, 14, juce::Justification::centred);
    g.drawText ("Back",  (int) (centre.x - 40), (int) (centre.y + R + 2 * k),  80, 14, juce::Justification::centred);

    // Source
    const float az     = apvts.getRawParameterValue ("azimuth")->load();
    const float d      = apvts.getRawParameterValue ("distance")->load();
    const float height = apvts.getRawParameterValue ("height")->load();

    const float a  = juce::degreesToRadians (az);
    const float rr = R * (0.2f + 0.8f * d);
    const float x  = centre.x + std::sin (a) * rr;
    const float y  = centre.y - std::cos (a) * rr;
    const float hr = (10.0f + height * 5.0f) * k;

    g.setColour (kDot.withAlpha (0.35f));
    g.drawLine (centre.x, centre.y, x, y, 1.5f);
    g.setColour (kDot.withAlpha (0.4f));
    g.drawEllipse (x - hr - 5 * k, y - hr - 5 * k, 2 * (hr + 5 * k), 2 * (hr + 5 * k), 3.0f);
    g.setColour (kDot);
    g.fillEllipse (x - hr, y - hr, 2 * hr, 2 * hr);

    g.setColour (kBg);
    g.setFont (juce::FontOptions (12.0f * k, juce::Font::bold));
    const juce::String glyph = height > 0.15f ? "+" : (height < -0.15f ? "-" : "");
    g.drawText (glyph, (int) (x - hr), (int) (y - hr), (int) (2 * hr), (int) (2 * hr), juce::Justification::centred);
}

void HeadView::beginGestures (bool begin)
{
    for (auto* id : { "azimuth", "distance" })
        if (auto* p = apvts.getParameter (id))
        {
            if (begin) p->beginChangeGesture();
            else       p->endChangeGesture();
        }
}

void HeadView::mouseDown (const juce::MouseEvent& e) { beginGestures (true); update (e); }
void HeadView::mouseDrag (const juce::MouseEvent& e) { update (e); }
void HeadView::mouseUp   (const juce::MouseEvent&)   { beginGestures (false); }

void HeadView::update (const juce::MouseEvent& e)
{
    const auto b = getLocalBounds().toFloat();
    const auto centre = b.getCentre();
    const float R = juce::jmin (b.getWidth(), b.getHeight()) * 0.46f;
    const float dx = e.position.x - centre.x;
    const float dy = e.position.y - centre.y;

    const float az = juce::radiansToDegrees (std::atan2 (dx, -dy));
    const float d  = juce::jlimit (0.0f, 1.0f, (std::sqrt (dx * dx + dy * dy) / R - 0.2f) / 0.8f);

    if (auto* pa = apvts.getParameter ("azimuth"))  pa->setValueNotifyingHost (pa->convertTo0to1 (az));
    if (auto* pd = apvts.getParameter ("distance")) pd->setValueNotifyingHost (pd->convertTo0to1 (d));
}

//==============================================================================
DepthHeadEditor::DepthHeadEditor (DepthHeadProcessor& p)
    : AudioProcessorEditor (&p), proc (p), head (p.apvts)
{
    addAndMakeVisible (head);

    titleLabel.setText ("DepthHead", juce::dontSendNotification);
    titleLabel.setFont (juce::FontOptions (22.0f, juce::Font::bold));
    titleLabel.setColour (juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible (titleLabel);

    int idx = 0;
    for (const auto& preset : kPresets)
    {
        auto* b = presetButtons.add (new juce::TextButton (preset.name));
        b->setColour (juce::TextButton::buttonColourId, juce::Colour (0xff242839));
        b->setColour (juce::TextButton::textColourOffId, juce::Colours::white);
        const int i = idx++;
        b->onClick = [this, i] { applyPreset (i); };
        addAndMakeVisible (b);
    }

    auto setupSlider = [this] (juce::Slider& s, juce::Label& l, const juce::String& text)
    {
        s.setSliderStyle (juce::Slider::LinearHorizontal);
        s.setTextBoxStyle (juce::Slider::TextBoxRight, false, 56, 20);
        s.setColour (juce::Slider::thumbColourId, kAcc);
        s.setColour (juce::Slider::trackColourId, kAcc.withAlpha (0.6f));
        s.setColour (juce::Slider::backgroundColourId, kLine);
        s.setColour (juce::Slider::textBoxTextColourId, juce::Colours::white);
        s.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        addAndMakeVisible (s);
        l.setText (text, juce::dontSendNotification);
        l.setColour (juce::Label::textColourId, kMute);
        addAndMakeVisible (l);
    };
    setupSlider (heightSlider, heightLabel, "Height");
    setupSlider (roomSlider, roomLabel, "Room");

    heightAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, "height", heightSlider);
    roomAtt   = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, "room", roomSlider);

    setSize (720, 460);
}

void DepthHeadEditor::paint (juce::Graphics& g)
{
    g.fillAll (kPanel);
}

void DepthHeadEditor::resized()
{
    head.setBounds (20, 20, 420, 420);

    auto col = juce::Rectangle<int> (460, 20, 240, 420);
    titleLabel.setBounds (col.removeFromTop (36));
    col.removeFromTop (12);

    // Preset grid: 3 x 2
    const int bw = (col.getWidth() - 16) / 3;
    auto grid = col.removeFromTop (88);
    for (int i = 0; i < presetButtons.size(); ++i)
    {
        const int r = i / 3, c = i % 3;
        presetButtons[i]->setBounds (grid.getX() + c * (bw + 8), grid.getY() + r * 44, bw, 36);
    }

    col.removeFromTop (24);
    heightLabel.setBounds (col.removeFromTop (20));
    heightSlider.setBounds (col.removeFromTop (30));
    col.removeFromTop (16);
    roomLabel.setBounds (col.removeFromTop (20));
    roomSlider.setBounds (col.removeFromTop (30));
}

void DepthHeadEditor::applyPreset (int index)
{
    const auto& p = kPresets[index];
    setParam (proc.apvts, "azimuth",  p.az);
    setParam (proc.apvts, "distance", p.dist);
    setParam (proc.apvts, "height",   p.height);
    setParam (proc.apvts, "room",     p.room);
}
