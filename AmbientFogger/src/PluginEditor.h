#pragma once

#include "PluginProcessor.h"

class AmbientFoggerAudioProcessorEditor final : public juce::AudioProcessorEditor
{
public:
    explicit AmbientFoggerAudioProcessorEditor (AmbientFoggerAudioProcessor&);
    ~AmbientFoggerAudioProcessorEditor() override = default;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    AmbientFoggerAudioProcessor& processor;

    juce::OwnedArray<juce::Slider> sliders;
    juce::OwnedArray<juce::Label> labels;
    std::vector<std::unique_ptr<SliderAttachment>> attachments;

    juce::TextButton droneButton { "DRONE" };
    juce::TextButton randomButton { "RANDOMIZE" };
    juce::TextButton initButton { "INIT" };
    std::unique_ptr<ButtonAttachment> droneAttachment;


    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AmbientFoggerAudioProcessorEditor)
};
