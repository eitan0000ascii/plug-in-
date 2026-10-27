#pragma once
#include "PluginProcessor.h"

// Top-down view of a head. Drag the dot to place the sound around the listener.
class HeadView : public juce::Component, private juce::Timer
{
public:
    explicit HeadView (juce::AudioProcessorValueTreeState& s);
    void paint (juce::Graphics& g) override;
    void mouseDown (const juce::MouseEvent& e) override;
    void mouseDrag (const juce::MouseEvent& e) override;
    void mouseUp (const juce::MouseEvent& e) override;

private:
    void timerCallback() override { repaint(); }
    void update (const juce::MouseEvent& e);
    void beginGestures (bool begin);

    juce::AudioProcessorValueTreeState& apvts;
};

class DepthHeadEditor : public juce::AudioProcessorEditor
{
public:
    explicit DepthHeadEditor (DepthHeadProcessor&);
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void applyPreset (int index);

    DepthHeadProcessor& proc;
    HeadView head;
    juce::OwnedArray<juce::TextButton> presetButtons;
    juce::Slider heightSlider, roomSlider;
    juce::Label heightLabel, roomLabel, titleLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> heightAtt, roomAtt;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DepthHeadEditor)
};
