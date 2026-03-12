# Flambi Atmosphere (JUCE VST3 + Standalone)

Flambi Atmosphere is a **lightweight atmospheric texture generator instrument** for ambient/dub-techno style backgrounds.

It is intentionally simple:
- 8 macro knobs
- a few buttons
- one-screen UI
- low CPU design
- MIDI playable + drone mode

## Features

- JUCE-based C++ plugin
- Targets:
  - **VST3** (for FL Studio)
  - **Standalone** app
- Two morphing oscillators (sine/triangle/saw), optional sub, and air/noise layer
- Slow drift and motion for evolving textures
- Soft saturation, filter, chorus-like motion, delay, reverb
- Startup preset + 8 included starter presets
- Randomize and Init buttons with tasteful ranges
- Automation-ready via `AudioProcessorValueTreeState`
- Plugin state save/load implemented

## Folder tree

```text
flambi/
├─ CMakeLists.txt
├─ README.md
├─ BUILD_WINDOWS.md
├─ SIMPLE_USER_GUIDE.md
├─ PRESETS.md
└─ Source/
   ├─ PluginProcessor.h
   ├─ PluginProcessor.cpp
   ├─ PluginEditor.h
   ├─ PluginEditor.cpp
   ├─ SynthVoice.h
   └─ SynthVoice.cpp
```

## Quick start (Windows x64)

See:
- `BUILD_WINDOWS.md` for exact build steps
- `SIMPLE_USER_GUIDE.md` for usage in plain language
- `PRESETS.md` for starter sound names and intent

## FL Studio VST3 location notes

After building, the VST3 is typically found under your build folder in a path like:

- `build/FlambiAtmosphere_artefacts/Release/VST3/Flambi Atmosphere.vst3`

Copy it to a VST3 folder scanned by FL Studio, commonly:

- `C:\Program Files\Common Files\VST3\`

Then rescan plugins in FL Studio Plugin Manager.

## Troubleshooting (quick)

- **Plugin not showing**: verify you built `VST3` target in `Release` x64 and scanned the correct folder.
- **Load fails**: ensure Microsoft VC++ runtime is installed.
- **No sound**: arm MIDI input, play notes, or enable DRONE.
- **Crackles**: raise FL Studio audio buffer size and reduce polyphony usage.

## License

Source provided as a practical starter project. Add your own license if distributing.
