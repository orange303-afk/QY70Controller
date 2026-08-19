# QY70 Controller

A modern MIDI editor/controller for the Yamaha QY70 music sequencer and XG tone
generator. The project currently builds as VST3, Audio Unit and standalone using
JUCE 8 and CMake.

## Current status

The first vertical slice translates DAW-automatable controls into Yamaha XG
Multi Part SysEx:

- part/MIDI-channel selection 1–16;
- bank MSB/LSB and patch selection 1–128;
- volume and pan;
- filter cutoff and resonance;
- attack and release;
- chorus, reverb and variation sends;
- parameter requests and complete part snapshots;
- byte-level protocol tests.

This is an early development build. MIDI is emitted through the host output;
bidirectional response parsing and direct physical MIDI-port selection are not
implemented yet.

Part, bank and patch selectors use discrete previous/next buttons. Part is the
live MIDI channel: incoming channel messages are remapped to the selected part.
Changing either bank value sends standard MIDI Bank MSB (CC 0), Bank LSB (CC
32), then Program Change on that channel.

The coloured **XG ON / XG OFF** button switches between XG System On and GM
System On. After either reset, the controller waits 60 ms and resends the
current voice and part parameters. This is the commanded state; hardware state
confirmation requires future MIDI-response parsing.

Selectors reject silent/unsupported combinations from the official QY70 voice
tables. XG normal voices use MSB 0; MSB 64 selects SFX voices, MSB 126 XG SFX
kits, and MSB 127 XG drum kits. Patch choices automatically follow the selected
MSB. LSB choices automatically follow the selected normal-voice patch because
LSB is a per-program variation, not a complete parallel bank. For example,
Program 1 offers LSB 0 (GrandPno), 1 (GrndPnoK), 18 (MelloGrP), 40 (PianoStr),
and 41 (Dream). Hover a Part, Bank, LSB, or Patch number and use the mouse wheel
for rapid selection.

The UI calls these selectors **Voice Mode** and **XG Variation** and displays
the selected voice name from the complete QY70 catalog: 474 XG normal
program/variation combinations, 45 SFX voices, 18 drum kits, and two SFX kits.
Click the voice-name field to open a browser grouped by instrument family and
select any voice directly. The adjacent arrow buttons step through the complete
catalog without opening the menu.

## Requirements

- macOS or Windows development environment
- CMake 3.22+
- C++17 compiler
- JUCE submodule

## Build

```sh
git submodule update --init --recursive
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --target QY70Controller_Standalone
cmake --build build --target QY70Controller_VST3
ctest --test-dir build --output-on-failure
```

After a successful VST3 build, CMake automatically copies the plug-in to
`~/Library/Audio/Plug-Ins/VST3` on macOS.

After a successful Standalone build, macOS automatically opens the generated
`QY70 Controller.app`. To disable this behaviour, configure with
`-DQY70_LAUNCH_STANDALONE_AFTER_BUILD=OFF`.

## Hardware connection

The QY70 has 5-pin MIDI IN/OUT rather than USB. Use a bidirectional USB–MIDI
interface that passes SysEx correctly, set the QY70 HOST SELECT switch to MIDI,
and route the plug-in MIDI output to that interface in the host.

## Documentation

- [Architecture](docs/ARCHITECTURE.md)
- [Yamaha QY70 List Book / MIDI Data Format](https://usa.yamaha.com/files/download/other_assets/6/317936/QY70E2.PDF)
- [Yamaha QY70 Owner's Manual](https://usa.yamaha.com/files/download/other_assets/8/318058/QY70E1.PDF)

## License

MIT
