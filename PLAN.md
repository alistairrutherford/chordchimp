# ChordChimp: Project Plan

A macOS VST3 MIDI plug-in for Ableton Live that plays a full chord from one key. It is a
**simplified fork of Chordism** (`../chordism`). It keeps Chordism's chord engine and factory
presets, and drops the Smart Scale and everything that exists only to support it.

**The brief (2026-10-25):** take Chordism's functionality, remove the Smart Scale, so the keyboard
no longer needs a split; keep the presets; make the whole thing much simpler.

**How this plan reads the brief:** in Chordism the Split exists only to divide the keyboard into a
chord zone and a Smart Scale zone (§4.1 of Chordism's plan). With the Smart Scale gone, **every
key plays chords and there is no Split parameter.** If a split was meant to *stay* (for example,
with the upper zone passing notes through), only §3.1 and the router in Phase 1 change.

---

## 1. Goals

| # | Requirement | Acceptance criterion |
|---|-------------|----------------------|
| G1 | Loads in Live 12 as a VST3 and sends MIDI to an instrument on another track | `pluginval` strictness 10 passes. In Live, an instrument with *MIDI From: ChordChimp* plays and records the output. |
| G2 | **No split:** every white key, in any octave, plays the chord for its letter | Router tests: all 128 notes; C D E F G A B → progression slots 1–7 in every octave. |
| G3 | **No Smart Scale:** no mappings, pattern slots, melody, note repeat or look-ahead | None of that code, parameters or UI remains (the grep check in §8). |
| G4 | **The presets carry over:** Init plus Chordism's 21 factory presets, same names and categories | Each preset's chord-side values are identical to Chordism's (preset table test). Chordism user-preset XML files load, with dropped settings ignored. |
| G5 | **Much simpler:** one page, about 26 parameters (Chordism has 123) | Every parameter has a control on the single page (snapshot test). |
| G6 | Real-time safe, no stuck notes | Chordism's rules (§5) and its fuzz and malloc-hook tests still pass on the reduced engine. |
| G7 | Coexists with Chordism | Its own name, bundle ID, plug-in code and preset folder, so both load side by side in Live. |

**Out of scope:** Windows, AU, AAX, the Smart Scale, the Chord Editor grid, MIDI import, Instance
Sync, QWERTY playing, plug-in hosting. (Export drag-out came back in Phase 6.)

---

## 2. Feature map (Chordism → ChordChimp)

| Area | Chordism | ChordChimp |
|---|---|---|
| Keyboard | Split (default MIDI 60): chord zone below, Smart Scale above | **No split.** The whole keyboard is the chord keyboard (§3.1) |
| Global | Key, Scale (13), Chord Group (17), Progression (8 presets + Custom) | **Kept**, except Progression *Custom* and its 7 *Custom n* degrees (**dropped**) |
| Global | Chord Octave, Voice leading (Off / Smooth) | **Kept** |
| Chord keys | White keys = 7 chords, with name and Roman numeral | **Kept** (display only: no right-click degree or chord-type menus) |
| Chord keys | Black keys = 5 shifters (group or key/scale), Toggle/Standard | Shifters **dropped**. Since Phase 7 the black keys play **borrowed chords** (§3.1) |
| Chord Editor | 7 degrees × 7 steps grid (on, accidental, octave mask), chord-type menu | **Dropped.** Chords are the Chord Group's shapes |
| Chord FX | Stack, Strum + Strum Rate + Strum time (ms), Arp (12 patterns, rate, range, gate), Repeat (rate, gate) | **Kept**, except *Strum time (ms)* (§3.3) |
| Chord FX | Step Sequencer: on, 4 editable 16-step patterns, Swing, Gate, Sustain | **Simplified:** on, choice of the 4 factory patterns, Swing, Gate. No step editing, no Sustain |
| Chord FX | Velocity Algorithm (13 curves) + Peak + Range, Hold (Off / Latch / Bar) | **Kept** |
| Smart Scale | 5 mappings, Inversions, Note Repeat, 5 pattern slots (arp/melody), its own Octave, Stack, Step Seq, Velocity, Hold | **Dropped** |
| Smart Scale | Look-ahead transposing (`ChordTimeline`) | **Dropped** |
| Routing | Mute CG, Mute SS | **Dropped** (one zone; Enable covers it) |
| I/O | Enable (pass input through) | **Kept** |
| I/O | MIDI import (drag a .mid in), Export drag-out (32 bars) | Import **dropped**; Export **back in Phase 6** (open question 3), as Chordism's |
| I/O | QWERTY playing | **Dropped**; the on-screen chord keys still play |
| Presets | 21 factory + Init, user presets, browser with search | **Kept** (§3.4) |
| Multi | Instance Sync (groups A–D) | **Dropped** |
| Test rig | Test pattern, Test shifter, Test smart (for LiveBridge) | *Test pattern* **kept** (hidden on the page, not in presets); the other two **dropped** |

---

## 3. Functional design

### 3.1 Keyboard

```
 every octave, 0–127:
  white C D E F G A B → progression slots 1–7  (the octave doesn't matter; Chord Octave sets the register)
  black C# D# F# G# A# → the borrowed chords bII bIII #iv° bVI bVII (Phase 7)
```

- This is Chordism's chord zone, extended over the whole keyboard: a 25-key controller has every
  chord under either hand, and the controller's octave buttons do nothing harmful.
- **Black keys play borrowed chords** (Phase 7; open question 1). Each plays the chord on its
  semitone above the tonic, borrowed from the parallel mode that has it (modal mixture), in the
  Chord Group's shape: C# bII (Phrygian), D# bIII (natural minor), F# #iv° (Lydian), G# bVI and
  A# bVII (natural minor). In C with Triads: Db, Eb, F#dim, Ab, Bb; with Sevenths: Dbmaj7, Ebmaj7,
  F#m7b5, Abmaj7, Bb7. They move with Key and don't depend on the Progression. A borrowed chord is
  the Key's scale's degree with accidentals (`makeBorrowedSlot`, `ChordGroups.h`), so its name and
  numeral are the Key's: in a natural minor key D# is "III", the same chord as E's. Until Phase 7
  black keys were swallowed.
- With **Enable** off, every note passes through unchanged, as in Chordism.
- **Polyphony between keys is as in Chordism:** each chord key owns its performance; two keys held
  sound two chords; Hold's "new selection" is the next chord key.

### 3.2 Chords

Unchanged from Chordism §4.2, minus editing:

- Each chord is the stack of thirds on its degree, shaped by the **Chord Group** (on-steps and
  octave masks), with the scale's default accidentals (natural minor V raises its 3rd).
- **Progression** orders the degrees on the white keys: the 8 presets only.
- **Voice leading** (Off / Smooth) works as in Chordism Phase 7: inversions of each chord, bass
  anchored within ±6 semitones of the Chord Octave's tonic.
- The keys show the chord name and Roman numeral, from Chordism's `ChordNamer` and `RomanNumeral`.

### 3.3 Chord FX

The Chordism chord-zone chain, unchanged in order:

```
key → chord slot → voicing (+ voice leading) → Octave → Stack → [Strum | Arp] → Repeat → Step Seq gate → Velocity → Hold → out
```

- **Strum time (ms) is dropped.** While the transport is stopped, Strum Rate runs at the internal
  clock's tempo (Chordism's clock already runs at the last known tempo), so one control covers
  both cases.
