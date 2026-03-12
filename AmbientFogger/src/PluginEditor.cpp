#include "PluginEditor.h"

AmbientFoggerAudioProcessorEditor::AmbientFoggerAudioProcessorEditor (AmbientFoggerAudioProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    setSize (860, 420);

    const std::array<std::pair<const char*, const char*>, 8> specs {{
        {"tone", "TONE"}, {"air", "AIR"}, {"motion", "MOTION"}, {"drift", "DRIFT"},
        {"dirt", "DIRT"}, {"delay", "DELAY"}, {"space", "SPACE"}, {"output", "OUTPUT"}
    }};

    for (auto [id, title] : specs)
    {
        auto* s = sliders.add (new juce::Slider());
        s->setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        s->setTextBoxStyle (juce::Slider::TextBoxBelow, false, 70, 20);
        s->setColour (juce::Slider::rotarySliderFillColourId, juce::Colour (0xff8aa0ff));
        addAndMakeVisible (s);
        attachments.push_back (std::make_unique<SliderAttachment> (processor.getAPVTS(), id, *s));

        auto* l = labels.add (new juce::Label());
        l->setText (title, juce::dontSendNotification);
        l->setJustificationType (juce::Justification::centred);
        l->setColour (juce::Label::textColourId, juce::Colour (0xffe3e8ff));
        addAndMakeVisible (l);
    }

    droneButton.setClickingTogglesState (true);
    addAndMakeVisible (droneButton);
    droneAttachment = std::make_unique<ButtonAttachment> (processor.getAPVTS(), "drone", droneButton);

    randomButton.onClick = [this] { processor.randomizeTasteful(); };
    initButton.onClick = [this] { processor.loadInit(); };
    addAndMakeVisible (randomButton);
    addAndMakeVisible (initButton);

    for (auto* b : { &droneButton, &randomButton, &initButton })
    {
        b->setColour (juce::TextButton::buttonColourId, juce::Colour (0xff222531));
        b->setColour (juce::TextButton::textColourOffId, juce::Colours::white);
    }
}

void AmbientFoggerAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff101217));
    g.setColour (juce::Colour (0xff5e6ca8));
    g.setFont (28.0f);
    g.drawText ("AMBIENT FOGGER", 20, 12, getWidth() - 40, 32, juce::Justification::centredLeft);
}

void AmbientFoggerAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced (18);
    area.removeFromTop (50);

    auto top = area.removeFromTop (300);
    const int knobW = top.getWidth() / 8;

    for (int i = 0; i < sliders.size(); ++i)
    {
        auto box = top.removeFromLeft (knobW).reduced (6);
        labels[i]->setBounds (box.removeFromTop (24));
        sliders[i]->setBounds (box);
    }

    auto bottom = area.removeFromTop (50);
    droneButton.setBounds (bottom.removeFromLeft (140).reduced (6));
    randomButton.setBounds (bottom.removeFromLeft (170).reduced (6));
    initButton.setBounds (bottom.removeFromLeft (120).reduced (6));
}
