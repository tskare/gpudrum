# GPU Drum Synth

A GPU-accelerated drum synth consisting of three separate pieces in the following subdirectories:

1. `plugin`: A JUCE-enabled plugin that calls into the GPU server.
2. `webui`: ui for the plugin.
3. `gpu/cuda`: CUDA implementation of the server for Windows/Linux*
4. `gpu/metal`: Metal implementation of the server for MacOS.

## Instructions

This project currently requires Windows and a CUDA GPU.

`plugin` requires JUCE 8.0.6 which will be downloaded automatically via CPM.

Build with CMake, e.g.:

```
# presets include default, release, vs for Visual Studio, XCode for XCode. 
cmake --preset default
cmake --build --preset default
```

`gpu` has one or more GPU server processes. At the moment there is a basic massively parallel modal resonators. These include Visual Studio projects based on CUDA Visual Studio sample code; setup your toolkit then build with Visual Studio projects.

`res` contains data etc. Please ensure the directory `modecoeffs` resides on a path referenced by your plugin, such as c:\src\res\modecoeffs.

## Future work

The published version of the GPU server process currently leans into modal synthesis with modal processor processing effects.

The development version supports loading additional ptx modules developed by users (CUDA) and additional synthesis blocks: a network of 1D digital waveguides, and 