- **Step Sequencer** keeps its timing rules (Chordism §4.5 Phase 3) but plays one of the 4 factory
  patterns, which become fixed tables. *Sustain* goes: no factory preset uses it.
- Everything else (grid, gates, loops, locates, tempo changes, the stopped clock) is Chordism's code
  and behaviour, untouched.

### 3.4 Presets

- **Parameter IDs are Chordism's** for every parameter kept, and **choice lists keep their order**
  (`Scale`, `ChordGroup`, `Progression`, `ArpPattern`, `VelocityCurve`, …). So the factory table
  ports as is, and a Chordism user preset XML loads: unknown IDs (Smart Scale, shifters, Custom
  degrees, step masks) are ignored, and a missing value takes the default.
- **Factory presets:** the same 22 entries (Init; Genre 10, Artist-style 5, Vibe 6) in
  `Source/plugin/FactoryPresets.cpp`, with the dropped settings removed:
  - 7 presets lose a Melody pattern slot (Neo-Soul Keys, Lo-Fi Sevenths, Synthwave Drive, Ambient
    Pads, Dreamy, Melancholy, Bright Melody); 2 lose a Mapping (Neo-Soul Keys, Jazz Two-Five);
    Ambient Pads loses *SS hold*. Their chord settings are unchanged.
  - **Bright Melody** relied on its melody for its character. It gets a chord arp instead
    (Linear Up, 1/8, range 2) so it keeps a moving line; open question 4 is whether to rename it.
  - A preset with *Step seq* on and *Seq pattern* 1–3 keeps those values: the factory patterns are
    the same tables.
- **User presets** go to `~/Music/ChordChimp/Presets/<Category>/<Name>.xml` (Genre, Artist-style,
  Vibe, User). The browser (◀ name ▶ · Save, a call-out with search and *Show folder*) is
  Chordism's `PresetBar`, minus *Import MIDI chords…*.
- **The presets are also the plug-in's programs** (program 0 = Init), as in Chordism, so LiveBridge
  can load them through the VST3 *Program* parameter.
- A preset holds every host parameter except Enable and Test pattern.

### 3.5 Parameters

About 26, all host parameters (automatable, reachable by LiveBridge):

| Group | Parameters (Chordism IDs) |
|---|---|
| Global | `enable`, `key`, `scale`, `chordGroup`, `progression` (without Custom, see below), `chordOctave`, `voiceLeading` |
| Stack / Strum | `stack`, `strum`, `strumRate` |
| Arp | `arp`, `arpRate`, `arpRange`, `arpGate` |
| Repeat | `repeat`, `repeatRate`, `repeatGate` |
| Step Seq | `seq`, `seqPattern`, `seqSwing`, `seqGate` |
| Velocity | `velocity`, `velocityPeak`, `velocityRange` |
| Hold | `hold` |
| Test rig | `testPattern` (chord part only) |

