# ChordChimp: Key detection plan

A plan (2026-11-02; revised 2026-11-03 for Analyse mode and the bridge), **built and checked in Live
2026-11-03** (Phases 9-11; see *As built* below and PLAN.md §11). It adds one
feature to ChordChimp as it stands after Phase 8 (`b3c0d55`): **an Analyse mode that stops
playing chords, listens to the incoming MIDI notes and guesses their key, shows the guess next to
the Key menu, and sets Key to it in one step: from the editor, or from Live's device parameters
(so the ClaudeLiveBridge can do it).** Section numbers like §3.1 refer to
[PLAN.md](../PLAN.md); this plan's own sections are K1-K8.

**Recommendation in one paragraph:** a new host parameter, **Analyse** (Off / Listen / Use
guess), appended as the 28th and kept out of presets and saved state (K5.4). In **Listen**,
ChordChimp ends its chords and passes the input through unchanged while it analyses it (K2).
Setting **Use guess** (a click on the guess, or the bridge's `set_parameter`) sets Key, and Scale
if needed, to the guess and returns to Off, so chords play in the new key (K5.3). A
Krumhansl-Schmuckler estimate (Krumhansl-Kessler profiles, 24 major and minor keys) runs over a
pitch-class histogram weighted by duration and velocity, cleared when Listen starts and decayed
with an 8 s half-life of listening time, with hysteresis (K3). It runs on the audio thread in
header-only plain C++ in `Source/core`, fixed-size, and reaches the editor as one packed atomic
(K4). The guess and the Analyse control share one small column, **Heard**, between Key and Scale
(K5). For the bridge to read the guess, the test rig writes it to a file, as it does the test
take (K5.5). Three phases, about 2¾ days (K7).

### As built (2026-11-03)

The plan below held, with these changes from tuning on the corpus (Phase 9) and from the layout
check (Phase 10):

| Plan | Built | Why |
|---|---|---|
| Krumhansl-Kessler profiles unless Temperley's do better | **Temperley's** (Kostka-Payne, as music21 has them) | The bake-off in `test_keys`: both get the 13 required tunes and progressions right, but Temperley 180 of ChordChimp's 192 own-chord cases against 156; the Dorian tunes come out D minor |
| Half-life 8 s | **12 s** | At 8 s the histogram follows single chords: a four-chord loop at 100 bpm is ~10 s |
| First guess as soon as it's Tentative | **after it has been the best for 2 s** | Random notes flickered through two wrong keys first |
| A change after a 0.05 lead for 1.5 s | the same, **or any lead for 4 s** | A wrong early key otherwise stayed forever when the right one led by less than 0.05 |
| A modulation in about 6-10 s | **within 20 s** (C→G 19.5 s, Am→Cm 15 s via C major) | Close keys cross over after about one half-life; the hold barely matters |
| Bass weighting only if the corpus asks | **none** | Weighting the lowest note helped ChordChimp's own chords but broke I-V-vi-IV |
| Column weights for the globals | **sized to their content**: each menu its longest choice, Heard up to its longest guess, the rest by weight; combo text insets 3 px, not 5 | No weights fit both 720 and 780 px; Key's "C#/Db" was already squashed at 720 before this work |
| Open question 5 | notes **pass through** while listening | The user's answer (2026-11-03) |
| Open question 8 | **Analyse** | The user's answer (2026-11-03) |

Known misses, recorded in the tests rather than tuned away: the 12-bar blues in sevenths (F major
and C major tie), Am F C G (it is also vi IV I V in C), and natural minor sevenths under Canon
(i7 v7 VImaj7 IIImaj7, heard as the relative major in all 12 keys).

---

## K1. Goals

| # | Requirement | Acceptance criterion |
|---|-------------|----------------------|
| KD1 | Guesses the key of music played into ChordChimp in Analyse mode | Core corpus (K6): the right tonic and mode for every unambiguous case; ambiguous cases show no guess or a tentative one |
| KD2 | Shows Analyse and the guess beside Key | Snapshot app: the Heard column shows "Analyse", "Listening…", "A minor?" or "A minor 82%"; it fits from 720×520 to 1400×960 |
| KD3 | One step sets Key (and Scale if needed) to the guess and goes back to playing chords | Snapshot app: a click on the guess, or the host setting Analyse to *Use guess*, sets Key and Scale inside begin/end gestures and Analyse to Off |
| KD4 | **The bridge can do all of it** | In Live, through device parameters only: Analyse → Listen, play a clip, read the guess, Analyse → Use guess, read Key and Scale (K6.4) |
| KD5 | Real-time safe and cheap | 0 allocations in `processBlock` while listening (malloc hook); benchmark worst mean stays under 0.2% per instance (0.14% now) |
| KD6 | Changes nothing else | With Analyse Off, output is byte-identical to Phase 8 (existing tests unchanged); in Listen, notes pass through unchanged |

**Out of scope:** detecting the key from audio, from Live's song scale (a plug-in can't read
it), or from ChordChimp's own output (K2); detecting modes (Dorian, Mixolydian …) beyond major
and minor (open question 3).

