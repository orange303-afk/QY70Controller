#include "PluginEditor.h"
#include "PluginProcessor.h"
#include "QY70Midi.h"
#include "VoiceCatalog.h"

#include <algorithm>
#include <cmath>
#include <iterator>

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
      parameterIds { "part", "bankMsb", "program", "bankLsb",
                     "volume", "pan", "cutoff", "resonance",
                     "attack", "release", "chorus", "reverb", "variation" }
{
    title.setText("YAMAHA QY70 CONTROLLER", juce::dontSendNotification);
    title.setFont(juce::FontOptions(22.0f, juce::Font::bold));
    title.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(title);

    voiceNameButton.setColour(juce::TextButton::buttonColourId,
                              juce::Colour::fromRGB(33, 40, 48));
    voiceNameButton.setColour(juce::TextButton::buttonOnColourId,
                              juce::Colour::fromRGB(42, 54, 64));
    voiceNameButton.setColour(juce::TextButton::textColourOffId,
                              juce::Colour::fromRGB(88, 190, 224));
    voiceNameButton.setColour(juce::TextButton::textColourOnId,
                              juce::Colour::fromRGB(110, 205, 235));
    voiceNameButton.setTooltip("Open the categorized QY70 voice browser");
    voiceNameButton.onClick = [this] { showVoiceMenu(); };
    voicePreviousButton.setTooltip("Previous voice in the catalog");
    voiceNextButton.setTooltip("Next voice in the catalog");
    voicePreviousButton.setRepeatSpeed(400, 90, 20);
    voiceNextButton.setRepeatSpeed(400, 90, 20);
    voicePreviousButton.onClick = [this] { stepCatalogVoice(-1); };
    voiceNextButton.onClick = [this] { stepCatalogVoice(1); };
    addAndMakeVisible(voicePreviousButton);
    addAndMakeVisible(voiceNameButton);
    addAndMakeVisible(voiceNextButton);

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
    steppers[2].setValueChangeCallback([this] { updateVoiceChoices(false, true); });
    steppers[3].setValueChangeCallback([this] { updateVoiceName(); });
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
    steppers[2].setDiscreteValues(std::vector<double>(programs.begin(), programs.end()));

    if (bankChanged && !programs.empty())
        steppers[2].attachmentSlider().setValue(programs.front(), juce::sendNotificationSync);

    const auto program = juce::roundToInt(steppers[2].attachmentSlider().getValue());
    const auto lsbValues = qy70::validLsbValues(bankMsb, program);
    steppers[3].setDiscreteValues(std::vector<double>(lsbValues.begin(), lsbValues.end()));
    if (resetLsb)
        steppers[3].attachmentSlider().setValue(0, juce::sendNotificationSync);

    updateVoiceName();
}

void QY70ControllerAudioProcessorEditor::updateVoiceName()
{
    const auto bankMsb = juce::roundToInt(steppers[1].attachmentSlider().getValue());
    const auto program = juce::roundToInt(steppers[2].attachmentSlider().getValue());
    const auto bankLsb = juce::roundToInt(steppers[3].attachmentSlider().getValue());
    const auto mode = qy70::voiceModeName(bankMsb);
    const auto voice = qy70::voiceName(bankMsb, bankLsb, program);
    const auto modeText = juce::String::fromUTF8(mode.data(), static_cast<int>(mode.size()));
    const auto voiceText = juce::String::fromUTF8(voice.data(), static_cast<int>(voice.size()));

    voiceNameButton.setButtonText(modeText + "  ·  " + voiceText
                                  + "   (Patch " + juce::String(program)
                                  + ", Variation " + juce::String(bankLsb) + ")  ▼");
}