**Dropped** (about 97): `split`, `muteChords`, `muteScale`, `shifterMode`, `testShifter`,
`customDegree1-7`, `shifter1-5Kind/Group/Key/Scale`, `strumMs`, `seqSustain`, `seq1-4Steps`, every
Smart Scale and pattern parameter (`mapping`, `inversions`, `noteRepeat`, `patternSlot`, `smart*`,
`pattern1-5*`, `melodySwing`, `melodySeed`, `testSmart`), `syncGroup`, `lookAhead`.

*Custom* is the **second** choice of Chordism's `progression` list (Diatonic, Custom, Pop, …), so
removing it would shift every later index and break preset and automation values. The enum keeps
its order; the parameter's choice list hides Custom, and the value is stored with a small mapping
(index ↔ enum) in `Parameters.cpp`. A Chordism preset with Custom loads as Diatonic.

---

## 4. Architecture

Same stack as the sibling projects: **JUCE 8.0.14, C++17, Projucer + xcodebuild**, JUCE at
`~/Development/JUCE`. VST3 + Standalone.

```
 MIDI in ─► KeyRouter ─► ChordEngine ─► ChordPerformer (Chord FX chain) ─► VoiceTracker ─► EventQueue ─► MIDI out
 (host or on-screen key)                              ▲
             Transport (AudioPlayHead) ─► Clock ──────┘
```

Comments in the code cite this plan as `PLAN.md §…`. Where they cite *Chordism's PLAN.md §…*
(`../chordism/PLAN.md`), the code is Chordism's, unchanged, and that plan holds the detailed design
decisions behind it (chord model, effects timing, scheduling, real-time rules) that §3 here only
summarises.

### 4.1 Starting point: a copy, not a shared library

Copy Chordism's `Source/`, `Tests/`, `Tools/` and `.jucer` into this folder, then delete. Chordism
is finished and the cut is large, so a shared core (a submodule, or `#include "../chordism/…"`)
would tie the two projects together for little gain. Chordism's core is header-only plain C++,
so the copy is cheap.

### 4.2 What stays, what goes

| Module | Keep | Remove |
|---|---|---|
| `core/theory` | `Scales`, `ChordModel`, `ChordGroups`, `Progressions` (presets only), `VoiceLeading`, `ChordNamer`, `RomanNumeral`; `ChordEditor` only if `ChordModel` needs its step types | `ChordTypes` (the chord-type menu) |
| `core/engine` | `Clock`, `Transport`, `EventQueue`, `MidiEvents`, `VoiceTracker`, `ChordFx` (`ChordPerformer`), `Strum`, `Arp`, `Repeat`, `Stack`, `StepSequencer`, `VelocityAlgorithm`, `Hold`, `NoteValues`, `SnapshotExchange`, `TestPattern` (chord part) | `ScaleMapper`, `PatternEngine`, `Melody`, `ChordTimeline`, `SharedSession`, `Shifters` |
| `core/engine` (rewritten) | `ChordismEngine.h` → `ChordEngine.h`: one performer, no smart performer, no sync events, no look-ahead, no shifter state. `InputRouter.h` → `KeyRouter.h`: letter → slot, black keys swallowed, no split | |
| `core/midiio` | — | `MidiFile`, `ChordImporter`, `ExportRecorder` (the whole folder) |
| `plugin` | `ChordismProcessor` → `ChordChimpProcessor`, `Parameters`, `PresetManager`, `FactoryPresets` | `ChordEditorState` (the edits), the export/import and sync code in the processor |
| `ui` | `Theme`, `ParamControl`, `PresetBar`, `ChordKeyboard`, `OutputKeyboard`, the `ParamGrid` / `TopBar` / `ChordFxPanel` panels | `SmartKeyboard`, `SmartPanel`, `SetupPanel`, `ChordEditorPanel`, the export handle and drop target, QWERTY |

**Coupling to cut carefully** (found while reading Chordism):
- `ChordPerformer::setSibling` lets the stopped-clock re-anchor check the Smart performer. With one
  performer the sibling is null, which the code already handles; remove the call, not the check.
- `VoiceTracker` sources: Chordism sizes them for both zones (640). One zone needs 384 (128 keys
  + 2 per performance).
- `EngineSettings` carries `smart`, `testSmart` and the shifter/sync fields; the processor's
  `Snapshot` reads them. Both shrink together.
- The processor's saved-state migration (Chordism's Phase 4 step masks) goes; a Chordism state is
  only ever read as a preset, never as ChordChimp's own state.

### 4.3 Identity

| | ChordChimp | (Chordism) |
|---|---|---|
| Name | ChordChimp | Chordism |
| Bundle ID | `com.alistairrutherford.chordchimp` | `com.alistairrutherford.chordism` |
| Manufacturer code | `ARut` (same) | `ARut` |
| Plug-in code | `Cchp` | `Cdsm` |
| Installed as | `~/Library/Audio/Plug-Ins/VST3/ChordChimp.vst3`, `~/Applications/ChordChimp.app` | |
| Presets | `~/Music/ChordChimp/Presets/` | `~/Music/Chordism/Presets/` |

