#pragma once

#include <JuceHeader.h>

#include <atomic>

class QY70ControllerAudioProcessor final : public juce::AudioProcessor,
                                           private juce::AudioProcessorValueTreeState::Listener
{
public:
    QY70ControllerAudioProcessor();
    ~QY70ControllerAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return true; }
    bool isMidiEffect() const override { return true; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destinationData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState parameters;

    void requestCurrentPart();
    void sendCurrentPartSnapshot();

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

private:
    enum DirtyBit : std::uint32_t
    {
        bankMsbDirty = 1u << 0,
        bankLsbDirty = 1u << 1,
        programDirty = 1u << 2,
        volumeDirty = 1u << 3,
        panDirty = 1u << 4,
        cutoffDirty = 1u << 5,
        resonanceDirty = 1u << 6,
        attackDirty = 1u << 7,
        releaseDirty = 1u << 8,
        chorusDirty = 1u << 9,
        reverbDirty = 1u << 10,
        variationDirty = 1u << 11,
        requestDirty = 1u << 29,
        snapshotDirty = 1u << 30
    };

    void parameterChanged(const juce::String& parameterID, float newValue) override;
    void emitPendingMessages(juce::MidiBuffer& midiMessages);
    int parameterValue(const char* id) const;

    std::atomic<std::uint32_t> dirtyMask { snapshotDirty };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(QY70ControllerAudioProcessor)
};
