#include "../Source/PluginProcessor.h"

#include <cstdlib>
#include <iostream>

namespace
{
bool contains(const juce::MidiBuffer& buffer,
              const std::function<bool(const juce::MidiMessage&)>& predicate)
{
    for (const auto metadata : buffer)
        if (predicate(metadata.getMessage()))
            return true;
    return false;
}

void process(QY70ControllerAudioProcessor& processor,
             juce::MidiBuffer& midi,
             int blockSize = 480)
{
    juce::AudioBuffer<float> audio(1, blockSize);
    midi.clear();
    processor.processBlock(audio, midi);
}
} // namespace

int main()
{
    juce::ScopedJuceInitialiser_GUI juceInitialiser;
    QY70ControllerAudioProcessor processor;
    processor.prepareToPlay(48000.0, 480);
    processor.setTrackNote(0, 60);
    processor.setTrackVelocity(0, 110);
    processor.setTrackChannel(0, 3);
    processor.setTrackGate(0, 80);
    processor.setStepEnabled(0, 0, true);
    processor.startSequencer(true);

    juce::MidiBuffer midi;
    process(processor, midi);
    bool passed = contains(midi, [](const auto& message) { return message.isMidiStart(); });
    passed &= contains(midi, [](const auto& message) { return message.isMidiClock(); });
    passed &= contains(midi, [](const auto& message)
    {
        return message.isNoteOn() && message.getChannel() == 3
               && message.getNoteNumber() == 60
               && message.getVelocity() == 110;
    });

    bool foundNoteOff = false;
    for (int block = 0; block < 12 && !foundNoteOff; ++block)
    {
        process(processor, midi);
        foundNoteOff = contains(midi, [](const auto& message)
        {
            return message.isNoteOff() && message.getChannel() == 3
                   && message.getNoteNumber() == 60;
        });
    }
    passed &= foundNoteOff;

    processor.stopSequencer();
    process(processor, midi);
    passed &= contains(midi, [](const auto& message) { return message.isMidiStop(); });
    passed &= !processor.isSequencerRunning();

    juce::MemoryBlock state;
    processor.getStateInformation(state);
    QY70ControllerAudioProcessor restored;
    restored.setStateInformation(state.getData(), static_cast<int>(state.getSize()));
    passed &= restored.isStepEnabled(0, 0);
    passed &= restored.trackNote(0) == 60;
    passed &= restored.trackVelocity(0) == 110;
    passed &= restored.trackChannel(0) == 3;
    passed &= restored.trackGate(0) == 80;

    if (!passed)
    {
        std::cerr << "FAILED: QY70 sequencer integration test\n";
        return EXIT_FAILURE;
    }

    std::cout << "All QY70 sequencer integration tests passed.\n";
    return EXIT_SUCCESS;
}
