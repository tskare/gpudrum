#include "JuceGPUDrum/PluginProcessor.h"

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

#include <chrono>
#include <cmath>
#include <functional>
#include <vector>

#include "JuceGPUDrum/ParameterIDs.hpp"
#include "JuceGPUDrum/PluginEditor.h"
#include "JuceGPUDrum/globals.h"

namespace webview_plugin {

// TODO: Handshake this between plugin and server, or at least read from the environment.
// It's fragile to keep magic constants in sync between one plugin and server, and now we're adding copies of each.
constexpr int BUFFERSIZE = 256;
constexpr int NDRUMS = 10;
constexpr int NMODES = NDRUMS * 1024;

// From v1 Modal Plugin
#if (JUCE_WINDOWS)
#include <tchar.h>
#include <windows.h>

// Synchronization Primitive names
// Should match those in the CUDA code.
TCHAR szName[] = TEXT("Local\\GPUModalBankMem");
TCHAR szNameSemaphore[] = TEXT("Local\\GPUModalBankSemaphore");
TCHAR szNameSemaphoreGPU[] = TEXT("Local\\GPUModalBankSemaphoreGPU");
#define SHAREDMEMSIZE 1024 * 512 * 2
#endif  // JUCE_WINDOWS

#if (JUCE_MAC)
#include "JuceGPUDrum/MacSharedMemoryRegion.h"
MacSharedMemoryRegion macSharedMemoryRegion;
#endif  // JUCE_MAC

struct ModeInfo {
    bool enabled;
    bool reset;

    bool amp_changed;
    float amp_real;
    float amp_imag;

    float damp;

