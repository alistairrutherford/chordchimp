#include "Panels.h"
#include "Theme.h"

namespace chordchimp::ui
{
    namespace
    {
        void caption (juce::Label& l, const juce::String& text, juce::Colour c = colours::dim)
        {
            l.setText (text, juce::dontSendNotification);
            l.setFont (font (11.5f, true));
            l.setColour (juce::Label::textColourId, c);
        }
    }

    //==========================================================================
    ParamControl& ParamGrid::add (const juce::String& paramId, const juce::String& label)
    {
        auto* c = owned.add (new ParamControl (bindings, paramId, label));
        c->setAccent (colour);
        addAndMakeVisible (c);
        cells.add (c);
        return *c;
    }

    void ParamGrid::addSection (const juce::String& title)
    {
        while (cells.size() % cols != 0)
            cells.add (nullptr);
        auto* l = sections.add (new juce::Label());
        caption (*l, title.toUpperCase(), colours::faint);
        addAndMakeVisible (l);
        cells.add (l);
        for (int i = 1; i < cols; ++i)
            cells.add (nullptr);
    }

    int ParamGrid::rows() const noexcept { return (cells.size() + cols - 1) / cols; }

    void ParamGrid::resized()
    {
        const float w = (float) getWidth() / (float) cols;
        int y = 0;
        for (int row = 0; row < rows(); ++row)
        {
            auto* first = cells[row * cols];
            const bool section = dynamic_cast<juce::Label*> (first) != nullptr;
            const int h = section ? sectionHeight : rowHeight;
            if (section)
                first->setBounds (0, y, getWidth(), h);
            else
                for (int c = 0; c < cols; ++c)
                    if (auto* cell = cells[row * cols + c])
                        cell->setBounds (juce::Rectangle<float> (w * (float) c, (float) y, w, (float) h).reduced (4.0f, 2.0f).toNearestInt());
            y += h;
        }
    }

    //==========================================================================
    TopBar::TopBar (ChordChimpProcessor& p, Bindings& b, std::function<void()> openSetup, std::function<void (bool)> showKeyboard)
        : presets (p),
          enable (b, params::enable, "On"), key (b, params::key), scale (b, params::scale), progression (b, params::progression),
          group (b, params::chordGroup)
    {
        logo.setTooltip ("Setup: custom progression, test pattern");
        logo.onClick = std::move (openSetup);
        logo.setColour (juce::TextButton::buttonColourId, colours::background);
        logo.setColour (juce::TextButton::textColourOffId, colours::text);
        addAndMakeVisible (logo);
        keyboardButton.setClickingTogglesState (true);
        keyboardButton.setTooltip ("Show or hide the keyboard at the bottom");
        keyboardButton.onClick = [this, showKeyboard] { showKeyboard (keyboardButton.getToggleState()); };
        addAndMakeVisible (keyboardButton);
        for (auto* c : { &enable, &key, &scale, &progression, &group })
            addAndMakeVisible (c);
        addAndMakeVisible (presets);
    }

    void TopBar::setKeyboardShown (bool shown) { keyboardButton.setToggleState (shown, juce::dontSendNotification); }

    void TopBar::refresh()
    {
        presets.refresh();
    }

    void TopBar::paint (juce::Graphics& g)
    {
        g.setColour (colours::panel);
        g.fillRect (getLocalBounds());
        g.setColour (colours::outline);
        g.fillRect (getLocalBounds().removeFromBottom (1));
    }

    void TopBar::resized()
    {
        auto r = getLocalBounds().reduced (8, 4);
        logo.setBounds (r.removeFromLeft (84).withSizeKeepingCentre (84, 30));
        logo.setButtonText ("ChordChimp");
        r.removeFromLeft (6);
        enable.setBounds (r.removeFromLeft (40).withTrimmedTop (14));
        r.removeFromLeft (4);
        presets.setBounds (r.removeFromLeft (juce::jlimit (150, 250, r.getWidth() * 2 / 9)));
        r.removeFromLeft (4);
        keyboardButton.setBounds (r.removeFromRight (52).withTrimmedTop (14).withTrimmedBottom (0));
        r.removeFromRight (10);
        const struct { ParamControl* c; int w; } cols[] = { { &key, 66 }, { &scale, 118 }, { &progression, 150 }, { &group, 118 } };
        int total = 0;
        for (const auto& c : cols)
            total += c.w;
        const float k = juce::jmin (1.4f, (float) r.getWidth() / (float) total);
        for (const auto& c : cols)
            c.c->setBounds (r.removeFromLeft ((int) ((float) c.w * k)).reduced (3, 0));
    }

