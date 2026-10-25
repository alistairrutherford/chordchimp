// The editor (PLAN.md §5), about 1100×640 and resizable:
//
//   top bar      logo (Setup) · Enable · Presets · Key · Scale · Progression · Chord Group · Keys
//   main         the chord keys and, below them, the effects (Chord FX)
//   bottom       the output keyboard (can be hidden), which also plays
//
// Everything here is a host parameter; the UI keeps only its own preferences (keyboard shown,
// size) in the plug-in state. Refreshed at 30 Hz from the processor's published display and
// note states.
//
// ChordChimp Phase 1 (2026-10-25) took out Chordism's Smart Scale half, the Chord Editor page,
// MIDI drop-in and QWERTY playing; the single-page layout comes in Phase 3.
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
        // the parameters some control is (or was) bound to; the pages.
        void refreshNow() { timerCallback(); }
        const std::set<juce::String>& boundParameters() const noexcept { return bindings.boundIds(); }
        void showSetup (bool shown);
        void showKeyboard (bool shown);

        static constexpr int defaultWidth = 1100, defaultHeight = 640;

    private:
        void timerCallback() override;

        ChordChimpProcessor& processor;
        ui::LookAndFeel lookAndFeel;
        ui::Bindings bindings;
        juce::TooltipWindow tooltips { this, 600 };

        ui::TopBar topBar;
        juce::Label chordTitle;
        ui::ChordKeyboard chordKeys;
        ui::ChordFxPanel chordFx;
        ui::OutputKeyboard outputKeys;
        ui::SetupPanel setup;

        juce::Rectangle<int> mainArea;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ChordChimpEditor)
    };
}