    float freq;
    bool freq_changed;
};

struct DrumInfo {
    float pan;
};


AudioPluginAudioProcessor::AudioPluginAudioProcessor()
    : AudioProcessor(
          BusesProperties()
#if !JucePlugin_IsMidiEffect
#if !JucePlugin_IsSynth
              .withInput("Input", juce::AudioChannelSet::stereo(), true)
#endif
              .withOutput("Output", juce::AudioChannelSet::stereo(), true)
#endif
              ),
      state{*this, nullptr, "PARAMETERS", createParameterLayout(parameters)} {

    // Install file-based logger.
    // On Mac, this will likely be in ~/Library/Logs/DrumGPU.log
    // On Windows, this will likely be in %APPDATA%/DrumGPU.log
    // Default parameters will trim the log from growing indefinitely.
    if (kEnableFileLogging) {
        juce::File logFile = juce::FileLogger::getSystemLogFileFolder().getChildFile("DrumGPU.log");
        fileLogger = std::unique_ptr<juce::FileLogger>(new juce::FileLogger(logFile, "drum.GPU logfile"));
        juce::Logger::setCurrentLogger(fileLogger.get());
    }
    juce::Logger::writeToLog("drum.GPU: Starting up");

    // Windows: Set up shared memory and synchronization.
#if (JUCE_WINDOWS)
    // CLEANUP: Merge this into MacSharedMemoryRegion
    hMapFile = CreateFileMapping(
        INVALID_HANDLE_VALUE,  // use paging file,
        NULL,                  // default security
        PAGE_READWRITE,
        0,              // max object size (high-order)
        SHAREDMEMSIZE,  // max obj size (low-order),
        szName);
    if (hMapFile == nullptr) {
        fprintf(stderr, "shared memory init failed! %d", GetLastError());
        jassert(0);
    }
    pSharedMem = (void*)MapViewOfFile(hMapFile,
                                      FILE_MAP_ALL_ACCESS,
                                      0,
                                      0,
                                      SHAREDMEMSIZE);
    if (pSharedMem == nullptr) {
        // fprintf(stderr, "mapviewoffile failed! %d", GetLastError());
        CloseHandle(hMapFile);
        jassert(0);
    }
    hSemaphore = CreateSemaphoreA(NULL, 0, 1, szNameSemaphore);
    if (hSemaphore == nullptr) {
        // fprintf(stderr, "could not create semaphore %d", GetLastError());
        CloseHandle(hMapFile);
        jassert(0);
    }
    hSemaphoreGPU = CreateSemaphoreA(NULL, 0, 1, szNameSemaphoreGPU);
    if (hSemaphoreGPU == nullptr) {
        // fprintf(stderr, "could not create semaphore %d", GetLastError());
        CloseHandle(hMapFile);
        jassert(0);
    }
#endif  // WINDOWS

    // Mac: Set up shared memory and synchronization.
#if (JUCE_MAC)
    macSharedMemoryRegion.init();
    if (macSharedMemoryRegion.ready()) {
        juce::Logger::writeToLog("Mac: Startup: Shared memory and Semaphores ready");
    } else {
        juce::Logger::writeToLog("Mac: Startup: Shared memory and Semaphores not ready");
        jassertfalse;
    }
#endif  // JUCE_MAC

    modefiles.loadDefaultSet();
    drum_assignments[0] = &modefiles.mode_sets["kick22yamahabirch"];
    drum_assignments[1] = &modefiles.mode_sets["tom10dwcoll"];
    drum_assignments[2] = &modefiles.mode_sets["tom12dwcoll"];
    drum_assignments[3] = &modefiles.mode_sets["tom16dwcustom"];

    drum_assignments[4] = &modefiles.mode_sets["snrLudwigMahogany"];
    drum_assignments[5] = &modefiles.mode_sets["13_sab_dejonte_crash"];
    drum_assignments[6] = &modefiles.mode_sets["16_zild_1960s_vintage"];
    drum_assignments[7] = &modefiles.mode_sets["19_sab_aa_medthin"];

    for (std::size_t i = 0; i < 1024; i++) {
        empty_assignment.amps[i] = {0, 0};
        empty_assignment.freqs[i] = 0;
        empty_assignment.damps[i] = 0.5;
    }

    int s = 0;
    for (int i = 0; i < kMaxDrums; i++) {
        for (int j = 0; j < kNumParamsPerDrum; j++) {
            float defaultval = 0.5f;
            if (j == 2 || j == 3 || j == 4 || j == 5) defaultval = 0.0f;
            drumParams[s++] = defaultval;
        }
    }
    for (int i = 0; i < kNumCommonParams; i++) {
        commonParams[i] = 0.5f;
    }
}

AudioPluginAudioProcessor::~AudioPluginAudioProcessor() {}

const juce::String AudioPluginAudioProcessor::getName() const {
    return JucePlugin_Name;
}

bool AudioPluginAudioProcessor::acceptsMidi() const {
#if JucePlugin_WantsMidiInput
    return true;
#else
    return false;
#endif
}

bool AudioPluginAudioProcessor::producesMidi() const {
#if JucePlugin_ProducesMidiOutput
    return true;
#else
    return false;
#endif
}

bool AudioPluginAudioProcessor::isMidiEffect() const {
#if JucePlugin_IsMidiEffect
    return true;
#else
    return false;
#endif
}

double AudioPluginAudioProcessor::getTailLengthSeconds() const {
    return 0.0;
}

// Suggested from example:
// Some hosts require nPrograms >= 1.
int AudioPluginAudioProcessor::getNumPrograms() {
    return 1;
}

int AudioPluginAudioProcessor::getCurrentProgram() {
    return 0;
}

void AudioPluginAudioProcessor::setCurrentProgram(int index) {
    juce::ignoreUnused(index);
}

const juce::String AudioPluginAudioProcessor::getProgramName(int index) {
    juce::ignoreUnused(index);
    return {};
}

void AudioPluginAudioProcessor::changeProgramName(int index,
                                                  const juce::String& newName) {
    juce::ignoreUnused(index, newName);
}

void AudioPluginAudioProcessor::prepareToPlay(double sampleRate,
                                              int samplesPerBlock) {
    using namespace juce;

    // TODO: Restore to N channels for release build.
    // Set to mono for video recording and demo with one stage monitor.
    const int numChannels = kForceMono ? 1 : getTotalNumOutputChannels();
    juce::dsp::ProcessSpec spec{sampleRate, static_cast<juce::uint32>(samplesPerBlock), numChannels};
    initObjects(spec);
}

void AudioPluginAudioProcessor::releaseResources() {}

bool AudioPluginAudioProcessor::isBusesLayoutSupported(
    const BusesLayout& layouts) const {
#if JucePlugin_IsMidiEffect
    juce::ignoreUnused(layouts);
    return true;
#else
    //  We support mono or stereo.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono() &&
        layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
#if !JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != static_cast<int>(layouts.getMainInputChannelSet()))
        return false;
#endif

    return true;
#endif
}

void AudioPluginAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                             juce::MidiBuffer& midiMessages) {
    // Track processBlock times for debugging. Can break and inspect, or print.
    using namespace std::chrono;
    static std::vector<double> histogram;

    auto start = high_resolution_clock::now();

    juce::ScopedNoDenormals noDenormals;

    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    float timestretch = 1.0f;
    float pitchshift = 1.0f;

#if (JUCE_MAC)
    // Server in this repo is Windows-only. Zero out all channels, but run the rest of the function
    // as we bring in the Metal GPU server.
    // Update -- brought in the Metal implementation.
    // CLEANUP: We can likely remove this. Then we can also run DAW audio through the plugin as inputs.
    for (auto i = 0; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());
#endif

    // This code clears any output channels that didn't contain input data,
    // (because these aren't guaranteed to be empty - they may contain garbage).
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());
    // If forcing mono with internal stereo filter process (live demo through PA speaker), clear all but first channel.
    // CLEANUP: Remove debug flag to force mono. Forcing mono should be done with postprocessing or audio driver.
    // Demos will also now be with two PA speakers.
    if (kForceMono) {
        for (auto i = 1; i < totalNumOutputChannels; ++i) {
            buffer.clear(i, 0, buffer.getNumSamples());
        }
    }
    
    // Process MIDI
    static float drumVel[NDRUMS];
    for (int drumi = 0; drumi < NDRUMS; drumi++) {
        drumVel[drumi] = 0.0f;
    }
    // int samplepos = 0;
    // for (juce::MidiBuffer::Iterator midii(midiMessages); midii.getNextEvent(m, samplepos);)
    for (const auto& mmsg : midiMessages) {
        auto m = mmsg.getMessage();
        if (m.isNoteOn()) {
            int notenum = m.getNoteNumber();
            int whichdrum = -1;
            // We map to Octapad with the following base note.
            // TODO: map to general midi drum notes by default, or make configurable in the UI.
            int basenote = 68;
            // juce::Logger::writeToLog("Midi NoteOn: " + std::to_string(notenum));
            if (notenum >= basenote && notenum < basenote + 8) {
                whichdrum = notenum - basenote;
                float volControl = getVolume(whichdrum);
                float vel = volControl * (m.getVelocity() / 127.0f);
                if (vel > 0.99f)
                    vel = 0.99f;
                DBG("Drum caught NoteOn: " << notenum << " " << vel);
                drumVel[whichdrum] = vel;
            }
        }
    }

    for (int i = 0; i < kMaxDrums; i++) {
        if (dbgHits[i] > 0) {
            drumVel[i] = 0.48f * getVolume(i);  // TODO: remove magic number.
            dbgHits[i] = 0;
        }
    }

    // Update LFOs
    // CLEANUP: do this while waiting for GPU, and/or at slower rate.
    for (auto& lfo : lfos_shimmer) {
    	lfo.processSample(0.0f);
    }

    // Set up modes
    static bool first_block = true;
    #if (JUCE_WINDOWS)
        int* sharedmem_modeinfoptr = (int*)pSharedMem;
    #elif (JUCE_MAC)
        int* sharedmem_modeinfoptr = (int*)macSharedMemoryRegion.getAddr();
    #endif
    float* sharedmem_druminfoptr = (float*)((char*)sharedmem_modeinfoptr + NMODES * sizeof(ModeInfo));
    float* sharedmem_inputptr = (float*)((char*)sharedmem_druminfoptr + NDRUMS * sizeof(float) * 8);
    int* sharedmem_outputptr = (int*)((char*)sharedmem_inputptr + NDRUMS * BUFFERSIZE * sizeof(float));
    // int* sharedmem_outputptr2 = (int*)((char*)sharedmem_outputptr + BUFFERSIZE * sizeof(float));

    // TODO: Discuss running some of this only outside of a block, or at block N-1 in parallel with GPU
    // in a streaming setup.
    for (int drumi = 0; drumi < NDRUMS; drumi++) {
        auto* mf = drum_assignments[drumi];
        if (mf == nullptr) {
            mf = &empty_assignment;
        }

        // Cleanup: Move this to an input scaling knob, either in the plugin or in the host.
        /*
        // Allow modifying velocity scaling for Demo @ open house
        // Shows effect of Attack Mod parameter and allows adjusting the velocity curves
        // for different players.
        size_t num_velocity_layers = mf->lowvel_amps.size() + 1;
        float vel_layer_space_occupied = 1.0f / num_velocity_layers;
        float scalar = getScaledVelocityLayerSelection(drumi);
        vel_layer_space_occupied /= scalar;
        int which_velocity_layer = static_cast<int>(drumVel[drumi] / vel_layer_space_occupied);
        int dbg_layer = static_cast<int>(getDebugVelocityLayer(drumi, static_cast<int>(num_velocity_layers)));
        if (dbg_layer >= 0) {
            which_velocity_layer = dbg_layer;
            if (dbg_layer > 0) {
                DBG("DBG: scaled velocity layer: " << dbg_layer);
            }
        }
        std::array<std::complex<float>, 1024>* velocityApplication;
        if (which_velocity_layer >= static_cast<int>(mf->lowvel_amps.size())) {
            velocityApplication = &mf->amps;
        } else {
            velocityApplication = &mf->lowvel_amps[which_velocity_layer];
        }
        */
        pitchshift = getPitchshift(drumi);
        timestretch = getTimestretch(drumi);
        // Process shimmer for this drum.
        float shimmer_sample = lfos_shimmer[drumi].processSample(0.0f);
        float shimmer_low = 0.0f;
        float shimmer_high = 0.0f;
        // Shimmer LFOs may not be used with some versions of the filter bank; ignore warnings.
        juce::ignoreUnused(shimmer_low, shimmer_high);
        float shimmer_scale = drumParams[drumi * kNumParamsPerDrum + 3];
        bool do_shimmer = false;
        // First section of control range turns the feature off.
        if (shimmer_scale > 0.08f) {
            shimmer_scale /= 80.0f;
            do_shimmer = true;
            shimmer_low = 1.0f + shimmer_scale * shimmer_sample;
            shimmer_high = 1.0f + shimmer_scale * -shimmer_sample;
        }
        for (int modei = 0; modei < 1024; modei++) {
            int modeidx = drumi * 1024 + modei;

            ModeInfo* mode = &((ModeInfo*)sharedmem_modeinfoptr)[modeidx];

            mode->enabled = true;
            mode->reset = reset || first_block;
            mode->freq = mf->freqs[modei] * pitchshift;

            // Shimmer test extension to frequency
            if (do_shimmer) {
                if (modei < 500) {
                    // No-op
                } else {
                    mode->freq *= shimmer_high;
                }
            }

            mode->freq_changed = true;
            mode->amp_changed = true;
            mode->amp_real = mf->amps[modei].real();
            mode->amp_imag = mf->amps[modei].imag();

            mode->damp = mf->damps[modei] * timestretch;
        }
    }

    // Set up drum controls
    // Set up inputs
    for (int input_drum = 0; input_drum < NDRUMS; input_drum++) {
        float suppressModes [[maybe_unused]] = drumParams[input_drum * kNumParamsPerDrum + 4];
        float attackMod = drumParams[input_drum * kNumParamsPerDrum + 5];
        // Control params accessible to GPU.
        for (int dpi = 0; dpi < 8; dpi++) {
            sharedmem_druminfoptr[input_drum * 8 + dpi] = 0.0f;
        }
        sharedmem_druminfoptr[input_drum * 8 + 0] = getPan(input_drum);

        for (int input_samp = 0; input_samp < BUFFERSIZE; input_samp++) {
            float input = 0.0f;
            if (first_block) {
                // AudioUnits historically had reference to some bug where the first buffer did not process.
                // CLEANUP: This is likely not needed anymore; check and remove
            } else {
                // Apply attack modification
                if (attackMod < 0.1f) {
                    float scale = 0.0f;
                    if (input_samp < 100) {
                        scale = (input_samp / 100.0f);
                    }
                    scale /= 10.0f;  // Arbitrary and data-dependent. TODO: formalize.
                    input = drumVel[input_drum] * scale;
                } else {
                    // Changing scaling range for Octapad.
                    // CLEANUP: Remove
                    float scale = 0.0f;
                    float spread = (BUFFERSIZE * attackMod);
                    if (input_samp < (int)(spread)) {
                        scale = (input_samp / spread);
                    }
                    scale /= 8.0f;
                    input = drumVel[input_drum] * scale;
                }
            }
            sharedmem_inputptr[input_drum * BUFFERSIZE + input_samp] = input;
        }
    }
    first_block = false;

    // We have work available for the GPU: Signal our semaphore and wait on the GPU process's.
