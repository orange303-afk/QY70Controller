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
    return makeMultiPartParameterChange(partNumber,
                                        parameter,
                                        std::vector<std::uint8_t> { sevenBit(value) },
                                        deviceNumber);
}

juce::MidiMessage makeMultiPartParameterChange(int partNumber,
                                               MultiPartParameter parameter,
                                               const std::vector<std::uint8_t>& data,
                                               std::uint8_t deviceNumber)
{
    const auto partIndex = static_cast<std::uint8_t>(std::clamp(partNumber, 1, 32) - 1);
    const std::array<std::uint8_t, 3> address {
        0x08,
        partIndex,
        static_cast<std::uint8_t>(parameter)
    };

    return makeXgParameterChange(deviceNumber, address, data);
}

juce::MidiMessage makeEffectParameterChange(std::uint8_t address,
                                            const std::vector<std::uint8_t>& data,
                                            std::uint8_t deviceNumber)
{
    return makeXgParameterChange(deviceNumber, { 0x02, 0x01, address }, data);
}

std::vector<std::uint8_t> encodeDetuneTenthsHz(int tenthsHz)
{
    const auto encoded = std::clamp(tenthsHz, -128, 127) + 128;
    return {
        static_cast<std::uint8_t>((encoded >> 4) & 0x0F),
        static_cast<std::uint8_t>(encoded & 0x0F)
    };
}

std::vector<std::uint8_t> encode14Bit(int value)
{
    const auto encoded = std::clamp(value, 0, 16383);
    return {
        static_cast<std::uint8_t>((encoded >> 7) & 0x7F),
        static_cast<std::uint8_t>(encoded & 0x7F)
    };
}

const std::vector<EffectTypeDescriptor>& effectTypes(EffectBlock block)
{
    static const std::vector<EffectTypeDescriptor> reverbTypes {
        { "No Effect", 0x00, 0x00 },
        { "Rev Hall 1", 0x01, 0x00 }, { "Rev Hall 2", 0x01, 0x01 },
        { "Rev Room 1", 0x02, 0x00 }, { "Rev Room 2", 0x02, 0x01 },
        { "Rev Room 3", 0x02, 0x02 }, { "Rev Stage 1", 0x03, 0x00 },
        { "Rev Stage 2", 0x03, 0x01 }, { "Rev Plate", 0x04, 0x00 },
        { "Rev White Room", 0x10, 0x00 }, { "Rev Tunnel", 0x11, 0x00 },
        { "Rev Basement", 0x13, 0x00 }
    };
    static const std::vector<EffectTypeDescriptor> chorusTypes {
        { "No Effect", 0x00, 0x00 },
        { "Chorus 1", 0x41, 0x00 }, { "Chorus 2", 0x41, 0x01 },
        { "Chorus 3", 0x41, 0x02 }, { "Chorus 4", 0x41, 0x08 },
        { "Celeste 1", 0x42, 0x00 }, { "Celeste 2", 0x42, 0x01 },
        { "Celeste 3", 0x42, 0x02 }, { "Celeste 4", 0x42, 0x08 },
        { "Flanger 1", 0x43, 0x00 }, { "Flanger 2", 0x43, 0x01 },
        { "Flanger 3", 0x43, 0x02 }
    };
    static const std::vector<EffectTypeDescriptor> variationTypes {
        { "No Effect", 0x00, 0x00 },
        { "Rev Hall 1", 0x01, 0x00 }, { "Rev Hall 2", 0x01, 0x01 },
        { "Rev Room 1", 0x02, 0x00 }, { "Rev Room 2", 0x02, 0x01 },
        { "Rev Room 3", 0x02, 0x02 }, { "Rev Stage 1", 0x03, 0x00 },
        { "Rev Stage 2", 0x03, 0x01 }, { "Rev Plate", 0x04, 0x00 },
        { "Delay L,C,R", 0x05, 0x00 }, { "Delay L,R", 0x06, 0x00 },
        { "Echo", 0x07, 0x00 }, { "Cross Delay", 0x08, 0x00 },
        { "Early Reflection 1", 0x09, 0x00 }, { "Early Reflection 2", 0x09, 0x01 },
        { "Gate Reverb", 0x0A, 0x00 }, { "Reverse Gate", 0x0B, 0x00 },
        { "Karaoke Reverb 1", 0x14, 0x00 }, { "Karaoke Reverb 2", 0x14, 0x01 },
        { "Karaoke Reverb 3", 0x14, 0x02 }, { "Thru", 0x40, 0x00 },
        { "Chorus 1", 0x41, 0x00 }, { "Chorus 2", 0x41, 0x01 },
        { "Chorus 3", 0x41, 0x02 }, { "Chorus 4", 0x41, 0x08 },
        { "Celeste 1", 0x42, 0x00 }, { "Celeste 2", 0x42, 0x01 },
        { "Celeste 3", 0x42, 0x02 }, { "Celeste 4", 0x42, 0x08 },
        { "Flanger 1", 0x43, 0x00 }, { "Flanger 2", 0x43, 0x01 },
        { "Flanger 3", 0x43, 0x02 }, { "Symphonic", 0x44, 0x00 },
        { "Rotary Speaker", 0x45, 0x00 }, { "Tremolo", 0x46, 0x00 },
        { "Auto Pan", 0x47, 0x00 }, { "Phaser 1", 0x48, 0x00 },
        { "Phaser 2", 0x48, 0x01 }, { "Distortion", 0x49, 0x00 },
        { "Overdrive", 0x4A, 0x00 }, { "Amp Simulator", 0x4B, 0x00 },
        { "3 Band EQ", 0x4C, 0x00 }, { "2 Band EQ", 0x4D, 0x00 },
        { "Auto Wah", 0x4E, 0x00 }
    };

    switch (block)
    {
        case EffectBlock::reverb: return reverbTypes;
        case EffectBlock::chorus: return chorusTypes;
        case EffectBlock::variation: return variationTypes;
    }

    return variationTypes;
}

