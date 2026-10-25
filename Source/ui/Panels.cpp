#include "Panels.h"
#include "Theme.h"

namespace chordchimp::ui
{
    //==========================================================================
    TopBar::TopBar (ChordChimpProcessor& p, Bindings& b, std::function<void (bool)> showKeyboard)
        : presets (p), enable (b, params::enable, "On")
    {
        enable.setTooltip ("On: chords from every white key. Off: notes pass through unchanged");
        addAndMakeVisible (enable);
        addAndMakeVisible (presets);
        keyboardButton.setClickingTogglesState (true);
        keyboardButton.setTooltip ("Show or hide the keyboard at the bottom");
        keyboardButton.onClick = [this, showKeyboard] { showKeyboard (keyboardButton.getToggleState()); };
        addAndMakeVisible (keyboardButton);
    }

    void TopBar::setKeyboardShown (bool shown) { keyboardButton.setToggleState (shown, juce::dontSendNotification); }

    void TopBar::refresh() { presets.refresh(); }

    void TopBar::paint (juce::Graphics& g)
    {
        g.setColour (colours::panel);
        g.fillRect (getLocalBounds());
        g.setColour (colours::outline);
        g.fillRect (getLocalBounds().removeFromBottom (1));
        g.setColour (colours::chord);
        g.setFont (font (17.0f, true));
        g.drawText ("ChordChimp", nameArea, juce::Justification::centredLeft);
    }

    void TopBar::resized()
    {
        auto r = getLocalBounds().reduced (12, 6);
        nameArea = r.removeFromLeft (112);
        enable.setBounds (r.removeFromLeft (44).withSizeKeepingCentre (44, 28));
        r.removeFromLeft (12);
        keyboardButton.setBounds (r.removeFromRight (52).withSizeKeepingCentre (52, 28));
        r.removeFromRight (12);
        presets.setBounds (r.removeFromLeft (juce::jmin (r.getWidth(), 360)).withTrimmedBottom (2));
    }

    //==========================================================================
    GlobalRow::GlobalRow (Bindings& b)
        : key (b, params::key), scale (b, params::scale), group (b, params::chordGroup), progression (b, params::progression),
          octave (b, params::chordOctave, "Octave")
    {
        progression.setTooltip ("Which chord each white key plays");
        octave.setTooltip ("Chord Octave: the register the chords play in");
        for (auto* c : { &key, &scale, &group, &progression, &octave })
            addAndMakeVisible (c);
    }

    void GlobalRow::resized()
    {
        auto r = getLocalBounds();
        const struct { ParamControl* c; int w; } cols[] = { { &key, 70 }, { &scale, 150 }, { &group, 130 }, { &progression, 180 }, { &octave, 110 } };
        int total = 0;
        for (const auto& c : cols)
            total += c.w;
        const float k = (float) r.getWidth() / (float) total;
        for (const auto& c : cols)
            c.c->setBounds (r.removeFromLeft ((int) ((float) c.w * k)).reduced (4, 0));
    }

    //==========================================================================
    EffectsPanel::EffectsPanel (Bindings& b) : bindings (b), steps (colours::chord)
    {
        auto& strum = cards[0];
        strum.title = "Strum";
        strum.controls = { &add (params::strum, "Direction"), &add (params::strumRate, "Rate") };

        auto& velocity = cards[1];
        velocity.title = "Velocity";
        velocity.controls = { &add (params::velocity, "Curve"), &add (params::velocityPeak, "Peak"), &add (params::velocityRange, "Range") };

        auto& arp = cards[2];
        arp.title = "Arp";
        arp.controls = { &add (params::arp, "Pattern"), &add (params::arpRate, "Rate"), &add (params::arpRange, "Range"),
                         &add (params::arpGate, "Gate") };

        auto& voicing = cards[3];
        voicing.title = "Voicing";
        voicing.controls = { &add (params::stack), &add (params::hold), &add (params::voiceLeading, "Voice leading") };
        voicing.controls.back()->setTooltip ("Smooth: each chord takes the inversion nearest the one before");

        auto& repeat = cards[4];
        repeat.title = "Repeat";
        repeat.toggle = &add (params::repeat, "On");
        repeat.controls = { &add (params::repeatRate, "Rate"), &add (params::repeatGate, "Gate") };

        auto& seq = cards[5];
        seq.title = "Step seq";
        seq.toggle = &add (params::seq, "On");
        seq.controls = { &add (params::seqPattern, "Pattern"), &add (params::seqSwing, "Swing"), &add (params::seqGate, "Gate") };
        seq.extra = &steps;
        addAndMakeVisible (steps);
    }

    ParamControl& EffectsPanel::add (const juce::String& paramId, const juce::String& label)
    {
        auto* c = controls.add (new ParamControl (bindings, paramId, label));
        addAndMakeVisible (c);
        return *c;
    }

    void EffectsPanel::refresh (const UiModel& m) { steps.setSteps (m.settings.fx.seq.steps()); }

    int EffectsPanel::preferredHeight()
    {
        const int card = titleHeight + controlHeight + 8;
        return 2 * card + (card + stepsHeight + 4) + 2 * gap;
    }

    void EffectsPanel::paint (juce::Graphics& g)
    {
        for (const auto& c : cards)
        {
            g.setColour (colours::raised.withAlpha (0.55f));
            g.fillRoundedRectangle (c.bounds.toFloat(), 6.0f);
            g.setColour (colours::dim);
            g.setFont (font (11.5f, true));
            g.drawText (c.title.toUpperCase(), c.bounds.reduced (10, 0).removeFromTop (titleHeight), juce::Justification::centredLeft);
        }
    }

    void EffectsPanel::layoutCard (Card& c)
    {
        auto r = c.bounds.reduced (6, 0);
        auto head = r.removeFromTop (titleHeight);
        if (c.toggle != nullptr)
            c.toggle->setBounds (head.removeFromRight (48).reduced (0, 2));
        auto row = r.removeFromTop (controlHeight);
        const int w = row.getWidth() / juce::jmax (1, (int) c.controls.size());
        for (auto* control : c.controls)
            control->setBounds (row.removeFromLeft (w).reduced (3, 0));
        if (c.extra != nullptr)
            c.extra->setBounds (r.withTrimmedTop (4).removeFromTop (stepsHeight).reduced (3, 0));
    }

    void EffectsPanel::resized()
    {
        auto r = getLocalBounds();
        const int card = titleHeight + controlHeight + 8;
        const int heights[] = { card, card, card + stepsHeight + 4 };
        for (int row = 0; row < 3; ++row)
        {
            auto line = r.removeFromTop (heights[row]);
            r.removeFromTop (gap);
            cards[(size_t) (2 * row)].bounds = line.removeFromLeft ((line.getWidth() - gap) / 2);
            line.removeFromLeft (gap);
            cards[(size_t) (2 * row + 1)].bounds = line;
        }
        for (auto& c : cards)
            layoutCard (c);
    }
}