#if (JUCE_WINDOWS)
    ReleaseSemaphore(hSemaphore, 1, NULL);
    // Cleanup: consider waiting a finite time and disconnecting if we don't hear back.
    WaitForSingleObject(hSemaphoreGPU, INFINITE);
#elif (JUCE_MAC)
    macSharedMemoryRegion.signalCPU();
    macSharedMemoryRegion.waitGPU();
#endif

    // GPU process populated shared memory.
    float* sampsBuf = ((float*)sharedmem_outputptr);

    float sampCompIn = 0.0f, sampCompIn2 = 0.0f;

    for (int sample = 0; sample < buffer.getNumSamples(); sample++) {
        // float input = 0.0f;
        float sampOut = 0.0f;
        float sampOut2 = 0.0f;

        // Temporary -- adjust to get more volume in room. TODO: remove
        constexpr float SCALE_DEMO_LOUD = 2.0f;
        sampsBuf[2 * sample] *= SCALE_DEMO_LOUD;
        sampsBuf[2 * sample + 1] *= SCALE_DEMO_LOUD;

        float mag_final = std::abs(sampsBuf[2 * sample]);
        if (mag_final > 1.0f) {
            sampsBuf[2 * sample] /= mag_final;
        }
        mag_final = std::abs(sampsBuf[2 * sample + 1]);
        if (mag_final > 1.0f) {
            sampsBuf[2 * sample + 1] /= mag_final;
        }

        // Simple bus comp, software version.
        // TODO: fix for stereo or remove
        sampCompIn = sampsBuf[2 * sample];
        sampCompIn2 = sampsBuf[2 * sample + 1];
        float compOut = dspBusComp.processSample(0, sampCompIn);
        float compOut2 = dspBusComp2.processSample(0, sampCompIn2);

        // Output is the sum of drums
        sampOut = sampsBuf[2 * sample];
        sampOut2 = sampsBuf[2 * sample + 1];
        // Optionally with bus comp mixed in in parallel.
        // Lower 10% of slider omits the connection.
        if (getBusCompValue() > 0.1) {
            sampOut += getBusCompValue() * compOut;
            sampOut2 += getBusCompValue() * compOut2;
        }

        // Mono demos
        // buffer.setSample(0, sample, sampOut);

        // Stereo demos
        buffer.setSample(0, sample, sampOut);
        buffer.setSample(1, sample, sampOut2);
    }
    // Reverb.
    reverb.processStereo(buffer.getWritePointer(0), buffer.getWritePointer(1), buffer.getNumSamples());

    reset = false;

    if (kLogBufferProcessingTimes) {
        auto end = high_resolution_clock::now();
        duration<double, std::milli> elapsed = end - start;
        histogram.push_back(elapsed.count());
    }
}

