#include "PluginEditor.h"
#include "PluginProcessor.h"
#include "QY70Midi.h"
#include "VoiceCatalog.h"

#include <algorithm>
#include <cmath>
#include <iterator>

ParameterPage::ParameterPage(juce::AudioProcessorValueTreeState& state,
                             const juce::StringArray& parameterIds,
                             juce::String description)
{
    viewport.setScrollBarsShown(true, false);
    viewport.setViewedComponent(&content, false);
    addAndMakeVisible(viewport);

    descriptionLabel.setText(std::move(description), juce::dontSendNotification);
    descriptionLabel.setJustificationType(juce::Justification::centredLeft);
    descriptionLabel.setColour(juce::Label::textColourId,
                               juce::Colours::white.withAlpha(0.72f));
    descriptionLabel.setMinimumHorizontalScale(0.8f);
    content.addAndMakeVisible(descriptionLabel);

    for (const auto& id : parameterIds)
    {
        auto* parameter = state.getParameter(id);
        if (parameter == nullptr)
            continue;

        ids.add(id);

        auto label = std::make_unique<juce::Label>();
        label->setText(parameter->getName(96), juce::dontSendNotification);
        label->setJustificationType(juce::Justification::centred);
        label->setColour(juce::Label::textColourId,
                         juce::Colours::white.withAlpha(0.82f));
        content.addAndMakeVisible(*label);
        labels.push_back(std::move(label));

        if (auto* choice = dynamic_cast<juce::AudioParameterChoice*>(parameter))
        {
            auto combo = std::make_unique<juce::ComboBox>();
            combo->addItemList(choice->choices, 1);
            combo->setJustificationType(juce::Justification::centred);
            content.addAndMakeVisible(*combo);
            comboAttachments.push_back(std::make_unique<ComboAttachment>(state, id, *combo));
            controls.push_back(std::move(combo));
        }
        else if (dynamic_cast<juce::AudioParameterBool*>(parameter) != nullptr)
        {
            auto button = std::make_unique<juce::ToggleButton>("Enabled");
            button->setClickingTogglesState(true);
            content.addAndMakeVisible(*button);
            buttonAttachments.push_back(std::make_unique<ButtonAttachment>(state, id, *button));
            controls.push_back(std::move(button));
        }
        else
        {
            auto slider = std::make_unique<juce::Slider>();
            slider->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
            slider->setTextBoxStyle(juce::Slider::TextBoxBelow, false, 74, 22);
            slider->setDoubleClickReturnValue(
                true,
                state.getParameterRange(id).convertFrom0to1(parameter->getDefaultValue()));
            content.addAndMakeVisible(*slider);
            sliderAttachments.push_back(std::make_unique<SliderAttachment>(state, id, *slider));
            controls.push_back(std::move(slider));
        }
    }
}

void ParameterPage::setParameterLabel(const juce::String& parameterId,
                                      const juce::String& text)
{
    const auto index = ids.indexOf(parameterId);
    if (juce::isPositiveAndBelow(index, static_cast<int>(labels.size())))
        labels[static_cast<std::size_t>(index)]->setText(text, juce::dontSendNotification);
}

