#include "PluginEditor.h"

FlambiAtmosphereAudioProcessorEditor::FlambiAtmosphereAudioProcessorEditor (FlambiAtmosphereAudioProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p)
{
    setSize (760, 420);

    titleLabel.setText("FLAMBI ATMOSPHERE", juce::dontSendNotification);
    titleLabel.setFont(juce::FontOptions(24.0f, juce::Font::bold));
    titleLabel.setJustificationType(juce::Justification::centred);
    titleLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible(titleLabel);

    const std::array<juce::String, 8> ids = { "tone", "air", "motion", "drift", "dirt", "delay", "space", "output" };
    const std::array<juce::String, 8> names = { "TONE", "AIR", "MOTION", "DRIFT", "DIRT", "DELAY", "SPACE", "OUTPUT" };
    const std::array<juce::String, 8> tips = {
        "Dark/soft to brighter shape.", "Adds filtered hiss/air.", "Movement depth for modulation.",
        "Slow random tape-like instability.", "Gentle saturation dirt amount.", "Dubby delay amount.",
        "Reverb room and wash depth.", "Final output volume."
    };

    for (size_t i = 0; i < knobs.size(); ++i)
        buildKnob(knobs[i], ids[i], names[i], tips[i]);

    droneButton.setClickingTogglesState(true);
    droneAttachment = std::make_unique<ButtonAttachment>(processorRef.apvts, "drone", droneButton);
    droneButton.setTooltip("Holds the selected root note continuously.");
    addAndMakeVisible(droneButton);

    monoAttachment = std::make_unique<ButtonAttachment>(processorRef.apvts, "mono", monoButton);
    monoButton.setTooltip("Monophonic note behavior for single drones.");
    addAndMakeVisible(monoButton);

    randomButton.onClick = [this] { processorRef.randomizePreset(); };
    randomButton.setTooltip("Creates a tasteful new atmosphere variation.");
    addAndMakeVisible(randomButton);

    initButton.onClick = [this] { processorRef.initPreset(); };
    initButton.setTooltip("Resets to startup preset (Mist Floor).");
    addAndMakeVisible(initButton);

    rootNoteBox.setTooltip("Drone root note.");
    for (int note = 24; note <= 72; ++note)
        rootNoteBox.addItem(juce::MidiMessage::getMidiNoteName(note, true, true, 4), note - 23);
    rootNoteBox.setSelectedId(36 - 23);
    rootNoteBox.onChange = [this]
    {
        if (auto* param = processorRef.apvts.getParameter("rootNote"))
            param->setValueNotifyingHost(juce::jmap((float)(rootNoteBox.getSelectedId() + 23), 24.0f, 72.0f, 0.0f, 1.0f));
    };
    addAndMakeVisible(rootNoteBox);

    octaveBox.setTooltip("Drone octave shift.");
    octaveBox.addItem("-2", 1);
    octaveBox.addItem("-1", 2);
    octaveBox.addItem("0", 3);
    octaveBox.addItem("+1", 4);
    octaveBox.addItem("+2", 5);
    octaveBox.setSelectedId(3);
    octaveBox.onChange = [this]
    {
        if (auto* param = processorRef.apvts.getParameter("octave"))
            param->setValueNotifyingHost(juce::jmap((float)(octaveBox.getSelectedId() - 3), -2.0f, 2.0f, 0.0f, 1.0f));
    };
    addAndMakeVisible(octaveBox);

    filterModeBox.addItem("LP", 1);
    filterModeBox.addItem("BP", 2);
    filterAttachment = std::make_unique<ComboAttachment>(processorRef.apvts, "filterMode", filterModeBox);
    filterModeBox.setTooltip("Main filter mode.");
    addAndMakeVisible(filterModeBox);

}

void FlambiAtmosphereAudioProcessorEditor::buildKnob(Knob& knob, const juce::String& id, const juce::String& label, const juce::String& tip)
{
    knob.slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    knob.slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
    knob.slider.setTooltip(tip);
    knob.slider.setColour(juce::Slider::rotarySliderFillColourId, juce::Colour::fromRGB(94, 116, 131));
    knob.slider.setColour(juce::Slider::thumbColourId, juce::Colour::fromRGB(190, 198, 205));
    addAndMakeVisible(knob.slider);

    knob.label.setText(label, juce::dontSendNotification);
    knob.label.setJustificationType(juce::Justification::centred);
    knob.label.setColour(juce::Label::textColourId, juce::Colours::gainsboro);
    knob.label.setFont(juce::FontOptions(14.0f, juce::Font::bold));
    addAndMakeVisible(knob.label);

    knob.attachment = std::make_unique<SliderAttachment>(processorRef.apvts, id, knob.slider);
}

void FlambiAtmosphereAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll(juce::Colour::fromRGB(17, 20, 24));
    g.setColour(juce::Colour::fromRGB(40, 45, 52));
    g.fillRoundedRectangle(getLocalBounds().toFloat().reduced(8.0f), 10.0f);
}

void FlambiAtmosphereAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced(18);
    titleLabel.setBounds(area.removeFromTop(34));
    area.removeFromTop(10);

    auto knobsArea = area.removeFromTop(280);
    const int columns = 4;
    const int rows = 2;
    const int cellW = knobsArea.getWidth() / columns;
    const int cellH = knobsArea.getHeight() / rows;

    for (int i = 0; i < (int) knobs.size(); ++i)
    {
        auto row = i / columns;
        auto col = i % columns;
        auto cell = juce::Rectangle<int>(knobsArea.getX() + col * cellW, knobsArea.getY() + row * cellH, cellW, cellH).reduced(12);
        knobs[(size_t) i].slider.setBounds(cell.removeFromTop(cell.getHeight() - 28));
        knobs[(size_t) i].label.setBounds(cell);
    }

    auto buttons = area.reduced(4);
    auto left = buttons.removeFromLeft(420);
    droneButton.setBounds(left.removeFromLeft(95).reduced(4));
    monoButton.setBounds(left.removeFromLeft(80).reduced(4));
    randomButton.setBounds(left.removeFromLeft(120).reduced(4));
    initButton.setBounds(left.removeFromLeft(80).reduced(4));

    rootNoteBox.setBounds(buttons.removeFromLeft(120).reduced(4));
    octaveBox.setBounds(buttons.removeFromLeft(90).reduced(4));
    filterModeBox.setBounds(buttons.removeFromLeft(90).reduced(4));
}
