#pragma once

#include <JuceHeader.h>

#include <array>
#include <functional>
#include <memory>
#include <vector>

#include "VoiceCatalog.h"

class QY70ControllerAudioProcessor;

class ParameterPage final : public juce::Component
{
public:
    ParameterPage(juce::AudioProcessorValueTreeState& state,
                  const juce::StringArray& parameterIds,
                  juce::String description = {});
    void setParameterLabel(const juce::String& parameterId, const juce::String& text);
    void resized() override;

private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    juce::Viewport viewport;
    juce::Component content;
    juce::Label descriptionLabel;
    juce::StringArray ids;
    std::vector<std::unique_ptr<juce::Label>> labels;
    std::vector<std::unique_ptr<juce::Component>> controls;
    std::vector<std::unique_ptr<SliderAttachment>> sliderAttachments;
    std::vector<std::unique_ptr<ComboAttachment>> comboAttachments;
    std::vector<std::unique_ptr<ButtonAttachment>> buttonAttachments;
};

class ParameterStepper final : public juce::Component
{
public:
    ParameterStepper();

    juce::Slider& attachmentSlider() { return valueSlider; }
    void setDiscreteValues(std::vector<double> values);
    void setValueChangeCallback(std::function<void()> callback);
    void syncDisplayedValue();
    void resized() override;
    void mouseWheelMove(const juce::MouseEvent& event,
                        const juce::MouseWheelDetails& wheel) override;

private:
    void stepBy(double amount);
    double nearestDiscreteValue(double value) const;

    juce::TextButton previousButton { juce::String::fromUTF8("\xe2\x97\x80") };
    juce::TextButton nextButton { juce::String::fromUTF8("\xe2\x96\xb6") };
    juce::Label valueLabel;
    juce::Slider valueSlider;
    std::vector<double> discreteValues;
    std::function<void()> valueChangeCallback;
};

class QY70ControllerAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                                  private juce::Timer
{
public:
    explicit QY70ControllerAudioProcessorEditor(QY70ControllerAudioProcessor&);
    ~QY70ControllerAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;

    QY70ControllerAudioProcessor& owner;
    void refreshXgButton();
    void updateVoiceChoices(bool bankChanged, bool resetLsb);
    void updateVoiceName();
    void showVoiceMenu();
    void selectVoice(const qy70::VoiceDescriptor& voice);
    void stepCatalogVoice(int direction);
    void layoutVoicePage();
    void timerCallback() override;
    void updateEffectLabels();

    juce::Label title;
    juce::TextButton voicePreviousButton { juce::String::fromUTF8("\xe2\x97\x80") };
    juce::TextButton voiceNameButton;
    juce::TextButton voiceNextButton { juce::String::fromUTF8("\xe2\x96\xb6") };
    juce::TextButton xgButton { "XG OFF" };
    juce::TextButton fetchButton { "Fetch Part" };
    juce::TextButton sendButton { "Send Snapshot" };
    juce::TextButton resetButton { "Reset Edits" };
    juce::TabbedComponent tabs { juce::TabbedButtonBar::TabsAtTop };
    juce::Component voicePage;
    std::unique_ptr<ParameterPage> partPage;
    std::unique_ptr<ParameterPage> controllerPage;
    std::unique_ptr<ParameterPage> pitchPage;
    std::unique_ptr<ParameterPage> reverbPage;
    std::unique_ptr<ParameterPage> chorusPage;
    std::unique_ptr<ParameterPage> variationPage;
    std::array<ParameterStepper, 4> steppers;
    std::array<juce::Slider, 9> knobSliders;
    std::array<juce::Label, 13> parameterLabels;
    std::array<std::unique_ptr<SliderAttachment>, 13> attachments;
    std::array<juce::String, 13> parameterIds;
    bool updatingVoiceChoices = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(QY70ControllerAudioProcessorEditor)
};
