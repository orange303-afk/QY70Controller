#pragma once

#include <JuceHeader.h>

#include <array>
#include <atomic>

#include "QY70Midi.h"

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
    bool isMidiEffect() const override { return false; }
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
    void resetEditingParameters();
    void setXgModeEnabled(bool shouldBeEnabled);
    bool isXgModeEnabled() const { return xgModeEnabled.load(std::memory_order_acquire); }

    void setHardwareMidiOutputDevice(const juce::String& deviceIdentifier);
    juce::String getHardwareMidiOutputDeviceId() const;

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
        gmSystemOnDirty = 1u << 27,
        xgSystemOnDirty = 1u << 28,
        requestDirty = 1u << 29,
        snapshotDirty = 1u << 30
    };

    void parameterChanged(const juce::String& parameterID, float newValue) override;
    void remapInputToSelectedPart(juce::MidiBuffer& midiMessages) const;
    void emitPendingMessages(juce::MidiBuffer& midiMessages, int blockSize);
    int parameterValue(const char* id) const;

    std::atomic<std::uint32_t> dirtyMask { snapshotDirty };
    std::atomic<std::uint64_t> advancedPartDirtyMask { 0 };
    std::array<std::atomic<std::uint64_t>, 2> effectDirtyMasks {};
    std::atomic<bool> effectSnapshotDirty { false };
    std::atomic<bool> xgModeEnabled { false };
    double currentSampleRate = 44100.0;
    int xgWaitSamples = 0;

    juce::CriticalSection midiOutLock;
    std::unique_ptr<juce::MidiOutput> hardwareMidiOutput;
    juce::String currentMidiDeviceId;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(QY70ControllerAudioProcessor)
};