void QY70ControllerAudioProcessorEditor::showVoiceMenu()
{
    auto catalog = qy70::voiceCatalog();
    struct CategoryMenu
    {
        juce::String name;
        juce::PopupMenu menu;
    };
    std::vector<CategoryMenu> categories;

    const auto currentMsb = juce::roundToInt(steppers[1].attachmentSlider().getValue());
    const auto currentProgram = juce::roundToInt(steppers[2].attachmentSlider().getValue());
    const auto currentLsb = juce::roundToInt(steppers[3].attachmentSlider().getValue());

    for (std::size_t index = 0; index < catalog.size(); ++index)
    {
        const auto& voice = catalog[index];
        const auto categoryName = juce::String::fromUTF8(voice.category.data(),
                                                         static_cast<int>(voice.category.size()));
        auto category = std::find_if(categories.begin(), categories.end(),
                                     [&categoryName](const CategoryMenu& item)
                                     {
                                         return item.name == categoryName;
                                     });
        if (category == categories.end())
        {
            categories.push_back({ categoryName, {} });
            category = std::prev(categories.end());
        }

        const auto name = juce::String::fromUTF8(voice.name.data(),
                                                 static_cast<int>(voice.name.size()));
        auto itemText = name + "   P" + juce::String(voice.program);
        if (voice.bankLsb != 0)
            itemText += " · V" + juce::String(voice.bankLsb);

        const auto isCurrent = voice.bankMsb == currentMsb
                               && voice.program == currentProgram
                               && voice.bankLsb == currentLsb;
        category->menu.addItem(static_cast<int>(index) + 1, itemText, true, isCurrent);
    }

    juce::PopupMenu menu;
    for (const auto& category : categories)
        menu.addSubMenu(category.name, category.menu);

    const juce::Component::SafePointer<QY70ControllerAudioProcessorEditor> safeThis(this);
    menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&voiceNameButton),
                       [safeThis, catalogCopy = std::move(catalog)](int result)
                       {
                           if (safeThis != nullptr && result > 0
                               && result <= static_cast<int>(catalogCopy.size()))
                               safeThis->selectVoice(catalogCopy[static_cast<std::size_t>(result - 1)]);
                       });
}

void QY70ControllerAudioProcessorEditor::selectVoice(const qy70::VoiceDescriptor& voice)
{
    const juce::ScopedValueSetter<bool> guard(updatingVoiceChoices, true);
    steppers[1].attachmentSlider().setValue(voice.bankMsb, juce::sendNotificationSync);

    const auto programs = qy70::validProgramsForBank(voice.bankMsb);
    steppers[2].setDiscreteValues(std::vector<double>(programs.begin(), programs.end()));
    steppers[2].attachmentSlider().setValue(voice.program, juce::sendNotificationSync);

    const auto variations = qy70::validLsbValues(voice.bankMsb, voice.program);
    steppers[3].setDiscreteValues(std::vector<double>(variations.begin(), variations.end()));
    steppers[3].attachmentSlider().setValue(voice.bankLsb, juce::sendNotificationSync);
    updateVoiceName();
}

void QY70ControllerAudioProcessorEditor::stepCatalogVoice(int direction)
{
    const auto catalog = qy70::voiceCatalog();
    if (catalog.empty())
        return;

    const auto currentMsb = juce::roundToInt(steppers[1].attachmentSlider().getValue());
    const auto currentProgram = juce::roundToInt(steppers[2].attachmentSlider().getValue());
    const auto currentLsb = juce::roundToInt(steppers[3].attachmentSlider().getValue());
    const auto current = std::find_if(catalog.begin(), catalog.end(),
                                      [=](const qy70::VoiceDescriptor& voice)
                                      {
                                          return voice.bankMsb == currentMsb
                                                 && voice.program == currentProgram
                                                 && voice.bankLsb == currentLsb;
                                      });
    const auto currentIndex = current != catalog.end()
                                  ? static_cast<int>(std::distance(catalog.begin(), current))
                                  : 0;
    const auto count = static_cast<int>(catalog.size());
    const auto nextIndex = (currentIndex + (direction < 0 ? -1 : 1) + count) % count;
    selectVoice(catalog[static_cast<std::size_t>(nextIndex)]);
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

    area.removeFromTop(4);
    auto voiceSelector = area.removeFromTop(34).reduced(100, 2);
    voicePreviousButton.setBounds(voiceSelector.removeFromLeft(40));
    voiceNextButton.setBounds(voiceSelector.removeFromRight(40));
    voiceNameButton.setBounds(voiceSelector.reduced(5, 0));
    area.removeFromTop(6);

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
