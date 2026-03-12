#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class FlambiAtmosphereAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    FlambiAtmosphereAudioProcessorEditor (FlambiAtmosphereAudioProcessor&);
    ~FlambiAtmosphereAudioProcessorEditor() override = default;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using ComboAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    FlambiAtmosphereAudioProcessor& processorRef;

    struct Knob
    {
        juce::Slider slider;
        juce::Label label;
        std::unique_ptr<SliderAttachment> attachment;
    };

    std::array<Knob, 8> knobs;
    juce::TextButton droneButton {"DRONE"};
    juce::TextButton randomButton {"RANDOMIZE"};
    juce::TextButton initButton {"INIT"};
    juce::ToggleButton monoButton {"MONO"};

    juce::ComboBox rootNoteBox;
    juce::ComboBox octaveBox;
    juce::ComboBox filterModeBox;

    std::unique_ptr<ButtonAttachment> droneAttachment;
    std::unique_ptr<ButtonAttachment> monoAttachment;
    std::unique_ptr<ComboAttachment> filterAttachment;

    juce::Label titleLabel;

    void buildKnob(Knob& knob, const juce::String& id, const juce::String& label, const juce::String& tip);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FlambiAtmosphereAudioProcessorEditor)
};
