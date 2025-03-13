#pragma once

#include <array>
#include <complex>
#include <map>
#include <vector>

#include "ModeLoader.h"

class ModeParams;

using std::complex;

typedef std::vector<ModeParams> ModeList;

// Supports storing and tracking modes in a variety of approaches.
class Drum {
   public:
    Drum();

    // Could be subclasses, but not complicated for that yet.
    enum class SynthesisMode {
        // One set of modes.
        SINGLE = 1,
        // N sets of modes, scaled by input MIDI velocities
        N_BY_VELOCITY = 2,
        // N sets of modes, scaled by input. Differs by N_BY_VELOCITY
        // as restrikes may scale up.
        N_BY_INPUT_FOLLOWER = 3,
    };

    // Whether to compute MaxFilters and render CPU audio.
    bool do_cpu_;
    SynthesisMode synth_mode_;

    void tick(float input);
};

class ModeParams {
   public:
    ModeParams();

    // ModeParams& set_enabled(bool on) {		on_ = on;	}
    void set_input_gain(complex<float> input_gain) {
        input_gain_ = input_gain;
    }
    void set_freq(float mode_freq) {
        mode_freq_ = mode_freq;
    }
    void set_damp(complex<float> damp) {
        damp_ = damp;
    }

   public:
    // these are currently directly accessed.
    // bool on_;
    complex<float> input_gain_;
    float mode_freq_;
    complex<float> damp_;
};

struct ModeFile {
   public:
    std::array<std::complex<float>, 1024> amps;
    std::array<float, 1024> freqs;
    std::array<float, 1024> damps;
    std::vector<std::array<std::complex<float>, 1024>> lowvel_amps;
};

class ModeLoader {
   public:
    ModeLoader();

    void loadDefaultSet();

    void loadSwitchedModalFromFile(std::string fname, std::string label);

    std::map<std::string, ModeFile> mode_sets;
};