The name and codes are fixed in Phase 0 and must not change after that (Live sets refer to them).

---

## 5. UI

One page, about **760 × 420**, resizable within a small range; Chordism's dark theme and amber
accent (`Theme.*`).

```
┌──────────────────────────────────────────────────────────────────────────────┐
│ ChordChimp  [On]  ◀ Genre / House Stabs ▶ [Save]   Key [A] Scale [Nat. Minor] │
│                                            Group [Sevenths] Prog [50s] Oct [0]│
├──────────────────────────────────────────────────────────────────────────────┤
│   Am7     Bm7♭5    Cmaj7    Dm7     Em7     Fmaj7    G7                     │
│   i7      ii*7     III7     iv7     v7      VI7      VII7    ← 7 chord keys  │
├──────────────────────────────────────────────────────────────────────────────┤
│ Strum [Off ▾] Rate [1/64]   Arp [Off ▾] Rate Range Gate   Repeat [ ] Rate Gate│
│ Step seq [ ] Pattern [2] Swing Gate   Velocity [House ▾] Peak Range           │
│ Stack [Off ▾]  Hold [Off ▾]  Voice leading [Off ▾]                            │
├──────────────────────────────────────────────────────────────────────────────┤
│ ▁▁▂▂▁▁▂▂▁▁ output keyboard (shows sounding notes) ▁▁▂▂▁▁▂▂▁▁                  │
└──────────────────────────────────────────────────────────────────────────────┘
```

- **Chord keys:** seven large pads, one per white-key letter, with the chord name, Roman numeral
  and a stripe on the last-played one; above them (Phase 7) five smaller pads for the black keys,
  each centred over the gap where its key sits. Clicking plays through the engine (Chordism's
  lock-free queue), exactly like host MIDI.
- **Controls** build themselves from the parameters (`ParamControl`), so the snapshot test proves
  every parameter has a control.
- **No Setup page:** nothing is left that needs one. *Test pattern* is reachable from Live's device
  parameters only.
