# DepthHead

A VST3 spatializer plugin for FL Studio (and any other VST3 host). Place an instrument anywhere around a listener's head, move it closer or farther for depth, and raise or lower it with the height control. Includes presets for piano, guitar, vocal, bass, drums and pad.

Use headphones for the best effect.

## Get the plugin without installing anything

1. Push this repo to GitHub.
2. Open the **Actions** tab and wait for the **Build DepthHead** run to finish (about 10 minutes the first time).
3. Open the run and download the artifact for your system (`DepthHead-windows-latest` or `DepthHead-macos-latest`).
4. Windows: copy the `DepthHead.vst3` folder from `VST3` into `C:\Program Files\Common Files\VST3`.
   macOS: copy it into `/Library/Audio/Plug-Ins/VST3`.
5. In FL Studio: Options > Manage plugins > Find installed plugins. Add DepthHead to a mixer insert as an effect.

## Build locally

    cmake -B build -DCMAKE_BUILD_TYPE=Release
    cmake --build build --config Release

Output is in `build/DepthHead_artefacts/`. Requires CMake 3.22+ and a C++17 compiler.

## How it works

- Angle: time difference between the ears (up to 0.66 ms), level difference (up to 6 dB) and head shadow (low-pass on the far ear).
- Distance: attenuation, gentle high-frequency loss, and more reverb the farther the source is.
- Height: high-shelf boost or cut around 6.5 kHz, a simple stand-in for the pinna cue.
- Input is summed to mono, then placed in the stereo field.

`docs/prototype.html` is a browser prototype of the UI.
