#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

#include <array>
#include <cstdint>
#include <vector>

namespace qy70
{
enum class MultiPartParameter : std::uint8_t
{
    bankMsb = 0x01,
    bankLsb = 0x02,
    program = 0x03,
    receiveChannel = 0x04,
    noteShift = 0x08,
    volume = 0x0B,
    pan = 0x0E,
    dryLevel = 0x11,
    chorusSend = 0x12,
    reverbSend = 0x13,
    variationSend = 0x14,
    vibratoRate = 0x15,
    vibratoDepth = 0x16,
    vibratoDelay = 0x17,
    filterCutoff = 0x18,
    filterResonance = 0x19,
    attackTime = 0x1A,
    decayTime = 0x1B,
    releaseTime = 0x1C,
    portamentoSwitch = 0x67,
    portamentoTime = 0x68
};

juce::MidiMessage makeXgParameterChange(std::uint8_t deviceNumber,
                                        const std::array<std::uint8_t, 3>& address,
                                        const std::vector<std::uint8_t>& data);

juce::MidiMessage makeXgSystemOn(std::uint8_t deviceNumber = 0);
juce::MidiMessage makeGeneralMidiSystemOn();

juce::MidiMessage makeMultiPartParameterChange(int partNumber,
                                               MultiPartParameter parameter,
                                               int value,
                                               std::uint8_t deviceNumber = 0);

std::vector<juce::MidiMessage> makeChannelVoiceSelection(int midiChannel,
                                                         int bankMsb,
                                                         int bankLsb,
                                                         int programNumber);

std::vector<int> validProgramsForBank(int bankMsb);
std::vector<int> validLsbValues(int bankMsb, int programNumber);

juce::MidiMessage makeXgParameterRequest(std::uint8_t deviceNumber,
                                         const std::array<std::uint8_t, 3>& address);

std::vector<juce::MidiMessage> makeNrpn(int midiChannel,
                                       std::uint8_t parameterMsb,
                                       std::uint8_t parameterLsb,
                                       std::uint8_t value);
} // namespace qy70
