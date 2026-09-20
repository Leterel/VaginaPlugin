#include "PluginProcessor.h"

VaginaPluginAudioProcessor::VaginaPluginAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "Parameters", createParameterLayout())
{
}

VaginaPluginAudioProcessor::~VaginaPluginAudioProcessor() = default;

juce::AudioProcessorValueTreeState::ParameterLayout VaginaPluginAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        paramIntensity, "Intensity", juce::NormalisableRange<float> (0.0f, 1.0f), 0.85f));

    layout.add (std::make_unique<juce::AudioParameterBool> (
        paramSquirt, "Squirt", false));

    return layout;
}

void VaginaPluginAudioProcessor::prepareToPlay (double newSampleRate, int)
{
    sampleRate = static_cast<float> (newSampleRate);

    for (auto& b : bursts)
        b.active = false;
}

void VaginaPluginAudioProcessor::releaseResources() {}

bool VaginaPluginAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo();
}

void VaginaPluginAudioProcessor::fireSquirt (float intensity)
{
    for (auto& b : bursts)
    {
        if (! b.active)
        {
            b.active = true;
            b.env = 0.6f + intensity * 0.4f;
            b.decay = 1.0f - (0.008f + (1.0f - intensity) * 0.012f);
            b.hp = 0.0f;
            b.bp = 0.0f;
            b.freq = 6000.0f + intensity * 4000.0f;
            b.freqDecay = 0.997f - intensity * 0.002f;
            b.microBurstsLeft = 3 + static_cast<int> (intensity * 5.0f);
            b.microTimer = 0.0f;
            return;
        }
    }
}

float VaginaPluginAudioProcessor::processBurst (SquirtBurst& b, float intensity)
{
    if (! b.active)
        return 0.0f;

    const float noise = noiseDist (rng);

    // high-pass
    const float hpCoeff = 0.92f;
    b.hp = hpCoeff * b.hp + (1.0f - hpCoeff) * noise;

    // bandpass around sliding freq
    const float bpCoeff = juce::jlimit (0.02f, 0.35f, b.freq / (sampleRate * 0.5f));
    b.bp += bpCoeff * (b.hp - b.bp);

    float out = b.bp * b.env * (0.5f + intensity);

    // micro-splatter re-triggers
    b.microTimer += 1.0f;
    if (b.microTimer > sampleRate * 0.018f && b.microBurstsLeft > 0)
    {
        b.microTimer = 0.0f;
        --b.microBurstsLeft;
        b.env = juce::jmax (b.env, 0.25f + intensity * 0.35f);
        b.freq = 4000.0f + noiseDist (rng) * 2000.0f;
    }

    b.freq *= b.freqDecay;
    b.env *= b.decay;

    if (b.env < 0.002f)
        b.active = false;

    return out;
}

void VaginaPluginAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();

    const float intensity = apvts.getRawParameterValue (paramIntensity)->load();
    const bool squirtBtn  = apvts.getRawParameterValue (paramSquirt)->load() > 0.5f;

    if (squirtBtn || squirtQueued)
    {
        squirtQueued = false;
        apvts.getParameter (paramSquirt)->setValueNotifyingHost (0.0f);
        fireSquirt (intensity);
    }

    for (const auto metadata : midi)
    {
        if (metadata.getMessage().isNoteOn())
            fireSquirt (intensity);
    }

    const int numChannels = buffer.getNumChannels();
    const int numSamples  = buffer.getNumSamples();

    for (int i = 0; i < numSamples; ++i)
    {
        float s = 0.0f;
        for (auto& b : bursts)
            s += processBurst (b, intensity);

        s = std::tanh (s * 3.5f);

        for (int ch = 0; ch < numChannels; ++ch)
            buffer.setSample (ch, i, s);
    }
}

juce::AudioProcessorEditor* VaginaPluginAudioProcessor::createEditor()
{
    return nullptr;
}

void VaginaPluginAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    const auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void VaginaPluginAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));
    if (xml != nullptr && xml->hasTagName (apvts.state.getType()))
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new VaginaPluginAudioProcessor();
}
