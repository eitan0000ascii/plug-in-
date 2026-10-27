#include "PluginProcessor.h"
#include "PluginEditor.h"

using APVTS = juce::AudioProcessorValueTreeState;

APVTS::ParameterLayout DepthHeadProcessor::createLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;
    p.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "azimuth", 1 }, "Angle",
                                                              juce::NormalisableRange<float> (-180.0f, 180.0f, 0.1f), 0.0f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "distance", 1 }, "Distance",
                                                              juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.5f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "height", 1 }, "Height",
                                                              juce::NormalisableRange<float> (-1.0f, 1.0f, 0.001f), 0.0f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "room", 1 }, "Room",
                                                              juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.3f));
    return { p.begin(), p.end() };
}

DepthHeadProcessor::DepthHeadProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", createLayout())
{
}

bool DepthHeadProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainInputChannelSet()  == juce::AudioChannelSet::stereo()
        && layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void DepthHeadProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    sr = sampleRate;
    juce::dsp::ProcessSpec mono { sampleRate, (juce::uint32) samplesPerBlock, 1 };
    juce::dsp::ProcessSpec stereo { sampleRate, (juce::uint32) samplesPerBlock, 2 };

    for (int ch = 0; ch < 2; ++ch)
    {
        delay[ch].setMaximumDelayInSamples (512);
        delay[ch].prepare (mono);
        delay[ch].reset();
        shelf[ch].prepare (mono);
        shelf[ch].reset();
        headLP[ch].prepare (mono);
        headLP[ch].setType (juce::dsp::StateVariableTPTFilterType::lowpass);
        distLP[ch].prepare (mono);
        distLP[ch].setType (juce::dsp::StateVariableTPTFilterType::lowpass);
    }

    reverb.prepare (stereo);
    reverb.reset();

    for (auto* s : { &delayL, &delayR, &gainL, &gainR })
        s->reset (sampleRate, 0.03);

    lastHeight = 1000.0f; // force shelf update
}

void DepthHeadProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    if (buffer.getNumChannels() < 2)
        return;

    const float az     = apvts.getRawParameterValue ("azimuth")->load();
    const float d      = apvts.getRawParameterValue ("distance")->load();
    const float height = apvts.getRawParameterValue ("height")->load();
    const float room   = apvts.getRawParameterValue ("room")->load();

    const float rad = juce::degreesToRadians (az);
    const float s = std::sin (rad);   // +1 = fully right
    const float c = std::cos (rad);   // +1 = front
    const float meters = 0.6f + d * 7.0f;

    // Interaural time difference: the far ear hears it slightly later.
    const float maxItd = 0.00066f * (float) sr;
    delayL.setTargetValue (maxItd * juce::jmax (0.0f,  s) + 1.0f);
    delayR.setTargetValue (maxItd * juce::jmax (0.0f, -s) + 1.0f);

    // Interaural level difference + distance attenuation.
    const float distGain = std::pow (1.0f / juce::jmax (1.0f, meters), 0.6f);
    gainL.setTargetValue (juce::Decibels::decibelsToGain (-s * 6.0f) * distGain);
    gainR.setTargetValue (juce::Decibels::decibelsToGain ( s * 6.0f) * distGain);

    // Head shadow: far ear loses highs. Sounds from behind are slightly duller.
    const float back = juce::jmax (0.0f, -c);
    const float base = 20000.0f * (1.0f - 0.5f * back);
    const float farL = juce::jmap (juce::jmax (0.0f,  s), base, 2800.0f);
    const float farR = juce::jmap (juce::jmax (0.0f, -s), base, 2800.0f);
    headLP[0].setCutoffFrequency (juce::jlimit (500.0f, 20000.0f, farL));
    headLP[1].setCutoffFrequency (juce::jlimit (500.0f, 20000.0f, farR));

    // Distance: farther sources lose highs.
    const float distCut = 19000.0f - d * 13000.0f;
    distLP[0].setCutoffFrequency (distCut);
    distLP[1].setCutoffFrequency (distCut);

    // Height: high-shelf boost/cut around 6.5 kHz (pinna cue).
    if (std::abs (height - lastHeight) > 0.001f)
    {
        lastHeight = height;
        const float gain = juce::Decibels::decibelsToGain (height * 7.0f);
        for (int ch = 0; ch < 2; ++ch)
            shelf[ch].coefficients = juce::dsp::IIR::Coefficients<float>::makeHighShelf (sr, 6500.0f, 0.7f, gain);
    }

    auto* L = buffer.getWritePointer (0);
    auto* R = buffer.getWritePointer (1);

    for (int i = 0; i < n; ++i)
    {
        const float in = 0.5f * (L[i] + R[i]);

        delay[0].pushSample (0, in);
        delay[1].pushSample (0, in);
        float l = delay[0].popSample (0, delayL.getNextValue());
        float r = delay[1].popSample (0, delayR.getNextValue());

        l = shelf[0].processSample (l);
        r = shelf[1].processSample (r);
        l = headLP[0].processSample (0, l);
        r = headLP[1].processSample (0, r);
        l = distLP[0].processSample (0, l);
        r = distLP[1].processSample (0, r);

        L[i] = l * gainL.getNextValue();
        R[i] = r * gainR.getNextValue();
    }

    // Room: more reverb as the source moves away.
    const float mix = juce::jlimit (0.0f, 0.5f, 0.33f * room * (0.3f + 0.9f * d));
    juce::Reverb::Parameters rp;
    rp.roomSize = 0.7f;
    rp.damping = 0.5f;
    rp.width = 1.0f;
    rp.wetLevel = mix;
    rp.dryLevel = 0.5f; // juce reverb scales dry by 2
    rp.freezeMode = 0.0f;
    reverb.setParameters (rp);

    juce::dsp::AudioBlock<float> block (buffer);
    juce::dsp::ProcessContextReplacing<float> ctx (block);
    reverb.process (ctx);
}

juce::AudioProcessorEditor* DepthHeadProcessor::createEditor()
{
    return new DepthHeadEditor (*this);
}

void DepthHeadProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void DepthHeadProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new DepthHeadProcessor();
}
