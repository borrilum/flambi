#pragma once

#include <JuceHeader.h>
#include "SynthVoice.h"

class FlambiAtmosphereAudioProcessor : public juce::AudioProcessor
{
public:
    FlambiAtmosphereAudioProcessor();
    ~FlambiAtmosphereAudioProcessor() override = default;

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
    double getTailLengthSeconds() const override { return 3.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    using APVTS = juce::AudioProcessorValueTreeState;
    APVTS apvts;

    struct Preset
    {
        juce::String name;
        float tone, air, motion, drift, dirt, delay, space, output;
        bool mono = false;
        int rootNote = 36;
        int octave = 0;
        int filterMode = 0;
    };

    const std::vector<Preset>& getPresets() const { return presets; }
    void loadPreset(int index);
    void initPreset();
    void randomizePreset();

    void setDroneEnabled(bool enabled);
    bool isDroneEnabled() const;
    void setMonoEnabled(bool enabled);

    static APVTS::ParameterLayout createParameterLayout();

private:
    juce::Synthesiser synth;

    juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>> safetyHP;
    juce::dsp::Compressor<float> compressor;
    juce::dsp::Chorus<float> chorus;
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> delayLine { 22050 };
    juce::dsp::Reverb reverb;

    juce::AudioBuffer<float> delayBuffer;
    int delayWritePos = 0;

    bool droneWasEnabled = false;
    int droneCurrentNote = 36;

    std::vector<Preset> presets;

    void setupPresets();
    void refreshDroneState();
    void applyFxFromParameters(float sampleRate, int numSamples);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FlambiAtmosphereAudioProcessor)
};
