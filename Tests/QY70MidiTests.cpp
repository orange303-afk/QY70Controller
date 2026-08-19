#include "../Source/QY70Midi.h"

#include <cstdlib>
#include <iostream>
#include <vector>

namespace
{
bool expectRaw(const juce::MidiMessage& message,
               const std::vector<std::uint8_t>& expected,
               const char* testName)
{
    const std::vector<std::uint8_t> actual(message.getRawData(),
                                            message.getRawData() + message.getRawDataSize());
    if (actual == expected)
        return true;

    std::cerr << "FAILED: " << testName << '\n';
    return false;
}
} // namespace

int main()
{
    bool passed = true;

    passed &= expectRaw(qy70::makeMultiPartParameterChange(
                            1, qy70::MultiPartParameter::filterCutoff, 127),
                        { 0xF0, 0x43, 0x10, 0x4C, 0x08, 0x00, 0x18, 0x7F, 0xF7 },
                        "part 1 cutoff SysEx");

    passed &= expectRaw(qy70::makeMultiPartParameterChange(
                            32, qy70::MultiPartParameter::volume, 100, 3),
                        { 0xF0, 0x43, 0x13, 0x4C, 0x08, 0x1F, 0x0B, 0x64, 0xF7 },
                        "part 32 volume SysEx");

    passed &= expectRaw(qy70::makeMultiPartParameterChange(
                            1, qy70::MultiPartParameter::bankLsb, 40),
                        { 0xF0, 0x43, 0x10, 0x4C, 0x08, 0x00, 0x02, 0x28, 0xF7 },
                        "part 1 bank LSB SysEx");

    passed &= expectRaw(qy70::makeMultiPartParameterChange(
                            1, qy70::MultiPartParameter::program, 127),
                        { 0xF0, 0x43, 0x10, 0x4C, 0x08, 0x00, 0x03, 0x7F, 0xF7 },
                        "part 1 patch 128 SysEx");

    passed &= expectRaw(qy70::makeXgSystemOn(),
                        { 0xF0, 0x43, 0x10, 0x4C, 0x00, 0x00, 0x7E, 0x00, 0xF7 },
                        "XG System On");

    const auto voiceSelection = qy70::makeChannelVoiceSelection(1, 0, 10, 54);
    passed &= voiceSelection.size() == 3;
    passed &= expectRaw(voiceSelection[0],
                        { 0xB0, 0x00, 0x00 },
                        "voice selection bank MSB first");
    passed &= expectRaw(voiceSelection[1],
                        { 0xB0, 0x20, 0x0A },
                        "voice selection bank LSB second");
    passed &= expectRaw(voiceSelection[2],
                        { 0xC0, 0x35 },
                        "voice selection program last");

    passed &= expectRaw(qy70::makeXgParameterRequest(0, { 0x08, 0x00, 0x18 }),
                        { 0xF0, 0x43, 0x30, 0x4C, 0x08, 0x00, 0x18, 0xF7 },
                        "cutoff parameter request");

    const auto nrpn = qy70::makeNrpn(1, 0x01, 0x20, 0x40);
    passed &= nrpn.size() == 3;
    passed &= expectRaw(nrpn[0], { 0xB0, 0x63, 0x01 }, "NRPN MSB");
    passed &= expectRaw(nrpn[1], { 0xB0, 0x62, 0x20 }, "NRPN LSB");
    passed &= expectRaw(nrpn[2], { 0xB0, 0x06, 0x40 }, "NRPN value");

    if (!passed)
        return EXIT_FAILURE;

    std::cout << "All QY70 MIDI protocol tests passed.\n";
    return EXIT_SUCCESS;
}
