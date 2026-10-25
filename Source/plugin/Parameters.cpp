#include "Parameters.h"

namespace chordchimp::params
{
    namespace
    {
        juce::ParameterID id (const char* name) { return { name, 1 }; }

        juce::StringArray keyNames()
        {
            juce::StringArray a;
            for (int k = 0; k < numKeys; ++k)
                a.add (keyName (k));
            return a;
        }

        juce::StringArray scaleNames()
        {
            juce::StringArray a;
            for (int s = 0; s < numScales; ++s)
                a.add (scaleInfo ((Scale) s).name);
            return a;
        }

        juce::StringArray groupNames()
        {
            juce::StringArray a;
            for (int g = 0; g < numChordGroups; ++g)
                a.add (chordGroupInfo ((ChordGroup) g).name);
            return a;
        }

        juce::ParameterID id (const juce::String& name) { return { name, 1 }; }

        juce::StringArray progressionNames()
        {
            juce::StringArray a;
            for (int p = 0; p < numProgressions; ++p)
                a.add (progressionInfo ((Progression) p).name);
            return a;
        }

        juce::StringArray degreeNames() { return { "I", "II", "III", "IV", "V", "VI", "VII" }; }

        juce::String numbered (const char* prefix, int slot, const char* suffix)
        {
            return juce::String (prefix) + juce::String (slot + 1) + suffix;
        }

        bool on (const std::atomic<float>* p) noexcept { return p->load() > 0.5f; }
        int whole (const std::atomic<float>* p) noexcept { return (int) std::lround (p->load()); }
        int choice (const std::atomic<float>* p, int count) noexcept { return juce::jlimit (0, count - 1, whole (p)); }
        double fraction (const std::atomic<float>* p) noexcept { return juce::jlimit (0.0, 1.0, (double) p->load() / 100.0); }

        template <class Name>
        juce::StringArray names (int count, Name name)
        {
            juce::StringArray a;
            for (int i = 0; i < count; ++i)
                a.add (name (i));
            return a;
        }

