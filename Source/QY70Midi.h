#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

#include <array>
#include <cstdint>
#include <string_view>
#include <vector>

namespace qy70
{
enum class MultiPartParameter : std::uint8_t
{
    bankMsb = 0x01,
    bankLsb = 0x02,
    program = 0x03,
    receiveChannel = 0x04,
    monoPolyMode = 0x05,
    sameNoteKeyAssign = 0x06,
    partMode = 0x07,
    noteShift = 0x08,
    detune = 0x09,
    volume = 0x0B,
    velocitySenseDepth = 0x0C,
    velocitySenseOffset = 0x0D,
    pan = 0x0E,
    noteLimitLow = 0x0F,
    noteLimitHigh = 0x10,
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
    modulationWheelPitch = 0x1D,
    modulationWheelFilter = 0x1E,
    modulationWheelAmplitude = 0x1F,
    modulationWheelLfoPitchDepth = 0x20,
    modulationWheelLfoFilterDepth = 0x21,
    modulationWheelLfoAmplitudeDepth = 0x22,
    pitchBendPitch = 0x23,
    pitchBendFilter = 0x24,
    pitchBendAmplitude = 0x25,
    pitchBendLfoPitchDepth = 0x26,
    pitchBendLfoFilterDepth = 0x27,
    pitchBendLfoAmplitudeDepth = 0x28,
    aftertouchPitch = 0x4D,
    aftertouchFilter = 0x4E,
    aftertouchAmplitude = 0x4F,
    aftertouchLfoPitchDepth = 0x50,
    aftertouchLfoFilterDepth = 0x51,
    aftertouchLfoAmplitudeDepth = 0x52,
    portamentoSwitch = 0x67,
    portamentoTime = 0x68,
    pitchEgInitialLevel = 0x69,
    pitchEgAttackTime = 0x6A,
    pitchEgReleaseLevel = 0x6B,
    pitchEgReleaseTime = 0x6C
};

enum class EffectBlock
{
    reverb,
    chorus,
    variation
};

struct EffectTypeDescriptor
{
    std::string_view name;
    std::uint8_t msb;
    std::uint8_t lsb;
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

juce::MidiMessage makeMultiPartParameterChange(int partNumber,
                                               MultiPartParameter parameter,
                                               const std::vector<std::uint8_t>& data,
                                               std::uint8_t deviceNumber = 0);

juce::MidiMessage makeEffectParameterChange(std::uint8_t address,
                                            const std::vector<std::uint8_t>& data,
                                            std::uint8_t deviceNumber = 0);

std::vector<std::uint8_t> encodeDetuneTenthsHz(int tenthsHz);
std::vector<std::uint8_t> encode14Bit(int value);
const std::vector<EffectTypeDescriptor>& effectTypes(EffectBlock block);
std::array<std::string_view, 16> effectParameterNames(EffectBlock block,
                                                     int typeIndex);

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
