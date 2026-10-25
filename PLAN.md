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

**Out of scope:** Windows, AU, AAX, the Smart Scale, the Chord Editor grid, MIDI import and export,
Instance Sync, QWERTY playing, plug-in hosting.

---

## 2. Feature map (Chordism → ChordChimp)

| Area | Chordism | ChordChimp |
|---|---|---|
| Keyboard | Split (default MIDI 60): chord zone below, Smart Scale above | **No split.** The whole keyboard is the chord keyboard (§3.1) |
| Global | Key, Scale (13), Chord Group (17), Progression (8 presets + Custom) | **Kept**, except Progression *Custom* and its 7 *Custom n* degrees (**dropped**) |
| Global | Chord Octave, Voice leading (Off / Smooth) | **Kept** |
| Chord keys | White keys = 7 chords, with name and Roman numeral | **Kept** (display only: no right-click degree or chord-type menus) |
| Chord keys | Black keys = 5 shifters (group or key/scale), Toggle/Standard | **Dropped.** Black keys do nothing (§3.1) |
| Chord Editor | 7 degrees × 7 steps grid (on, accidental, octave mask), chord-type menu | **Dropped.** Chords are the Chord Group's shapes |
| Chord FX | Stack, Strum + Strum Rate + Strum time (ms), Arp (12 patterns, rate, range, gate), Repeat (rate, gate) | **Kept**, except *Strum time (ms)* (§3.3) |
| Chord FX | Step Sequencer: on, 4 editable 16-step patterns, Swing, Gate, Sustain | **Simplified:** on, choice of the 4 factory patterns, Swing, Gate. No step editing, no Sustain |
| Chord FX | Velocity Algorithm (13 curves) + Peak + Range, Hold (Off / Latch / Bar) | **Kept** |
| Smart Scale | 5 mappings, Inversions, Note Repeat, 5 pattern slots (arp/melody), its own Octave, Stack, Step Seq, Velocity, Hold | **Dropped** |
| Smart Scale | Look-ahead transposing (`ChordTimeline`) | **Dropped** |
| Routing | Mute CG, Mute SS | **Dropped** (one zone; Enable covers it) |
| I/O | Enable (pass input through) | **Kept** |
| I/O | MIDI import (drag a .mid in), Export drag-out (32 bars) | **Dropped** (open question 3) |
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
  black C# D# F# G# A# → nothing (swallowed, not passed through)
```

- This is Chordism's chord zone, extended over the whole keyboard: a 25-key controller has every
  chord under either hand, and the controller's octave buttons do nothing harmful.
- **Black keys are swallowed** (not passed through), so a stray black key never sounds an
  out-of-key note on the instrument. Open question 1 lists the alternatives.
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
  and a stripe on the last-played one. Clicking plays through the engine (Chordism's lock-free
  queue), exactly like host MIDI.
- **Controls** build themselves from the parameters (`ParamControl`), so the snapshot test proves
  every parameter has a control.
- **No Setup page:** nothing is left that needs one. *Test pattern* is reachable from Live's device
  parameters only.
- The output keyboard can be hidden; the size and that choice are saved in the plug-in state.

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
   play the chord on that semitone as a borrowed chord)?
2. **Key octave.** Ignored, with Chord Octave setting the register (this plan, as Chordism's chord
   zone), or should the octave played move the chord's register?
3. **Anything dropped that should stay?** The candidates are Progression *Custom*, Export
   drag-out, and QWERTY playing.
4. **Bright Melody:** keep the name with a chord arp in place of the melody (this plan), rename it,
   or drop it?
5. **Name:** *ChordChimp*, from this folder's name; it is fixed for good in Phase 0.

---

## 11. Status

| Phase | State | Notes |
|---|---|---|
| 0. Fork and identity | **Done (2026-10-25)**, except the check in Live | Copied from Chordism at `07f6e1a` (`Source/`, `Tests/`, `Tools/`, CI; not LiveBridge or the Live test set). Renamed throughout: namespace `chordchimp`, `ChordChimpProcessor`, `ChordChimpEditor`, `ChordChimpEngine` (becomes `ChordEngine` in Phase 1), bundle ID `com.alistairrutherford.chordchimp`, plug-in code `Cchp` (VST3 class IDs differ from Chordism's), presets in `~/Music/ChordChimp/Presets`. `PresetManager::readFile` also reads Chordism preset files (root tag `Chordism`). Checks: Release build; `Tests/run_tests.sh` all passed; `Tests/run_snapshot.sh` all passed (reference images updated for the new logo); `pluginval` strictness 10 passed; `Tools/install.sh` installed `ChordChimp.vst3` and `ChordChimp.app` next to Chordism. **Not checked:** loading both in Live (Live wasn't running). |
