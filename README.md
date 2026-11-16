# MASTERING PRO 2.0 — Windows 10 x64 / FL Studio / VST3

This is a complete JUCE source project prepared for building on a Windows 10/11 x64 machine.

## Features in this source version
- Five-band EQ: low shelf, low-mid, mid, high-mid, high shelf (fixed center frequencies)
- Broadband compressor with threshold, ratio, attack and release
- Mid/side stereo width control
- Soft saturation
- Output peak ceiling with release control
- FFT-based real-time spectrum display
- Loudness readouts (integrated and short-term estimates)
- Sample peak readout and stereo correlation
- Six starting presets: Transparent, Streaming, Pop/Vocal, EDM, Hip-Hop, Acoustic
- A/B settings memory (STORE/RECALL A and B)
- Host automation and DAW state recall

## Honest limitations
This is an advanced prototype/source build, not a commercially certified mastering processor.
The loudness readout is a **rough RMS-derived estimate**, not a standards-compliant ITU-R BS.1770 / EBU R128 LUFS meter. The limiter ceiling is a sample-peak clamp with release smoothing; it is **not yet a verified oversampled true-peak limiter**, despite the ceiling control label. The spectrum is a basic FFT display. EQ frequencies/Q are fixed. Always audition, level-match, check mono compatibility, and verify final loudness/true peak with a trusted meter before release.

## Build requirements
- Windows 10/11 64-bit
- Visual Studio 2022, workload **Desktop development with C++**
- CMake 3.22 or newer
- Git
- Internet on first build (JUCE 8.0.6 is fetched from GitHub)

## Build
Open **x64 Native Tools Command Prompt for VS 2022** in this folder, then run:

```bat
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

Expected output:
`build\\MASTERING_PRO_2_artefacts\\Release\\VST3\\MASTERING PRO 2.0.vst3`

Copy the `.vst3` bundle to:
`C:\\Program Files\\Common Files\\VST3`

Restart FL Studio → Options → Manage plugins → Find plugins.

## Files
- `Source/PluginProcessor.*` — audio processing, parameter state, FFT and meters
- `Source/PluginEditor.*` — interface, spectrum, presets and A/B
- `CMakeLists.txt` — JUCE VST3 build
- `BUILD_WINDOWS.bat` — one-click build launcher

## Safety
Start with conservative settings and low monitor volume. Disable/adjust processing if you hear pumping, distortion, clipping or unstable stereo image. Presets are starting points, not mastering recommendations for every mix.