---

## K2. Where the notes come from

ChordChimp's input isn't music in a key. A white key picks a chord slot by its letter and a black
key a borrowed chord (§3.1), so a player who wants the IV chord presses F whatever the song's key
is. A histogram of those key presses describes the player's hand, not the key. The input has to
stop being chord selection while it's analysed.

| Source | How it works | What it means in Live | Verdict |
|---|---|---|---|
| **A. The input in Analyse mode** (recommended) | Analyse → Listen: chords end, every input note passes through unchanged (the engine's Enable-off path, `HeldKey::passThrough`) and is analysed. | Click **Analyse** (or the bridge sets it), then play, or let a clip play on ChordChimp's own track (a MIDI clip there goes through it), or set the track's *MIDI From* to another track for a moment. The instrument plays what's analysed. Click the guess: Key is set and chords are back. | **Recommended.** It's "the notes passed into the plug-in", the intent is explicit, it's one control in and one out, and it's reachable from Live's device parameters. |
| A′. The input whenever Enable is off | The same, but tied to Enable (this plan's first version). | Turn On off, play, click the guess, turn On on. | **Superseded by A:** two controls for one job, nothing to clear old evidence, and Enable off for other reasons feeds the detector. |
| B. A separate "listen" input | Notes from a second source (a second MIDI input bus, or a MIDI channel) are analysed while the player's keys keep playing chords. | A Live MIDI track has one input (one port, *All Ins*, or one other track), and Live gives a VST3 one MIDI input, so a second source can't reach ChordChimp next to the player's keyboard. (Checked in Phase 11.) | **Rejected:** in Live it reduces to A plus a channel filter clips can't use. |
| C. ChordChimp's own output | Analyse the chords ChordChimp plays. | The output is built from Key and Scale, so the answer is the Key already set. | **Rejected:** circular. Useful as a test corpus (K6). |
| D. A dropped clip | Drag a MIDI clip onto the editor; the message thread reads it (`readMidiFile`, `MidiFile.h`) and analyses it whole. | One drag, no routing; but not "notes passed into the plug-in", and MIDI import was dropped in Phase 1. | **Not in this plan:** a cheap follow-up on the same core (open question 2). |

**Decisions that follow from A:**

- **Entering Listen ends every chord and clears the evidence**, so each analysis starts fresh
  (no *Forget* needed). Leaving it (Off or Use guess) ends the passed-through notes, as an Enable
  switch does (`releaseAll`), and chords play again from the next key.
- **The notes pass through while listening**, so you hear what's analysed (open question 5 asks
  whether to mute them instead). The test pattern doesn't play in Analyse mode.
- **Enable is unchanged.** Enable off no longer means anything for detection; with Enable off
  and Analyse on, notes pass through either way and are analysed.
- **Silence doesn't forget.** The evidence decays only during *listening time*: while a listened
  note sounds, and for 2 s after the last one ends. A guess changes only when new notes argue for
  another key.
- **The guess outlives the mode.** Leaving with Off keeps the last guess (shown in the tooltip,
  and usable with Use guess) until the next Listen clears it. It isn't saved with the set.
- **The on-screen keyboard** plays through the engine like host input (Phase 5), so in Listen
  what you play on it is analysed too.

---

## K3. The algorithm

### K3.1 Evidence: a decayed pitch-class histogram

`w[pc]`, 12 numbers, zeroed when Listen starts. A listened note adds to its pitch class:

| Part | Amount | Why |
|---|---|---|
| Onset | `0.25 × v` at the note-on | Short notes still count (staccato melodies, short clip notes) |
| Duration | `dt × v` for each second it's held, **at most 2 s per note** | Long notes weigh more (the profiles were measured on durations), but one pedal tone can't swamp the rest |
| Velocity weight | `v = 0.5 + 0.5 × velocity / 127` | Ghost notes count less, loud ones a little more; never zero |

Every pitch class decays together: `w *= 2^(-dt / H)` with **half-life H = 8 s** of listening
time (about 4 bars at 120 bpm), so a long analysis that crosses a modulation follows it. A
decayed count of onsets `n` and of distinct pitch classes with weight above 5% of the largest
(`distinct`) decay the same way, for the "too little to go on" test. Time is seconds (samples ÷
sample rate), not beats, so it works with the transport stopped. MIDI channels are ignored.

No bass weighting in the first version: it helps with chords (the lowest note is often the
root), but listened input is as often a melody. It's a one-line change if the corpus says
otherwise.

### K3.2 Estimate: Krumhansl-Schmuckler with Krumhansl-Kessler profiles

For each of the 24 keys (12 tonics × major, minor), the Pearson correlation `r` between `w` and
the key's profile rotated to its tonic. The best key is the guess; `r` is its **fit**.

| | 1 | ♭2 | 2 | ♭3 | 3 | 4 | ♯4 | 5 | ♭6 | 6 | ♭7 | 7 |
|---|---|---|---|---|---|---|---|---|---|---|---|---|
| Major | 6.35 | 2.23 | 3.48 | 2.33 | 4.38 | 4.09 | 2.52 | 5.19 | 2.39 | 3.66 | 2.29 | 2.88 |
| Minor | 6.33 | 2.68 | 3.52 | 5.38 | 2.60 | 3.53 | 2.54 | 4.75 | 3.98 | 2.69 | 3.34 | 3.17 |

- **Why this one:** it's the standard method, the numbers are well documented, correlation
  doesn't care about the histogram's units (so the weighting above can change freely), and it's
  288 multiply-adds per estimate. Temperley's (Kostka-Payne) profiles are sharper and are known
  to confuse relative major and minor less; they're a table swap in one header. **Phase 9 runs the
  corpus with both and keeps KK unless Temperley's gets more cases right**, then fixes the choice
  with a test.
- A flat histogram (variance 0: nothing, or all 12 pitch classes equally) has no correlation:
  no guess.
- Ties are broken by fixed order (major before minor, then lower tonic), so results are
  deterministic.

### K3.3 Confidence: when to show a guess

The runner-up is the second-best key. Starting values, tuned once on the corpus in Phase 9 and
then fixed as constants with tests:

| State | When | Shown as |
|---|---|---|
| **None** | `n < 6` onsets, or `distinct < 3`, or fit < 0.50, or the histogram is flat | "Listening…" |
| **Tentative** | fit 0.50-0.70, or within 0.04 of the runner-up | "A minor?" (dim) |
| **Confident** | fit ≥ 0.70 and ahead of the runner-up by ≥ 0.04 | "A minor 82%" (accent) |

The percentage is the fit (`round (100 × r)`), labelled in the tooltip as *how well the notes fit
A minor's profile*, not a probability. The tooltip also names the runner-up ("next: C major
76%"), since the relative major or minor is often close behind. Open question 4 is whether to
show the number at all.

### K3.4 Stability: hysteresis

- Estimates are made every **50 ms of listening time** (not per block, so results don't depend on
  block size).
- The **first** guess shows as soon as the state is Tentative or better.
- **Changing** the shown key needs a challenger that beats the shown key's own current fit by
  ≥ 0.05 at every estimate for **1.5 s** of listening time. One estimate that falls short starts
  the 1.5 s again. The shown key's fit and state keep updating meanwhile.
- With H = 8 s and the hold, a clear modulation (16 s of C major, then G major) moves the guess
  to G within about 6-10 s of the change, and moves it once. Phase 9 measures this and fixes the
  bound in a test.

### K3.5 Mapping onto ChordChimp's 13 scales

The guess is a tonic and major or minor. The 13 scales sort into families by their tonic triad,
computed from `scaleInfo` (3rd and 5th), not a hand table:

| Family | Scales |
|---|---|
| Major (3rd 4, 5th 7) | Major, Lydian, Mixolydian, Harmonic Major, Phrygian Dominant, Double Harmonic |
| Minor (3rd 3, 5th 7) | Natural Minor, Harmonic Minor, Melodic Minor, Dorian, Phrygian, Hungarian Minor |
| Neither (5th 6) | Locrian |

**Using a guess sets Key to its tonic, and changes Scale only if the current Scale isn't in the
guess's family:** to Major for a major guess, Natural Minor for a minor one (Natural Minor's V is
already major, `defaultAccidental`, so a harmonic-minor tune still gets its dominant). So a
player who chose Dorian and plays a D minor tune keeps Dorian, and only Key moves. Locrian is in
neither family, so it's always replaced. Telling Dorian from Natural Minor (and so on) means
weighing one or two scale degrees, which short input rarely settles; that's open question 3.

