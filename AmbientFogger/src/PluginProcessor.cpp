#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
float midiToHz (int note) { return 440.0f * std::pow (2.0f, (note - 69) / 12.0f); }
float softClip (float x) { return std::tanh (x); }
}

AmbientFoggerAudioProcessor::AmbientFoggerAudioProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createParameterLayout()),
      presets {{{"Mist Floor", 0.35f, 0.22f, 0.28f, 0.30f, 0.12f, 0.20f, 0.45f, 0.70f},
                {"Rusted Air", 0.50f, 0.45f, 0.20f, 0.35f, 0.18f, 0.15f, 0.40f, 0.68f},
                {"Deep Tunnel", 0.22f, 0.15f, 0.40f, 0.45f, 0.14f, 0.28f, 0.50f, 0.72f},
                {"Hollow Pad", 0.40f, 0.30f, 0.35f, 0.30f, 0.10f, 0.24f, 0.52f, 0.70f},
                {"Fog Chord", 0.28f, 0.34f, 0.25f, 0.40f, 0.15f, 0.18f, 0.56f, 0.68f},
                {"Broken Cloud", 0.55f, 0.36f, 0.42f, 0.48f, 0.20f, 0.22f, 0.44f, 0.64f},
                {"Soft Dub Bloom", 0.30f, 0.26f, 0.24f, 0.32f, 0.12f, 0.35f, 0.62f, 0.72f},
                {"Night Rail", 0.46f, 0.20f, 0.38f, 0.50f, 0.16f, 0.26f, 0.50f, 0.66f}}}
{
    loadInit();
    applyPreset (presets[(size_t) currentPreset]);
}

juce::AudioProcessorValueTreeState::ParameterLayout AmbientFoggerAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
    auto norm = juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f);

    for (auto pair : { std::pair{"tone", "TONE"}, {"air", "AIR"}, {"motion", "MOTION"}, {"drift", "DRIFT"},
                       {"dirt", "DIRT"}, {"delay", "DELAY"}, {"space", "SPACE"}, {"output", "OUTPUT"} })
        params.push_back (std::make_unique<juce::AudioParameterFloat> (pair.first, pair.second, norm, 0.4f));

    params.push_back (std::make_unique<juce::AudioParameterBool> ("drone", "DRONE", false));
    params.push_back (std::make_unique<juce::AudioParameterInt> ("root", "ROOT", 36, 60, 48));
    params.push_back (std::make_unique<juce::AudioParameterInt> ("oct", "OCTAVE", -2, 2, 0));

    return { params.begin(), params.end() };
}

void AmbientFoggerAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    sr = sampleRate;
    juce::dsp::ProcessSpec spec{ sampleRate, (juce::uint32) samplesPerBlock, 2 };
    filterL.prepare (spec);
    filterR.prepare (spec);
    filterL.setType (juce::dsp::StateVariableTPTFilterType::lowpass);
    filterR.setType (juce::dsp::StateVariableTPTFilterType::lowpass);

    chorus.prepare (spec);
    chorus.setRate (0.09f);
    chorus.setDepth (0.15f);
    chorus.setMix (0.12f);

    reverb.reset();
    delayL.reset();
    delayR.reset();

    for (auto* sm : { &smTone, &smAir, &smMotion, &smDrift, &smDirt, &smDelay, &smSpace, &smOutput })
    {
        sm->reset (sampleRate, 0.05);
        sm->setCurrentAndTargetValue (0.5f);
    }
}

void AmbientFoggerAudioProcessor::releaseResources() {}

bool AmbientFoggerAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

float AmbientFoggerAudioProcessor::oscSample (float& phase, float freq, float morph)
{
    phase += freq / (float) sr;
    if (phase >= 1.0f)
        phase -= 1.0f;

    auto sine = std::sin (juce::MathConstants<float>::twoPi * phase);
    auto tri = 4.0f * std::abs (phase - 0.5f) - 1.0f;
    auto saw = 2.0f * phase - 1.0f;

    if (morph < 0.5f)
        return juce::jmap (morph * 2.0f, sine, tri);
    return juce::jmap ((morph - 0.5f) * 2.0f, tri, saw);
}

