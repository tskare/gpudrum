# GPU Drum Synth

A GPU-accelerated drum synth consisting of three separate pieces in the following subdirectories:

1. `plugin`: A JUCE-enabled plugin that calls into the GPU server.
2. `webui`: ui for the plugin.
3. `gpu/cuda`: CUDA implementation of the server for Windows/Linux; however the semaphore code is currently Windows-specific.
4. `gpu/metal`: Metal implementation of the server for MacOS (incoming)

The code for this project itself is MIT-Licensed; libraries may have different licenses where noted.

## System Requirements

The plugin and GPU server in this repository currently requires Windows and a CUDA GPU. Standalone and VST3 targets are supported.

For MacOS, Metal support is being merged in. An Apple Silicon processor is required.

The `simple-modal-filterbank` and plugin that drives it assume the host is running at 44.1kHz with a 256-sample buffer. This constraint will be lifted -- please see https://github.com/tskare/gpudrum/issues/1

## Issues

Please feel free to report issues on GitHub or by email.

## Instructions

`plugin` requires JUCE 8.0.6 which will be downloaded automatically via CPM. You may wish to set the environment variable `CPM_SOURCE_CACHE=~/.cache/CPM` or your preferred location to share library downloads between projects and not fetch on each clean build.

Build with CMake, e.g.:

```
# presets include default, release, vs for Visual Studio, XCode for XCode. 
cmake --preset default
cmake --build --preset default
```

To create a clean build:
```
cmake --build --clean-first --preset default
```

`gpu/cuda` contains moment there is a basic massively parallel modal resonator. CUDA code attempts to build on top of NVIDIA-provided Visual Studio projects, so that you may set up your machine for CUDA development and then simply open that project file in Visual Studio (Community edition works) and build.

`res` contains filter coefficient data and in the future, runtime PTX modules. Please ensure the directory `modecoeffs` resides inside a resources path referenced by the plugin. Search path uses the environment variable `DRUM_GPU_RESOURCES_DIR`, then `~/drumgpu` and `~/.drumgpu` if you do not wish to set an environment variable.

## Future work

The published version of the GPU server process currently leans into modal synthesis with modal processor processing effects.

The development version supports loading additional ptx modules developed by users (CUDA) and additional synthesis blocks (Metal): a network of 1D digital waveguides, and small/medium meshes.