The guess is spelled the conventional way for its key, independent of the Key menu's "C#/Db":
major C Db D Eb E F F# G Ab A Bb B; minor C C# D Eb E F F# G G# A Bb B.

---

## K4. Real-time safety and where the code goes

The engine's rules hold (Chordism's PLAN.md §5): no allocation, locks or waits on the audio
thread; header-only plain C++ in `Source/core`; JUCE only in the adapter. Parameters set from
inside the plug-in are set **on the message thread only**, as program changes already are
(`pendingProgram`, loaded by the processor's timer).

| Piece | File | What it holds |
|---|---|---|
| Profiles and estimate | `Source/core/theory/KeyProfiles.h` (new) | The two profile tables; `estimateKey (const std::array<double, 12>&)` → best and runner-up (tonic, minor, fit); `scaleFamily (Scale)`; `scaleForGuess (current, guess)` (K3.5); the guess's spelling. Pure functions, `noexcept` |
| The listener | `Source/core/engine/KeyListener.h` (new) | `std::array<double, 12>` weights; per-note held start, credited time and velocity (`std::array<…, 128>`); the decayed onset count; listening time; the hysteresis state; the `KeyGuess`. `reset()`, `noteOn (note, velocity, t)`, `noteOff (note, t)`, `allUp (t)`, `advance (t)`, `guess()` |
| Engine | `Source/core/engine/ChordEngine.h` | `EngineSettings::analysing` (Analyse is Listen or Use guess). `applySettings`: a change of `analysing` releases everything (like an Enable change), and Off → on calls `listener.reset()`. `handle`: while analysing a note-on is a pass-through (as with Enable off) and calls `listener.noteOn`; a pass-through key's `keyUp` calls `noteOff`; `releaseAll` calls `allUp`. The test pattern is off while analysing. Time is a running sample count (`int64_t`) ÷ `t.sampleRate` plus the event's sample. `advance` once at the end of `process`. `getKeyGuess()` |
| Parameter | `Source/plugin/Parameters.*` | `params::analyse` (K5.4); `Snapshot::engineSettings` sets `analysing` |
| Publishing | `ChordChimpProcessor.*` | `std::atomic<uint64_t> keyGuess`, stored after `engine.process` each block like `lastChordSlot`, relaxed |
| Using the guess | `ChordChimpProcessor.*` | `applyGuess()` (message thread, K5.3); the 20 Hz timer calls it when the Analyse parameter reads *Use guess*; the test rig's guess file (K5.5) |
| Reading | `Source/ui` | `UiModel` gains a `KeyGuess` and the Analyse state, read in the editor's 30 Hz timer; `GlobalRow::refresh` |

- **Why a packed atomic, not `SnapshotExchange`:** `KeyGuess` fits in 64 bits (tonic −1..11,
  minor, state, fit 0-100, runner-up tonic, minor and fit), any number of threads can read it
  (the editor, the processor's timer, the snapshot app), and it needs no "changed" test.
  `SnapshotExchange` stays for `ChordDisplay`.
- **Why the timer, not a parameter listener:** a host's parameter change can arrive on the audio
  thread (in VST3, with the block), where setting Key and Scale with gestures isn't allowed. The
  timer polls the raw value instead, so no listener runs anywhere, and the audio thread meanwhile
  treats *Use guess* as still analysing (no chord sounds in the old key in between). The delay is
  at most 50 ms.
- **Cost:** per block, 12 multiplies for the decay (one `std::exp2`), a loop over the notes
  actually held (a 128-bit mask, as `publishNoteStates` uses), and at most one 288-MAC estimate
  per 50 ms. Nothing at all while Analyse is Off. Negligible next to the 0.14% the engine costs
  now.
- **Numbers stay sane:** weights that decay below 1e-12 are set to 0 (no denormals; the processor
  already has `ScopedNoDenormals`); `dt` is clamped at 0; a sample rate of 0 skips the block.
- **Nothing else changes:** `KeyRouter`, `ChordPerformer` and `VoiceTracker` are untouched; the
  listener only reads input that's passing through.

---

## K5. UI and the parameter

### K5.1 Where: a Heard column in `GlobalRow`

```
│ Key       Heard          Scale            Chord Group     Progression          Octave  Key octave │
│ [A    ▾]  [A minor 82% ✕] [Natural Minor▾] [Sevenths   ▾]  [Pop (1-5-6-4)    ▾] [ 0  ]  [Ignore ▾] │
```

`GlobalRow::resized` gives columns by weight: today Key 70, Scale 150, Group 130, Progression
180, Octave 110, Key octave 96 (736 in all; at the 720 px minimum the row is 696 px wide, so a
factor 0.95). Heard goes **right after Key**, weight 100, and the wide columns give up room so the
factor stays near 0.9:

| Column | Now | With Heard | Width at 720 px (factor 0.89) |
|---|---|---|---|
| Key | 70 | 72 | 64 |
| **Heard** | — | 100 | 89 |
| Scale | 150 | 144 | 128 |
| Chord Group | 130 | 124 | 110 |
| Progression | 180 | 170 | 151 |
| Octave | 110 | 76 | 68 (a −3..3 slider) |
| Key octave | 96 | 96 | 85 |

The weights are a starting point; the rule the snapshot check enforces (K6) is that **every
menu's longest choice and the longest Heard text fit their box at 720 px**, with the theme's
font. If "F# minor 82%" and the ✕ don't fit, the text drops to "F#m 82%", then "F#m": the column
picks the first form that fits. Nothing moves vertically; the row stays 42 px.

### K5.2 What it shows

The box is the Analyse control (`HeardKey`, in `Panels.*`; it registers `params::analyse` with
`Bindings`, so the "every parameter has a control" check counts it). It follows the parameter,
so a change from Live or the bridge shows at once.

| Analyse | Guess | Box shows | Click the box | Click ✕ |
|---|---|---|---|---|
| Off | — | "Analyse" | Listen | — |
| Off | matches Key and Scale's family | "✓ A minor" | Listen | — |
| Listen | none yet | "Listening…", a dot in the caption (like Export's recording dot), ✕ | nothing | Off |
| Listen | Tentative | "A minor?" (dim), ✕ | Use guess | Off |
| Listen | Confident | "A minor 82%" (accent), ✕ | Use guess | Off |
| Use guess | — | as Listen, for the ≤ 50 ms until the timer acts | — | — |

Tooltips say what a click does: *Analyse: stop playing chords and listen to the notes coming
in*; *Use A minor: set Key to A (Scale to Natural Minor) and play chords again*.

### K5.3 Using the guess

`ChordChimpProcessor::applyGuess()`, on the message thread, is the one path for the editor's
click and for the host or bridge setting *Use guess* (seen by the timer):

1. Read `keyGuess`. If there's a guess (Tentative or better, from this analysis or the last one):
   Key = its tonic, Scale = `scaleForGuess` (K3.5), Analyse = Off.
2. If there's none: Key and Scale stay, Analyse = Listen (a bridge that asked too early keeps
   listening; from Off it starts an analysis).
3. The changed parameters are set together: a new `Bindings::setTogether`-style helper in the
   processor begins every gesture, sets every value, then ends every gesture, so a host that
   groups by gesture can make it one undo step. Phase 11 counts the steps Live records (open
   question 6).

Like any Key change, held chords retune; there are none at that moment anyway (Analyse ended
them). If Key is automated in Live, using the guess overrides the automation as a mouse change
would (Live lights *Re-enable automation*).

### K5.4 The Analyse parameter

| | |
|---|---|
| ID, name | `analyse`, "Analyse" (ChordChimp's own ID, like `keyOctave`) |
| Type | Choice: **Off** (default), **Listen**, **Use guess** |
| Position | Appended as the 28th; `params::numParameters` = 28 |
| Presets | Not a preset parameter (`isPresetParameter`, like Enable and Key octave): loading a preset leaves it alone |
| Saved state | **Not saved:** removed from the tree `getStateInformation` writes and ignored by `setStateInformation`, so a set saved mid-analysis opens with Analyse Off and chords playing |
| Automation | Left automatable, since Live lists and the bridge sets parameters through the same path. *Use guess* is a command, so automating it fights the plug-in, which sets it back to Off; the parameter's name says nothing, but the README will |

**Why a host parameter now:** the bridge reaches ChordChimp only through Live's device parameters
(PLAN.md §8, `Parameters.h`), and the user asked for the bridge to switch Analyse on and use the
guess. **Why one parameter with three values, not two switches:** one new parameter instead of
two, and the mode and the command can't disagree (Listen and Use guess are both "analysing").
**The cost:** Live keeps a plug-in's parameter list with the device in a set, so a ChordChimp
saved before this phase lists 29 device parameters, not 30, until it's put on the track again
(HANDOFF, *Testing in Live*), as Phase 8 found with Key octave.

No parameter holds the guess itself: a read-only parameter would have to be written by the
plug-in whenever the guess changes, which marks the set as edited, adds undo steps and records
automation. The bridge reads it from a file instead (K5.5).

### K5.5 The guess for the bridge

While **Test pattern** is on (the LiveBridge rig, as for `Test take.mid`), the processor's timer
writes the guess whenever it changes to `~/Music/ChordChimp/Exports/Heard key.json`:

```json
{ "analyse": "Listen", "state": "confident", "key": "A", "mode": "minor", "fit": 0.82,
  "runnerUp": { "key": "C", "mode": "major", "fit": 0.76 }, "notes": 41 }
```

`state` is `none` before there's a guess. The bridge can then use the guess (Analyse → Use guess)
or set Key and Scale itself. Phase 11 also tries whether Live shows a changing value text for
Analyse (for example "Listen: A minor 82%"); if Live re-reads it, the bridge could read the guess
from the parameter's display text with no file, but the plan doesn't rely on it.

---

## K6. Testing

### K6.1 Core tests: `Tests/test_keys.cpp` (new, added to `Tests/run_tests.sh`)

| Group | Cases | Expected |
|---|---|---|
| Profiles | Each of the 24 profiles fed in as a histogram | Its own key, fit 1.0 |
| Transposition | Every case below in all 12 keys | The same answer, transposed; same fit to 1e-9 (KS is transposition-invariant, so this tests the indexing, not the method) |
| Melodies (public-domain tunes, hand-entered note lists with durations) | Twinkle Twinkle, Ode to Joy, Frère Jacques, Amazing Grace (major); God Rest Ye Merry Gentlemen, House of the Rising Sun (minor); Greensleeves, Scarborough Fair, Drunken Sailor (Dorian) | Major and minor tunes: right tonic and mode, Confident, within the first 8 bars at 100 bpm. Dorian tunes: the result is recorded (minor is hoped for, the relative major is the known KS failure) and informs open question 3, rather than tuned for |
| Progressions as chords (block chords, a bar each, ending on the tonic held 2 bars) | I-V-vi-IV, I-vi-IV-V, ii-V-I, I-IV-V-I; i-iv-V-i (raised 7th), i-bVII-bVI-V, i-bVI-bIII-bVII; a 12-bar blues in sevenths (I7 IV7 V7: expect major) | Right tonic and mode |
| ChordChimp's own output (source C, as a corpus) | Run `ChordEngine` with Analyse Off for every Key × {Major, Natural Minor} × {Triads, Sevenths} × {Pop, Fifties, Two-Five-One, Canon}, playing the white keys in order and ending on the key whose slot is the tonic, and feed the output notes to a separate `KeyListener` | The Key that played them, in every case (192 cases) |
| Modulation | 16 s of C major melody, then 16 s in G major; then A minor → C minor | Exactly one change each time, within the bound Phase 9 fixes (about 10 s) |
| Silence | Nothing; then a tune followed by 60 s of silence | No guess; then the tune's guess, unchanged after the silence (listening time stops) |
| Ambiguous input | One note repeated; a fifth (C G); a C major triad held; all 12 pitch classes evenly; a whole-tone scale; the 7 diatonic triads of C played evenly | No guess, or Tentative; the evenly-played diatonic set may give C major or A minor, but only Tentative |
| Weighting | The same tune with every other note at velocity 20; with one note held 30 s | Same guess; the long note counts as 2 s |
| Hysteresis | Input built to sit within 0.03 of the boundary between two keys for 60 s | At most one change |
| Scale mapping | Every current Scale × major and minor guesses | `scaleForGuess` as K3.5: same-family scales kept, others → Major / Natural Minor, Locrian always replaced |
| Time base | A tune through `ChordEngine` in Listen at 44.1, 48 and 96 kHz and blocks of 32-2048 | The same guess; switch times within one 50 ms estimate |
| Engine | Off: nothing analysed, output as Phase 8. Off → Listen while chords are held: they end, the evidence is cleared, input passes through. Listen → Off while notes are held: they end, chords play from the next key. *Use guess* plays like Listen. Enable off with Analyse Off: not analysed. All-notes-off, transport stop. On-screen notes in Listen. Test pattern on in Listen: silent | As K2; no held note left in the listener or the `VoiceTracker` |
| Fuzz | The stuck-note fuzz gains random Analyse changes; 10 minutes of random notes, velocities and sample rates into a `KeyListener` | `VoiceTracker` empty at the end; weights finite and ≥ 0, fit in −1..1, `KeyGuess` fields in range |

### K6.2 Snapshot app (`Tests/run_snapshot.sh`)

- **From the editor:** clicking "Analyse" sets the parameter to Listen; an A-minor tune fed
  through `processBlock` makes the box show "A minor" and a percentage; clicking it sets Key = A,
  Scale = Natural Minor (from Major) and Analyse = Off, each between a gesture begin and end (a
  listener on the processor records the calls); ✕ returns to Off and leaves Key alone.
- **From the host (the bridge's path):** the parameter set to Listen from another thread, as a
  host would; the box follows; after the tune, set to *Use guess*: after the next timer callback
  Key, Scale and Analyse are set as above. *Use guess* with nothing heard leaves Key and goes to
  Listen. From Dorian, a D-minor guess sets Key D and keeps Dorian.
- **The test rig file:** with Test pattern on, `Heard key.json` is written with the guess, and
  rewritten when it changes; with Test pattern off it isn't written.
- **State and presets:** a state saved in Listen restores with Analyse Off; loading a preset leaves
  Analyse alone; 28 parameters, every one but Test pattern with a control; the preset comparison
  is unchanged (24 preset parameters, 525 values).
- **Layout fits** at 720×520, 780×560, 1200×800 and 1400×960: no two globals overlap, and every
  menu's longest choice and the Heard box's longest text (in the form it picks) fit their bounds,
  measured with the theme's font.
- **References:** `editor`, `editor-small` and `editor-large` updated (the new column), and a new
  `editor-heard` with a confident guess showing; `docs/screenshot.png` updated.
- **Hardening:** the malloc-hook check gains a run in Listen with a dense tune (0 allocations);
  the random-state and NaN checks cover the new parameter.

### K6.3 pluginval and the benchmark

- `Tests/run_validation.sh` at strictness 10. pluginval sets every parameter at random, so
  *Use guess* arrives with no guess, mid-block and repeatedly; it must be harmless, and the plug-in
  setting parameters back from its timer must not upset pluginval's parameter tests.
- `Tests/run_bench.sh` gains a fourth scenario, **listening** (Analyse Listen, a 16-note-per-bar
  tune passing through). Every scenario's worst mean must stay under 0.2% per instance.

### K6.4 In Live, through the ClaudeLiveBridge (HANDOFF, *Testing in Live*)

On a **new** ChordChimp in the test set (after `Tools/install.sh` and a browser re-index), steps
1-4 through the bridge alone:

1. `list_parameters` shows Analyse (30 device parameters: 28, Program, Device On). A MIDI clip of
   an A-minor tune on ChordChimp's own track; Test pattern on; `set_parameter` Analyse = Listen.
2. Play 8 bars. The Drift track plays the tune unchanged (meters, or a recorded take compared
   with the clip). `Heard key.json` says A minor, confident.
3. `set_parameter` Analyse = Use guess. Within a second: Key = A, Scale = Natural Minor,
   Analyse = Off.
4. Play on: the test pattern's take (`Test take.mid`) is A-minor chords.
5. By computer use (Live in front): click Analyse, see "Listening…" then the guess, click it; the
   same result. Cmd-Z: count the undo steps until Key and Scale are back (open question 6).
6. Routing variant: the tune on a second track, ChordChimp's track *MIDI From* that track
   (`set_routing`), Analyse = Listen through the bridge: the same guess.
7. Save while in Listen, quit and reopen: Analyse Off, chords play, no guess; Key and Scale as
   last applied.
8. Confirm Live offers ChordChimp only one MIDI input (K2, source B), and whether Live's display
   text for Analyse can show the guess (K5.5).

Not checkable by the bridge: playing from a hardware keyboard (still unchecked since Phase 5).

---

## K7. Phases and estimates (rows for PLAN.md §6)

| Phase | Est. | Deliverable | Exit criteria |
|---|---|---|---|
| **9. Key detection core** | 1 day | `KeyProfiles.h`, `KeyListener.h`, `Tests/test_keys.cpp` with the corpus (K6.1); KK vs Temperley run on the corpus; thresholds, half-life and hold tuned, then fixed | `Tests/run_tests.sh` passes, including every K6.1 group but the engine ones; the chosen profile and constants recorded in PLAN.md §11 |
| **10. Analyse mode** | 1¼ days | The `analyse` parameter (K5.4), the engine's analysing path, `keyGuess`, `applyGuess` and the timer, `Heard key.json`, `UiModel`, `HeardKey` in `GlobalRow`, the bench scenario | `Tests/run_tests.sh`, `Tests/run_snapshot.sh` (K6.2, new references), `pluginval` 10, benchmark worst mean < 0.2%; then PLAN.md §11, README and HANDOFF updated; one commit |
| **11. Analyse in Live** | ½ day | The K6.4 run | Steps 1-8 pass (1-4 by the bridge alone), undo steps recorded; HANDOFF updated |

**Total: about 2¾ working days.** Phases 9 and 10 are one commit each, as before. A Phase 12 for
dropped clips (source D) or mode detection would follow only if open questions 2 or 3 say so
(about ½ day and 1 day).

PLAN.md would also gain Analyse mode in §3.1, the 28th parameter in §3.5's table (group
*Keyboard*, not in presets or saved state), the column in §5's sketch, and the new test file and
checks in §8.

---

## K8. Open questions for the user

Answered by Analyse mode: a way to forget what was heard (each Listen starts fresh) and whether
using the guess turns chords back on (it does: Analyse goes to Off).

1. **The source (K2).** Analyse mode (recommended), or something the plan rejects: a second input
   (not possible in Live as far as we know), or analysing ChordChimp's own chords?
2. **Dropping a clip (source D).** Worth a follow-up phase? One drag with no routing, reusing the
   reader in `MidiFile.h`, but it brings back a drop target (MIDI import was dropped in Phase 1).
3. **Scale on use (K3.5).** Keep the current Scale when it's the right family (recommended),
   always set Major or Natural Minor, or try to detect modes (Dorian, Mixolydian, Harmonic Minor
   …)? Modes need longer input and are wrong more often.
4. **The number.** "A minor 82%" (the fit, explained in the tooltip), words instead ("A minor,
   likely" / "A minor?"), or no number?
5. **Through or muted.** While listening, pass the notes through so you hear them (recommended),
   or mute them? Through doubles a routed-in part on ChordChimp's instrument until you leave the
   mode; muted means a clip on ChordChimp's own track goes silent while it's analysed.
   **Answered (2026-11-03): through.**
6. **Undo.** Using the guess changes up to three parameters (Key, Scale, Analyse). If Live
   records them as separate undo steps despite the shared gesture, is that acceptable?
   **Found in Phase 11:** Live records three steps (Analyse, then Scale, then Key). Still open.
7. **The bridge's readout (K5.5).** A file written while Test pattern is on (recommended, like the
   test take), or always while analysing, or only Live's parameter display text if Phase 11 shows
   it works?
8. **Names.** "Analyse" or "Analyze" (and "Use guess"), and "Heard" for the column, or *Detected*,
   *Guess*, *Sounds like* …? **Answered (2026-11-03): Analyse.** "Heard" and "Use guess" are as
   planned.