bool AudioPluginAudioProcessor::hasEditor() const {
    return true;  // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* AudioPluginAudioProcessor::createEditor() {
    return new AudioPluginAudioProcessorEditor(*this);
}

void AudioPluginAudioProcessor::getStateInformation(
    juce::MemoryBlock& destData) {
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
    juce::ignoreUnused(destData);
}

void AudioPluginAudioProcessor::setStateInformation(const void* data,
                                                    int sizeInBytes) {
    // You should use this method to restore your parameters from this memory
    // block, whose contents will have been created by the getStateInformation()
    // call.
    juce::ignoreUnused(data, sizeInBytes);
}

juce::AudioProcessorValueTreeState::ParameterLayout
AudioPluginAudioProcessor::createParameterLayout(
    AudioPluginAudioProcessor::Parameters& parameters) {
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout layout;

    {
        auto parameter = std::make_unique<AudioParameterFloat>(
            id::GAIN, "gain", NormalisableRange<float>{0.f, 1.f, 0.01f, 0.9f}, 1.f);
        parameters.gain = parameter.get();
        layout.add(std::move(parameter));
    }

    {
        auto parameter = std::make_unique<AudioParameterBool>(
            id::BYPASS, "bypass", false,
            AudioParameterBoolAttributes{}.withLabel("Bypass"));
        parameters.bypass = parameter.get();
        layout.add(std::move(parameter));
    }

    {
        auto parameter = std::make_unique<AudioParameterChoice>(
            id::DISTORTION_TYPE, "distortion type",
            StringArray{"none", "tanh(kx)/tanh(k)", "sigmoid"}, 0);
        parameters.distortionType = parameter.get();
        layout.add(std::move(parameter));
    }

    return layout;
}

