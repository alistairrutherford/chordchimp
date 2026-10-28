# ChordChimp: session handoff (2026-10-28)

Where the work stands, for whoever picks it up next. The plan is [PLAN.md](../PLAN.md); its §11
Status table has the detail of each phase, and its §10 the open questions.

## What ChordChimp is

A VST3 (and Standalone) MIDI chord plug-in: a simplified fork of Chordism (`../chordism`, forked
at `07f6e1a`). It drops Chordism's Smart Scale and keyboard split, so every white key plays a
chord, and keeps Chordism's 22 factory presets. JUCE 8.0.14, C++17, Projucer + xcodebuild.

## State

- **Phases 0-7 of the plan are done** and committed on `main`, one commit per phase, with the
  README screenshot (`5a35ac7`) and the editor-in-Live check and size fix (`1ea392c`) in between.
  Phase 6 brought Export drag-out back (open question 3); Phase 7 gave the black keys borrowed
  chords (open question 1). Both were checked in Live together.
- **Remote:** `origin` is `github.com/alistairrutherford/chordchimp`, and CI
  (`.github/workflows/build.yml`) has passed there.
- Installed: `ChordChimp.vst3` and `ChordChimp.app` (by `Tools/install.sh`), next to Chordism.

## How things fit

- **Core** (`Source/core`, header-only, no JUCE):
  - `engine/ChordEngine.h`: one `ChordPerformer` and a `VoiceTracker` with 384 sources.
  - `engine/KeyRouter.h`: a white key's letter picks slot 0-6 in any octave, a black key slot
    7-11 (C# … A#).
  - `theory/ChordGroups.h`: `makeBorrowedSlot`, the black keys' chords (bII Phrygian, bIII
    minor, #iv° Lydian, bVI and bVII minor), as the Key's scale's degree with accidentals.
  - `engine/{ChordFx, StepSequencer, TestPattern, VoiceTracker}.h`. The step sequencer's four
    patterns are fixed (`seqPatterns()` = 0x1111, 0x4444, 0x1449, 0x4949). The test pattern
    plays the home octave (C = MIDI 48).
  - `theory/Progressions.h`: the enum keeps Chordism's order (Custom second) so preset values
    still mean the same; the parameter's choices skip Custom (`progressionChoice` /
    `progressionFromChoice`); Custom plays as Diatonic.
  - `midiio/{ExportRecorder, MidiFile}.h`: Chordism's Export, unchanged but for names (no
    `ChordImporter`).
- **Plug-in** (`Source/plugin`): `Parameters.*` (26 parameters, `params::numParameters`, all with
  Chordism's IDs; `params::fromChordism` converts a Chordism preset's Progression value),
  `ChordChimpProcessor.*`, `FactoryPresets.*`, `PresetManager.*` (also reads Chordism preset
  files; user presets in `~/Music/ChordChimp/Presets`).
- **UI** (`Source/ui`): one page, 780×560, resizable 720×520 to 1400×960. Top bar, a row of
  globals, seven chord pads (capped at 200 px high), six effect cards, the output keyboard
  (hideable). Test pattern has no control; Live's device parameters reach it.
- Strum while stopped uses Strum Rate at the last tempo the host gave.

## Build and test

```bash
Tests/run_tests.sh            # core tests (clang only)
Tests/run_snapshot.sh         # editor snapshots, presets vs the Chordism fixture, hardening checks
Tests/run_validation.sh       # pluginval, strictness 10
Tests/run_bench.sh            # CPU per instance (worst mean has been 0.11%)
Tools/install.sh --validate   # build, install, then pluginval
```

- Snapshot references are in `Tests/fixtures/snapshot/` (`editor`, `editor-small`,
  `editor-large`).
- `Tests/fixtures/chordism_presets.json` is Chordism's presets as data (22 presets, 123
  defaults). `Tools/dump_chordism_presets.sh` regenerates it from `../chordism`; it's committed,
  so CI doesn't need Chordism.

## Testing in Live

- Live 12.4.7 with the ClaudeLiveBridge Remote Script (port 9873), driven through the Ableton MCP
  tools. Chordism's `LiveBridge/livebridge/client.py` also works against port 9873.
- The test set is `~/Music/Ableton/Projects/ChordChimp Phase 5 Project`. It has one extra unsaved
  take from the check after reopening.
- With Test pattern on, each finished take is also written to
  `~/Music/ChordChimp/Exports/Test take.mid` (Phase 6), for comparing takes without recording.
- To drag Export into Live by computer use, use a real click-drag; separate mouse moves with the
  button down don't start the drop. Live sizes the dropped clip to its notes.
- The editor check (PLAN.md §11 *Editor in Live*; it found and fixed the editor reopening at
  720×520) used a copy, `~/Music/Ableton/Projects/ChordChimp Editor Check Project`. Live's
  plug-in windows hide while Live isn't the front app, so editor checks need Live in front. The
  editor's saved size and keyboard flag are in the `UI` child of ChordChimp's state in the `.als`.
- After a new install, re-index Live's browser so it lists the plug-in. The browser category for
  plug-ins is `plugins`.
- Bridge quirks: arrangement recording ignores the start beat (always records from 0) and starts a
  few seconds late. `open_project` on the set that's already open does nothing; quit Live and
  reopen the `.als` instead.

## Not yet checked

- Playing from a hardware MIDI keyboard (now including the black keys).

## Open questions for the user (PLAN.md §10)

1. **Black keys:** answered 2026-10-28: borrowed chords by modal mixture, with their own pads
   (Phase 7).
2. **Key octave:** ignored, with Chord Octave setting the register (current), or should the
   octave played move the chord?
3. **Dropped features:** answered 2026-10-28: Export drag-out came back (Phase 6); Custom and
   QWERTY stay out.
4. **Bright Melody:** keep the name (it now has a chord arp in place of the melody), rename it,
   or drop it?
5. **Name:** *ChordChimp* (fixed in Phase 0).

## Known leftovers

- One Font deprecation warning that predates the fork; left as is.

## Ways of working that held up

- Each phase ended with all of: core tests, snapshot checks, pluginval 10, the benchmark, then an
  update to PLAN.md §11 and the README status, then one commit.
- Comments cite this repo's `PLAN.md §…`, or say *Chordism's PLAN.md §…* for unchanged code
  whose design decisions live there.
