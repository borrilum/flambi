# BUILD_WINDOWS.md

## Requirements

1. Windows 10/11 x64
2. Visual Studio 2022 (Desktop development with C++)
3. CMake 3.22+
4. Git

## 1) Clone project

```bash
git clone <your-repo-url> flambi
cd flambi
```

## 2) Configure (Visual Studio 2022, x64)

```bash
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
```

> JUCE will be downloaded automatically by CMake FetchContent.

## 3) Build Release

```bash
cmake --build build --config Release
```

## 4) Output locations

Common output path (depends on generator/JUCE version):

- `build/FlambiAtmosphere_artefacts/Release/VST3/Flambi Atmosphere.vst3`
- `build/FlambiAtmosphere_artefacts/Release/Standalone/Flambi Atmosphere.exe`

If not present there, search inside `build/` for `*.vst3`.

## 5) Install VST3 for FL Studio

Copy:

- `Flambi Atmosphere.vst3`

To:

- `C:\Program Files\Common Files\VST3\`

(or any custom VST3 folder you scan in FL Studio)

## 6) Scan in FL Studio

1. Open **Options > Manage plugins**
2. Ensure your VST3 path is in plugin search paths
3. Click **Find installed plugins**
4. Add **Flambi Atmosphere** to your favorites/plugin database

## Common scan issues

- Built `Debug` instead of `Release` (build Release for normal use)
- Built x86 by mistake (must be x64)
- Wrong plugin folder path in FL scan list
- Missing VC++ runtime redistributable
