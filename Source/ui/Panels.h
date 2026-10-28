// The editor's panels (PLAN.md §5):
//
//   TopBar         the name · On · the preset browser · Export (drag out the last bars played) ·
//                  Keys (show or hide the output keyboard)
//   GlobalRow      Key · Scale · Chord Group · Progression · Octave · Key octave
//   EffectsPanel   six cards: Strum, Velocity; Arp, Voicing (Stack, Hold, Voice leading);
//                  Repeat and Step seq, each with its on switch in the card's corner, and the
//                  chosen pattern's steps under Step seq
//
// Test pattern (the LiveBridge rig) has no control: Live's device parameters reach it.
#pragma once

#include "Keyboards.h"
#include "PresetBar.h"

namespace chordchimp::ui
{
    class TopBar : public juce::Component
    {
    public:
        TopBar (ChordChimpProcessor&, Bindings&, std::function<void (bool)> showKeyboard);
        void setKeyboardShown (bool shown);
        void refresh();
        void paint (juce::Graphics&) override;
        void resized() override;

        PresetBar presets;
        ExportHandle exportHandle;

    private:
        ParamControl enable;
        juce::TextButton keyboardButton { "Keys" };
        juce::Rectangle<int> nameArea;
    };

    class GlobalRow : public juce::Component
    {
    public:
        explicit GlobalRow (Bindings&);
        void resized() override;

    private:
        ParamControl key, scale, group, progression, octave, keyOctave;
    };

    class EffectsPanel : public juce::Component
    {
    public:
        explicit EffectsPanel (Bindings&);
        void refresh (const UiModel&);
        void paint (juce::Graphics&) override;
        void resized() override;

        // The height the cards need at this width (the editor sizes the chord pads around it).
        static int preferredHeight();

    private:
        struct Card
        {
            juce::String title;
            ParamControl* toggle = nullptr;           // the card's on switch, if it has one
            std::vector<ParamControl*> controls;
            juce::Component* extra = nullptr;         // under the controls (the steps)
            juce::Rectangle<int> bounds;
        };

        ParamControl& add (const juce::String& paramId, const juce::String& label = {});
        void layoutCard (Card&);

        Bindings& bindings;
        juce::OwnedArray<ParamControl> controls;
        std::array<Card, 6> cards;
        StepSeqView steps;

        static constexpr int titleHeight = 22, controlHeight = 40, stepsHeight = 22, gap = 8;
    };
}
