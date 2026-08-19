# Architecture

## Product shape

QY70 Controller is a MIDI-only JUCE plug-in. It does not emulate or process the
QY70 audio engine. DAW automation is translated to Yamaha XG parameter-change
messages and emitted through the plug-in MIDI output.

The project deliberately separates three layers:

1. `QY70Midi` builds byte-exact CC, NRPN and Yamaha XG SysEx messages.
2. `PluginProcessor` maps automatable parameters to QY70 part addresses.
3. `PluginEditor` exposes the current vertical slice without knowing SysEx bytes.

## Current vertical slice

- Select XG part 1–32.
- Automate volume, pan, cutoff, resonance, attack, release and effect sends.
- Send a complete snapshot for the selected part.
- Request the corresponding values from the QY70.
- Preserve state in the DAW project.

Incoming parameter-response parsing and direct physical MIDI-port ownership are
planned next. The initial plug-in emits MIDI through the host so routing remains
explicit and testable.

## Real-time policy

Parameter listeners only set atomic dirty bits. MIDI messages are emitted during
`processBlock`. Continuous UI movement is therefore coalesced to at most one
message per parameter per audio block.
