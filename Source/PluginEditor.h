#pragma once

#include <JuceHeader.h>

#include <array>
#include <memory>

class QY70ControllerAudioProcessor;

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

    std::array<juce::Slider, 13> sliders;
    std::array<juce::Label, 13> parameterLabels;
    std::array<std::unique_ptr<SliderAttachment>, 13> attachments;
    std::array<juce::String, 13> parameterIds;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(QY70ControllerAudioProcessorEditor)
};
