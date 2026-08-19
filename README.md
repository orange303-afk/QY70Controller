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

Press **Enable XG** after connecting the QY70. The controller sends XG System
On, waits 60 ms for the tone generator to reset, then resends the current voice
and part parameters. XG normal voices use MSB 0; MSB 64 selects SFX normal
voices, MSB 126 XG SFX kits, and MSB 127 XG drum kits. LSB is a variation for a
particular program, not a complete parallel bank: only combinations listed in
the QY70 XG Normal Voice List produce a different voice. For example, Program 1
supports LSB 0 (GrandPno), 1 (GrndPnoK), 18 (MelloGrP), 40 (PianoStr), and 41
(Dream).

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
