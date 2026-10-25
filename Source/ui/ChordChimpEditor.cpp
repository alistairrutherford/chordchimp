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
          topBar (p, bindings, [this] { showSetup (true); }, [this] (bool shown) { showKeyboard (shown); }),
          chordKeys (p), chordFx (bindings), outputKeys (p),
          setup (bindings, [this] { showSetup (false); })
    {
        setLookAndFeel (&lookAndFeel);

        chordTitle.setText ("CHORDS", juce::dontSendNotification);
        chordTitle.setFont (ui::font (13.0f, true));
        chordTitle.setColour (juce::Label::textColourId, ui::colours::chord);
        for (auto* c : std::initializer_list<juce::Component*> { &topBar, &chordTitle, &chordKeys, &chordFx, &outputKeys })
            addAndMakeVisible (c);
        addChildComponent (setup);

        auto& ui = processor.uiState;
        showKeyboard (ui.getProperty (keyboardId, true));

        setResizable (true, true);
        setResizeLimits (960, 600, 1800, 1100);
        setSize (ui.getProperty (widthId, defaultWidth), ui.getProperty (heightId, defaultHeight));
        timerCallback();
        startTimerHz (30);
    }

    ChordChimpEditor::~ChordChimpEditor()
    {
        stopTimer();
        setLookAndFeel (nullptr);
    }

    void ChordChimpEditor::showSetup (bool shown)
    {
        setup.setVisible (shown);
        if (shown)
            setup.toFront (false);
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
        g.setColour (ui::colours::panel);
        g.fillRoundedRectangle (mainArea.toFloat(), 6.0f);
    }

    void ChordChimpEditor::resized()
    {
        auto r = getLocalBounds();
        topBar.setBounds (r.removeFromTop (48));
        setup.setBounds (r);
        if (outputKeys.isVisible())
            outputKeys.setBounds (r.removeFromBottom (64).reduced (8, 4));

        mainArea = r.reduced (8, 8);
        const int keyboardHeight = juce::jlimit (120, 200, getHeight() / 4 + 8);
        auto a = mainArea.reduced (8, 6);
        chordTitle.setBounds (a.removeFromTop (26).removeFromLeft (150));
        chordKeys.setBounds (a.removeFromTop (keyboardHeight).reduced (0, 4));
        a.removeFromTop (6);
        chordFx.setBounds (a);

        processor.uiState.setProperty (widthId, getWidth(), nullptr);
        processor.uiState.setProperty (heightId, getHeight(), nullptr);
    }

    void ChordChimpEditor::timerCallback()
    {
        topBar.refresh();

        ui::UiModel m;
        m.display = processor.readDisplay();
        m.settings = processor.currentSettings();

        chordKeys.refresh (m);
        outputKeys.refresh (m);
        chordFx.refresh (m);
    }
}