/// V1
constexpr int kCommonParamIdxBusComp = 0;
constexpr int kCommonParamIdxVerb = 1;
constexpr int kCommonParamIdxVerbSize = 2;
constexpr int kCommonParamIdxVerbConfig = 3;
constexpr int kCommonParamComp0 = 4;
constexpr int kCommonParamComp1 = 5;
constexpr int kCommonParamComp2 = 6;
constexpr int kCommonParamComp3 = 7;

constexpr int kDrumParamModeBleed = 5;
constexpr int kDrumParamIdxAttack = 5;

void AudioPluginAudioProcessor::setCommonParam(int pidx, float val) {
    commonParams[pidx] = val;

    if (pidx == kCommonParamIdxVerb) {
        reverbParams.wetLevel = val;
        reverbParams.dryLevel = 1.0f - (val / 3.0f);
        reverb.setParameters(reverbParams);
    }

    if (pidx == kCommonParamIdxVerbSize) {
        float roomsize = 0.9f * val;
        reverbParams.roomSize = roomsize;
        reverb.setParameters(reverbParams);
    }
    if (pidx == kCommonParamIdxVerbConfig) {
        float damp = 0.9f * val;
        reverbParams.damping = damp;
        reverb.setParameters(reverbParams);
    }
    if (pidx == kCommonParamComp0) {
        dspBusComp.setThreshold(val * -22.0f);
        dspBusComp2.setThreshold(val * -22.0f);
    }
    if (pidx == kCommonParamComp1) {
        dspBusComp.setAttack(val * 200.0f);
        dspBusComp2.setAttack(val * 200.0f);
    }
    if (pidx == kCommonParamComp2) {
        dspBusComp.setRelease(val * 2000.0f);
        dspBusComp2.setRelease(val * 2000.0f);
    }
}

