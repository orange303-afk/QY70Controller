#pragma once

#include <JuceHeader.h>

#include <array>
#include <functional>
#include <memory>
#include <vector>

class QY70ControllerAudioProcessor;

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

class QY70ControllerAudioProcessorEditor final : public juce::AudioProcessorEditor
{
public:
    explicit QY70ControllerAudioProcessorEditor(QY70ControllerAudioProcessor&);
    ~QY70ControllerAudioProcessorEditor() override = default;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;

    QY70ControllerAudioProcessor& owner;
    void refreshXgButton();
    void updateVoiceChoices(bool bankChanged, bool resetLsb);

    juce::Label title;
    juce::TextButton xgButton { "XG OFF" };
    juce::TextButton fetchButton { "Fetch Part" };
    juce::TextButton sendButton { "Send Snapshot" };

    std::array<ParameterStepper, 4> steppers;
    std::array<juce::Slider, 9> knobSliders;
    std::array<juce::Label, 13> parameterLabels;
    std::array<std::unique_ptr<SliderAttachment>, 13> attachments;
    std::array<juce::String, 13> parameterIds;
    bool updatingVoiceChoices = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(QY70ControllerAudioProcessorEditor)
};
