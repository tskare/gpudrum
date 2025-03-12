/*
  ==============================================================================

    ModeLoader.cpp

  ==============================================================================
*/

#include "JuceGPUDrum/ModeLoader.h"

#include <juce_core/juce_core.h>

#include <iostream>
#include <fstream>

using juce::File;

ModeLoader::ModeLoader()
{
}


void ModeLoader::loadDefaultSet()
{
    // CLEANUP: Load from user data dir
    bool isRecursive = false;
    for (juce::DirectoryEntry entry :juce:: RangedDirectoryIterator(File("C:\\src\\res\\modecoeffs\\"), isRecursive))
    {
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
		//infile >> modes.freqs[i];
	}

	float scale = 1.0f;
	// original capture set temporary line -- make cymbals louder
	if (label[0] == '1' || label[0] == '2' ||
		(label[0]=='c' && label[1] == 'o' && label[2]=='w')) {
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

		// Temporarily suppress low-end.
		// Or Temporarily don't
#if 1
		if (modes.freqs[i] < _hz2rad * 35) {
			modes.amps[i] *= 0.5;
		} else if (modes.freqs[i] < _hz2rad * 80) {
			modes.amps[i] *= 0.75;
		}
#endif
	}
	// should normalize everything by mag later.
	//mag = 0.01f;
	for (int i = 0; i < NM; i++) {
		modes.amps[i] /= mag;
		modes.amps[i] *= scale;
	}

	for (int i = 0; i < NM; i++) {
		infile >> modes.damps[i];
		//float mindamp = 0.00001;
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

ModeParams::ModeParams()
{
}

// CLEANUP: Finish removing CPU mode; plugin is entirely GPU based now.
Drum::Drum()
	: do_cpu_(true),
		synth_mode_(Drum::SynthesisMode::SINGLE)
{
}

void Drum::tick([[maybe_unused]] float input) {
    /*
	// In both 
	float input_abs = std::abs(input);

	if (synth_mode_ == Drum::SynthesisMode::SINGLE) {

	}
	else if (synth_mode_ == Drum::SynthesisMode::N_BY_VELOCITY) {

	}

	if (do_cpu_) {
		// Excite those modes 
	}
        */
}