#include "PluginEditor.h"
#include "PluginProcessor.h"
#include "QY70Midi.h"

#include <algorithm>
#include <cmath>

ParameterStepper::ParameterStepper()
{
    valueSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    valueSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    valueSlider.onValueChange = [this]
    {
        syncDisplayedValue();
        if (valueChangeCallback)
            valueChangeCallback();
    };

    previousButton.setTooltip("Previous value");
    nextButton.setTooltip("Next value");
    previousButton.setRepeatSpeed(400, 90, 20);
    nextButton.setRepeatSpeed(400, 90, 20);
    previousButton.onClick = [this] { stepBy(-1.0); };
    nextButton.onClick = [this] { stepBy(1.0); };

    valueLabel.setJustificationType(juce::Justification::centred);
    valueLabel.setEditable(true, true, false);
    valueLabel.setColour(juce::Label::backgroundColourId, juce::Colour::fromRGB(33, 40, 48));
    valueLabel.setColour(juce::Label::outlineColourId, juce::Colours::white.withAlpha(0.45f));
    valueLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    valueLabel.onTextChange = [this]
    {
        valueSlider.setValue(nearestDiscreteValue(valueLabel.getText().getDoubleValue()),
                             juce::sendNotificationSync);
        syncDisplayedValue();
    };
    valueLabel.addMouseListener(this, false);

    addAndMakeVisible(previousButton);
    addAndMakeVisible(valueLabel);
    addAndMakeVisible(nextButton);
}

void ParameterStepper::stepBy(double amount)
{
    if (!discreteValues.empty())
    {
        const auto current = nearestDiscreteValue(valueSlider.getValue());
        const auto iterator = std::lower_bound(discreteValues.begin(), discreteValues.end(), current);
        const auto index = static_cast<int>(std::distance(discreteValues.begin(), iterator));
        const auto nextIndex = juce::jlimit(0,
                                            static_cast<int>(discreteValues.size()) - 1,
                                            index + (amount < 0.0 ? -1 : 1));
        valueSlider.setValue(discreteValues[static_cast<std::size_t>(nextIndex)],
                             juce::sendNotificationSync);
        return;
    }

    valueSlider.setValue(valueSlider.getValue() + amount, juce::sendNotificationSync);
}

double ParameterStepper::nearestDiscreteValue(double value) const
{
    if (discreteValues.empty())
        return value;

    return *std::min_element(discreteValues.begin(), discreteValues.end(),
                             [value](double left, double right)
                             {
                                 return std::abs(left - value) < std::abs(right - value);
                             });
}

void ParameterStepper::setDiscreteValues(std::vector<double> values)
{
    std::sort(values.begin(), values.end());
    values.erase(std::unique(values.begin(), values.end()), values.end());
    discreteValues = std::move(values);
    valueSlider.setValue(nearestDiscreteValue(valueSlider.getValue()),
                         juce::sendNotificationSync);
}

void ParameterStepper::setValueChangeCallback(std::function<void()> callback)
{
    valueChangeCallback = std::move(callback);
}

void ParameterStepper::mouseWheelMove(const juce::MouseEvent&,
                                      const juce::MouseWheelDetails& wheel)
{
    const auto delta = wheel.deltaY != 0.0f ? wheel.deltaY : -wheel.deltaX;
    if (delta != 0.0f)
        stepBy(delta > 0.0f ? 1.0 : -1.0);
}

void ParameterStepper::syncDisplayedValue()
{
    valueLabel.setText(juce::String(juce::roundToInt(valueSlider.getValue())),
                       juce::dontSendNotification);
}

