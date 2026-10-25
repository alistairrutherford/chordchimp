// Routes incoming notes (PLAN.md §3.1). There is no split: the whole keyboard plays chords.
//
//   white C D E F G A B → chord slots 0–6, in every octave (the octave doesn't matter;
//                         Chord Octave sets the register)
//   black C# D# F# G# A# → nothing (swallowed, not passed through)
#pragma once

namespace chordchimp
{
    namespace keys
    {
        // Letter of a white key (C = 0 … B = 6), or -1.
        constexpr int whiteLetter (int pitchClass) noexcept
        {
            constexpr int t[12] = { 0, -1, 1, -1, 2, 3, -1, 4, -1, 5, -1, 6 };
            return t[((pitchClass % 12) + 12) % 12];
        }

        constexpr int whiteOffsets[7] = { 0, 2, 4, 5, 7, 9, 11 };
    }

    class KeyRouter
    {
    public:
        // The octave the test pattern and the editor's chord keys play in: C at MIDI 48 (Live's
        // C2), the chord zone's octave in Chordism at its default split.
        static constexpr int homeC = 48;

        // The chord slot (0..6) a note plays, or -1 for a black key.
        static constexpr int slotOf (int note) noexcept { return keys::whiteLetter (note % 12); }

        // The key in the home octave that plays `slot`, or -1.
        static constexpr int keyForSlot (int slot) noexcept
        {
            return slot < 0 || slot > 6 ? -1 : homeC + keys::whiteOffsets[slot];
        }
    };
}
