#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class MasteringPro2AudioProcessorEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit MasteringPro2AudioProcessorEditor(MasteringPro2AudioProcessor&);
    ~MasteringPro2AudioProcessorEditor() override = default;
    void paint(juce::Graphics&) override;
    void resized() override;
private:
    MasteringPro2AudioProcessor& processor;
    struct Knob { juce::Slider slider; juce::Label label; std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attach; };
    std::vector<std::unique_ptr<Knob>> knobs;
    juce::ComboBox presetBox;
    juce::TextButton loadPreset {"LOAD PRESET"}, resetMeter {"RESET LUFS"}, saveA {"STORE A"}, loadA {"RECALL A"}, saveB {"STORE B"}, loadB {"RECALL B"};
    juce::Label meterLabel;
    juce::Image spectrum;
    std::array<float,2048> fft {};
    juce::AudioProcessorValueTreeState::ParameterLayout* unused = nullptr;
    juce::ValueTree slotA, slotB;
    void addKnob(const juce::String&,const juce::String&);
    void timerCallback() override;
    void applyPreset(int);
    void storeSlot(juce::ValueTree&);
    void recallSlot(const juce::ValueTree&);
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MasteringPro2AudioProcessorEditor)
};