    //==========================================================================
    ChordFxPanel::ChordFxPanel (Bindings& b) : grid (b, 5, colours::chord), steps (b, colours::chord)
    {
        grid.add (params::stack);
        grid.add (params::chordOctave, "Octave");
        grid.add (params::hold);
        grid.add (params::velocity);
        grid.addGap();
        grid.add (params::strum);
        grid.add (params::strumRate, "Strum rate");
        grid.add (params::strumMs, "Strum time (stopped)");
        grid.add (params::velocityPeak, "Velocity peak");
        grid.add (params::velocityRange, "Velocity range");
        grid.add (params::arp);
        grid.add (params::arpRate, "Arp rate");
        grid.add (params::arpRange, "Arp range");
        grid.add (params::arpGate, "Arp gate");
        grid.addGap();
        grid.add (params::repeat);
        grid.add (params::repeatRate, "Repeat rate");
        grid.add (params::repeatGate, "Repeat gate");
        grid.addGap();
        grid.add (params::voiceLeading, "Voice leading").setTooltip ("Smooth: each chord takes the inversion nearest the one before");
        grid.add (params::seq, "Step seq");
        grid.add (params::seqPattern, "Pattern");
        grid.add (params::seqSwing, "Swing");
        grid.add (params::seqGate, "Gate");
        grid.add (params::seqSustain, "Sustain");
        addAndMakeVisible (grid);
        caption (stepsCaption, "STEPS");
        addAndMakeVisible (stepsCaption);
        addAndMakeVisible (steps);
        steps.bind (params::seqSteps (0));
    }

    void ChordFxPanel::refresh (const UiModel& m) { steps.bind (params::seqSteps (m.settings.fx.seq.pattern)); }

    void ChordFxPanel::resized()
    {
        auto r = getLocalBounds();
        grid.setBounds (r.removeFromTop (grid.rows() * ParamGrid::rowHeight));
        auto s = r.removeFromTop (30).reduced (4, 2);
        stepsCaption.setBounds (s.removeFromLeft (52));
        steps.setBounds (s);
    }

    //==========================================================================
    SetupPanel::SetupPanel (Bindings& b, std::function<void()> close)
        : left (b, 2, colours::chord), right (b, 2, colours::chord)
    {
        heading.setText ("Setup", juce::dontSendNotification);
        heading.setFont (font (18.0f, true));
        heading.setColour (juce::Label::textColourId, colours::text);
        addAndMakeVisible (heading);
        closeButton.onClick = std::move (close);
        addAndMakeVisible (closeButton);

        left.addSection ("Custom progression (degree per white key)");
        for (int n = 0; n < numChordSlots; ++n)
            left.add (params::customDegree (n), "Key " + juce::String::charToString ("CDEFGAB"[n]));
        addAndMakeVisible (left);

        right.addSection ("Test pattern (LiveBridge)");
        right.add (params::testPattern);
        addAndMakeVisible (right);
    }

    void SetupPanel::paint (juce::Graphics& g)
    {
        g.setColour (colours::background);
        g.fillRect (getLocalBounds());
    }

    void SetupPanel::resized()
    {
        auto r = getLocalBounds().reduced (16, 10);
        auto top = r.removeFromTop (32);
        closeButton.setBounds (top.removeFromRight (80).reduced (0, 3));
        heading.setBounds (top);
        const int w = r.getWidth() / 3;
        left.setBounds (r.removeFromLeft (w - 8));
        r.removeFromLeft (8);
        right.setBounds (r.removeFromLeft (w - 8).removeFromTop (ParamGrid::sectionHeight + ParamGrid::rowHeight));
    }
}