        std::unique_ptr<juce::AudioParameterFloat> percent (const juce::String& pid, const juce::String& name, float initial)
        {
            return std::make_unique<juce::AudioParameterFloat> (
                id (pid), name, juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), initial,
                juce::AudioParameterFloatAttributes().withLabel ("%"));
        }
    }

    juce::String customDegree (int slot) { return numbered ("customDegree", slot, ""); }
    juce::String seqStep (int pattern, int step) { return "seq" + juce::String (pattern + 1) + "Step" + juce::String (step + 1); }
    juce::String seqSteps (int pattern)      { return "seq" + juce::String (pattern + 1) + "Steps"; }
    bool isPresetParameter (const juce::String& paramId)
    {
        for (const char* excluded : { enable, testPattern })
            if (paramId == excluded)
                return false;
        return true;
    }

    juce::String stepsText (int mask)
    {
        juce::String t;
        for (int s = 0; s < numSeqSteps; ++s)
            t << (((mask >> s) & 1) != 0 ? "x" : ".");
        return t;
    }

    int stepsFromText (const juce::String& text)
    {
        const auto t = text.trim();
        if (t.containsOnly ("xX.-_o0 ") && t.containsAnyOf ("xX.-_"))
        {
            int mask = 0, s = 0;
            for (auto c : t)
            {
                if (c == ' ')
                    continue;
                if (s < numSeqSteps && (c == 'x' || c == 'X'))
                    mask |= 1 << s;
                ++s;
            }
            return mask;
        }
        return juce::jlimit (0, 0xffff, t.getIntValue());
    }

    juce::String liveNoteName (int note)
    {
        static const char* names[12] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
        return juce::String (names[((note % 12) + 12) % 12]) + juce::String (note / 12 - 2);
    }

    juce::AudioProcessorValueTreeState::ParameterLayout createLayout()
    {
        using namespace juce;
        AudioProcessorValueTreeState::ParameterLayout layout;

        layout.add (std::make_unique<AudioParameterBool> (id (enable), "Enable", true));
        layout.add (std::make_unique<AudioParameterChoice> (id (key), "Key", keyNames(), 0));
        layout.add (std::make_unique<AudioParameterChoice> (id (scale), "Scale", scaleNames(), 0));
        layout.add (std::make_unique<AudioParameterChoice> (id (chordGroup), "Chord Group", groupNames(), 0));
        layout.add (std::make_unique<AudioParameterInt> (id (chordOctave), "Chord Octave", -3, 3, 0));
        layout.add (std::make_unique<AudioParameterBool> (id (testPattern), "Test pattern", false));

        // The virtual port's three parameters ("port", "portCompensate", "portTrimMs") were
        // removed with the AU builds on 2026-10-20; a saved state's values for them are ignored.

        // Phase 2: Progression and its custom order. Appended, never reordered.
        layout.add (std::make_unique<AudioParameterChoice> (id (progression), "Progression", progressionNames(), 0));
        for (int n = 0; n < numChordSlots; ++n)
            layout.add (std::make_unique<AudioParameterChoice> (id (customDegree (n)), "Custom " + String (n + 1), degreeNames(), n));

        // Phase 3: the effects (Source/core/engine/ChordFx.h). Appended, never
        // reordered; the choice lists come from the engine's enums, which only ever append.
        const ChordFxSettings fx;
        layout.add (std::make_unique<AudioParameterChoice> (id (stack), "Stack",
                                                            names (numStackModes, [] (int i) { return stackModeName ((StackMode) i); }), 0));
        layout.add (std::make_unique<AudioParameterChoice> (id (strum), "Strum",
                                                            names (numStrumModes, [] (int i) { return strumModeName ((StrumMode) i); }), 0));
        layout.add (std::make_unique<AudioParameterChoice> (id (strumRate), "Strum rate", names (numStrumRates, strumRateName), fx.strumRate));
        layout.add (std::make_unique<AudioParameterFloat> (
            id (strumMs), "Strum time", NormalisableRange<float> (0.0f, 200.0f, 0.1f), (float) fx.strumMs,
            AudioParameterFloatAttributes().withLabel ("ms")));
        layout.add (std::make_unique<AudioParameterChoice> (id (arp), "Arp",
                                                            names (numArpPatterns, [] (int i) { return arpPatternName ((ArpPattern) i); }), 0));
        layout.add (std::make_unique<AudioParameterChoice> (id (arpRate), "Arp rate", names (numPatternRates, patternRateName), fx.arpRate));
        layout.add (std::make_unique<AudioParameterInt> (id (arpRange), "Arp range", minArpRange, maxArpRange, fx.arpRange));
        layout.add (percent (arpGate, "Arp gate", (float) (fx.arpGate * 100.0)));
        layout.add (std::make_unique<AudioParameterBool> (id (repeat), "Repeat", false));
        layout.add (std::make_unique<AudioParameterChoice> (id (repeatRate), "Repeat rate", names (numPatternRates, patternRateName), fx.repeat.rate));
        layout.add (percent (repeatGate, "Repeat gate", (float) (fx.repeat.gate * 100.0)));
        layout.add (std::make_unique<AudioParameterBool> (id (seq), "Step seq", false));
        layout.add (std::make_unique<AudioParameterChoice> (id (seqPattern), "Seq pattern", StringArray { "1", "2", "3", "4" }, 0));
        layout.add (percent (seqSwing, "Seq swing", 0.0f));
        layout.add (percent (seqGate, "Seq gate", (float) (fx.seq.gate * 100.0)));
        layout.add (std::make_unique<AudioParameterBool> (id (seqSustain), "Seq sustain", false));
        layout.add (std::make_unique<AudioParameterChoice> (id (velocity), "Velocity",
                                                            names (numVelocityCurves, [] (int i) { return velocityCurveName ((VelocityCurve) i); }), 0));
        layout.add (std::make_unique<AudioParameterInt> (id (velocityPeak), "Velocity peak", 1, 127, fx.velocity.peak));
        layout.add (percent (velocityRange, "Velocity range", (float) (fx.velocity.range * 100.0)));
        layout.add (std::make_unique<AudioParameterChoice> (id (hold), "Hold",
                                                            names (numHoldModes, [] (int i) { return holdModeName ((HoldMode) i); }), 0));

        // The four sequencer patterns, 16 steps each as a mask (bit s = step s + 1), starting as
        // the factory patterns. (Phase 3's 64 step bools; replaced in Phase 4, see Parameters.h.)
        auto stepsParam = [] (const String& pid, const String& name, int initial)
        {
            return std::make_unique<AudioParameterInt> (
                id (pid), name, 0, 0xffff, initial,
                AudioParameterIntAttributes().withStringFromValueFunction ([] (int v, int) { return stepsText (v); })
                                             .withValueFromStringFunction ([] (const String& t) { return stepsFromText (t); }));
        };
        const auto patterns = defaultSeqPatterns();
        for (int p = 0; p < numSeqPatterns; ++p)
            layout.add (stepsParam (seqSteps (p), "Seq " + String (p + 1) + " steps", patterns[(size_t) p]));

        // Phase 7: voice leading (VoiceLeading.h). Appended, never reordered.
        layout.add (std::make_unique<AudioParameterChoice> (id (voiceLeading), "Voice leading",
                                                            names (numVoiceLeadingModes, [] (int i) { return voiceLeadingName ((VoiceLeadingMode) i); }), 0));
        return layout;
    }

    Snapshot::Snapshot (juce::AudioProcessorValueTreeState& state)
        : enable (state.getRawParameterValue (params::enable)),
          keyParam (state.getRawParameterValue (params::key)),
          scale (state.getRawParameterValue (params::scale)),
          group (state.getRawParameterValue (params::chordGroup)),
          chordOctave (state.getRawParameterValue (params::chordOctave)),
          testPattern (state.getRawParameterValue (params::testPattern)),
          progression (state.getRawParameterValue (params::progression)),
          fxStack (state.getRawParameterValue (params::stack)),
          fxStrum (state.getRawParameterValue (params::strum)),
          fxStrumRate (state.getRawParameterValue (params::strumRate)),
          fxStrumMs (state.getRawParameterValue (params::strumMs)),
          fxArp (state.getRawParameterValue (params::arp)),
          fxArpRate (state.getRawParameterValue (params::arpRate)),
          fxArpRange (state.getRawParameterValue (params::arpRange)),
          fxArpGate (state.getRawParameterValue (params::arpGate)),
          fxRepeat (state.getRawParameterValue (params::repeat)),
          fxRepeatRate (state.getRawParameterValue (params::repeatRate)),
          fxRepeatGate (state.getRawParameterValue (params::repeatGate)),
          fxSeq (state.getRawParameterValue (params::seq)),
          fxSeqPattern (state.getRawParameterValue (params::seqPattern)),
          fxSeqSwing (state.getRawParameterValue (params::seqSwing)),
          fxSeqGate (state.getRawParameterValue (params::seqGate)),
          fxSeqSustain (state.getRawParameterValue (params::seqSustain)),
          fxVelocity (state.getRawParameterValue (params::velocity)),
          fxVelocityPeak (state.getRawParameterValue (params::velocityPeak)),
          fxVelocityRange (state.getRawParameterValue (params::velocityRange)),
          fxHold (state.getRawParameterValue (params::hold)),
          fxVoiceLeading (state.getRawParameterValue (params::voiceLeading))
    {
        jassert (enable && keyParam && scale && group && chordOctave && testPattern && progression);

        for (int n = 0; n < numChordSlots; ++n)
        {
            customDegree[(size_t) n] = state.getRawParameterValue (params::customDegree (n));
            jassert (customDegree[(size_t) n] != nullptr);
        }
        jassert (fxStack && fxStrum && fxStrumRate && fxStrumMs && fxArp && fxArpRate && fxArpRange && fxArpGate && fxRepeat
                 && fxRepeatRate && fxRepeatGate && fxSeq && fxSeqPattern && fxSeqSwing && fxSeqGate && fxSeqSustain && fxVelocity
                 && fxVelocityPeak && fxVelocityRange && fxHold);
        for (int p = 0; p < numSeqPatterns; ++p)
        {
            fxSeqSteps[(size_t) p] = state.getRawParameterValue (params::seqSteps (p));
            jassert (fxSeqSteps[(size_t) p] != nullptr);
        }

        jassert (fxVoiceLeading);
    }

    ChordFxSettings Snapshot::chordFx() const noexcept
    {
        ChordFxSettings fx;
        fx.stack = (StackMode) choice (fxStack, numStackModes);
        fx.strum = (StrumMode) choice (fxStrum, numStrumModes);
        fx.strumRate = choice (fxStrumRate, numStrumRates);
        fx.strumMs = juce::jlimit (0.0, 200.0, (double) fxStrumMs->load());
        fx.arp = (ArpPattern) choice (fxArp, numArpPatterns);
        fx.arpRate = choice (fxArpRate, numPatternRates);
        fx.arpRange = juce::jlimit (minArpRange, maxArpRange, whole (fxArpRange));
        fx.arpGate = fraction (fxArpGate);
        fx.repeat.on = on (fxRepeat);
        fx.repeat.rate = choice (fxRepeatRate, numPatternRates);
        fx.repeat.gate = fraction (fxRepeatGate);
        fx.seq.on = on (fxSeq);
        fx.seq.pattern = choice (fxSeqPattern, numSeqPatterns);
        fx.seq.swing = fraction (fxSeqSwing);
        fx.seq.gate = fraction (fxSeqGate);
        fx.seq.sustain = on (fxSeqSustain);
        for (int p = 0; p < numSeqPatterns; ++p)
            fx.seq.patterns[(size_t) p] = (uint16_t) juce::jlimit (0, 0xffff, whole (fxSeqSteps[(size_t) p]));
        fx.velocity.curve = (VelocityCurve) choice (fxVelocity, numVelocityCurves);
        fx.velocity.peak = juce::jlimit (1, 127, whole (fxVelocityPeak));
        fx.velocity.range = fraction (fxVelocityRange);
        fx.hold = (HoldMode) choice (fxHold, numHoldModes);
        fx.voiceLeading = (VoiceLeadingMode) choice (fxVoiceLeading, numVoiceLeadingModes);
        return fx;
    }

    ChordGroup Snapshot::chordGroup() const noexcept
    {
        return (ChordGroup) juce::jlimit (0, numChordGroups - 1, whole (group));
    }

    Scale Snapshot::currentScale() const noexcept
    {
        return (Scale) juce::jlimit (0, numScales - 1, whole (scale));
    }

    int Snapshot::key() const noexcept { return juce::jlimit (0, numKeys - 1, whole (keyParam)); }

    EngineSettings Snapshot::engineSettings() const noexcept
    {
        EngineSettings s;
        s.enabled = on (enable);
        s.key = key();
        s.scale = currentScale();
        s.group = chordGroup();
        s.chordOctave = juce::jlimit (-3, 3, whole (chordOctave));
        s.testPattern = on (testPattern);

        s.progression = (Progression) juce::jlimit (0, numProgressions - 1, whole (progression));
        for (int n = 0; n < numChordSlots; ++n)
            s.customOrder[(size_t) n] = (int8_t) juce::jlimit (0, numChordSlots - 1, whole (customDegree[(size_t) n]));
        s.fx = chordFx();
        return s;
    }
}
