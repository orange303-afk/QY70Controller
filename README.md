# QY70 Controller

A modern MIDI editor/controller for the Yamaha QY70 music sequencer and XG tone
generator. The project currently builds as VST3, Audio Unit and standalone using
JUCE 8 and CMake.

## Current status

The first vertical slice translates DAW-automatable controls into Yamaha XG
Multi Part SysEx:

- part selection 1–32;
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
