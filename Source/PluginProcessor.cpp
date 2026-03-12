#include "PluginProcessor.h"
#include "PluginEditor.h"

FlambiAtmosphereAudioProcessor::FlambiAtmosphereAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
       apvts(*this, nullptr, "PARAMETERS", createParameterLayout())
#endif
{
    setupPresets();

    for (int i = 0; i < 8; ++i)
        synth.addVoice(new AtmosphereVoice(apvts));
    synth.addSound(new AtmosphereSound());
    initPreset();
}

void FlambiAtmosphereAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    for (int i = 0; i < synth.getNumVoices(); ++i)
        if (auto* v = dynamic_cast<AtmosphereVoice*>(synth.getVoice(i)))
            v->prepare(sampleRate, samplesPerBlock, getTotalNumOutputChannels());

    juce::dsp::ProcessSpec spec { sampleRate, (juce::uint32) samplesPerBlock, (juce::uint32) getTotalNumOutputChannels() };
    safetyHP.prepare(spec);
    *safetyHP.state = *juce::dsp::IIR::Coefficients<float>::makeHighPass(sampleRate, 20.0f);

    chorus.prepare(spec);
    delayLine.prepare(spec);
    compressor.prepare(spec);
    compressor.setThreshold(-6.0f);
    compressor.setRatio(2.0f);

    reverb.prepare(spec);

    delayBuffer.setSize(getTotalNumOutputChannels(), (int) sampleRate * 2);
    delayBuffer.clear();
    delayWritePos = 0;
}

void FlambiAtmosphereAudioProcessor::releaseResources() {}

bool FlambiAtmosphereAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::mono()
        || layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void FlambiAtmosphereAudioProcessor::applyFxFromParameters(float sampleRate, int numSamples)
{
    juce::ignoreUnused(numSamples);
    const float motion = apvts.getRawParameterValue("motion")->load();
    const float delay = apvts.getRawParameterValue("delay")->load();
    const float space = apvts.getRawParameterValue("space")->load();

    chorus.setRate(0.05f + motion * 0.6f);
    chorus.setDepth(0.1f + motion * 0.35f);
    chorus.setMix(0.2f + motion * 0.3f);

    reverb.setParameters({
        0.25f + space * 0.6f,
        0.2f + space * 0.6f,
        0.45f + space * 0.4f,
        0.3f + space * 0.6f,
        0.4f,
        0.0f,
        1.0f,
        0.0f
    });

    delayLine.setMaximumDelayInSamples((int) sampleRate);
    delayLine.setDelay(juce::jmap(delay, 20.0f, sampleRate * 0.75f));
}

void FlambiAtmosphereAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();

    refreshDroneState();
    setMonoEnabled(apvts.getRawParameterValue("mono")->load() > 0.5f);
    synth.setCurrentPlaybackSampleRate(getSampleRate());
    synth.renderNextBlock(buffer, midiMessages, 0, buffer.getNumSamples());

    auto block = juce::dsp::AudioBlock<float>(buffer);
    juce::dsp::ProcessContextReplacing<float> context(block);

    applyFxFromParameters((float) getSampleRate(), buffer.getNumSamples());

    chorus.process(context);

    const float delayAmt = apvts.getRawParameterValue("delay")->load();
    const float fb = 0.2f + delayAmt * 0.55f;
    const float wet = 0.1f + delayAmt * 0.45f;

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        auto* channelData = buffer.getWritePointer(ch);
        auto* delayData = delayBuffer.getWritePointer(ch);
        const int delaySize = delayBuffer.getNumSamples();

        int localWrite = delayWritePos;
        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            const float delayed = delayLine.popSample(ch);
            delayLine.pushSample(ch, channelData[i] + delayed * fb);
            channelData[i] = channelData[i] * (1.0f - wet) + delayed * wet;

            delayData[localWrite] = channelData[i];
            localWrite = (localWrite + 1) % delaySize;
        }
    }

    delayWritePos = (delayWritePos + buffer.getNumSamples()) % delayBuffer.getNumSamples();

    reverb.process(context);
    safetyHP.process(context);
    compressor.process(context);

    const float output = apvts.getRawParameterValue("output")->load();
    buffer.applyGain(juce::Decibels::decibelsToGain(juce::jmap(output, -18.0f, 6.0f)));
}

juce::AudioProcessorEditor* FlambiAtmosphereAudioProcessor::createEditor()
{
    return new FlambiAtmosphereAudioProcessorEditor (*this);
}

void FlambiAtmosphereAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void FlambiAtmosphereAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState != nullptr && xmlState->hasTagName(apvts.state.getType()))
        apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
}

FlambiAtmosphereAudioProcessor::APVTS::ParameterLayout FlambiAtmosphereAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    auto addFloat = [&params](const juce::String& id, const juce::String& name, float def)
    {
        params.push_back(std::make_unique<juce::AudioParameterFloat>(id, name, 0.0f, 1.0f, def));
    };

    addFloat("tone", "Tone", 0.35f);
    addFloat("air", "Air", 0.25f);
    addFloat("motion", "Motion", 0.4f);
    addFloat("drift", "Drift", 0.3f);
    addFloat("dirt", "Dirt", 0.2f);
    addFloat("delay", "Delay", 0.35f);
    addFloat("space", "Space", 0.5f);
    addFloat("output", "Output", 0.65f);

    params.push_back(std::make_unique<juce::AudioParameterBool>("drone", "Drone", false));
    params.push_back(std::make_unique<juce::AudioParameterBool>("mono", "Mono", false));
    params.push_back(std::make_unique<juce::AudioParameterBool>("sub", "Sub", true));
    params.push_back(std::make_unique<juce::AudioParameterInt>("rootNote", "Root Note", 24, 72, 36));
    params.push_back(std::make_unique<juce::AudioParameterInt>("octave", "Octave", -2, 2, 0));
    params.push_back(std::make_unique<juce::AudioParameterChoice>("filterMode", "Filter Mode", juce::StringArray { "LP", "BP" }, 0));

    return { params.begin(), params.end() };
}

