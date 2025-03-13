# GPU Drum Synth

A GPU-accelerated drum synth consisting of three separate pieces in the following subdirectories:

1. `plugin`: A JUCE-enabled plugin that calls into the GPU server.
2. `webui`: ui for the plugin.
3. `gpu/cuda`: CUDA implementation of the server for Windows/Linux; however the semaphore code is currently Windows-specific.
4. `gpu/metal`: Metal implementation of the server for MacOS (incoming)

## Licensing

The code written for this project itself is MIT-Licensed.

Please note that libraries may have more restrictive licenses. Most directly, the `plugin` directory depends on the JUCE framework which is dual-licensed commercially and under AGPL. JUCE is fetched at build time as a module, but the `webui` includes some native/web interop code from the framework in `webui/src/js/juce`, which is similarly dual-licensed.

## System Requirements

The plugin and GPU server in this repository currently requires Windows and a CUDA GPU. Standalone and VST3 targets are supported.

For MacOS, Metal support is being merged in. An Apple Silicon processor is required.

The `simple-modal-filterbank` and plugin that drives it assume the host is running at 44.1kHz with a 256-sample buffer. This constraint will be lifted, tracked in https://github.com/tskare/gpudrum/issues/1

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

`gpu/cuda` contains Windows and/or Linux server prosses. At the moment `simple-modal-filterbank` contains a basic massively parallel switched-modal resonator, without the nonliear coupling described in the work. 

For ease of building, CUDA code attempts to build on top of NVIDIA-provided Visual Studio project files, so that you may set up your machine for CUDA development and then simply open a project file in this repository in Visual Studio. VS Community edition works.

`res` contains shared required resources for the plugins such as filter coefficient data. Please ensure the directory `modecoeffs` resides inside a resources path referenced by the plugin. Search path uses the environment variable `DRUM_GPU_RESOURCES_DIR`, then `~/drumgpu` and `~/.drumgpu` if you do not wish to set an environment variable.

Work to compress this data, have it bundled into the plugin binary, and only search the resources directory for additional modes is beting tracked in https://github.com/tskare/gpudrum/issues/2

## Future work

The published version of the GPU server process currently leans into modal synthesis with modal processor processing effects. A development version supports loading additional ptx modules developed by users (CUDA) and additional synthesis blocks (Metal): a network of 1D digital waveguides, and small/medium meshes.


