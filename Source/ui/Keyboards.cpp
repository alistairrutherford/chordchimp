#include "Keyboards.h"
#include "Theme.h"
#include "../core/theory/ChordNamer.h"
#include "../core/theory/RomanNumeral.h"

namespace chordchimp::ui
{
    //==========================================================================
    bool PianoLayout::isBlack (int note) noexcept { return keys::whiteLetter (note) < 0; }

    int PianoLayout::whiteCount() const noexcept
    {
        int n = 0;
        for (int k = lo; k <= hi; ++k)
            n += isBlack (k) ? 0 : 1;
        return juce::jmax (1, n);
    }

    juce::Rectangle<float> PianoLayout::keyBounds (int note, float blackHeight) const
    {
        const float ww = area.getWidth() / (float) whiteCount();
        int whitesBefore = 0;
        for (int k = lo; k < note; ++k)
            whitesBefore += isBlack (k) ? 0 : 1;
        if (! isBlack (note))
            return { area.getX() + ww * (float) whitesBefore, area.getY(), ww, area.getHeight() };
        const float bw = ww * 0.62f;
        return { area.getX() + ww * (float) whitesBefore - bw * 0.5f, area.getY(), bw, area.getHeight() * blackHeight };
    }

    int PianoLayout::noteAt (juce::Point<float> p, float blackHeight) const
    {
        for (int k = lo; k <= hi; ++k)
            if (isBlack (k) && keyBounds (k, blackHeight).contains (p))
                return k;
        for (int k = lo; k <= hi; ++k)
            if (! isBlack (k) && keyBounds (k, blackHeight).contains (p))
                return k;
        return -1;
    }

    //==========================================================================
    void PlayingKeys::press (int note, int velocity)
    {
        if (note == held)
            return;
        release();
        if (note < 0 || note > 127)
            return;
        processor.pushUiNote (note, velocity, true);
        held = note;
    }

    void PlayingKeys::release()
    {
        if (held >= 0)
            processor.pushUiNote (held, 0, false);
        held = -1;
    }

    //==========================================================================
    namespace
    {
        constexpr float chordBlackHeight = 0.5f;

        void drawWhiteKey (juce::Graphics& g, juce::Rectangle<float> r, juce::Colour tint, bool lit)
        {
            r = r.reduced (1.0f, 0.0f);
            g.setColour (lit ? colours::whiteKey.interpolatedWith (tint, 0.45f) : colours::whiteKey);
            g.fillRoundedRectangle (r, 4.0f);
        }

        void drawBlackKey (juce::Graphics& g, juce::Rectangle<float> r)
        {
            g.setColour (colours::blackKey);
            g.fillRoundedRectangle (r, 3.0f);
        }
    }

    ChordKeyboard::ChordKeyboard (ChordChimpProcessor& p) : processor (p), playing (p)
    {
        layout.lo = 0;
        layout.hi = 11;
        setTooltip ("Chord keys: click to play.");
    }

    void ChordKeyboard::refresh (const UiModel& m)
    {
        std::array<bool, 12> now {};
        for (int n = 0; n < 128; ++n)
            if (processor.isKeyDown (n) || processor.isPerforming (n))
                now[(size_t) (n % 12)] = true;

        const bool changed = now != active || ! (m.display == model.display);
        model = m;
        active = now;
        if (changed)
            repaint();
    }

    void ChordKeyboard::paint (juce::Graphics& g)
    {
        layout.area = getLocalBounds().toFloat();
        const auto& d = model.display;
        for (int pc = 0; pc < 12; ++pc)
        {
            if (PianoLayout::isBlack (pc))
                continue;
            const int slot = keys::whiteLetter (pc);
            auto r = layout.keyBounds (pc, chordBlackHeight);
            drawWhiteKey (g, r, colours::chord, active[(size_t) pc]);

            auto text = r.reduced (3.0f, 6.0f).withTrimmedTop (r.getHeight() * chordBlackHeight);
            const auto& slotData = d.chords[(size_t) slot];
            g.setColour (colours::whiteKeyText);
            g.setFont (font (19.0f, true));
            g.drawFittedText (juce::String (romanNumeral (slotData, d.context.scale)), text.removeFromTop (text.getHeight() * 0.5f).toNearestInt(),
                              juce::Justification::centredBottom, 1);
            g.setFont (font (13.0f));
            g.setColour (colours::whiteKeyText.withAlpha (0.8f));
            g.drawFittedText (juce::String (nameSlot (slotData, d.context).text()), text.toNearestInt(), juce::Justification::centredTop, 1, 0.7f);

            if (slot == d.lastSlot)
            {
                g.setColour (colours::chord);
                g.fillRoundedRectangle (r.reduced (5.0f, 0.0f).removeFromBottom (5.0f).translated (0.0f, -4.0f), 2.0f);
            }
        }
        for (int pc = 0; pc < 12; ++pc)
        {
            if (! PianoLayout::isBlack (pc))
                continue;
            drawBlackKey (g, layout.keyBounds (pc, chordBlackHeight));
        }
    }

