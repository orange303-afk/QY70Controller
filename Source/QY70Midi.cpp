#include "QY70Midi.h"

#include <algorithm>

namespace qy70
{
namespace
{
std::uint8_t sevenBit(int value)
{
    return static_cast<std::uint8_t>(std::clamp(value, 0, 127));
}
} // namespace

juce::MidiMessage makeXgParameterChange(std::uint8_t deviceNumber,
                                        const std::array<std::uint8_t, 3>& address,
                                        const std::vector<std::uint8_t>& data)
{
    std::vector<std::uint8_t> payload;
    payload.reserve(6 + data.size());
    payload.push_back(0x43);
    payload.push_back(static_cast<std::uint8_t>(0x10 | (deviceNumber & 0x0F)));
    payload.push_back(0x4C);
    payload.insert(payload.end(), address.begin(), address.end());

    for (const auto value : data)
        payload.push_back(sevenBit(value));

    return juce::MidiMessage::createSysExMessage(payload.data(),
                                                 static_cast<int>(payload.size()));
}

juce::MidiMessage makeXgSystemOn(std::uint8_t deviceNumber)
{
    return makeXgParameterChange(deviceNumber, { 0x00, 0x00, 0x7E }, { 0x00 });
}

juce::MidiMessage makeGeneralMidiSystemOn()
{
    constexpr std::array<std::uint8_t, 4> payload { 0x7E, 0x7F, 0x09, 0x01 };
    return juce::MidiMessage::createSysExMessage(payload.data(),
                                                 static_cast<int>(payload.size()));
}

juce::MidiMessage makeMultiPartParameterChange(int partNumber,
                                               MultiPartParameter parameter,
                                               int value,
                                               std::uint8_t deviceNumber)
{
    const auto partIndex = static_cast<std::uint8_t>(std::clamp(partNumber, 1, 32) - 1);
    const std::array<std::uint8_t, 3> address {
        0x08,
        partIndex,
        static_cast<std::uint8_t>(parameter)
    };

    return makeXgParameterChange(deviceNumber, address, { sevenBit(value) });
}

std::vector<juce::MidiMessage> makeChannelVoiceSelection(int midiChannel,
                                                         int bankMsb,
                                                         int bankLsb,
                                                         int programNumber)
{
    const auto channel = std::clamp(midiChannel, 1, 16);

    return {
        juce::MidiMessage::controllerEvent(channel, 0, sevenBit(bankMsb)),
        juce::MidiMessage::controllerEvent(channel, 32, sevenBit(bankLsb)),
        juce::MidiMessage::programChange(channel,
                                         std::clamp(programNumber, 1, 128) - 1)
    };
}

std::vector<int> validProgramsForBank(int bankMsb)
{
    if (bankMsb == 64)
        return { 1, 2, 3, 4, 5, 6, 17, 33, 34, 35, 36, 37, 38, 49, 50, 51,
                 55, 56, 65, 66, 67, 68, 69, 70, 71, 81, 82, 83, 84, 85, 86,
                 87, 88, 89, 90, 91, 97, 98, 99, 100, 101, 113, 114, 115, 116 };

    if (bankMsb == 126)
        return { 1, 2 };

    if (bankMsb == 127)
        return { 1, 2, 3, 4, 9, 10, 17, 18, 25, 26, 27, 28, 29, 30,
                 33, 34, 41, 49 };

    std::vector<int> programs(128);
    for (int i = 0; i < 128; ++i)
        programs[static_cast<std::size_t>(i)] = i + 1;
    return programs;
}

std::vector<int> validLsbValues(int bankMsb, int programNumber)
{
    if (bankMsb != 0)
        return { 0 };

    switch (std::clamp(programNumber, 1, 128))
    {
        case 1: return { 0, 1, 18, 40, 41 };
        case 2: return { 0, 1 };
        case 3: return { 0, 1, 32, 40, 41 };
        case 4: return { 0, 1 };
        case 5: return { 0, 1, 18, 32, 40, 45, 64 };
        case 6: return { 0, 1, 32, 33, 34, 40, 41, 42, 45 };
        case 7: return { 0, 1, 25, 35 };
        case 8: return { 0, 1, 27, 64, 65 };
        case 11: return { 0, 64 };
        case 12: return { 0, 1, 45 };
        case 13: return { 0, 1, 64, 97, 98 };
        case 15: return { 0, 96, 97 };
        case 16: return { 0, 35, 96, 97 };
        case 17: return { 0, 32, 33, 34, 35, 36, 37, 38, 40, 64, 65, 66, 67 };
        case 18: return { 0, 24, 32, 33, 37 };
        case 19: return { 0, 64, 65, 66 };
        case 20: return { 0, 32, 35, 40, 64, 65 };
        case 21: return { 0, 40 };
        case 22: return { 0, 32 };
        case 23: return { 0, 32 };
        case 24: return { 0, 64 };
        case 25: return { 0, 16, 25, 43, 96 };
        case 26: return { 0, 16, 35, 40, 41, 96 };
        case 27: return { 0, 18, 32, 96 };
        case 28: return { 0, 32, 65, 66 };
        case 29: return { 0, 40, 41, 43, 45, 96 };
        case 30: return { 0, 43 };
        case 31: return { 0, 12, 24, 35, 36, 37, 38, 40, 41, 43, 45 };
        case 32: return { 0, 65, 66 };
        case 33: return { 0, 40, 45 };
        case 34: return { 0, 18, 27, 40, 43, 45, 64, 65 };
        case 35: return { 0, 28 };
        case 36: return { 0, 32, 33, 34, 96, 97 };
        case 37: return { 0, 27, 32, 64, 65 };
        case 38: return { 0, 43 };
        case 39: return { 0, 18, 20, 24, 27, 35, 40, 64, 65, 66, 67, 68, 96 };
        case 40: return { 0, 6, 12, 18, 19, 32, 40, 41, 64, 65, 66, 67 };
        case 41: return { 0, 8 };
        case 45: return { 0, 8, 40 };
        case 47: return { 0, 40 };
        case 49: return { 0, 3, 8, 24, 35, 40, 41, 42, 45 };
        case 50: return { 0, 3, 8, 40, 41, 64, 65 };
        case 51: return { 0, 27, 64, 65 };
        case 53: return { 0, 3, 16, 32, 40 };
        case 55: return { 0, 40, 41, 64 };
        case 56: return { 0, 35, 64, 68, 70, 71, 72, 73 };
        case 57: return { 0, 16, 17, 32 };
        case 58: return { 0, 18 };
        case 59: return { 0, 16 };
        case 61: return { 0, 6, 32, 37 };
        case 62: return { 0, 3, 35, 40, 41, 42 };
        case 63: return { 0, 12, 20, 24, 27, 32, 40, 45, 64 };
        case 64: return { 0, 18, 40, 41, 45, 64 };
        case 66: return { 0, 40, 43 };
        case 67: return { 0, 40, 41, 64 };
        case 81: return { 0, 6, 8, 18, 19, 64, 65, 66, 67 };
        case 82: return { 0, 6, 8, 18, 19, 20, 24, 25, 27, 32, 35, 36, 40, 41, 45, 64, 96 };
        case 83: return { 0, 65 };
        case 84: return { 0, 64, 65 };
        case 85: return { 0, 64, 65, 66 };
        case 86: return { 0, 24, 64 };
        case 87: return { 0, 35 };
        case 88: return { 0, 16, 64, 65 };
        case 89: return { 0, 64 };
        case 90: return { 0, 16, 17, 18, 64, 65 };
        case 91: return { 0, 64, 65, 66, 67 };
        case 92: return { 0, 64, 66, 67 };
        case 93: return { 0, 64, 65 };
        case 94: return { 0, 64, 65 };
        case 96: return { 0, 20, 27, 64, 66 };
        case 97: return { 0, 45, 64, 65, 66 };
        case 98: return { 0, 27, 64 };
        case 99: return { 0, 12, 14, 18, 35, 40, 41, 42, 64, 65, 66, 67, 68, 69, 70, 71, 72 };
        case 100: return { 0, 18, 19, 40, 64, 65, 66, 67 };
        case 101: return { 0, 64, 96 };
        case 102: return { 0, 64, 65, 66, 67, 68, 70, 71, 96 };
        case 103: return { 0, 8, 14, 64, 65, 66, 67, 68, 69 };
        case 104: return { 0, 64 };
        case 105: return { 0, 32, 35, 96, 97 };
        case 106: return { 0, 28, 96, 97, 98 };
        case 108: return { 0, 96, 97 };
        case 112: return { 0, 64, 96, 97 };
        case 113: return { 0, 96, 97, 98, 99, 100, 101 };
        case 115: return { 0, 97, 98 };
        case 116: return { 0, 96 };
        case 117: return { 0, 96 };
        case 118: return { 0, 64, 65, 66 };
        case 119: return { 0, 64, 65 };
        default: return { 0 };
    }
}

juce::MidiMessage makeXgParameterRequest(std::uint8_t deviceNumber,
                                         const std::array<std::uint8_t, 3>& address)
{
    const std::array<std::uint8_t, 6> payload {
        0x43,
        static_cast<std::uint8_t>(0x30 | (deviceNumber & 0x0F)),
        0x4C,
        address[0],
        address[1],
        address[2]
    };

    return juce::MidiMessage::createSysExMessage(payload.data(),
                                                 static_cast<int>(payload.size()));
}

std::vector<juce::MidiMessage> makeNrpn(int midiChannel,
                                       std::uint8_t parameterMsb,
                                       std::uint8_t parameterLsb,
                                       std::uint8_t value)
{
    const auto channel = std::clamp(midiChannel, 1, 16);

    return {
        juce::MidiMessage::controllerEvent(channel, 99, sevenBit(parameterMsb)),
        juce::MidiMessage::controllerEvent(channel, 98, sevenBit(parameterLsb)),
        juce::MidiMessage::controllerEvent(channel, 6, sevenBit(value))
    };
}
} // namespace qy70
