#pragma once
#include <JuceHeader.h>
#include "DSP.h"
#include <atomic>
#include <array>

class UniversalReferenceAudioProcessor final : public juce::AudioProcessor {
public:
    UniversalReferenceAudioProcessor();
    ~UniversalReferenceAudioProcessor() override = default;
    void prepareToPlay(double, int) override;
    void releaseResources() override {}
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;
    bool isBusesLayoutSupported(const BusesLayout&) const override;

    juce::AudioProcessorValueTreeState apvts;
    struct Profile { juce::String name; std::array<float, 6> gains{}; bool verified=false; };
    // Stored in a fixed-size profile array; switching is by index, never by audio-thread allocation.
    bool importProfile(const juce::File&, juce::String& error);
    juce::String getProfileName() const;
    bool hasMeasuredProfile() const { return profileLoaded.load(); }
    std::atomic<float> peakDb{-100}, rmsDb{-100}, correlation{0}, lowRatioDb{0}, lowMidRatioDb{0}, presenceRatioDb{0};
    std::array<std::atomic<float>, 24> spectrum{};
    static constexpr std::array<double,6> centres{60,150,350,1000,3500,9000};
private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameters();
    struct Filters { std::array<ur::Biquad,6> eq; ur::Biquad hp,lp; };
    std::array<Filters,2> filters;
    std::array<std::atomic<float>,6> userProfile{};
    std::atomic<bool> profileLoaded{false};
    juce::String profileName="Uncalibrated";
    double currentSampleRate=44100;
    int lastDevice=-1;
    float lastCorrection=-1;
    bool lastCalibrated=false;
    int statCount=0;
    ur::StereoStats stats;
    std::array<double,24> bandEnergy{};
    std::array<ur::Biquad,2> analysisLow, analysisMid, analysisPresence;
    void configureFilters();
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(UniversalReferenceAudioProcessor)
};