void ParameterPage::resized()
{
    viewport.setBounds(getLocalBounds());
    const auto contentWidth = juce::jmax(400, viewport.getWidth() - 14);
    const auto columns = contentWidth >= 900 ? 5 : (contentWidth >= 680 ? 4 : 3);
    constexpr int cellHeight = 126;
    const auto top = descriptionLabel.getText().isNotEmpty() ? 48 : 8;
    const auto rows = (static_cast<int>(controls.size()) + columns - 1) / columns;
    content.setSize(contentWidth, top + rows * cellHeight + 16);
    descriptionLabel.setBounds(18, 6, contentWidth - 36, top - 8);

    const auto cellWidth = contentWidth / columns;
    for (std::size_t index = 0; index < controls.size(); ++index)
    {
        const auto column = static_cast<int>(index) % columns;
        const auto row = static_cast<int>(index) / columns;
        auto cell = juce::Rectangle<int>(column * cellWidth,
                                         top + row * cellHeight,
                                         cellWidth,
                                         cellHeight).reduced(8, 3);
        labels[index]->setBounds(cell.removeFromTop(25));

        if (dynamic_cast<juce::ComboBox*>(controls[index].get()) != nullptr)
            controls[index]->setBounds(cell.removeFromTop(38).reduced(5, 3));
        else if (dynamic_cast<juce::ToggleButton*>(controls[index].get()) != nullptr)
            controls[index]->setBounds(cell.removeFromTop(38).withSizeKeepingCentre(110, 32));
        else
            controls[index]->setBounds(cell.reduced(8, 0));
    }
}

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
    voicePage.addAndMakeVisible(voicePreviousButton);
    voicePage.addAndMakeVisible(voiceNameButton);
    voicePage.addAndMakeVisible(voiceNextButton);

    for (std::size_t i = 0; i < parameterIds.size(); ++i)
    {
        const auto parameterName = owner.parameters.getParameter(parameterIds[i])->getName(64);
        auto& label = parameterLabels[i];
        label.setText(parameterName, juce::dontSendNotification);
        label.setJustificationType(juce::Justification::centred);
        label.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.78f));
        label.setInterceptsMouseClicks(false, false);
        voicePage.addAndMakeVisible(label);

        if (i < steppers.size())
        {
            auto& stepper = steppers[i];
            voicePage.addAndMakeVisible(stepper);
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
        voicePage.addAndMakeVisible(slider);
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
    resetButton.setTooltip("Reset sound, controller, envelope and effect edits; keep Part and voice");
    resetButton.onClick = [this] { owner.resetEditingParameters(); };
    addAndMakeVisible(xgButton);
    addAndMakeVisible(fetchButton);
    addAndMakeVisible(sendButton);
    addAndMakeVisible(resetButton);

    const auto makeIds = [](std::initializer_list<const char*> values)
    {
        juce::StringArray result;
        for (const auto* value : values)
            result.add(value);
        return result;
    };

    partPage = std::make_unique<ParameterPage>(
        owner.parameters,
        makeIds({ "monoPoly", "keyAssign", "partMode", "noteShift", "detune",
                  "velocityDepth", "velocityOffset", "noteLimitLow", "noteLimitHigh",
                  "dryLevel", "decay", "portamentoSwitch", "portamentoTime" }),
        "Per-part performance, keyboard range, dynamics and portamento.");
    controllerPage = std::make_unique<ParameterPage>(
        owner.parameters,
        makeIds({ "vibratoRate", "vibratoDepth", "vibratoDelay",
                  "mwPitch", "mwFilter", "mwAmplitude", "mwLfoPitch", "mwLfoFilter",
                  "mwLfoAmplitude", "bendPitch", "bendFilter", "bendAmplitude",
                  "bendLfoPitch", "bendLfoFilter", "bendLfoAmplitude",
                  "aftertouchPitch", "aftertouchFilter", "aftertouchAmplitude",
                  "aftertouchLfoPitch", "aftertouchLfoFilter", "aftertouchLfoAmplitude" }),
        "Mod Wheel, Pitch Bend and Channel Aftertouch response for the selected part.");
    pitchPage = std::make_unique<ParameterPage>(
        owner.parameters,
        makeIds({ "pitchEgInitial", "pitchEgAttack", "pitchEgReleaseLevel",
                  "pitchEgReleaseTime" }),
        "Pitch envelope values are relative to the selected voice (-64...+63).");

    juce::StringArray reverbIds { "reverbType", "reverbReturn", "reverbPan" };
    juce::StringArray chorusIds { "chorusType", "chorusReturn", "chorusPan",
                                  "chorusToReverb" };
    juce::StringArray variationIds { "variationType", "variationReturn", "variationPan",
                                     "variationToReverb", "variationToChorus",
                                     "variationConnection", "variationPart",
                                     "mwVariationDepth", "pbVariationDepth",
                                     "atVariationDepth", "ac1VariationDepth",
                                     "ac2VariationDepth" };
    for (int number = 1; number <= 16; ++number)
    {
        reverbIds.add("reverbParam" + juce::String(number));
        chorusIds.add("chorusParam" + juce::String(number));
        variationIds.add("variationParam" + juce::String(number));
    }
    reverbPage = std::make_unique<ParameterPage>(
        owner.parameters, reverbIds,
        "Effect parameters 1-16 follow the selected Yamaha algorithm; values are shown raw (0-127).");
    chorusPage = std::make_unique<ParameterPage>(
        owner.parameters, chorusIds,
        "Effect parameters 1-16 follow the selected Yamaha algorithm; values are shown raw (0-127).");
    variationPage = std::make_unique<ParameterPage>(
        owner.parameters, variationIds,
        "Variation includes delays and insertion effects. Params 1-10 are raw 14-bit values; 11-16 are 7-bit.");
    const auto tabColour = juce::Colour::fromRGB(33, 40, 48);
    tabs.setTabBarDepth(34);
    tabs.addTab("Voice", tabColour, &voicePage, false);
    tabs.addTab("Part", tabColour, partPage.get(), false);
    tabs.addTab("Controllers", tabColour, controllerPage.get(), false);
    tabs.addTab("Pitch EG", tabColour, pitchPage.get(), false);
    tabs.addTab("Reverb", tabColour, reverbPage.get(), false);
    tabs.addTab("Chorus", tabColour, chorusPage.get(), false);
    tabs.addTab("Variation / Delay", tabColour, variationPage.get(), false);
    addAndMakeVisible(tabs);

    setResizable(true, true);
    setResizeLimits(900, 650, 1500, 1050);
    setSize(1120, 780);
    updateEffectLabels();
    startTimerHz(4);
}