void FlambiAtmosphereAudioProcessor::setupPresets()
{
    presets = {
        {"Mist Floor", 0.28f, 0.22f, 0.40f, 0.25f, 0.18f, 0.35f, 0.58f, 0.62f, true, 36, 0, 0},
        {"Rusted Air", 0.45f, 0.52f, 0.32f, 0.42f, 0.46f, 0.18f, 0.33f, 0.55f, false, 38, 0, 1},
        {"Deep Tunnel", 0.18f, 0.15f, 0.55f, 0.48f, 0.24f, 0.42f, 0.70f, 0.57f, true, 33, -1, 0},
        {"Hollow Pad", 0.36f, 0.29f, 0.28f, 0.22f, 0.14f, 0.31f, 0.63f, 0.66f, false, 41, 0, 1},
        {"Fog Chord", 0.31f, 0.26f, 0.49f, 0.30f, 0.20f, 0.48f, 0.68f, 0.58f, false, 36, 0, 0},
        {"Broken Cloud", 0.53f, 0.47f, 0.45f, 0.60f, 0.50f, 0.28f, 0.52f, 0.54f, true, 35, 0, 1},
        {"Soft Dub Bloom", 0.24f, 0.18f, 0.37f, 0.22f, 0.12f, 0.44f, 0.73f, 0.67f, true, 36, -1, 0},
        {"Night Rail", 0.42f, 0.38f, 0.62f, 0.44f, 0.35f, 0.54f, 0.62f, 0.53f, false, 43, -1, 1}
    };
}

void FlambiAtmosphereAudioProcessor::loadPreset(int index)
{
    if (index < 0 || index >= (int) presets.size())
        return;

    const auto& p = presets[(size_t) index];
    auto set = [this](const char* id, float value)
    {
        if (auto* param = apvts.getParameter(id))
            param->setValueNotifyingHost(value);
    };

    set("tone", p.tone);
    set("air", p.air);
    set("motion", p.motion);
    set("drift", p.drift);
    set("dirt", p.dirt);
    set("delay", p.delay);
    set("space", p.space);
    set("output", p.output);
    set("mono", p.mono ? 1.0f : 0.0f);
    set("rootNote", juce::jmap((float) p.rootNote, 24.0f, 72.0f, 0.0f, 1.0f));
    set("octave", juce::jmap((float) p.octave, -2.0f, 2.0f, 0.0f, 1.0f));
    set("filterMode", p.filterMode == 0 ? 0.0f : 1.0f);

    setMonoEnabled(p.mono);
}

void FlambiAtmosphereAudioProcessor::initPreset()
{
    loadPreset(0);
}

void FlambiAtmosphereAudioProcessor::randomizePreset()
{
    juce::Random r;

    auto taste = [&r](float min, float max)
    {
        return juce::jlimit(0.0f, 1.0f, juce::jmap(r.nextFloat(), min, max));
    };

    auto set = [this](const char* id, float value)
    {
        if (auto* param = apvts.getParameter(id))
            param->setValueNotifyingHost(value);
    };

    set("tone", taste(0.12f, 0.55f));
    set("air", taste(0.08f, 0.52f));
    set("motion", taste(0.15f, 0.72f));
    set("drift", taste(0.08f, 0.58f));
    set("dirt", taste(0.05f, 0.50f));
    set("delay", taste(0.12f, 0.64f));
    set("space", taste(0.28f, 0.82f));
    set("output", taste(0.46f, 0.75f));
}

void FlambiAtmosphereAudioProcessor::setDroneEnabled(bool enabled)
{
    if (auto* p = apvts.getParameter("drone"))
        p->setValueNotifyingHost(enabled ? 1.0f : 0.0f);
}

bool FlambiAtmosphereAudioProcessor::isDroneEnabled() const
{
    return apvts.getRawParameterValue("drone")->load() > 0.5f;
}

void FlambiAtmosphereAudioProcessor::setMonoEnabled(bool enabled)
{
    synth.setNoteStealingEnabled(enabled);
    synth.setMinimumRenderingSubdivisionSize(enabled ? 64 : 32);
}

void FlambiAtmosphereAudioProcessor::refreshDroneState()
{
    const bool droneOn = isDroneEnabled();
    if (droneOn == droneWasEnabled)
        return;

    droneWasEnabled = droneOn;

    const int root = (int) apvts.getRawParameterValue("rootNote")->load();
    const int octave = (int) apvts.getRawParameterValue("octave")->load();
    droneCurrentNote = juce::jlimit(0, 127, root + octave * 12);

    if (droneOn)
    {
        synth.noteOn(1, droneCurrentNote, 0.8f);
    }
    else
    {
        synth.noteOff(1, droneCurrentNote, 0.0f, true);
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new FlambiAtmosphereAudioProcessor();
}
