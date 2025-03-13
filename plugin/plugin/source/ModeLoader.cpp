// ModeLoader loads filter bank coefficients at different regimes from our binary file format.

#include "JuceGPUDrum/ModeLoader.h"

#include <juce_core/juce_core.h>

#include <fstream>
#include <iostream>

using juce::File;

ModeLoader::ModeLoader() {
}

void ModeLoader::loadDefaultSet() {
    juce::File baseDir;
    bool foundBaseDir = false;

    // We search directories in the following order:
    // 1. environment variable DRUM_GPU_RESOURCES_DIR
    // 2. ~/gpudrum/modecoeffs
    // 3. ~/.gpudrum/modecoeffs
    // 4. C:\src\res\modecoeffs (developer path)
    // CLEANUP: Remove (4) after presentation.
    if (const char* envPath = std::getenv("DRUM_GPU_RESOURCES_DIR")) {
        juce::File envDir(envPath);
        juce::File modeDir = envDir.getChildFile("modecoeffs");
        if (modeDir.isDirectory()) {
            baseDir = modeDir;
            foundBaseDir = true;
        }
    }
    if (!foundBaseDir) {
        juce::File homeDir = juce::File::getSpecialLocation(juce::File::userHomeDirectory);
        juce::File gpuDrumDir = homeDir.getChildFile("gpudrum").getChildFile("modecoeffs");
        if (gpuDrumDir.isDirectory()) {
            baseDir = gpuDrumDir;
            foundBaseDir = true;
        }
    }
    if (!foundBaseDir) {
        juce::File homeDir = juce::File::getSpecialLocation(juce::File::userHomeDirectory);
        juce::File hiddenGpuDrumDir = homeDir.getChildFile(".gpudrum").getChildFile("modecoeffs");
        if (hiddenGpuDrumDir.isDirectory()) {
            baseDir = hiddenGpuDrumDir;
            foundBaseDir = true;
        }
    }
    if (!foundBaseDir) {
        baseDir = juce::File("C:\\src\\res\\modecoeffs");
    }
    bool isRecursive = false;
    for (juce::DirectoryEntry entry : juce::RangedDirectoryIterator(File("C:\\src\\res\\modecoeffs\\"), isRecursive)) {
        auto foundfile = entry.getFile();
        auto fullpath = foundfile.getFullPathName().toStdString();
        auto fname = foundfile.getFileName().toStdString();
        loadSwitchedModalFromFile(fullpath, fname);
        juce::Logger::writeToLog("Loaded mode" + fname);
    }
}

void ModeLoader::loadSwitchedModalFromFile(std::string fname, std::string label) {
    std::ifstream infile;
    infile.open(fname, std::ios_base::in);

    // NMODES freqs
    // NMODES real/imaginary amps (repeated)
    // NMODES damps
    // count of low velocity amps
    //    for each: NMODES real/imaginary amps (repeated)
    if (mode_sets.count(label) > 0) {
        jassert(0);
    }

    constexpr int NM = 1024;

    std::string line;
    ModeFile& modes = mode_sets[label];
    for (int i = 0; i < NM; i++) {
        std::getline(infile, line);
        modes.freqs[i] = (float)::atof(line.c_str());
    }

    float scale = 1.0f;
    // For open house demo - make cymbals louder
    if (label[0] == '1' || label[0] == '2' ||
        (label[0] == 'c' && label[1] == 'o' && label[2] == 'w')) {
        scale = 5.0f;
    }
    if (label[0] == 's' && label[1] == 'n' && label[2] == 'r') {
        scale = 3.0f;
    }
    constexpr float _hz2rad = 2.0f * 3.141529f / 44100.0f;
    float mag = 0.0f;
    for (int i = 0; i < NM; i++) {
        float real, imag;
        infile >> real;
        infile >> imag;
        modes.amps[i] = std::complex<float>(real, imag);
        modes.amps[i] *= scale;
        mag += std::abs(modes.amps[i]);

        // Suppress low end for use with a specific PA speaker with live demos.
        // CLEANUP: We no longer need this.
#if 1
        if (modes.freqs[i] < _hz2rad * 35) {
            modes.amps[i] *= 0.5;
        } else if (modes.freqs[i] < _hz2rad * 80) {
            modes.amps[i] *= 0.75;
        }
#endif
    }
    // should normalize everything by mag later.
    // mag = 0.01f;
    for (int i = 0; i < NM; i++) {
        modes.amps[i] /= mag;
        modes.amps[i] *= scale;
    }

    for (int i = 0; i < NM; i++) {
        infile >> modes.damps[i];
        float mindamp = 0.00004f;
        if (std::abs(modes.damps[i]) < mindamp) {
            modes.damps[i] = mindamp;
        }
    }

    int nLowVel = 0;
    infile >> nLowVel;

    for (int j = 0; j < nLowVel; j++) {
        std::array<std::complex<float>, 1024> lowvel;
        for (int i = 0; i < NM; i++) {
            float real, imag;
            infile >> real;
            infile >> imag;
            lowvel[i] = std::complex<float>(real, imag);
            lowvel[i] /= mag;
        }
        modes.lowvel_amps.push_back(lowvel);
    }

    mode_sets[label] = modes;
}

ModeParams::ModeParams() {
}
