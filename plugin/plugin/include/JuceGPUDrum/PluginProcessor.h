#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

#if (JUCE_WINDOWS)
#include <Windows.h>
#endif
#include <array>

#include "JuceGPUDrum/ModeLoader.h"

namespace webview_plugin {

constexpr int kMaxDrums = 10;
constexpr int kMaxModesPerDrum = 1000;

constexpr int kNumParamsPerDrum = 10;
constexpr int kNumCommonParams = 10;

class AudioPluginAudioProcessor : public juce::AudioProcessor {
   public:
    AudioPluginAudioProcessor();
    ~AudioPluginAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    [[nodiscard]] juce::AudioProcessorValueTreeState& getState() noexcept {
        return state;
    }

    [[nodiscard]] const juce::AudioParameterChoice& getDistortionTypeParameter()
        const noexcept {
        return *parameters.distortionType;
    }

    std::atomic<float> outputLevelLeft;

    void loadDrumInSlot(std::string drumname, int i);

    void setDrum(int which, std::string name);
    void setFx(float fxa, float fxb);
    void setDrumParam(int drumidx, int pidx, float val) {
        drumParams[drumidx * kNumParamsPerDrum + pidx] = val;
        // Debugging shimmer controls with params 2/3
        if (pidx == 2) {
            lfos_shimmer->reset();
            // We update only every audio callback. Multiplying by 256 here.
            lfos_shimmer[drumidx].setFrequency(val * 256.0f * 6.0f);
        }
    }

    void setCommonParam(int pidx, float val);
    void setHit(int idx) {
        dbgHits[idx] = 1;
    }

   private:
    struct Parameters {
        juce::AudioParameterFloat* gain{nullptr};
        juce::AudioParameterBool* bypass{nullptr};
        juce::AudioParameterChoice* distortionType{nullptr};
    };

    [[nodiscard]] static juce::AudioProcessorValueTreeState::ParameterLayout
    createParameterLayout(Parameters&);

    Parameters parameters;
    juce::AudioProcessorValueTreeState state;
    juce::dsp::BallisticsFilter<float> envelopeFollower;
    juce::AudioBuffer<float> envelopeFollowerOutputBuffer;

#if (JUCE_WINDOWS)
    HANDLE hSemaphore;
    HANDLE hSemaphoreGPU;
    HANDLE hMapFile;
    void* pSharedMem;
#endif

    ModeLoader modefiles;

    std::array<ModeFile*, 10> drum_assignments = {nullptr};
    ModeFile empty_assignment;

    void initObjects(juce::dsp::ProcessSpec spec);
    juce::dsp::Oscillator<float> lfos_shimmer[kMaxDrums];

    // Params
    float getPitchshift(int idx);
    float getTimestretch(int idx);
    float getVolume(int idx);
    float getPan(int idx);

    float getScaledVelocityLayerSelection(int idx);
    float getBusCompValue();

    // Params, debug
    int getDebugVelocityLayer(int idx, int nLevels);

    // File-based logger, if set with kEnableFileLogger
    std::unique_ptr<juce::FileLogger> fileLogger;

    // Debug
    juce::Random randomGen;
    // Bus comp, software
    juce::dsp::Compressor<float> dspBusComp;
    juce::dsp::Compressor<float> dspBusComp2;
    // "Comfort" reverb if not using an external one.
    juce::Reverb reverb;
    juce::Reverb::Parameters reverbParams;

    bool reset = false;

    float fxa = 0.5f;
    float fxb = 0.5f;
    // Allow triggering by button click.
    int dbgHits[kMaxDrums] = {0};

    float drumParams[kMaxDrums * kNumParamsPerDrum] = {0.5f};
    float commonParams[kNumCommonParams] = {0.0f};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioPluginAudioProcessor)
};
}  // namespace webview_plugin