float AudioPluginAudioProcessor::getVolume(int idx) {
    float vol = drumParams[idx * kNumParamsPerDrum + 7];
    // 0-1 --> 0-2
    return vol * 2;
}

float AudioPluginAudioProcessor::getPan(int idx) {
    float pan = drumParams[idx * kNumParamsPerDrum + 6];
    return 1 - pan;  // invert L/R
}

int AudioPluginAudioProcessor::getDebugVelocityLayer(int idx, int nLevels) {
    float param = drumParams[idx * kNumParamsPerDrum + kDrumParamIdxAttack];

    // no-op.
    if (param >= 0.45f && param <= 0.55f)
        return -1;

    int whichLevel = (int)(param * nLevels);
    if (whichLevel >= nLevels)
        return nLevels - 1;

    if (whichLevel <= 0)
        return 0;

    // TODO: check against v1.
    return whichLevel;
}

float AudioPluginAudioProcessor::getScaledVelocityLayerSelection(int idx) {
    float param = drumParams[idx * kNumParamsPerDrum + kDrumParamIdxAttack];
    constexpr float maxScalar = 4.0f;
    if (param < 0.45) {
        return 0.5f + 0.5f * (param / 0.45f);
    }
    if (param > 0.55) {
        return 1.0f + maxScalar * ((param - 0.55f) / 0.45f);
    }
    // middle of the range, leave as normal.
    return 1.0f;
}

