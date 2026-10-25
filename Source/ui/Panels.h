// The editor's panels (PLAN.md §6):
//
//   TopBar         logo (opens Setup) · Enable · Preset browser · Key · Scale · Progression ·
//                  Chord Group · keyboard show/hide
//   ChordFxPanel   the effects: Stack, Octave, Hold; Strum; Arp; Repeat, Voice leading;
//                  Velocity; Step Sequencer and its steps
//   SetupPanel     everything else, as an overlay: the custom progression and the test
//                  pattern (LiveBridge)
#pragma once

#include "Keyboards.h"
#include "PresetBar.h"

namespace chordchimp::ui
{
    // ParamControls in rows of `columns` cells; empty cells with addGap().
    class ParamGrid : public juce::Component
    {
    public:
        ParamGrid (Bindings& b, int columns, juce::Colour accent) : bindings (b), cols (columns), colour (accent) {}

        ParamControl& add (const juce::String& paramId, const juce::String& label = {});
        void addGap() { cells.add (nullptr); }
        void addCell (juce::Component& c) { addAndMakeVisible (c); cells.add (&c); }   // not owned
        void addSection (const juce::String& title);   // a caption row across the grid

        int rows() const noexcept;
        void resized() override;

        static constexpr int rowHeight = 42, sectionHeight = 20;

    private:
        Bindings& bindings;
        int cols;
        juce::Colour colour;
        juce::OwnedArray<ParamControl> owned;
        juce::OwnedArray<juce::Label> sections;
        juce::Array<juce::Component*> cells;      // nullptr = gap; a Label = section
    };

    class TopBar : public juce::Component
    {
    public:
        TopBar (ChordChimpProcessor&, Bindings&, std::function<void()> openSetup, std::function<void (bool)> showKeyboard);
        void setKeyboardShown (bool shown);
        void refresh();
        void paint (juce::Graphics&) override;
        void resized() override;

        PresetBar presets;

    private:
        juce::TextButton logo { "ChordChimp" };
        juce::TextButton keyboardButton { "Keys" };
        ParamControl enable, key, scale, progression, group;
    };

    class ChordFxPanel : public juce::Component
    {
    public:
        explicit ChordFxPanel (Bindings&);
        void refresh (const UiModel&);
        void resized() override;

    private:
        ParamGrid grid;
        StepSeqView steps;
        juce::Label stepsCaption;
    };

    class SetupPanel : public juce::Component
    {
    public:
        SetupPanel (Bindings&, std::function<void()> close);
        void paint (juce::Graphics&) override;
        void resized() override;

    private:
        juce::TextButton closeButton { "Done" };
        juce::Label heading;
        ParamGrid left, right;
    };
}
