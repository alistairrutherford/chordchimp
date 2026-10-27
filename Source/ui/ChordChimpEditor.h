// The editor (PLAN.md §5): one page, about 780×560 and resizable.
//
//   top bar      the name · On · presets · Keys
//   globals      Key · Scale · Chord Group · Progression · Octave
//   chord pads   the seven chords (they take the space the rest leaves)
//   effects      Strum, Velocity; Arp, Voicing; Repeat, Step seq
//   bottom       the output keyboard (can be hidden), which also plays
//
// Everything here is a host parameter; the UI keeps only its own preferences (keyboard shown,
// size) in the plug-in state. Refreshed at 30 Hz from the processor's published display and
// note states.
#pragma once

#include "Panels.h"
#include "Theme.h"

namespace chordchimp
{
    class ChordChimpProcessor;

    class ChordChimpEditor : public juce::AudioProcessorEditor,
                             private juce::Timer
    {
    public:
        explicit ChordChimpEditor (ChordChimpProcessor&);
        ~ChordChimpEditor() override;

        void paint (juce::Graphics&) override;
        void resized() override;

        // For tests (Tests/Snapshot): read the processor now, without waiting for the timer;
        // the parameters some control is (or was) bound to; the keyboard.
        void refreshNow() { timerCallback(); }
        const std::set<juce::String>& boundParameters() const noexcept { return bindings.boundIds(); }
        void showKeyboard (bool shown);
        ui::ChordPads& getChordPads() noexcept { return chordPads; }

        static constexpr int defaultWidth = 780, defaultHeight = 560;
        static constexpr int minWidth = 720, minHeight = 520, maxWidth = 1400, maxHeight = 960;
        static constexpr int maxPadHeight = 200;

    private:
        void timerCallback() override;

        ChordChimpProcessor& processor;
        ui::LookAndFeel lookAndFeel;
        ui::Bindings bindings;
        juce::TooltipWindow tooltips { this, 600 };

        ui::TopBar topBar;
        ui::GlobalRow globals;
        ui::ChordPads chordPads;
        ui::EffectsPanel effects;
        ui::OutputKeyboard outputKeys;
        bool sizeRestored = false;      // until then, resized() leaves the saved size alone

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ChordChimpEditor)
    };
}