float AudioPluginAudioProcessor::getBusCompValue() {
    return commonParams[0];
}

float AudioPluginAudioProcessor::getPitchshift(int idx) {
    float fxbLocal = drumParams[idx * kNumParamsPerDrum + 0];
    // used to be size 0-1, flip for pitch 0-1.
    fxbLocal = 1.0f - fxbLocal;
    float pitchshift = 1.0f;
    if (fxbLocal < 0.48f) {
        pitchshift = 1.0f + 2.0f * (0.5f - fxbLocal);
    }
    if (fxbLocal > 0.52f) {
        float coeff = 2.0f * (fxbLocal - 0.5f);  // normalize to 0..1
        pitchshift = 1.0f / (1.0f + coeff);
    }
    return pitchshift;
}

float AudioPluginAudioProcessor::getTimestretch(int idx) {
    float fxaLocal = drumParams[idx * kNumParamsPerDrum + 1];
    float timestretch = 1.0f;
    if (fxaLocal < 0.48f) {
        // Scale control slider:
        // 0 is 4x
        // 0.5 is normal
        timestretch = 1.0f + 4.0f * (0.5f - fxaLocal);
    }
    if (fxaLocal > 0.52f) {
        // Scale control slider:
        // at 1.0, we want half
        // at 0.5, we want 0
        float coeff = 2.0f * (fxaLocal - 0.5f);  // normalize to 0..1
        timestretch = 1.0f / (1.0f + coeff);
    }
    return timestretch;
}

void AudioPluginAudioProcessor::loadDrumInSlot([[maybe_unused]] std::string drumname, [[maybe_unused]] int i) {
}

void AudioPluginAudioProcessor::setDrum(int which, std::string name) {
    if (modefiles.mode_sets.count(name) == 0) {
        // jassert(0);
        return;
    }
    juce::Logger::writeToLog("(ok) Setting drum " + std::to_string(which) + " to " + name);
    drum_assignments[which] = &modefiles.mode_sets[name];
    reset = true;
}

// Deprecated global parameters, replaced with a bank of |kNumCommonParams|.
void AudioPluginAudioProcessor::setFx(float fxaSetter, float fxbSetter) {
    this->fxa = fxaSetter;
    this->fxb = fxbSetter;
}

void AudioPluginAudioProcessor::initObjects(juce::dsp::ProcessSpec spec) {
    // Harmonic Tremolo for Shimmer
    for (auto& lfo : lfos_shimmer) {
        lfo.initialise([](float x) { return std::sin(x); }, 128);
        lfo.setFrequency(256 * 3.0f);
    }

    // Bus compressors A/B
    dspBusComp.prepare(spec);
    dspBusComp.setThreshold(-20.0f);
    dspBusComp.setRatio(4.0);
    dspBusComp.setAttack(20);
    dspBusComp.setRelease(200);
    dspBusComp.reset();
    dspBusComp2.prepare(spec);
    dspBusComp2.setThreshold(-20.0f);
    dspBusComp2.setRatio(4.0);
    dspBusComp2.setAttack(20);
    dspBusComp2.setRelease(200);
    dspBusComp2.reset();

    // Reverb
    reverb.setSampleRate(spec.sampleRate);
    reverbParams.roomSize = 0.5f;
    reverbParams.wetLevel = 0.0f;
    reverb.setParameters(reverbParams);
}

}  // namespace webview_plugin

// This creates new instances of the plugin.
// This function definition must be in the global namespace.
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new webview_plugin::AudioPluginAudioProcessor();
}
