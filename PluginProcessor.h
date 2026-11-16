#pragma once
#include <JuceHeader.h>
#include <array>
#include <atomic>

class MasteringPro2AudioProcessor : public juce::AudioProcessor
{
public:
    MasteringPro2AudioProcessor();
    ~MasteringPro2AudioProcessor() override = default;

    void prepareToPlay(double, int) override;
    void releaseResources() override {}
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;

    using APVTS = juce::AudioProcessorValueTreeState;
    APVTS parameters;
    static APVTS::ParameterLayout createParameterLayout();

    float getIntegratedLUFS() const { return integratedLUFS.load(); }
    float getShortTermLUFS() const { return shortTermLUFS.load(); }
    float getTruePeakDb() const { return truePeakDb.load(); }
    float getStereoCorrelation() const { return stereoCorrelation.load(); }
    bool getNextFFTBlock(float* dest);
    void resetLoudness();

private:
    static constexpr int fftOrder = 11;
    static constexpr int fftSize = 1 << fftOrder;
    juce::dsp::FFT forwardFFT { fftOrder };
    juce::dsp::WindowingFunction<float> window { fftSize, juce::dsp::WindowingFunction<float>::hann };
    juce::AbstractFifo fftFifo { 32768 };
    std::array<float, 32768> fftQueue {};
    std::array<float, fftSize> fftBlock {};
    std::array<float, fftSize * 2> fftData {};
    int fftBlockPos = 0;

    std::array<std::array<juce::dsp::IIR::Filter<float>, 2>, 5> eq;
    std::array<std::array<juce::dsp::IIR::Coefficients<float>::Ptr, 2>, 5> eqCoefs;
    juce::dsp::IIR::Filter<float> kWeightShelfL, kWeightShelfR, kWeightHighpassL, kWeightHighpassR;
    double sampleRate = 44100.0;
    float envL = 0.0f, envR = 0.0f, limiterGain = 1.0f;
    float corrXY = 0.0f, corrXX = 0.0f, corrYY = 0.0f;
    double loudnessEnergy = 0.0;
    uint64_t loudnessBlocks = 0;
    double blockEnergy = 0.0;
    int loudnessBlockSamples = 0;
    std::atomic<float> integratedLUFS {-100.0f}, shortTermLUFS {-100.0f};
    std::atomic<float> truePeakDb {-100.0f}, stereoCorrelation {0.0f};
    juce::dsp::Oversampling<float> oversampling { 2, 2, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true };

    void updateEQ();
    void pushFFT(float);
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MasteringPro2AudioProcessor)
};
