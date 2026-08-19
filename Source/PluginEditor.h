#pragma once

#include <JuceHeader.h>

#include <array>
#include <memory>

class QY70ControllerAudioProcessor;

class ParameterStepper final : public juce::Component
{
public:
    ParameterStepper();

    juce::Slider& attachmentSlider() { return valueSlider; }
    void syncDisplayedValue();
    void resized() override;

private:
    void stepBy(double amount);

    juce::TextButton previousButton { juce::String::fromUTF8("\xe2\x97\x80") };
    juce::TextButton nextButton { juce::String::fromUTF8("\xe2\x96\xb6") };
    juce::Label valueLabel;
    juce::Slider valueSlider;
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
    juce::Label title;
    juce::TextButton fetchButton { "Fetch Part" };
    juce::TextButton sendButton { "Send Snapshot" };

    std::array<ParameterStepper, 4> steppers;
    std::array<juce::Slider, 9> knobSliders;
    std::array<juce::Label, 13> parameterLabels;
    std::array<std::unique_ptr<SliderAttachment>, 13> attachments;
    std::array<juce::String, 13> parameterIds;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(QY70ControllerAudioProcessorEditor)
};