void ParameterStepper::resized()
{
    auto area = getLocalBounds();
    const auto buttonWidth = juce::jmin(38, area.getHeight());
    previousButton.setBounds(area.removeFromLeft(buttonWidth));
    nextButton.setBounds(area.removeFromRight(buttonWidth));
    valueLabel.setBounds(area.reduced(5, 0));
}

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

    for (std::size_t i = 0; i < parameterIds.size(); ++i)
    {
        const auto parameterName = owner.parameters.getParameter(parameterIds[i])->getName(64);
        auto& label = parameterLabels[i];
        label.setText(parameterName, juce::dontSendNotification);
        label.setJustificationType(juce::Justification::centred);
        label.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.78f));
        label.setInterceptsMouseClicks(false, false);
        addAndMakeVisible(label);

        if (i < steppers.size())
        {
            auto& stepper = steppers[i];
            addAndMakeVisible(stepper);
            attachments[i] = std::make_unique<SliderAttachment>(owner.parameters,
                                                                parameterIds[i],
                                                                stepper.attachmentSlider());
            stepper.syncDisplayedValue();
            continue;
        }

        auto& slider = knobSliders[i - steppers.size()];
        slider.setName(parameterName);
        slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 64, 22);
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

    steppers[1].setDiscreteValues({ 0, 64, 126, 127 });
    steppers[1].setValueChangeCallback([this] { updateVoiceChoices(true, true); });
    steppers[3].setValueChangeCallback([this] { updateVoiceChoices(false, true); });
    updateVoiceChoices(false, false);

    xgButton.setTooltip("Commanded MIDI mode; hardware confirmation is not available yet");
    xgButton.onClick = [this]
    {
        owner.setXgModeEnabled(!owner.isXgModeEnabled());
        refreshXgButton();
    };
    refreshXgButton();
    fetchButton.onClick = [this] { owner.requestCurrentPart(); };
    sendButton.onClick = [this] { owner.sendCurrentPartSnapshot(); };
    addAndMakeVisible(xgButton);
    addAndMakeVisible(fetchButton);
    addAndMakeVisible(sendButton);

    setResizable(true, true);
    setResizeLimits(700, 560, 1200, 900);
    setSize(860, 650);
}

void QY70ControllerAudioProcessorEditor::refreshXgButton()
{
    const auto enabled = owner.isXgModeEnabled();
    xgButton.setButtonText(enabled ? "XG ON" : "XG OFF");
    xgButton.setColour(juce::TextButton::buttonColourId,
                       enabled ? juce::Colour::fromRGB(35, 125, 76)
                               : juce::Colour::fromRGB(90, 47, 47));
}

void QY70ControllerAudioProcessorEditor::updateVoiceChoices(bool bankChanged, bool resetLsb)
{
    if (updatingVoiceChoices)
        return;

    const juce::ScopedValueSetter<bool> guard(updatingVoiceChoices, true);
    const auto bankMsb = juce::roundToInt(steppers[1].attachmentSlider().getValue());
    const auto programs = qy70::validProgramsForBank(bankMsb);
    steppers[3].setDiscreteValues(std::vector<double>(programs.begin(), programs.end()));

    if (bankChanged && !programs.empty())
        steppers[3].attachmentSlider().setValue(programs.front(), juce::sendNotificationSync);

    const auto program = juce::roundToInt(steppers[3].attachmentSlider().getValue());
    const auto lsbValues = qy70::validLsbValues(bankMsb, program);
    steppers[2].setDiscreteValues(std::vector<double>(lsbValues.begin(), lsbValues.end()));
    if (resetLsb)
        steppers[2].attachmentSlider().setValue(0, juce::sendNotificationSync);
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
    title.setBounds(header.removeFromLeft(280));
    sendButton.setBounds(header.removeFromRight(130).reduced(4));
    fetchButton.setBounds(header.removeFromRight(110).reduced(4));
    xgButton.setBounds(header.removeFromRight(100).reduced(4));

    area.removeFromTop(14);
    auto selectorArea = area.removeFromTop(76);
    const auto selectorWidth = selectorArea.getWidth() / 4;

    for (std::size_t i = 0; i < 4; ++i)
    {
        auto cell = selectorArea.withX(selectorArea.getX() + static_cast<int>(i) * selectorWidth)
                                .withWidth(selectorWidth)
                                .reduced(8, 0);
        parameterLabels[i].setBounds(cell.removeFromTop(24));
        steppers[i].setBounds(cell.reduced(4, 7));
    }

    area.removeFromTop(10);

    constexpr int columns = 3;
    constexpr int rows = 3;
    const auto cellWidth = area.getWidth() / columns;
    const auto cellHeight = area.getHeight() / rows;

    for (std::size_t i = 4; i < parameterIds.size(); ++i)
    {
        const auto cellIndex = static_cast<int>(i - 4);
        auto cell = juce::Rectangle<int>(area.getX() + (cellIndex % columns) * cellWidth,
                                         area.getY() + (cellIndex / columns) * cellHeight,
                                         cellWidth,
                                         cellHeight)
                        .reduced(10, 2);
        parameterLabels[i].setBounds(cell.removeFromTop(25));
        knobSliders[i - steppers.size()].setBounds(cell.reduced(6, 0));
    }
}