void AmbientFoggerAudioProcessor::updateSmoothedTargets()
{
    smTone.setTargetValue (*apvts.getRawParameterValue ("tone"));
    smAir.setTargetValue (*apvts.getRawParameterValue ("air"));
    smMotion.setTargetValue (*apvts.getRawParameterValue ("motion"));
    smDrift.setTargetValue (*apvts.getRawParameterValue ("drift"));
    smDirt.setTargetValue (*apvts.getRawParameterValue ("dirt"));
    smDelay.setTargetValue (*apvts.getRawParameterValue ("delay"));
    smSpace.setTargetValue (*apvts.getRawParameterValue ("space"));
    smOutput.setTargetValue (*apvts.getRawParameterValue ("output"));
}

void AmbientFoggerAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    updateSmoothedTargets();

    auto root = (int) *apvts.getRawParameterValue ("root");
    auto oct = (int) *apvts.getRawParameterValue ("oct");
    droneEnabled = *apvts.getRawParameterValue ("drone") > 0.5f;
    droneNote = root + (12 * oct);

    int activeNote = droneNote;
    for (const auto metadata : midi)
        if (metadata.getMessage().isNoteOn())
            activeNote = metadata.getMessage().getNoteNumber();

    auto* left = buffer.getWritePointer (0);
    auto* right = buffer.getWritePointer (1);

    auto baseHz = midiToHz (activeNote);

    for (int i = 0; i < buffer.getNumSamples(); ++i)
    {
        auto tone = smTone.getNextValue();
        auto air = smAir.getNextValue();
        auto motion = smMotion.getNextValue();
        auto drift = smDrift.getNextValue();
        auto dirt = smDirt.getNextValue();
        auto delayAmt = smDelay.getNextValue();
        auto space = smSpace.getNextValue();
        auto out = smOutput.getNextValue();

        drift1 += (random.nextFloat() - 0.5f) * 0.00002f * drift;
        drift2 += (random.nextFloat() - 0.5f) * 0.00002f * drift;
        drift1 = juce::jlimit (-0.02f, 0.02f, drift1);
        drift2 = juce::jlimit (-0.02f, 0.02f, drift2);

        auto o1 = oscSample (phase1, baseHz * (1.0f + drift1), tone);
        auto o2 = oscSample (phase2, baseHz * 1.002f * (1.0f + drift2), 1.0f - tone * 0.7f);
        auto sub = oscSample (phaseSub, baseHz * 0.5f, 0.1f) * 0.25f;

        auto noise = (random.nextFloat() * 2.0f - 1.0f) * air * 0.15f;
        lpNoise = (0.99f - air * 0.2f) * lpNoise + (0.01f + air * 0.2f) * noise;

        float mono = (o1 * 0.5f + o2 * 0.45f + sub + lpNoise) * 0.7f;

        auto cutoff = juce::jmap (tone, 180.0f, 4200.0f) + motion * 400.0f * std::sin ((float) i * 0.0008f);
        filterL.setCutoffFrequency (cutoff);
        filterR.setCutoffFrequency (cutoff * 1.02f);
        filterL.setResonance (0.3f + air * 0.5f);
        filterR.setResonance (0.3f + air * 0.5f);

        float l = filterL.processSample (mono);
        float r = filterR.processSample (mono);

        l = softClip (l * (1.0f + dirt * 1.5f));
        r = softClip (r * (1.0f + dirt * 1.5f));

        auto delaySamples = (int) juce::jmap (delayAmt, 0.0f, 1.0f, 0.0f, (float) (sr * 0.75));
        delayL.pushSample (0, l + lastDelayOutL * 0.4f);
        delayR.pushSample (0, r + lastDelayOutR * 0.4f);
        lastDelayOutL = delayL.popSample (0, delaySamples);
        lastDelayOutR = delayR.popSample (0, delaySamples + 33);

        l += lastDelayOutL * delayAmt * 0.35f;
        r += lastDelayOutR * delayAmt * 0.35f;

        left[i] = l;
        right[i] = r;
    }

    juce::dsp::AudioBlock<float> block (buffer);
    juce::dsp::ProcessContextReplacing<float> ctx (block);
    chorus.setDepth (0.08f + smMotion.getTargetValue() * 0.22f);
    chorus.process (ctx);

    juce::dsp::Reverb::Parameters rv;
    rv.roomSize = 0.35f + smSpace.getTargetValue() * 0.6f;
    rv.damping = 0.6f;
    rv.wetLevel = smSpace.getTargetValue() * 0.35f;
    rv.dryLevel = 1.0f - rv.wetLevel * 0.4f;
    rv.width = 0.95f;
    reverb.setParameters (rv);
    reverb.processStereo (left, right, buffer.getNumSamples());

    auto gain = juce::Decibels::decibelsToGain (juce::jmap (smOutput.getTargetValue(), -24.0f, -1.0f));
    buffer.applyGain (gain);
}