    void ChordKeyboard::mouseDown (const juce::MouseEvent& e)
    {
        layout.area = getLocalBounds().toFloat();
        const int pc = layout.noteAt (e.position, chordBlackHeight);
        if (pc >= 0 && ! PianoLayout::isBlack (pc) && ! e.mods.isPopupMenu())
            playing.press (KeyRouter::homeC + pc, 100);
    }

    void ChordKeyboard::mouseUp (const juce::MouseEvent&) { playing.release(); }

    //==========================================================================
    OutputKeyboard::OutputKeyboard (ChordChimpProcessor& p) : processor (p), playing (p)
    {
        layout.lo = 21;
        layout.hi = 108;
        labelFont = font (9.0f);
        setTooltip ("Output: the notes ChordChimp plays (red) and the keys held (outlined). Click to play.");
    }

    void OutputKeyboard::refresh (const UiModel&)
    {
        std::array<bool, 128> s {}, k {};
        for (int n = 0; n < 128; ++n)
        {
            s[(size_t) n] = processor.isNoteSounding (n);
            k[(size_t) n] = processor.isKeyDown (n);
        }
        // Phase 8: only the keys that changed (while chords play, the whole keyboard every frame
        // was most of the editor's CPU).
        layout.area = getLocalBounds().toFloat();
        for (int n = layout.lo; n <= layout.hi; ++n)
            if (s[(size_t) n] != sounding[(size_t) n] || k[(size_t) n] != down[(size_t) n])
                repaint (layout.keyBounds (n).getSmallestIntegerContainer().expanded (1));
        sounding = s;
        down = k;
    }

    void OutputKeyboard::paint (juce::Graphics& g)
    {
        layout.area = getLocalBounds().toFloat();
        const auto clip = g.getClipBounds().toFloat();
        for (int pass = 0; pass < 2; ++pass)
            for (int n = layout.lo; n <= layout.hi; ++n)
            {
                const bool black = PianoLayout::isBlack (n);
                if (black != (pass == 1))
                    continue;
                auto r = layout.keyBounds (n);
                if (! r.intersects (clip))
                    continue;
                if (black)
                {
                    g.setColour (sounding[(size_t) n] ? colours::sounding : colours::blackKey);
                    g.fillRect (r);
                }
                else
                {
                    g.setColour (sounding[(size_t) n] ? colours::sounding : colours::whiteKey);
                    g.fillRect (r.reduced (0.5f, 0.0f));
                    if (n % 12 == 0)
                    {
                        g.setColour (colours::whiteKeyText.withAlpha (0.5f));
                        g.setFont (labelFont);
                        g.drawText ("C" + juce::String (n / 12 - 2), r.removeFromBottom (12.0f), juce::Justification::centred);
                    }
                }
                if (down[(size_t) n])
                {
                    g.setColour (colours::chord);
                    g.drawRect (layout.keyBounds (n).reduced (1.0f), 2.0f);
                }
            }
    }

    void OutputKeyboard::mouseDown (const juce::MouseEvent& e) { mouseDrag (e); }

    void OutputKeyboard::mouseDrag (const juce::MouseEvent& e)
    {
        layout.area = getLocalBounds().toFloat();
        const int n = layout.noteAt (e.position);
        const int velocity = juce::jlimit (1, 127, 40 + (int) (87.0f * e.position.y / (float) juce::jmax (1, getHeight())));
        if (n >= 0)
            playing.press (n, velocity);
    }

    void OutputKeyboard::mouseUp (const juce::MouseEvent&) { playing.release(); }
}
