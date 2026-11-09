#pragma once
#include "PluginProcessor.h"

class HomeCard final : public juce::Button {
public:
    HomeCard(const juce::String& title,juce::Colour colour,int symbol);
    void paintButton(juce::Graphics&,bool,bool) override;
private:
    juce::Colour colour;
    int symbol;
};

class UniversalReferenceAudioProcessorEditor final : public juce::AudioProcessorEditor, private juce::Timer {
public:
    explicit UniversalReferenceAudioProcessorEditor(UniversalReferenceAudioProcessor&);
    ~UniversalReferenceAudioProcessorEditor() override { stopTimer(); }
    void paint(juce::Graphics&) override;
    void resized() override;
private:
    enum class Page { Home, Calibrate, Analyse, Translate, Settings };
    UniversalReferenceAudioProcessor& processor;
    Page page=Page::Home;
    HomeCard calibrateCard{"Calibrate",juce::Colour(0xffdfead9),0};
    HomeCard analyseCard{"Analyse",juce::Colour(0xfff2dfd3),1};
    HomeCard translateCard{"Translate",juce::Colour(0xffdee4f0),2};
    juce::TextButton back{"‹"}, settings{"⚙"}, importButton{"Import profile"}, analyseButton{"Check my mix"};
    juce::Label profile, readings, advice, note;
    juce::ToggleButton calibration{"Enable calibration"}, simulation{"Enable simulation"}, mix{"Mix gain"}, bypass{"Bypass everything"};
    juce::ComboBox device;
    juce::Slider intensity, mixGain;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> calAttachment,simAttachment,mixAttachment,bypassAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> deviceAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> intensityAttachment,gainAttachment;
    float displayLevel=-70, displayCorrelation=0, phase=0;
    bool showingAdvice=false;
    void show(Page);
    void timerCallback() override;
    void drawWave(juce::Graphics&,juce::Rectangle<float>,juce::Colour);
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(UniversalReferenceAudioProcessorEditor)
};
