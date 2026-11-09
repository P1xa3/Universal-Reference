# Universal Reference

**Native macOS AU/VST3 plugin project for Logic Pro.** Experimental v0.2 UI redesign, built with JUCE 8/C++17 and CMake. Warm cream, iPad-inspired native JUCE interface. Source can be uploaded directly to a new GitHub repository. Builds run automatically on `push`, `pull_request`, or manual dispatch.

> **Important:** This project has not yet been compiled or audio-validated on a Mac. The build workflow exists but has not been executed in this delivery environment. It is not a professionally validated calibration product. Do not rely on it for mastering decisions until validated with measurements and a DAW session.

## What works in source

| Feature | Status |
|---|---|
| Stereo AU and VST3 build targets | Configured, unverified on macOS |
| Host-automatable calibration, simulation, gain and bypass parameters | Implemented |
| Six-band user-imported correction EQ | Implemented, coarse approximation |
| Calibration profile persistence in host state | Implemented |
| Generic playback filter/width simulations | Implemented; **not measured device models** |
| Peak, RMS and stereo correlation meters | Implemented; basic block statistics |
| Basic peak/phase warning button | Implemented |
| Cream home screen with three illustrated navigation cards | Implemented in native JUCE |
| Calibrate / Analyse / Translate pages and discreet Settings | Implemented; parameter attachments retained |
| Soft audio-reactive waveform and hover/press feedback | Implemented; stylised waveform, **not an FFT** |
| User-controlled mix gain | Implemented; **not automatic mix optimisation** |
| Unit DSP checks | Included, not executed in this environment |

## Not implemented yet

Measured headphone database (including ATH-M30x), room measurement and microphone workflow, true peak, LUFS, full FFT spectrum, spectral tonal balance and bass/harshness diagnostics, preset browser, smooth crossfades when changing correction curves, automatic frequency-curve design from raw measurements, dynamic range/crest factor, intelligent one-click EQ/compressor assist, undo history and gain-matched A/B. Current simulations use generic high-pass/low-pass filters and stereo width only. No specific iPhone, car or headphone response is claimed. A profile is not automatically verified by importing it.

## Quick start — GitHub

1. Create an empty GitHub repository (do not add another README).
2. Upload **all files and folders inside this project directory** to the repository root, including `.github/workflows/build-macos.yml` (GitHub web uploads may hide dotfolders; use Git for reliability).
3. Push to the default branch; choose **Actions → Build macOS Audio Units**.
4. Wait for both architecture jobs. Download the `UniversalReference-macOS-arm64` artifact for Apple Silicon or `...-x86_64` for an Intel Mac (including a 2020 Intel MacBook Air).
5. Extract the downloaded artifact and then its inner ZIP.
6. Copy `Universal Reference.component` into `~/Library/Audio/Plug-Ins/Components/`. Optionally copy `Universal Reference.vst3` into `~/Library/Audio/Plug-Ins/VST3/`.
7. Quit/restart Logic Pro. Check **Logic Pro → Settings → Plug-in Manager**, look for **One Sound Records → Universal Reference**, and rescan if needed.

First-run macOS security prompts may occur: GitHub Actions artifacts are typically unsigned and unnotarised. Unsigned plug-ins can fail Gatekeeper/Logic validation; for broad distribution, configure Apple Developer ID signing, hardened runtime where needed, notarisation, and a suitable installer. Do not disable system security globally.

## Local build (macOS)

Install Xcode and CMake; internet access is required to fetch JUCE at configure time.

