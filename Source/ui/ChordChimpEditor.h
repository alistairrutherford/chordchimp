// The editor (PLAN.md §5): one page, about 780×624 and resizable.
//
//   top bar      the name · On · presets · Export · Keys
//   globals      Key · Heard (Analyse, the key heard) · Scale · Chord Group · Progression ·
//                Octave · Key octave
//   chord pads   the seven white-key chords and the five black-key ones above them (they take
//                the space the rest leaves)
//   effects      Strum, Velocity; Arp, Voicing; Repeat, Step seq
//   bottom       the output keyboard (can be hidden), which also plays
//   Info View    a strip along the bottom, like Live's: point at any control or display to see
//                what it does (as NoteCap's and DrumChimp's; InfoView.h). It replaces tooltips.
//
// Everything here is a host parameter; the UI keeps only its own preferences (keyboard shown,
// size) in the plug-in state. Refreshed at 30 Hz from the processor's published display and
// note states.
#pragma once

#include "InfoView.h"
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
        // the parameters some control is (or was) bound to; the keyboard; the pads; Export.
        void refreshNow() { timerCallback(); }
        const std::set<juce::String>& boundParameters() const noexcept { return bindings.boundIds(); }
        void showKeyboard (bool shown);
        ui::ChordPads& getChordPads() noexcept { return chordPads; }
        const ui::ExportHandle& getExportHandle() const noexcept { return topBar.exportHandle; }
        ui::GlobalRow& getGlobals() noexcept { return globals; }

        // The Info View: the key ("arpRate", "ui.export", "card.strum"…) of what is at `p` (editor
        // coordinates; empty over nothing described, the strip itself included), and showing one
        // as hovering does (tests call it).
        juce::String infoKeyAt (juce::Point<int> p);
        void showInfoAt (juce::Point<int> p);
        ui::InfoView& infoView() noexcept { return info; }

        void mouseEnter (const juce::MouseEvent&) override;
        void mouseMove (const juce::MouseEvent&) override;
        void mouseDrag (const juce::MouseEvent&) override;
        void mouseExit (const juce::MouseEvent&) override;

        // 64 px taller than before the Info View (780 × 560, 720 × 520): the strip and its gap.
        static constexpr int defaultWidth = 780, defaultHeight = 624;
        static constexpr int minWidth = 720, minHeight = 584, maxWidth = 1400, maxHeight = 1024;
        static constexpr int maxPadHeight = 300;   // both rows

    private:
        void timerCallback() override;
        juce::Component* infoComponentAt (juce::Point<int> p, juce::String& key);

        ChordChimpProcessor& processor;
        ui::LookAndFeel lookAndFeel;
        ui::Bindings bindings;

        ui::TopBar topBar;
        ui::GlobalRow globals;
        ui::ChordPads chordPads;
        ui::EffectsPanel effects;
        ui::OutputKeyboard outputKeys;
        ui::InfoView info;
        bool sizeRestored = false;      // until then, resized() leaves the saved size alone

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ChordChimpEditor)
    };
}
