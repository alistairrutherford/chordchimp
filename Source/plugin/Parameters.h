// Host parameters (APVTS). Every setting a test needs is one of these, because LiveBridge
// reaches the plug-in only through Live's device parameters (PLAN.md §8).
//
// IDs, names and choice lists are saved in Live sets and automation: never rename or reorder
// them, only add new ones.
#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "../core/engine/ChordEngine.h"

namespace chordchimp::params
{
    inline constexpr const char* enable       = "enable";
    inline constexpr const char* key          = "key";
    inline constexpr const char* scale        = "scale";
    inline constexpr const char* chordGroup   = "chordGroup";
    inline constexpr const char* chordOctave  = "chordOctave";
    inline constexpr const char* testPattern  = "testPattern";

    // Phase 2 (appended).
    inline constexpr const char* progression  = "progression";
    juce::String customDegree (int slot);     // "customDegree1".."customDegree7"

    // Phase 3 (appended): the effects.
    inline constexpr const char* stack        = "stack";
    inline constexpr const char* strum        = "strum";
    inline constexpr const char* strumRate    = "strumRate";
    inline constexpr const char* strumMs      = "strumMs";
    inline constexpr const char* arp          = "arp";
    inline constexpr const char* arpRate      = "arpRate";
    inline constexpr const char* arpRange     = "arpRange";
    inline constexpr const char* arpGate      = "arpGate";
    inline constexpr const char* repeat       = "repeat";
    inline constexpr const char* repeatRate   = "repeatRate";
    inline constexpr const char* repeatGate   = "repeatGate";
    inline constexpr const char* seq          = "seq";
    inline constexpr const char* seqPattern   = "seqPattern";
    inline constexpr const char* seqSwing     = "seqSwing";
    inline constexpr const char* seqGate      = "seqGate";
    inline constexpr const char* seqSustain   = "seqSustain";
    inline constexpr const char* velocity     = "velocity";
    inline constexpr const char* velocityPeak = "velocityPeak";
    inline constexpr const char* velocityRange = "velocityRange";
    inline constexpr const char* hold         = "hold";

    // Phase 4 (2026-10-19): the 64 "seqNStepM" bools of Phase 3 were replaced by one 16-bit
    // mask per pattern (bit s = step s + 1). Saved states are migrated (ChordChimpProcessor).
    juce::String seqStep (int pattern, int step);   // the old IDs "seq1Step1".."seq4Step16", for migration
    juce::String seqSteps (int pattern);            // "seq1Steps".."seq4Steps"

    // Phase 7 (appended): voice leading.
    inline constexpr const char* voiceLeading  = "voiceLeading";

    // ChordChimp Phase 1 (2026-10-25) removed Chordism's Split, Mute CG / SS, the shifters and
    // Test shifter, every Smart Scale and pattern parameter, Test smart, Sync group and
    // Look-ahead (PLAN.md §3.5). A saved state's or preset's values for them are ignored.

    // Parameters a preset doesn't touch: Enable and the test rig (PresetManager).
    bool isPresetParameter (const juce::String& paramId);

    // A sequencer pattern as Live shows it: "x" for an active step, "." for a rest, step 1 first.
    juce::String stepsText (int mask);
    int stepsFromText (const juce::String& text);

    juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    // Lock-free reads of the current values, for the audio thread.
    class Snapshot
    {
    public:
        explicit Snapshot (juce::AudioProcessorValueTreeState& state);

        EngineSettings engineSettings() const noexcept;
        ChordGroup chordGroup() const noexcept;
        int key() const noexcept;
        Scale currentScale() const noexcept;
        ChordFxSettings chordFx() const noexcept;

    private:
        std::atomic<float>* enable;
        std::atomic<float>* keyParam;
        std::atomic<float>* scale;
        std::atomic<float>* group;
        std::atomic<float>* chordOctave;
        std::atomic<float>* testPattern;
        std::atomic<float>* progression;
        std::array<std::atomic<float>*, numChordSlots> customDegree {};

        std::atomic<float>* fxStack;
        std::atomic<float>* fxStrum;
        std::atomic<float>* fxStrumRate;
        std::atomic<float>* fxStrumMs;
        std::atomic<float>* fxArp;
        std::atomic<float>* fxArpRate;
        std::atomic<float>* fxArpRange;
        std::atomic<float>* fxArpGate;
        std::atomic<float>* fxRepeat;
        std::atomic<float>* fxRepeatRate;
        std::atomic<float>* fxRepeatGate;
        std::atomic<float>* fxSeq;
        std::atomic<float>* fxSeqPattern;
        std::atomic<float>* fxSeqSwing;
        std::atomic<float>* fxSeqGate;
        std::atomic<float>* fxSeqSustain;
        std::atomic<float>* fxVelocity;
        std::atomic<float>* fxVelocityPeak;
        std::atomic<float>* fxVelocityRange;
        std::atomic<float>* fxHold;
        std::array<std::atomic<float>*, numSeqPatterns> fxSeqSteps {};
        std::atomic<float>* fxVoiceLeading;
    };

    // MIDI note name as Live writes it (60 = C3).
    juce::String liveNoteName (int note);
}
