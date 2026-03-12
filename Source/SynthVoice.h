#pragma once

#include <JuceHeader.h>

class AtmosphereSound : public juce::SynthesiserSound
{
public:
    bool appliesToNote (int) override { return true; }
    bool appliesToChannel (int) override { return true; }
};

class AtmosphereVoice : public juce::SynthesiserVoice
{
public:
    AtmosphereVoice(juce::AudioProcessorValueTreeState& apvts);

    bool canPlaySound (juce::SynthesiserSound* sound) override;
    void startNote (int midiNoteNumber, float velocity, juce::SynthesiserSound*, int pitchWheelPosition) override;
    void stopNote (float velocity, bool allowTailOff) override;
    void pitchWheelMoved (int) override {}
    void controllerMoved (int, int) override {}

    void renderNextBlock (juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples) override;

    void prepare(double sampleRate, int samplesPerBlock, int outputChannels);

private:
    juce::AudioProcessorValueTreeState& apvtsRef;

    juce::ADSR ampEnv;
    juce::ADSR::Parameters envParams;

    double currentSampleRate = 44100.0;
    float noteHz = 440.0f;
    float level = 0.0f;

    juce::dsp::Oscillator<float> osc1 { [](float x) { return std::sin(x); } };
    juce::dsp::Oscillator<float> osc2 { [](float x) { return std::sin(x); } };
    juce::dsp::Oscillator<float> subOsc { [](float x) { return std::sin(x); } };

    juce::dsp::StateVariableTPTFilter<float> voiceFilter;

    juce::Random random;
    float driftPhase = 0.0f;

    juce::SmoothedValue<float> toneSmoothed, airSmoothed, motionSmoothed, driftSmoothed, dirtSmoothed;

    inline float morphWave(float phase, float tone) const;
    inline float getNoiseSample(float air);
};
