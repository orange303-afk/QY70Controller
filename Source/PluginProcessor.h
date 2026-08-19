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
    void resetEditingParameters();
    void setXgModeEnabled(bool shouldBeEnabled);
    bool isXgModeEnabled() const { return xgModeEnabled.load(std::memory_order_acquire); }

    static constexpr int sequencerTrackCount = 8;
    static constexpr int sequencerStepCount = 16;
    void startSequencer(bool restartFromBeginning = true);
    void continueSequencer();
    void stopSequencer();
    bool isSequencerRunning() const;
    int currentSequencerStep() const;
    bool isStepEnabled(int track, int step) const;
    void setStepEnabled(int track, int step, bool enabled);
    int trackNote(int track) const;
    int trackVelocity(int track) const;
    int trackChannel(int track) const;
    int trackGate(int track) const;
    void setTrackNote(int track, int value);
    void setTrackVelocity(int track, int value);
    void setTrackChannel(int track, int value);
    void setTrackGate(int track, int value);
    void selectPattern(int patternNumber);
    void selectSection(qy70::PatternSection section);

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
    void emitSequencerMessages(juce::MidiBuffer& midiMessages, int blockSize);
    void stopActiveSequencerNotes(juce::MidiBuffer& midiMessages, int sampleOffset);
    int parameterValue(const char* id) const;

    std::atomic<std::uint32_t> dirtyMask { snapshotDirty };
    std::atomic<std::uint64_t> advancedPartDirtyMask { 0 };
    std::array<std::atomic<std::uint64_t>, 2> effectDirtyMasks {};
    std::atomic<bool> effectSnapshotDirty { false };
    std::array<std::atomic<std::uint8_t>, sequencerTrackCount * sequencerStepCount> sequenceSteps;
    std::array<std::atomic<int>, sequencerTrackCount> sequenceNotes;
    std::array<std::atomic<int>, sequencerTrackCount> sequenceVelocities;
    std::array<std::atomic<int>, sequencerTrackCount> sequenceChannels;
    std::array<std::atomic<int>, sequencerTrackCount> sequenceGates;
    std::atomic<std::uint32_t> sequencerCommands { 0 };
    std::atomic<int> pendingPatternSelection { -1 };
    std::atomic<int> pendingSectionSelection { -1 };
    std::atomic<bool> sequencerRunningState { false };
    std::atomic<int> displayedSequencerStep { -1 };
    bool audioSequencerRunning = false;
    int audioSequencerStep = 0;
    int clocksIntoStep = 0;
    double samplesUntilNextClock = 0.0;
    std::array<int, sequencerTrackCount> activeSequenceNotes;
    std::array<int, sequencerTrackCount> activeSequenceChannels;
    std::array<double, sequencerTrackCount> noteOffSamplesRemaining;
    std::atomic<bool> xgModeEnabled { false };
    double currentSampleRate = 44100.0;
    int xgWaitSamples = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(QY70ControllerAudioProcessor)
};
