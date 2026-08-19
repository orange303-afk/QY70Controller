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

std::vector<juce::MidiMessage> makeMultiPartVoiceSelection(int partNumber,
                                                           int bankMsb,
                                                           int bankLsb,
                                                           int programNumber,
                                                           std::uint8_t deviceNumber)
{
    return {
        makeMultiPartParameterChange(partNumber,
                                     MultiPartParameter::bankMsb,
                                     bankMsb,
                                     deviceNumber),
        makeMultiPartParameterChange(partNumber,
                                     MultiPartParameter::bankLsb,
                                     bankLsb,
                                     deviceNumber),
        makeMultiPartParameterChange(partNumber,
                                     MultiPartParameter::program,
                                     std::clamp(programNumber, 1, 128) - 1,
                                     deviceNumber)
    };
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