int AmbientFoggerAudioProcessor::getCurrentProgram() { return currentPreset; }

void AmbientFoggerAudioProcessor::applyPreset (const Preset& p)
{
    *apvts.getRawParameterValue ("tone") = p.tone;
    *apvts.getRawParameterValue ("air") = p.air;
    *apvts.getRawParameterValue ("motion") = p.motion;
    *apvts.getRawParameterValue ("drift") = p.drift;
    *apvts.getRawParameterValue ("dirt") = p.dirt;
    *apvts.getRawParameterValue ("delay") = p.delay;
    *apvts.getRawParameterValue ("space") = p.space;
    *apvts.getRawParameterValue ("output") = p.output;
}

void AmbientFoggerAudioProcessor::setCurrentProgram (int index)
{
    currentPreset = juce::jlimit (0, (int) presets.size() - 1, index);
    applyPreset (presets[(size_t) currentPreset]);
}

const juce::String AmbientFoggerAudioProcessor::getProgramName (int index)
{
    if (juce::isPositiveAndBelow (index, (int) presets.size()))
        return presets[(size_t) index].name;
    return "";
}

void AmbientFoggerAudioProcessor::setDroneEnabled (bool enabled)
{
    *apvts.getRawParameterValue ("drone") = enabled ? 1.0f : 0.0f;
}

void AmbientFoggerAudioProcessor::randomizeTasteful()
{
    auto gentle = [this] (float center, float spread)
    {
        return juce::jlimit (0.05f, 0.95f, center + (random.nextFloat() - 0.5f) * spread);
    };

    *apvts.getRawParameterValue ("tone") = gentle (0.4f, 0.3f);
    *apvts.getRawParameterValue ("air") = gentle (0.28f, 0.35f);
    *apvts.getRawParameterValue ("motion") = gentle (0.32f, 0.25f);
    *apvts.getRawParameterValue ("drift") = gentle (0.38f, 0.25f);
    *apvts.getRawParameterValue ("dirt") = gentle (0.14f, 0.18f);
    *apvts.getRawParameterValue ("delay") = gentle (0.24f, 0.28f);
    *apvts.getRawParameterValue ("space") = gentle (0.5f, 0.3f);
    *apvts.getRawParameterValue ("output") = gentle (0.68f, 0.16f);
}

void AmbientFoggerAudioProcessor::loadInit()
{
    *apvts.getRawParameterValue ("tone") = 0.35f;
    *apvts.getRawParameterValue ("air") = 0.22f;
    *apvts.getRawParameterValue ("motion") = 0.28f;
    *apvts.getRawParameterValue ("drift") = 0.30f;
    *apvts.getRawParameterValue ("dirt") = 0.1f;
    *apvts.getRawParameterValue ("delay") = 0.20f;
    *apvts.getRawParameterValue ("space") = 0.45f;
    *apvts.getRawParameterValue ("output") = 0.70f;
}

void AmbientFoggerAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.state.createXml())
        copyXmlToBinary (*xml, destData);
}

void AmbientFoggerAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessorEditor* AmbientFoggerAudioProcessor::createEditor()
{
    return new AmbientFoggerAudioProcessorEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AmbientFoggerAudioProcessor();
}
