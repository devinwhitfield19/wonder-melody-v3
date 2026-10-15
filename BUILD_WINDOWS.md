# Windows build helper

Run PowerShell from this folder after installing Visual Studio 2022 (Desktop development with C++), CMake 3.22+, and JUCE 7/8.

```powershell
Set-ExecutionPolicy -Scope Process Bypass
.\build_windows.ps1 -JuceDir "C:\JUCE" -Configuration Release
```

To copy the VST3 bundle to the per-user VST3 folder after a successful build:

```powershell
.\build_windows.ps1 -JuceDir "C:\JUCE" -Configuration Release -InstallToUserVst3
```

If JUCE is elsewhere, replace `C:\JUCE` with the folder that contains JUCE's `CMakeLists.txt`.

**Important:** This script automates configuration, compilation, artifact discovery and optional copying. It cannot certify correct audio/MIDI behavior; test playback, stop/start, seek/loop, MIDI note-offs and MIDI export in FL Studio before relying on the plugin.
