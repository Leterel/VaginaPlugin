#pragma once

#include <JuceHeader.h>
#include <random>

class VaginaPluginAudioProcessor : public juce::AudioProcessor
{
public:
    VaginaPluginAudioProcessor();
    ~VaginaPluginAudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return false; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;

    static constexpr const char* paramIntensity = "intensity";
    static constexpr const char* paramSquirt   = "squirt";

private:
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    struct SquirtBurst
    {
        bool active = false;
        float env = 0.0f;
        float decay = 0.99f;
        float hp = 0.0f;
        float bp = 0.0f;
        float freq = 3000.0f;
        float freqDecay = 0.9995f;
        int microBurstsLeft = 0;
        float microTimer = 0.0f;
    };

    std::array<SquirtBurst, 12> bursts {};
    std::mt19937 rng { 1337 };
    std::uniform_real_distribution<float> noiseDist { -1.0f, 1.0f };

    float sampleRate = 44100.0f;
    bool squirtQueued = false;

    void fireSquirt (float intensity);
    float processBurst (SquirtBurst& b, float intensity);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VaginaPluginAudioProcessor)
};
