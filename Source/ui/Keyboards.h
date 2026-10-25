// The editor's keyboards (PLAN.md §6). Each plays through the engine like host MIDI input
// (ChordChimpProcessor::pushUiNote), so clicking a key is the same as pressing it.
//
//   ChordKeyboard   one octave: each white key shows its Roman numeral and chord name, with a
//                   stripe on the last one played (black keys play nothing).
//   OutputKeyboard  88 keys: the notes ChordChimp sends (red) and the keys held (outlined).
//                   Hideable.
#pragma once

#include "../plugin/ChordChimpProcessor.h"
#include "ParamControl.h"

namespace chordchimp::ui
{
    // What the editor shows, refreshed on its timer.
    struct UiModel
    {
        ChordDisplay display;             // the chords, their context, last slot
        EngineSettings settings;          // the parameters now
    };

    // Piano geometry for notes lo..hi (both white keys) across a width.
    struct PianoLayout
    {
        int lo = 60, hi = 84;
        juce::Rectangle<float> area;

        static bool isBlack (int note) noexcept;
        int whiteCount() const noexcept;
        juce::Rectangle<float> keyBounds (int note, float blackHeight = 0.62f) const;
        int noteAt (juce::Point<float> p, float blackHeight = 0.62f) const;   // -1 if none
    };

    // Plays a key while the mouse holds it (and a drag moves along the keys).
    class PlayingKeys
    {
    public:
        explicit PlayingKeys (ChordChimpProcessor& p) : processor (p) {}
        ~PlayingKeys() { release(); }
        void press (int note, int velocity);
        void release();

    private:
        ChordChimpProcessor& processor;
        int held = -1;
    };

    class ChordKeyboard : public juce::Component,
                         public juce::SettableTooltipClient
    {
    public:
        explicit ChordKeyboard (ChordChimpProcessor&);
        void refresh (const UiModel&);

        void paint (juce::Graphics&) override;
        void mouseDown (const juce::MouseEvent&) override;
        void mouseUp (const juce::MouseEvent&) override;

    private:
        ChordChimpProcessor& processor;
        PlayingKeys playing;
        UiModel model;
        PianoLayout layout;
        std::array<bool, 12> active {};
    };

    class OutputKeyboard : public juce::Component,
                          public juce::SettableTooltipClient
    {
    public:
        explicit OutputKeyboard (ChordChimpProcessor&);
        void refresh (const UiModel&);

        void paint (juce::Graphics&) override;
        void mouseDown (const juce::MouseEvent&) override;
        void mouseDrag (const juce::MouseEvent&) override;
        void mouseUp (const juce::MouseEvent&) override;

    private:
        ChordChimpProcessor& processor;
        PlayingKeys playing;
        PianoLayout layout;
        std::array<bool, 128> sounding {}, down {};
        juce::Font labelFont;             // made once: a new Font on every paint costs a typeface lookup
    };
}
