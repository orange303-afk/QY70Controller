#include "PluginEditor.h"
#include "PluginProcessor.h"

QY70ControllerAudioProcessorEditor::QY70ControllerAudioProcessorEditor(
    QY70ControllerAudioProcessor& processorToUse)
    : AudioProcessorEditor(&processorToUse),
      owner(processorToUse),
      parameterIds { "part", "volume", "pan", "cutoff", "resonance",
                     "attack", "release", "chorus", "reverb", "variation" }
{
    title.setText("YAMAHA QY70 CONTROLLER", juce::dontSendNotification);
    title.setFont(juce::FontOptions(22.0f, juce::Font::bold));
    title.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(title);

    for (std::size_t i = 0; i < sliders.size(); ++i)
    {
        auto& slider = sliders[i];
        slider.setName(owner.parameters.getParameter(parameterIds[i])->getName(64));
        slider.setSliderStyle(i == 0 ? juce::Slider::LinearHorizontal
                                     : juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 64, 20);
        slider.setDoubleClickReturnValue(true,
                                         owner.parameters.getParameterRange(parameterIds[i])
                                             .convertFrom0to1(
                                                 owner.parameters.getParameter(parameterIds[i])
                                                     ->getDefaultValue()));
        addAndMakeVisible(slider);
        attachments[i] = std::make_unique<SliderAttachment>(owner.parameters,
                                                            parameterIds[i],
                                                            slider);
    }

    fetchButton.onClick = [this] { owner.requestCurrentPart(); };
    sendButton.onClick = [this] { owner.sendCurrentPartSnapshot(); };
    addAndMakeVisible(fetchButton);
    addAndMakeVisible(sendButton);

    setResizable(true, true);
    setResizeLimits(620, 430, 1100, 760);
    setSize(760, 520);
}

void QY70ControllerAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour::fromRGB(20, 24, 29));
    g.setColour(juce::Colour::fromRGB(41, 48, 57));
    g.fillRoundedRectangle(getLocalBounds().toFloat().reduced(12.0f), 10.0f);

    g.setColour(juce::Colours::white.withAlpha(0.72f));
    for (std::size_t i = 1; i < sliders.size(); ++i)
    {
        const auto bounds = sliders[i].getBounds();
        g.drawFittedText(sliders[i].getName(),
                         bounds.withHeight(22),
                         juce::Justification::centred,
                         1);
    }
}

void QY70ControllerAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced(24);
    auto header = area.removeFromTop(42);
    title.setBounds(header.removeFromLeft(330));
    sendButton.setBounds(header.removeFromRight(130).reduced(4));
    fetchButton.setBounds(header.removeFromRight(110).reduced(4));

    area.removeFromTop(10);
    sliders[0].setBounds(area.removeFromTop(54).reduced(6));
    area.removeFromTop(8);

    constexpr int columns = 3;
    constexpr int rows = 3;
    const auto cellWidth = area.getWidth() / columns;
    const auto cellHeight = area.getHeight() / rows;

    for (std::size_t i = 1; i < sliders.size(); ++i)
    {
        const auto cellIndex = static_cast<int>(i - 1);
        sliders[i].setBounds(area.getX() + (cellIndex % columns) * cellWidth,
                             area.getY() + (cellIndex / columns) * cellHeight + 20,
                             cellWidth,
                             cellHeight - 20);
    }
}
