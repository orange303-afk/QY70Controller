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
- Edit the complete useful QY70 Multi Part surface: play mode, tuning, velocity,
  note limits, vibrato, EG, controller response, portamento and Pitch EG.
- Edit global Effect 1 Reverb, Chorus and Variation/Delay algorithms, routing,
  return levels and algorithm parameters.
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

Multi Part values use display-friendly signed ranges in the host. The processor
converts them to Yamaha's centre-at-64 representation; Detune is packed as two
four-bit data bytes. Variation parameters 1-10 use two 7-bit data bytes.

Effect controls are global rather than per Part. XG System On is the authoritative
factory reset for that engine. The Reset Edits action therefore clears pending
effect writes before issuing XG System On, then resends the selected Part's
default edit state without overwriting Yamaha's algorithm defaults with zeroes.
