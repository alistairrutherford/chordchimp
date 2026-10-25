#include "ChordChimpEditor.h"
#include "../plugin/ChordChimpProcessor.h"

namespace chordchimp
{
    namespace
    {
        const juce::Identifier keyboardId ("keyboard"), widthId ("width"), heightId ("height");
    }

    ChordChimpEditor::ChordChimpEditor (ChordChimpProcessor& p)
        : AudioProcessorEditor (p), processor (p), bindings (p.state),
          topBar (p, bindings, [this] (bool shown) { showKeyboard (shown); }),
          globals (bindings), chordPads (p), effects (bindings), outputKeys (p)
    {
        setLookAndFeel (&lookAndFeel);
        for (auto* c : std::initializer_list<juce::Component*> { &topBar, &globals, &chordPads, &effects, &outputKeys })
            addAndMakeVisible (c);

        auto& ui = processor.uiState;
        showKeyboard (ui.getProperty (keyboardId, true));

        setResizable (true, true);
        setResizeLimits (minWidth, minHeight, maxWidth, maxHeight);
        setSize (juce::jlimit (minWidth, maxWidth, (int) ui.getProperty (widthId, defaultWidth)),
                 juce::jlimit (minHeight, maxHeight, (int) ui.getProperty (heightId, defaultHeight)));
        timerCallback();
        startTimerHz (30);
    }

    ChordChimpEditor::~ChordChimpEditor()
    {
        stopTimer();
        setLookAndFeel (nullptr);
    }

    void ChordChimpEditor::showKeyboard (bool shown)
    {
        outputKeys.setVisible (shown);
        topBar.setKeyboardShown (shown);
        processor.uiState.setProperty (keyboardId, shown, nullptr);
        resized();
    }

    void ChordChimpEditor::paint (juce::Graphics& g)
    {
        g.fillAll (ui::colours::background);
    }

    void ChordChimpEditor::resized()
    {
        auto r = getLocalBounds();
        topBar.setBounds (r.removeFromTop (44));
        if (outputKeys.isVisible())
            outputKeys.setBounds (r.removeFromBottom (60).reduced (12, 6));

        r.reduce (12, 10);
        globals.setBounds (r.removeFromTop (42));
        r.removeFromTop (12);
        effects.setBounds (r.removeFromBottom (ui::EffectsPanel::preferredHeight()));
        r.removeFromBottom (12);
        // The pads take the space left, up to a height that still looks like pads; any more is
        // shared above and below them.
        chordPads.setBounds (r.withSizeKeepingCentre (r.getWidth(), juce::jmin (r.getHeight(), maxPadHeight)));

        processor.uiState.setProperty (widthId, getWidth(), nullptr);
        processor.uiState.setProperty (heightId, getHeight(), nullptr);
    }

    void ChordChimpEditor::timerCallback()
    {
        topBar.refresh();

        ui::UiModel m;
        m.display = processor.readDisplay();
        m.settings = processor.currentSettings();

        chordPads.refresh (m);
        outputKeys.refresh (m);
        effects.refresh (m);
    }
}
