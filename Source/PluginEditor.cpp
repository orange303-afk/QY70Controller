#include "PluginEditor.h"
#include "PluginProcessor.h"

QY70ControllerAudioProcessorEditor::QY70ControllerAudioProcessorEditor(
    QY70ControllerAudioProcessor& processorToUse)
    : AudioProcessorEditor(&processorToUse),
      owner(processorToUse),
      parameterIds { "part", "bankMsb", "bankLsb", "program",
                     "volume", "pan", "cutoff", "resonance",
                     "attack", "release", "chorus", "reverb", "variation" }
{
    title.setText("YAMAHA QY70 CONTROLLER", juce::dontSendNotification);
    title.setFont(juce::FontOptions(22.0f, juce::Font::bold));
    title.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(title);

    for (std::size_t i = 0; i < sliders.size(); ++i)
    {
        auto& slider = sliders[i];
        const auto parameterName = owner.parameters.getParameter(parameterIds[i])->getName(64);
        slider.setName(parameterName);
        slider.setSliderStyle(i < 4 ? juce::Slider::LinearHorizontal
                                    : juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle(i < 4 ? juce::Slider::TextBoxRight
                                     : juce::Slider::TextBoxBelow,
                               false,
                               64,
                               22);
        slider.setDoubleClickReturnValue(true,
                                         owner.parameters.getParameterRange(parameterIds[i])
                                             .convertFrom0to1(
                                                 owner.parameters.getParameter(parameterIds[i])
                                                     ->getDefaultValue()));
        addAndMakeVisible(slider);
        attachments[i] = std::make_unique<SliderAttachment>(owner.parameters,
                                                            parameterIds[i],
                                                            slider);

        auto& label = parameterLabels[i];
        label.setText(parameterName, juce::dontSendNotification);
        label.setJustificationType(juce::Justification::centred);
        label.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.78f));
        label.setInterceptsMouseClicks(false, false);
        addAndMakeVisible(label);
    }

    fetchButton.onClick = [this] { owner.requestCurrentPart(); };
    sendButton.onClick = [this] { owner.sendCurrentPartSnapshot(); };
    addAndMakeVisible(fetchButton);
    addAndMakeVisible(sendButton);

    setResizable(true, true);
    setResizeLimits(700, 560, 1200, 900);
    setSize(860, 650);
}

void QY70ControllerAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour::fromRGB(20, 24, 29));
    g.setColour(juce::Colour::fromRGB(41, 48, 57));
    g.fillRoundedRectangle(getLocalBounds().toFloat().reduced(12.0f), 10.0f);

}

void QY70ControllerAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced(24);
    auto header = area.removeFromTop(42);
    title.setBounds(header.removeFromLeft(330));
    sendButton.setBounds(header.removeFromRight(130).reduced(4));
    fetchButton.setBounds(header.removeFromRight(110).reduced(4));

    area.removeFromTop(14);
    auto selectorArea = area.removeFromTop(76);
    const auto selectorWidth = selectorArea.getWidth() / 4;

    for (std::size_t i = 0; i < 4; ++i)
    {
        auto cell = selectorArea.withX(selectorArea.getX() + static_cast<int>(i) * selectorWidth)
                                .withWidth(selectorWidth)
                                .reduced(8, 0);
        parameterLabels[i].setBounds(cell.removeFromTop(24));
        sliders[i].setBounds(cell.reduced(4, 5));
    }

    area.removeFromTop(10);

    constexpr int columns = 3;
    constexpr int rows = 3;
    const auto cellWidth = area.getWidth() / columns;
    const auto cellHeight = area.getHeight() / rows;

    for (std::size_t i = 4; i < sliders.size(); ++i)
    {
        const auto cellIndex = static_cast<int>(i - 4);
        auto cell = juce::Rectangle<int>(area.getX() + (cellIndex % columns) * cellWidth,
                                         area.getY() + (cellIndex / columns) * cellHeight,
                                         cellWidth,
                                         cellHeight)
                        .reduced(10, 2);
        parameterLabels[i].setBounds(cell.removeFromTop(25));
        sliders[i].setBounds(cell.reduced(6, 0));
    }
}
