#pragma once

#include <JuceHeader.h>

class AmbientFoggerAudioProcessor final : public juce::AudioProcessor
{
public:
    AmbientFoggerAudioProcessor();
    ~AmbientFoggerAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 2.0; }

    int getNumPrograms() override { return 8; }
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState& getAPVTS() { return apvts; }
    void setDroneEnabled (bool enabled);
    void randomizeTasteful();
    void loadInit();

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

private:
    struct Preset
    {
        juce::String name;
        float tone, air, motion, drift, dirt, delay, space, output;
    };

    std::array<Preset, 8> presets;
    int currentPreset = 0;

    juce::AudioProcessorValueTreeState apvts;

    juce::dsp::StateVariableTPTFilter<float> filterL, filterR;
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> delayL{ 48000 }, delayR{ 48000 };
    juce::dsp::Reverb reverb;
    juce::dsp::Chorus<float> chorus;

    juce::LinearSmoothedValue<float> smTone, smAir, smMotion, smDrift, smDirt, smDelay, smSpace, smOutput;
    juce::Random random;
    double sr = 44100.0;

    float phase1 = 0.0f, phase2 = 0.0f, phaseSub = 0.0f;
    float lpNoise = 0.0f;
    float drift1 = 0.0f, drift2 = 0.0f;

    bool droneEnabled = false;
    int droneNote = 48;
    float lastDelayOutL = 0.0f, lastDelayOutR = 0.0f;

    float oscSample (float& phase, float freq, float morph);
    void updateSmoothedTargets();
    void applyPreset (const Preset& p);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AmbientFoggerAudioProcessor)
};