QY70ControllerAudioProcessorEditor::~QY70ControllerAudioProcessorEditor()
{
    stopTimer();
}

void QY70ControllerAudioProcessorEditor::timerCallback()
{
    refreshXgButton();
    updateEffectLabels();
}

void QY70ControllerAudioProcessorEditor::updateEffectLabels()
{
    const auto update = [this](ParameterPage* page,
                               qy70::EffectBlock block,
                               const char* typeId,
                               const char* parameterPrefix)
    {
        if (page == nullptr)
            return;
        const auto* raw = owner.parameters.getRawParameterValue(typeId);
        const auto typeIndex = raw != nullptr ? juce::roundToInt(raw->load()) : 0;
        const auto names = qy70::effectParameterNames(block, typeIndex);
        for (std::size_t index = 0; index < names.size(); ++index)
        {
            juce::String label;
            if (names[index].empty())
                label = typeIndex == 0 ? "Unused"
                                       : "Parameter " + juce::String(static_cast<int>(index) + 1)
                                             + " (raw)";
            else
                label = juce::String::fromUTF8(names[index].data(),
                                               static_cast<int>(names[index].size()));
            page->setParameterLabel(juce::String(parameterPrefix)
                                        + juce::String(static_cast<int>(index) + 1),
                                    label);
        }
    };

    update(reverbPage.get(), qy70::EffectBlock::reverb, "reverbType", "reverbParam");
    update(chorusPage.get(), qy70::EffectBlock::chorus, "chorusType", "chorusParam");
    update(variationPage.get(), qy70::EffectBlock::variation,
           "variationType", "variationParam");
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
    auto area = getLocalBounds().reduced(18);
    auto header = area.removeFromTop(42);
    title.setBounds(header.removeFromLeft(340));
    sendButton.setBounds(header.removeFromRight(145).reduced(4));
    fetchButton.setBounds(header.removeFromRight(120).reduced(4));
    xgButton.setBounds(header.removeFromRight(100).reduced(4));
    resetButton.setBounds(header.removeFromRight(120).reduced(4));

    area.removeFromTop(8);
    tabs.setBounds(area);
    layoutVoicePage();
}

void QY70ControllerAudioProcessorEditor::layoutVoicePage()
{
    auto area = voicePage.getLocalBounds().reduced(16);
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

    area.removeFromTop(2);
    auto voiceSelector = area.removeFromTop(38).reduced(80, 2);
    voicePreviousButton.setBounds(voiceSelector.removeFromLeft(40));
    voiceNextButton.setBounds(voiceSelector.removeFromRight(40));
    voiceNameButton.setBounds(voiceSelector.reduced(5, 0));
    area.removeFromTop(8);

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
                        .reduced(10, 4);
        parameterLabels[i].setBounds(cell.removeFromTop(25));
        knobSliders[i - steppers.size()].setBounds(cell.reduced(6, 0));
    }
}
