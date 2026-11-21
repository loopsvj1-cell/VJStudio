# VJ Studio 0.1 — Windows starter

This is the first source-code scaffold for an independent VJ application.

## Implemented in this starter
- Qt 6 desktop shell
- Media file import and library list
- Eight-slot clip deck
- Single-player video preview/playback
- Basic transport controls
- Four-layer UI placeholder
- Output 1 placeholder window

## Not implemented yet
- True multi-layer compositing
- Independent playback per clip/layer
- GPU shader/effects engine
- Independent fullscreen output routing
- Projection mapping / warping / edge blending
- MIDI / OSC
- Recording and project persistence

The UI placeholders are explicitly labeled so they are not mistaken for working production features.

## Requirements
- Windows 10/11 64-bit
- Qt 6.5+ with MSVC 2022 kit
- CMake 3.21+
- Visual Studio 2022 C++ Desktop Development workload

## Build with Qt Creator
1. Install Qt 6 (MSVC 2022 64-bit) and Qt Multimedia.
2. Open `CMakeLists.txt` in Qt Creator.
3. Select the MSVC 2022 64-bit kit.
4. Configure, build, and run.

## Build from a Visual Studio Developer PowerShell
```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

If CMake cannot find Qt, set `CMAKE_PREFIX_PATH` to your Qt MSVC installation, for example:
```powershell
cmake -S . -B build -DCMAKE_PREFIX_PATH="C:/Qt/6.8.*/msvc2022_64"
```

## Architecture direction
- UI: Qt Widgets
- Application/core logic: C++17
- First playback prototype: Qt Multimedia
- Planned media backend: FFmpeg integration
- Planned GPU compositor: Direct3D 11/12 (selected after GPU/driver validation)
- Persistence: SQLite
- Control: MIDI and OSC adapters

## Important performance note
The presence of an NVIDIA RTX GPU does not automatically guarantee hardware decoding or GPU compositing in this starter. Those capabilities must be implemented and validated separately against the exact GPU, driver, codec, and media format.
