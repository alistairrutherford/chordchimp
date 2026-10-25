# ChordChimp

A MIDI chord generator plug-in (VST3) for Ableton Live on macOS: every white key plays a full
chord. A simplified fork of [Chordism](../chordism): no Smart Scale and no keyboard split. The
plan and status are in [PLAN.md](PLAN.md).

![ChordChimp's editor: the preset bar and Key, Scale, Chord Group, Progression and Octave along the top; seven chord pads, two of them lit; the effects as six cards (Strum, Velocity, Arp, Voicing, Repeat, Step seq with its steps); the output keyboard along the bottom](docs/screenshot.png)

**Status:** Phases 0-5 done: every white key plays its chord in any octave (no Smart Scale, split,
shifters, Instance Sync, look-ahead, Chord Editor or MIDI import/export), with 26 parameters,
Chordism's 22 factory presets and a single-page editor (Phase 3). Hardening and install are done
(Phase 4), and it's checked in Live 12.4.7 next to Chordism (Phase 5).

## Install

Needs Xcode (open it once after installing) and JUCE 8.0.14:

```sh
git clone --branch 8.0.14 https://github.com/juce-framework/JUCE.git ~/Development/JUCE
Tools/install.sh              # build, sign ad hoc, install for this user
```

`Tools/install.sh` builds the VST3 and the Standalone (Release, universal) in `dist/build`, signs
both ad hoc and installs `~/Library/Audio/Plug-Ins/VST3/ChordChimp.vst3` and
`~/Applications/ChordChimp.app` for the current user. `--juce DIR` for JUCE elsewhere,
`--validate` to run pluginval on the installed plug-in, `--uninstall` to remove both. Then rescan
plug-ins in Live.

ChordChimp has its own bundle ID (`com.alistairrutherford.chordchimp`), plug-in code (`Cchp`) and
preset folder (`~/Music/ChordChimp/Presets`), so it installs and loads next to Chordism.

## Build (development)

```sh
PROJUCER=~/Development/JUCE/extras/Projucer/Builds/MacOSX/build/Release/Projucer.app/Contents/MacOS/Projucer
"$PROJUCER" --resave ChordChimp.jucer
xcodebuild -project Builds/MacOSX/ChordChimp.xcodeproj -target "ChordChimp - All" -configuration Release build
```

## Test

```sh
Tests/run_tests.sh          # core tests (no JUCE)
Tests/run_snapshot.sh       # editor snapshots, parameters, state, presets
Tests/run_validation.sh     # pluginval strictness 10 on the installed VST3
Tests/run_bench.sh          # CPU per instance
Tools/dump_chordism_presets.sh   # regenerate Tests/fixtures/chordism_presets.json from ../chordism
```