std::array<std::string_view, 16> effectParameterNames(EffectBlock block,
                                                     int typeIndex)
{
    using Names = std::array<std::string_view, 16>;
    static constexpr Names unused {};
    static constexpr Names reverbHall {
        "Reverb Time", "Diffusion", "Initial Delay", "HPF Cutoff",
        "LPF Cutoff", "Unused", "Unused", "Unused", "Unused", "Dry / Wet",
        "Reverb Delay", "Density", "ER / Reverb Balance", "Unused",
        "Feedback Level", "Unused"
    };
    static constexpr Names reverbSpace {
        "LFO Frequency", "LFO PM Depth", "Feedback Level", "Delay Offset",
        "Unused", "EQ Low Frequency", "EQ Low Gain", "EQ High Frequency",
        "EQ High Gain", "Dry / Wet", "Unused", "Unused", "Unused", "Unused",
        "Input Mode", "Unused"
    };
    static constexpr Names chorus {
        "LFO Frequency", "LFO Depth", "Feedback Level", "Delay Offset",
        "Unused", "EQ Low Frequency", "EQ Low Gain", "EQ High Frequency",
        "EQ High Gain", "Dry / Wet", "Unused", "Unused", "Unused",
        "LFO Phase Difference", "Unused", "Unused"
    };
    static constexpr Names flanger {
        "LFO Frequency", "LFO PM Depth", "Feedback Level", "Delay Offset",
        "Unused", "EQ Low Frequency", "EQ Low Gain", "EQ High Frequency",
        "EQ High Gain", "Dry / Wet", "Unused", "Unused", "Unused", "Unused",
        "Input Mode", "Unused"
    };
    static constexpr Names delayLcr {
        "Left Delay", "Right Delay", "Centre Delay", "Feedback Delay",
        "Feedback Level", "Centre Level", "High Damp", "Unused", "Unused",
        "Dry / Wet", "Unused", "Unused", "EQ Low Frequency", "EQ Low Gain",
        "EQ High Frequency", "EQ High Gain"
    };
    static constexpr Names delayLr {
        "Left Delay 1", "Left Feedback", "Right Delay 1", "Right Feedback",
        "High Damp", "Left Delay 2", "Right Delay 2", "Delay 2 Level",
        "Unused", "Dry / Wet", "Unused", "Unused", "EQ Low Frequency",
        "EQ Low Gain", "EQ High Frequency", "EQ High Gain"
    };
    static constexpr Names echo {
        "Left to Right Delay", "Right to Left Delay", "Feedback Level",
        "Input Select", "High Damp", "Unused", "Unused", "Unused", "Unused",
        "Dry / Wet", "Unused", "Unused", "EQ Low Frequency", "EQ Low Gain",
        "EQ High Frequency", "EQ High Gain"
    };
    static constexpr Names earlyReflection {
        "Type", "Room Size", "Diffusion", "Initial Delay", "Feedback Level",
        "HPF Cutoff", "LPF Cutoff", "Unused", "Unused", "Dry / Wet",
        "Liveness", "Density", "High Damp", "Unused", "Unused", "Unused"
    };
    static constexpr Names gateReverb {
        "Type", "Room Size", "Diffusion", "Initial Delay", "Feedback Level",
        "HPF Cutoff", "LPF Cutoff", "Unused", "Unused", "Dry / Wet",
        "Liveness", "Density", "High Damp", "Unused", "Unused", "Unused"
    };
    static constexpr Names variationChorus {
        "LFO Frequency", "LFO Depth", "Delay Offset", "Unused", "Unused",
        "EQ Low Frequency", "EQ Low Gain", "EQ High Frequency", "EQ High Gain",
        "Dry / Wet", "Unused", "Unused", "Unused", "Unused", "Unused", "Unused"
    };

    if (typeIndex <= 0)
        return unused;
    if (block == EffectBlock::reverb)
        return typeIndex <= 8 ? reverbHall : reverbSpace;
    if (block == EffectBlock::chorus)
        return typeIndex <= 8 ? chorus : flanger;

    if (typeIndex >= 1 && typeIndex <= 8) return reverbHall;
    if (typeIndex == 9) return delayLcr;
    if (typeIndex == 10) return delayLr;
    if (typeIndex == 11 || typeIndex == 12) return echo;
    if (typeIndex == 13 || typeIndex == 14) return earlyReflection;
    if (typeIndex == 15 || typeIndex == 16) return gateReverb;
    if (typeIndex >= 17 && typeIndex <= 19) return chorus;
    if (typeIndex >= 21 && typeIndex <= 28) return variationChorus;
    if (typeIndex >= 29 && typeIndex <= 31) return flanger;
    return unused;
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
