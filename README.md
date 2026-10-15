# Wonder Melody Generator v3 — engineering update

This is a source-level upgrade for a JUCE VST3/Standalone plugin. It is **not a compiled binary** and has not been built or tested in FL Studio in this environment.

## Implemented in source
- Host transport-aware generated MIDI scheduling based on host PPQ/BPM, with step-boundary event offsets and swing on offbeat sixteenths.
- Generated melody, chord, bass and drum events, plus four-track MIDI file export.
- 32-voice dual-oscillator synth with selectable oscillator flavors, low-pass state-variable filters, attack/release controls and four basic sound presets.
- Improved chord inversions, a minor-key progression, bass roots, and more varied drum probabilities.
- Editable 16-step drum sequencer.
- Thread-safe AI response handoff: network thread stores the result under a lock; UI timer consumes and applies it on the message thread. Errors are shown in the status label.
- Optional local Python service for genuine model-based generation using the OpenAI API.

## Important caveats
The MIDI scheduler is a practical starter implementation and needs host-by-host testing. Some DAW/plugin hosts differ in transport APIs and MIDI instrument routing. MIDI note timing, seek/loop edge cases, note-off correctness and generated MIDI routing must be verified in FL Studio. The synth is intentionally lightweight and not a polished commercial instrument. AI results are validated for basic array dimensions but still require more defensive musical-data validation before release.

## Build and test on Windows
Requirements:
- Windows 10/11
- Visual Studio 2022 with “Desktop development with C++”
- CMake 3.22+
- JUCE 7 or 8 downloaded from https://juce.com/get-juce/

From Developer PowerShell, in this project folder:
```powershell
cmake -S . -B build -DJUCE_DIR="C:/JUCE"
cmake --build build --config Release
```
The VST3 should be under `build/WonderMelodyGenerator_artefacts/Release/VST3/` (exact path can vary).

Then copy the `.vst3` bundle to a VST3 plugin directory, open FL Studio, rescan plugins, and load it as an instrument. Test:
1. Play/stop and seek in the host; check no stuck notes.
2. Confirm melody/chord/bass/drum routing and channel behavior.
3. Test at multiple buffer sizes and sample rates.
4. Export MIDI, import into FL Studio, and verify four tracks, note lengths, tempo and swing.
5. Check plugin scan, save/restore state, and repeated open/close cycles.

## AI setup
1. Python 3.10+.
2. Set `OPENAI_API_KEY` in the environment; do not put API keys in the plugin or source.
3. `cd ai_service`
4. `pip install -r requirements.txt`
5. `python server.py`
6. Keep endpoint at `http://127.0.0.1:8765/generate`; enter a prompt and click AI generation.

AI requests send the user's prompt and musical parameters to the configured OpenAI model. Internet access and API billing/access are required. The local bridge binds to loopback only.

## Licensing
JUCE has its own licensing terms. Review them before distributing a commercial plugin.
