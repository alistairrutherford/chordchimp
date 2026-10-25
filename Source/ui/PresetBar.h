// The top bar's preset browser (PLAN.md §3.4, §5).
//
//   PresetBar      ◀ preset name ▶ · Save. The name opens the browser: a search box over every
//                  preset (factory, then the user's files by category), click to load, and
//                  "Show folder". Save asks for a name and a category and writes
//                  ~/Music/ChordChimp/Presets/<Category>/<Name>.xml.
#pragma once

#include "Theme.h"
#include <juce_audio_processors/juce_audio_processors.h>

namespace chordchimp
{
    class ChordChimpProcessor;
}

namespace chordchimp::ui
{
    class PresetBar : public juce::Component
    {
    public:
        explicit PresetBar (ChordChimpProcessor&);
        ~PresetBar() override;

        void refresh();                   // the shown name, from the processor
        void resized() override;
        void paint (juce::Graphics&) override;

        void openBrowser();
        void askToSave();

    private:
        ChordChimpProcessor& processor;
        juce::TextButton previous { juce::String::fromUTF8 ("\xe2\x97\x80") }, next { juce::String::fromUTF8 ("\xe2\x96\xb6") };
        juce::TextButton name, save { "Save" };
        std::unique_ptr<juce::AlertWindow> saveWindow;
        juce::String shown;
    };
}