- The output keyboard can be hidden; the size and that choice are saved in the plug-in state.
- **Export** (Phase 6): a handle in the top bar, between the presets and Keys. Drag it into a
  track: the last ≤ 32 completed bars ChordChimp played since the host started, as a .mid
  (Chordism's design, its PLAN.md §4.6). Disabled until a bar has finished.

---

## 6. Milestones

Chordism took about a week of focused work for 8 phases; this is mostly deletion, so estimates are
in days.

| Phase | Est. | Deliverable | Exit criteria |
|---|---|---|---|
| **0. Fork and identity** | ½ day | `git init`; copy Chordism; rename the project, classes, bundle ID, plug-in code, preset folder; `Tools/install.sh` and CI renamed | Builds; `pluginval` 10 passes; loads in Live **next to Chordism**, behaving like Chordism |
| **1. Engine cut** | 1–2 days | Remove the Smart Scale, patterns, melody, look-ahead, sync, shifters, Chord Editor edits, MIDI I/O; `KeyRouter` without a split; `ChordEngine` with one performer | `Tests/run_tests.sh` passes (the Smart, Melody, Sync and MIDI I/O tests removed; the router tests rewritten for G2); stuck-note fuzz passes |
| **2. Parameters and presets** | 1 day | The ~26-parameter layout (§3.5), Strum Rate while stopped, fixed seq patterns; `FactoryPresets.cpp` ported (§3.4); `PresetManager` on the new folder | Preset table test (G4); a Chordism user preset loads; `pluginval` 10 |
| **3. UI** | 2 days | The single page (§5) | Snapshot test: every parameter has a control; reference images; state restores (size, keyboard shown) |
| **4. Hardening and install** | 1 day | Edge-case and malloc-hook tests carried over; benchmark; `Tools/install.sh`; CI | 0 allocations in `processBlock`; 8 instances < 1% CPU each; CI green; clean install |
| **5. Live check** | ½ day | LiveBridge run (Chordism's harness pointed at ChordChimp) and the manual checklist | Records the chords of a test take exactly; preset switching ends every note; set save/reopen restores state |
| **6. Export drag-out** | ½ day | Chordism's `ExportRecorder`, `MidiFile` and Export handle (open question 3) | Chordism's export tests; the handle exports in the snapshot app; a drag into Live |
| **7. Black keys** | 1 day | Borrowed chords on the black keys (§3.1) and their row of pads (§5) (open question 1) | Notes, names and numerals by hand in C major and A minor; every key × scale × group plays its mode's chord; the pads in the snapshot app; played in Live |

**Total: about 6–7 working days.**

---

## 7. Phase 1 in detail (the risky part)

Work from the outside in, keeping the tests green after each step:

1. Delete the Smart Scale UI (`SmartKeyboard`, `SmartPanel`) and its parameters, so nothing reads
   `SmartSettings`.
2. Remove the Smart performer, `PatternEngine`, `ScaleMapper`, `Melody` and `ChordTimeline` from
   the engine; shrink `VoiceTracker` to 384 sources.
3. Remove `SharedSession` and the sync events; then `Shifters` (the effective key and scale become
   the parameters).
4. Remove `ChordEditorState` and the edit snapshot path; chords come from the group only.
5. Remove `core/midiio`, the export recorder in the processor, and the drop target.
6. Replace `InputRouter` with `KeyRouter`; remove `split` everywhere.
7. Delete `Tests/test_smart.cpp`, `test_melody.cpp`, `test_sync.cpp`, `test_midiio.cpp`; trim
   `test_engine.cpp`, `test_edge.cpp` and the snapshot app to the chord zone.

---

## 8. Testing

Carried over from Chordism, trimmed to one zone:

- **Theory** (`test_theory`, `test_chords`): every Key × Scale × Chord Group × degree against the
  music21 fixture, unchanged.
- **Router** (new): all 128 notes; white keys in every octave → slots 1–7; black keys swallowed;
  Enable off passes everything through.
- **Engine and FX** (`test_engine`, `test_fx`, `test_edge`): fake transport, sample accuracy at
  32–2048 samples and 44.1–96 kHz, loops, locates, tempo changes; Strum while stopped now uses the
  internal clock's tempo.
- **Stuck-note fuzz:** random keys across the whole keyboard, parameter changes and preset loads;
  `VoiceTracker` empty at the end.
- **Presets** (new): for each of the 22 factory presets, every kept value equals Chordism's
  (generated once from Chordism's table into a fixture); a Chordism user-preset XML with Smart Scale
  settings loads without error and leaves those settings out.
- **G3 grep:** no `smart`, `Smart`, `pattern`, `melody`, `split`, `shifter`, `sync` identifiers left
  in `Source/` (a test script, with an allow-list for e.g. `seqPattern`).
- **Host:** `pluginval` strictness 10 in CI; the snapshot app's malloc hook; the CPU benchmark.
- **Live:** Chordism's LiveBridge, with the plug-in name as a setting; the Phase 1 (record chords)
  and Phase 3 (FX timing) tests only.

---

## 9. Risks

| Risk | Likelihood | Impact | Mitigation |
|---|---|---|---|
| Hidden coupling to the Smart zone in the engine (sibling checks, source numbering, shared clock re-anchoring) | Medium | Medium | Cut in the order of §7, tests green after each step; the fuzz test catches stuck notes |
| Preset values drift because an enum or choice list changed order | Low | High | Keep Chordism's enums and lists as they are; the preset fixture test |
| Live confuses ChordChimp with Chordism | Low | High | New plug-in code and bundle ID in Phase 0, checked by loading both |
| Simplifying too far (e.g. Custom progression or export is missed) | Medium | Low | Kept out of the cut code paths; can be brought back from Chordism later (open question 3) |

---

## 10. Open questions

1. **Black keys.** Swallowed (this plan), passed through unchanged, or given a simple job (e.g.
   play the chord on that semitone as a borrowed chord)? **Answered (2026-10-28): borrowed
   chords**, by modal mixture, with their own row of pads (Phase 7, §3.1).
2. **Key octave.** Ignored, with Chord Octave setting the register (this plan, as Chordism's chord
   zone), or should the octave played move the chord's register?
3. **Anything dropped that should stay?** The candidates are Progression *Custom*, Export
   drag-out, and QWERTY playing. **Answered (2026-10-28): Export drag-out comes back** (Phase 6);
   Custom and QWERTY stay out.
4. **Bright Melody:** keep the name with a chord arp in place of the melody (this plan), rename it,
   or drop it?
5. **Name:** *ChordChimp*, from this folder's name; it is fixed for good in Phase 0.

---

## 11. Status

| Phase | State | Notes |
|---|---|---|
| 0. Fork and identity | **Done (2026-10-25)**, except the check in Live | Copied from Chordism at `07f6e1a` (`Source/`, `Tests/`, `Tools/`, CI; not LiveBridge or the Live test set). Renamed throughout: namespace `chordchimp`, `ChordChimpProcessor`, `ChordChimpEditor`, `ChordChimpEngine` (becomes `ChordEngine` in Phase 1), bundle ID `com.alistairrutherford.chordchimp`, plug-in code `Cchp` (VST3 class IDs differ from Chordism's), presets in `~/Music/ChordChimp/Presets`. `PresetManager::readFile` also reads Chordism preset files (root tag `Chordism`). Checks: Release build; `Tests/run_tests.sh` all passed; `Tests/run_snapshot.sh` all passed (reference images updated for the new logo); `pluginval` strictness 10 passed; `Tools/install.sh` installed `ChordChimp.vst3` and `ChordChimp.app` next to Chordism. **Not checked:** loading both in Live (Live wasn't running). |
| 1. Engine cut | **Done (2026-10-25)** | **Removed:** the Smart Scale (`ScaleMapper`, `PatternEngine`, `Melody`, the second performer, Note Repeat), look-ahead (`ChordTimeline`), Instance Sync (`SharedSession`), shifters (`Shifters`), Chord Editor edits (`ChordEditor.h`, `ChordTypes.h`, `ChordEditorState`, the editor page and chord-type menu), MIDI import and export (`core/midiio`, the Export handle, the drop target, the test-take file), the split and `InputRouter`. `ChordChimpEngine` became `ChordEngine` (one performer, VoiceTracker 384 sources) and `KeyRouter` routes white keys to slots 0-6 in every octave and swallows black keys; the test pattern plays the home octave (C at MIDI 48). **Also removed, as they only served the split or the zones:** Mute CG / SS, QWERTY playing (its layout hung off the split) and the Split stepper. **Presets:** the Smart Scale and Melody settings are gone from the 7 presets that had them; Bright Melody got its chord arp (Linear Up, 1/8, range 2) now rather than in Phase 2. **UI (interim, until Phase 3):** the chord section fills the width; Setup holds the custom progression and the test pattern. **Parameters:** 39 (Chordism's IDs); Phase 2 takes out Custom 1-7, Strum time, Seq sustain and the four step masks. **Tests:** `test_smart`, `test_melody`, `test_midiio`, `test_sync` deleted; `test_chords` builds its 62 reference chords by step edits on the slot (no editor); `test_engine` has a no-split router test (all 128 notes) and its fuzz without shifters, edits or zones; two `test_fx` cases change the Chord Group mid-note instead of pressing a shifter; the snapshot app dropped its Smart Scale, editor, import, Export, QWERTY and sync checks and gained a Chordism-preset-file check. **Checks:** `Tests/run_tests.sh` all passed; `Tests/run_snapshot.sh` all passed (references updated for the new layout; `editor-chord-editor.png` removed); `pluginval` strictness 10 passed; benchmark worst mean 0.11% per instance. Not checked in Live. |
| 2. Parameters and presets | **Done (2026-10-25)** | **26 parameters** (`params::numParameters`), Chordism's IDs: Custom 1-7, Strum time, Seq sustain and the four step masks removed. **Progression:** the enum keeps Chordism's order (Custom second) so its values still name the same progressions; the parameter's choices are the enum without Custom (`progressionChoice` / `progressionFromChoice` in `Progressions.h`), Custom plays as Diatonic, and a Chordism preset file's Progression value is converted on load (`params::fromChordism`). The chord keys' right-click degree menu went with Custom. **Strum while stopped** uses Strum Rate at the internal clock's tempo (the last the host gave). **Step sequencer:** the four patterns are Chordism's factory ones, fixed (`seqPatterns()`); no Sustain; the steps row shows the chosen pattern, read-only. **Setup** holds only the test pattern. **Preset fixture:** `Tools/dump_chordism_presets.sh` builds `Tools/ChordismPresetDump` (a console app compiling Chordism's own `FactoryPresets.cpp` and `Parameters.cpp`, in `dist/presetdump`) and writes `Tests/fixtures/chordism_presets.json` (Chordism `07f6e1a`: 22 presets, 123 defaults). The snapshot app checks that the 22 presets have Chordism's names and order and that all 525 preset values (22 presets × 24 preset parameters, less Bright Melody's 3 deliberate arp settings) equal Chordism's effective values. **Tests:** step-grid and sequencer cases rewritten for the fixed patterns (swing on pattern 3, pattern 4 at gate 100%); Strum while stopped checked at 1/32 of the host's tempo; progression tests for the choice mapping. **Checks:** `Tests/run_tests.sh` all passed; `Tests/run_snapshot.sh` all passed (references updated); `pluginval` strictness 10 passed; benchmark worst mean 0.11% per instance. Not checked in Live. |
| 3. UI | **Done (2026-10-25)** | **One page**, default 780×560, resizable from 720×520 to 1400×960 (size and keyboard shown saved in the plug-in state). Top bar: the name, On, the preset browser, Keys. A row of globals: Key, Scale, Chord Group, Progression, Octave. **Seven chord pads** (`ChordPads`, replacing the one-octave piano): Roman numeral, chord name, the key letter in a corner, lit while the chord plays, a stripe on the last one played; click or drag across to play; they take the height left, up to 200 px, centred. **Effects** as six cards (`EffectsPanel`): Strum, Velocity; Arp, Voicing (Stack, Hold, Voice leading); Repeat and Step seq with their on switch in the card's corner, and the chosen pattern's steps under Step seq. The output keyboard below, hideable. **No Setup page:** Test pattern has no control (Live's device parameters reach it). Removed `ParamGrid`, `ChordFxPanel`, `SetupPanel`, `ChordKeyboard`. **Snapshot app:** references `editor` (780×560), `editor-small` (720×520, keyboard hidden) and the new `editor-large` (1200×800); `editor-setup` removed; a new check clicks pad G and gets Dm7 (Pop) and its release; every parameter but Test pattern has a control. **Checks:** `Tests/run_tests.sh` all passed; `Tests/run_snapshot.sh` all passed; `pluginval` strictness 10 passed; benchmark worst mean 0.11% per instance. **Not run:** `--bench-ui` (the editor's CPU in a window) and Live. |
| 4. Hardening and install | **Done (2026-10-25)** | **Editor CPU** (`--bench-ui`, one window, audio on its own real-time thread): 4.6% of a core with everything playing, 2.8-3.0% idle (Chordism: 7.0-7.3% and 2.5%). A `sample` profile of the idle editor shows no painting: the time is JUCE's display-link callbacks and run loop, so nothing to cut. **Install from a clean clone** (`git clone` of this repo, then `Tools/install.sh --validate`): builds Projucer's project in `dist/build`, installs `ChordChimp.vst3` and `ChordChimp.app` (universal, ad-hoc signed, `com.alistairrutherford.chordchimp`), pluginval strictness 10 passes. **CI** (`.github/workflows/build.yml`) calls only the runner scripts, so the trimmed test set needs no change; its steps were run on the clean clone as the runner would (core tests, install with `--validate`, the snapshot checks with `--no-pixels`, the benchmark: worst mean 0.11% per instance) and pass; the uploaded log paths exist (`dist/projucer-build.log` only when Projucer is built, which the workflow tolerates). Not run on GitHub: the repo has no remote yet. The preset fixture is committed, so CI needs no Chordism checkout. **Comments:** every `PLAN.md §…` reference now points at this plan's section, or says *Chordism's PLAN.md §…* for the unchanged code whose design decisions live there (§4 above). |
| 5. Live check | **Done (2026-10-25)** | Live 12.4.7, set `~/Music/Ableton/Projects/ChordChimp Phase 5 Project`, driven through the ClaudeLiveBridge Remote Script (Ableton MCP; Chordism's `LiveBridge/livebridge/client.py` also works against it on port 9873). **Side by side:** after a browser re-index Live lists ChordChimp and Chordism separately (VST3 › Alistair Rutherford); both load on their own MIDI tracks (ChordChimp: 26 parameters + Program + Device On = 28; Chordism 125), each feeding a Drift track through *MIDI From: <track> → <plug-in>*. **Test-pattern take (Init):** both recorded about 25 beats, identical note for note: C-major triads, slot = beat mod 7, every note on the beat and an eighth long. **Preset switch** (Program → Pop Anthem in both, from Live's device parameters): ChordChimp's parameters became Pop, Stack +1, Strum Up 1/64 (Test pattern stayed on); a 24-beat take from each was identical, 144 notes: Pop order I V vi IV ii iii vii°, each chord with its octave, strummed up 0.0625 beat apart, every note ending on the eighth. All meters at 0 once stopped. **Save and reopen:** ChordChimp's Key set to D on top of the preset; the set saved (Cmd-S); Live quit cleanly and the `.als` reopened in a fresh Live: the set opened unmodified, ChordChimp restored Key D, Pop Anthem's settings, Test pattern on and Program "Pop Anthem" (the restored program didn't reload over the state), Chordism Pop Anthem in C; a take after reopening played D-major Pop chords (D A Bm G with octaves) from ChordChimp and C from Chordism; meters at 0 after the stop. **Bridge quirks (not ChordChimp's):** arrangement recording ignored the start beat (always from 0) and started a few seconds late, so one short take overwrote the start of the first; the comparisons used each track's clip that starts at beat 0. **Not checked:** the editor in Live (opening, resizing, the keyboard toggle saved with the set), playing from a hardware keyboard. |
| Editor in Live | **Done (2026-10-27)** | Live 12.4.7, a copy of the Phase 5 set (`~/Music/Ableton/Projects/ChordChimp Editor Check Project`), driven by computer use; the size and keyboard flag read from the saved `.als` (the `UI` child of ChordChimp's state) and the window measured with `CGWindowList`. **Opens:** the wrench in the device header opens the editor, which draws and plays as in the snapshots; Live's plug-in window hides while Live isn't the front app (Live's behaviour). **Resizes:** dragging the corner from 720×520 to 919×689 relays out (taller pads) and Cmd-S saves `width="919" height="689"`. **Bug found and fixed: the saved size wasn't used.** Closing and reopening the window brought it back at 720×520, and the next save wrote 720×520. `ChordChimpEditor`'s constructor called `showKeyboard` (so `resized`) while the editor was 0×0, then `setResizeLimits` pushed it to the minimum; both wrote their size to `uiState` before `setSize` read it, so every editor opened at 720×520, a first one too (not the 780×560 default). The snapshot app's `state` check only round-tripped `uiState` through the processor, not into a new editor. **Fix:** the constructor reads the saved keyboard flag and size first and stores nothing until the editor is up (`sizeRestored`), so opening the editor no longer tells the host the state changed either. **Snapshot app:** a new `editor size` section: a new instance's editor opens at 780×560 without reporting a change; a reopened editor has the size the last one was left at (919×689), also after a save and restore (no change reported); one left at the smallest size reopens at it. The first three fail on the old code. It closes editors as a host does (`editorBeingDeleted` before the delete, which JUCE 8 asks of the host). **Re-checked in Live** after `Tools/install.sh`: the editor opened at the saved 720×520 with the keyboard hidden; resized to 919×689, closed and reopened, it came back at 919×689; Cmd-S saved `width="919" height="689"`, and after quitting Live and reopening the set it opened at 919×689, keyboard hidden. **Checks:** `Tests/run_tests.sh` all passed; `Tests/run_snapshot.sh` all passed; `pluginval` strictness 10 passed; benchmark worst mean 0.11% per instance. **Keyboard toggle:** Keys hides the keyboard; Cmd-S saves `keyboard="0"`; after quitting Live and reopening the set (opened unmodified, no undo history) the editor opens with the keyboard hidden. **Not checked:** playing from a hardware keyboard. |
| 6. Export drag-out | **Done (2026-10-28)** | Open question 3: Export comes back as Chordism's. **Core:** `core/midiio/ExportRecorder.h` and `MidiFile.h` copied from Chordism `07f6e1a` (namespace and names only; no `ChordImporter`): the audio thread records every note ChordChimp sends while the host plays, as a take in beats from the bar line at or before the start; Export is the last ≤ 32 completed bars, notes still sounding cut at the last bar line. **Processor:** `recorder.record` after the engine each block; `writeExport` writes `ChordChimp <Key> <Scale> <n> bars.mid` in the temporary folder for the drag; with Test pattern on, each finished take is also written to `~/Music/ChordChimp/Exports/Test take.mid` (for checks in Live), as Chordism did. **UI:** `ExportHandle` (`PresetBar.*`) in the top bar between the presets and Keys: one box with a grip and *Export n bars*, a red dot while recording, disabled with a tooltip until a bar has finished; dragging writes the file and hands it to the host. **Tests:** `Tests/test_export.cpp` (Chordism's `test_midiio` without import): file write/read round trip and hand-made files; the recorder (nothing before a bar, 6.5 bars stopped in every configuration, the last 32 of 40, held chords cut, loops run on like a tape, a start mid-bar, a new take replaces the old). Chordism's copy never reported its every-configuration check (it compared against a temporary `configs()`); fixed here. Snapshot app: a new `Export` section (after 2.25 bars the handle shows 2 bars and exports 24 notes as `ChordChimp C Major 2 bars.mid`; stopping with Test pattern on writes the test take); references updated for the handle. **Checks:** `Tests/run_tests.sh` all passed; `Tests/run_snapshot.sh` all passed; `pluginval` strictness 10 passed; benchmark worst mean 0.12% per instance (0.11% before: the recorder). Checked in Live with Phase 7. |
| 7. Black keys | **Done (2026-10-28)** | Open question 1: **borrowed chords** by modal mixture (§3.1), with their own row of pads (§5). **Core:** slots 0-6 are the white keys and 7-11 the black ones (`numWhiteSlots`, `numBlackSlots`, `numChordSlots` = 12 in `ChordModel.h`); `KeyRouter::slotOf` gives C# … A# slots 7-11 in every octave and `keyForSlot` their home-octave keys (49 … 58). `ChordGroups.h`: `borrowedChord` (C# Phrygian degree 2, D# natural minor 3, F# Lydian 4, G# natural minor 6, A# natural minor 7) and `makeBorrowedSlot`, the Key's scale's degree with per-step accidentals to the mode's notes (always within ±1), so `ChordNamer` and `RomanNumeral` need no change and a Scale change retunes held black keys like white ones. `makeGroupChords` and `makeChords` fill all twelve; the Progression reorders only the white seven. The engine needed no change: black keys go through the same performer. Test pattern still plays white keys only. **UI:** five smaller pads above the seven, each centred over its gap (C# over C\|D … A# over A\|B), darker when idle, with the key letter, numeral and name and the last-played stripe; the pads area grows to 300 px (both rows). **Tests:** `test_chords` checks the notes, numerals and names by hand (C major triads: bII Db, bIII Eb, #iv* F#dim, bVI Ab, bVII Bb; sevenths: Dbmaj7, Ebmaj7, F#m7b5, Abmaj7, Bb7; A natural minor: bII Bb, III C, #iv* D#dim, VI F, VII G) and that all 13,260 black-key chords (12 keys × 13 scales × 17 groups × 5, under Pop) are their mode's chord; `test_engine`'s router covers slots 7-11, C# at 73 plays bII and ends on key-up, A# under Pop plays bVII and follows a Key change while held; `test_theory` compares only the white seven with music21. The snapshot app clicks pad D# (above the D\|E gap) and gets Ebmaj7 under Pop; references and `docs/screenshot.png` updated. **Checks:** `Tests/run_tests.sh` all passed; `Tests/run_snapshot.sh` all passed; `pluginval` strictness 10 passed; benchmark worst mean 0.13% per instance (its keys now include black ones that play). **In Live** (12.4.7, the editor-check set, after `Tools/install.sh`; Phases 6 and 7 together): Init, Test pattern off; the editor shows the black row (bII Db, bIII Eb, #iv* F#dim, bVI Ab, bVII Bb over I … vii* in C); with the transport running, holding pads C#, D#, F#, G#, A# then C for about a second each lit each pad, and the handle counted the bars with its recording dot; after the stop, dragging the handle wrote `ChordChimp C Major 10 bars.mid` (read back: Db, Eb, F#dim, Ab, Bb, C at beats 0.33, 2.83, 5.53, 8.03, 10.50, 13.00) and dropping it on Live's track area made a MIDI track whose clip has those 18 notes at those beats. Live sizes the clip to its notes (4 bars, not 10): the file's track ends at its last note-off, as Chordism's does. A first drop made with separate mouse moves didn't take; a real click-drag did. **Not checked:** a hardware MIDI keyboard. |
