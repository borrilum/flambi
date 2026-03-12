#include "SynthVoice.h"

AtmosphereVoice::AtmosphereVoice(juce::AudioProcessorValueTreeState& apvts)
    : apvtsRef(apvts)
{
    voiceFilter.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
}

bool AtmosphereVoice::canPlaySound(juce::SynthesiserSound* sound)
{
    return dynamic_cast<AtmosphereSound*> (sound) != nullptr;
}

void AtmosphereVoice::prepare(double sampleRate, int samplesPerBlock, int outputChannels)
{
    juce::ignoreUnused(samplesPerBlock, outputChannels);
    currentSampleRate = sampleRate;

    juce::dsp::ProcessSpec spec { sampleRate, (juce::uint32) samplesPerBlock, (juce::uint32) outputChannels };
    osc1.prepare(spec);
    osc2.prepare(spec);
    subOsc.prepare(spec);
    voiceFilter.prepare(spec);

    toneSmoothed.reset(sampleRate, 0.05);
    airSmoothed.reset(sampleRate, 0.1);
    motionSmoothed.reset(sampleRate, 0.2);
    driftSmoothed.reset(sampleRate, 0.3);
    dirtSmoothed.reset(sampleRate, 0.1);

    envParams.attack = 0.4f;
    envParams.decay = 0.8f;
    envParams.sustain = 0.7f;
    envParams.release = 1.5f;
    ampEnv.setSampleRate(sampleRate);
    ampEnv.setParameters(envParams);
}

void AtmosphereVoice::startNote(int midiNoteNumber, float velocity, juce::SynthesiserSound*, int)
{
    noteHz = (float) juce::MidiMessage::getMidiNoteInHertz(midiNoteNumber);
    level = velocity;

    ampEnv.noteOn();
}

void AtmosphereVoice::stopNote(float, bool allowTailOff)
{
    if (allowTailOff)
        ampEnv.noteOff();
    else
    {
        ampEnv.reset();
        clearCurrentNote();
    }
}

inline float AtmosphereVoice::morphWave(float phase, float tone) const
{
    const float sine = std::sin(phase);
    const float tri = 2.0f * std::asin(std::sin(phase)) / juce::MathConstants<float>::pi;
    const float saw = (phase / juce::MathConstants<float>::pi) - 1.0f;

    if (tone < 0.5f)
    {
        const float t = tone * 2.0f;
        return juce::jmap(t, sine, tri);
    }

    const float t = (tone - 0.5f) * 2.0f;
    return juce::jmap(t, tri, saw);
}

inline float AtmosphereVoice::getNoiseSample(float air)
{
    const float n = random.nextFloat() * 2.0f - 1.0f;
    return n * air * 0.2f;
}

void AtmosphereVoice::renderNextBlock(juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples)
{
    if (!isVoiceActive())
        return;

    const float tone = apvtsRef.getRawParameterValue("tone")->load();
    const float air = apvtsRef.getRawParameterValue("air")->load();
    const float motion = apvtsRef.getRawParameterValue("motion")->load();
    const float drift = apvtsRef.getRawParameterValue("drift")->load();
    const float dirt = apvtsRef.getRawParameterValue("dirt")->load();

    toneSmoothed.setTargetValue(tone);
    airSmoothed.setTargetValue(air);
    motionSmoothed.setTargetValue(motion);
    driftSmoothed.setTargetValue(drift);
    dirtSmoothed.setTargetValue(dirt);

    const bool subOn = apvtsRef.getRawParameterValue("sub")->load() > 0.5f;
    const int filterMode = (int) apvtsRef.getRawParameterValue("filterMode")->load();
    voiceFilter.setType(filterMode == 0 ? juce::dsp::StateVariableTPTFilterType::lowpass
                                        : juce::dsp::StateVariableTPTFilterType::bandpass);

    for (int sample = 0; sample < numSamples; ++sample)
    {
        driftPhase += 0.00009f + motionSmoothed.getNextValue() * 0.0002f;
        const float driftMod = std::sin(driftPhase) * driftSmoothed.getNextValue() * 0.03f;

        const float freq1 = noteHz * (1.0f + driftMod);
        const float freq2 = noteHz * (1.0f + 0.01f + driftMod * 1.3f);
        const float subFreq = noteHz * 0.5f;

        const float phase1 = juce::MathConstants<float>::twoPi * freq1 * (float) sample / (float) currentSampleRate;
        const float phase2 = juce::MathConstants<float>::twoPi * freq2 * (float) sample / (float) currentSampleRate;

        const float localTone = toneSmoothed.getNextValue();
        float voice = morphWave(std::fmod(phase1, juce::MathConstants<float>::twoPi), localTone) * 0.45f
                    + morphWave(std::fmod(phase2, juce::MathConstants<float>::twoPi), localTone) * 0.4f;

        if (subOn)
            voice += std::sin(juce::MathConstants<float>::twoPi * subFreq * (float) sample / (float) currentSampleRate) * 0.25f;

        voice += getNoiseSample(airSmoothed.getNextValue());

        const float cutoff = juce::jmap(localTone, 200.0f, 5000.0f) + motionSmoothed.getCurrentValue() * 1400.0f;
        voiceFilter.setCutoffFrequency(juce::jlimit(120.0f, 8000.0f, cutoff));
        voiceFilter.setResonance(juce::jmap(motionSmoothed.getCurrentValue(), 0.5f, 1.2f));

        float filtered = voiceFilter.processSample(voice);

        const float satDrive = 1.0f + dirtSmoothed.getNextValue() * 4.0f;
        filtered = std::tanh(filtered * satDrive) * 0.65f;

        const float env = ampEnv.getNextSample();
        filtered *= env * level;

        for (int ch = 0; ch < outputBuffer.getNumChannels(); ++ch)
            outputBuffer.addSample(ch, startSample + sample, filtered);
    }

    if (!ampEnv.isActive())
        clearCurrentNote();
}