```bash
cmake -S . -B build -G Xcode -DCMAKE_OSX_DEPLOYMENT_TARGET=12.0
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

Look under `build/UniversalReference_artefacts/Release/AU` and `/VST3`. JUCE 8.0.10 is fetched at a pinned tag; the project's MIT license does **not** override JUCE's own licensing. Check JUCE's GPL/commercial terms before distributing closed-source binaries. The suggested deployment baseline is macOS 12+, Apple Silicon or Intel; actual compatibility is unverified until tested. 44.1/48/96 kHz and other host sample rates use dynamically configured filters.

## Calibration workflow

- Select a **six-band correction CSV** conforming to [Profiles/FORMAT.md](Profiles/FORMAT.md).
- Imported profile name appears in the UI. The user must ensure the profile is derived from measurements of the actual listening system; imported does not imply verified.
- Enable **CALIBRATION**; adjust correction intensity. The plugin uses peaking IIR filters at 60, 150, 350, 1000, 3500 and 9000 Hz with fixed Q=1. Corrections are approximate, not a precise inverse of the measured frequency response.
- A fixed -6 dB safety trim is applied whenever calibration is enabled. This reduces, but cannot guarantee avoidance of, clipping. Watch the output meter and level-match externally.
- For ATH-M30x: **no measured curve is bundled**. Obtain a trustworthy, properly licensed profile first; do not use an invented one.

## Monitoring safety (read before use)

**Calibration and simulations are MONITORING effects. The mix gain is MIX processing.** They share one DSP path in this early version, so inserting this plugin directly on Logic's **Stereo Out** can print monitoring corrections into a bounce, including offline bounce. There is **no automatic export exclusion**.

Safe procedure: keep a separate copy of the project or document your monitoring setup; insert the plugin last on Stereo Out only when monitoring; **turn off CALIBRATION and DEVICE SIMULATION before every bounce**, and ideally disable/bypass/remove the entire monitoring instance. Perform a null/bypass check of a short test export before trusting it. For a robust monitor-only workflow use dedicated monitor-processing routing or hardware/system-wide calibration that is not part of the bounce path. Logic's Stereo Out inserts are generally rendered in a bounce.

Do not use simulated playback as a substitute for checking on actual devices. Room EQ cannot fix deep cancellation nulls or reverberation, and loudness alone does not establish mix quality.

## Architecture and limitations

`Source/DSP.h`: fixed-size, allocation-free biquad and stereo statistics. `PluginProcessor`: stereo processing, APVTS host automation, profile import and state. `PluginEditor`: native JUCE UI with home/card navigation. `Tests`: portable biquad and correlation sanity checks. GUI file selection occurs off the audio thread. The audio callback allocates no heap memory and performs no disk access. Filter coefficients are recalculated on parameter changes: rapid automation can still cause clicks; smoothing is a future task. The audio thread reads atomically stored correction bands. Profile names are UI-only and should not be modified concurrently with host state save. The UI's page selection is transient; audio settings remain saved by the processor and the UI opens at Home.

Sample peak is **not true peak**; RMS dBFS is **not LUFS**. The waveform display is illustrative motion driven by RMS, not a measured FFT. The `ANALYSE` button presents only a near-clipping or strongly negative-correlation warning. No machine learning or subjective mix prediction is implemented.

## Troubleshooting

- **No build:** Inspect the failed GitHub Actions log. Confirm the JUCE download succeeded; retry transient network errors. Xcode/macOS runner changes may require pin updates.
- **AU not in Logic:** Confirm the correct architecture and `.component` folder, restart Logic, open Plug-in Manager and rescan. Validate with `auval -v aufx UnRf OsRc` in Terminal.
- **No audible calibration:** No calibration data is bundled. Import an appropriate correction CSV and enable calibration.
- **Audio changes in exported track:** Monitoring effects were still on Stereo Out during bounce; disable them and re-export.
- **Unexpected levels:** Calibration applies a -6 dB trim; simulation uses approximate filters. Check gain staging.

## Development roadmap

1. Verify macOS build and Logic AU validation; fix any SDK/host issues.
2. Add crossfade/smoothed coefficient changes, dedicated monitor/mix signal routing and safer export design.
3. Add measured profile import, licensed measurement dataset, FFT/true peak/LUFS, and test fixtures.
4. Add proper mono/downmix, detailed translation diagnostics and gain-matched A/B.
5. Extend Mix Assist to explicit reversible processing modules and complete visual refinement.

MIT license applies to this project's code; JUCE and third-party measurement datasets retain their own terms.
