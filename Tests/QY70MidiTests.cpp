#include "../Source/QY70Midi.h"
#include "../Source/VoiceCatalog.h"

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

    passed &= expectRaw(qy70::makeMultiPartParameterChange(
                            1, qy70::MultiPartParameter::detune,
                            qy70::encodeDetuneTenthsHz(0)),
                        { 0xF0, 0x43, 0x10, 0x4C, 0x08, 0x00, 0x09,
                          0x08, 0x00, 0xF7 },
                        "part 1 zero detune two-byte SysEx");
    passed &= qy70::encodeDetuneTenthsHz(-128)
              == std::vector<std::uint8_t>({ 0x00, 0x00 });
    passed &= qy70::encodeDetuneTenthsHz(127)
              == std::vector<std::uint8_t>({ 0x0F, 0x0F });
    passed &= qy70::encode14Bit(0x1234)
              == std::vector<std::uint8_t>({ 0x24, 0x34 });

    passed &= expectRaw(qy70::makeMultiPartParameterChange(
                            4, qy70::MultiPartParameter::pitchEgInitialLevel, 64),
                        { 0xF0, 0x43, 0x10, 0x4C, 0x08, 0x03, 0x69, 0x40, 0xF7 },
                        "part 4 pitch EG initial level SysEx");
    passed &= expectRaw(qy70::makeEffectParameterChange(0x00, { 0x01, 0x00 }),
                        { 0xF0, 0x43, 0x10, 0x4C, 0x02, 0x01, 0x00,
                          0x01, 0x00, 0xF7 },
                        "reverb Hall 1 type SysEx");
    passed &= expectRaw(qy70::makeEffectParameterChange(0x40, { 0x05, 0x00 }),
                        { 0xF0, 0x43, 0x10, 0x4C, 0x02, 0x01, 0x40,
                          0x05, 0x00, 0xF7 },
                        "variation Delay LCR type SysEx");
    passed &= qy70::effectTypes(qy70::EffectBlock::reverb).size() == 12;
    passed &= qy70::effectTypes(qy70::EffectBlock::chorus).size() == 12;
    passed &= qy70::effectTypes(qy70::EffectBlock::variation).size() == 44;
    passed &= qy70::effectParameterNames(qy70::EffectBlock::reverb, 1)[0]
              == "Reverb Time";
    passed &= qy70::effectParameterNames(qy70::EffectBlock::chorus, 1)[13]
              == "LFO Phase Difference";
    passed &= qy70::effectParameterNames(qy70::EffectBlock::variation, 9)[0]
              == "Left Delay";

    passed &= expectRaw(qy70::makeTransportStart(), { 0xFA }, "MIDI Start");
    passed &= expectRaw(qy70::makeTransportContinue(), { 0xFB }, "MIDI Continue");
    passed &= expectRaw(qy70::makeTransportStop(), { 0xFC }, "MIDI Stop");
    passed &= expectRaw(qy70::makeTimingClock(), { 0xF8 }, "MIDI Clock");
    passed &= expectRaw(qy70::makeSongSelect(64), { 0xF3, 0x3F },
                        "QY70 user pattern 64 select");
    passed &= expectRaw(qy70::makeSectionControl(qy70::PatternSection::mainA),
                        { 0xF0, 0x43, 0x7E, 0x00, 0x09, 0x01, 0xF7 },
                        "QY70 Main A section control");

    passed &= expectRaw(qy70::makeXgSystemOn(),
                        { 0xF0, 0x43, 0x10, 0x4C, 0x00, 0x00, 0x7E, 0x00, 0xF7 },
                        "XG System On");
    passed &= expectRaw(qy70::makeGeneralMidiSystemOn(),
                        { 0xF0, 0x7E, 0x7F, 0x09, 0x01, 0xF7 },
                        "GM System On");

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

    passed &= qy70::validLsbValues(0, 1) == std::vector<int>({ 0, 1, 18, 40, 41 });
    passed &= qy70::validLsbValues(64, 1) == std::vector<int>({ 0 });
    passed &= qy70::validProgramsForBank(126) == std::vector<int>({ 1, 2 });
    passed &= qy70::validProgramsForBank(127)
              == std::vector<int>({ 1, 2, 3, 4, 9, 10, 17, 18, 25, 26, 27,
                                    28, 29, 30, 33, 34, 41, 49 });
    passed &= qy70::voiceModeName(0) == "XG Normal";
    passed &= qy70::voiceName(0, 41, 1) == "Dream";
    passed &= qy70::voiceName(64, 0, 87) == "JetPlane";
    passed &= qy70::voiceName(126, 0, 2) == "SFX Kit 2";
    passed &= qy70::voiceName(127, 0, 41) == "Brush Kit";
    passed &= qy70::voiceCategory(0, 1) == "Piano";
    passed &= qy70::voiceCategory(0, 82) == "Synth Lead";
    passed &= qy70::voiceCategory(127, 41) == "Drum Kits";
    passed &= qy70::voiceCatalog().size() == 539;

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
