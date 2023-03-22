# A small planet generator demo

This is a small demo I built with a vulkan library I have been writing. It acts
as a simple planet generator by creating a sphere from the faces of a cube and
adding some noise to form some non uniform geometry. You may notice small visual
artifacts along the seams of the cube faces (particularly at low subdivisions), 
as I do not stitch them together.

The demo has some sliders to control the parameters of the noise being applied to
the surface and some very limited camera control (zooming a bit in and out with the
mouse wheel).

In the future I may come back to this and implement some modified noise algorithms
and a system for adding layers of noise with masking to create more interesting 
terrain.

Press 'q', 'Esc' or just close the window to exit.

## Requirements

To build the project you will need the Vulkan SDK and SDL2. The imgui
dependency is built alongside the project so you will need C and C++ toolchains.

The Linux build uses make and pkg-config

## Building

NOTE: the build system is not baking in shader data so the executable needs to be executed from the project root

### Linux:

```sh
make demo
./bin/demo
```

You can add extra flags if needed:

```sh
EXTRA_FLAGS="..." make demo
```

### Windows:

Set up the C toolchain environment (example):

```cmd
REM ProgramFiles\Microsoft Visual Studio\...\VC\Auxiliary\Build\...
vcvarsall x64
```

Execute build.bat

```cmd
build
bin\main.exe
```

With optimizations:

```cmd
set OPTIMIZE=/Ox
build
```

You can also manually set up your environment (see build.bat for ):

```cmd
set VULKAN_SDK=...
set SDL_INCLUDE=...
...
build
```

### MacOS:

Not supported because I don't have access for testing. I imagine it's fairly close to working as is with make and pkg-config, but don't know for sure.
