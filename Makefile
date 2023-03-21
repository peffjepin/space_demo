.PHONY: all clean shaders

CC ?= gcc
C++ ?= g++
GLSLC ?= glslc

EXTRA_FLAGS ?=
FLAGS = -Wall -Wextra -Wpedantic -std=c11 $(EXTRA_FLAGS)

SDL_CFLAGS ?= $$(pkg-config --cflags sdl2)
SDL_LIBS ?= $$(pkg-config --libs sdl2)
VULKAN_CFLAGS ?= $$(pkg-config --cflags vulkan)
VULKAN_LIBS ?= $$(pkg-config --libs vulkan)

IMGUI_SOURCES = $(wildcard imgui/*.cpp)
IMGUI_OBJECTS = $(patsubst imgui/%.cpp, build/%.o, $(IMGUI_SOURCES))

SIMPLEX_SOURCES = $(wildcard simplex/*.c)
SIMPLEX_OBJECTS = $(patsubst simplex/%.c, build/%.o, $(SIMPLEX_SOURCES))

SHADER_SOURCES = $(wildcard shaders/*)
COMPILED_SHADERS = $(patsubst shaders/%, build/%.spv, $(SHADER_SOURCES))

CSOURCES = $(wildcard src/*.c)
COBJECTS = $(patsubst src/%.c, build/%.o, $(CSOURCES))

CPPSOURCES = $(wildcard src/*.cpp)
CPPOBJECTS = $(patsubst src/%.cpp, build/%.o, $(CPPSOURCES))

build/%.o: imgui/%.cpp
	@mkdir -p build
	$(C++) -c $(SDL_CFLAGS)  $< -o $@

build/%.o: simplex/%.c
	@mkdir -p build
	$(CC) -c $< $(FLAGS) -o $@

build/%.o: src/%.c
	@mkdir -p build
	$(CC) -c $^ $(FLAGS) -o $@

build/%.o: src/%.cpp
	@mkdir -p build
	$(C++) -c $^ -o $@

build/%.spv: shaders/%
	@mkdir -p build
	$(GLSLC) $< -o $@

DEMO_OBJECTS = $(COBJECTS) $(CPPOBJECTS) $(IMGUI_OBJECTS) $(SIMPLEX_OBJECTS)

bin/demo: $(DEMO_OBJECTS) $(COMPILED_SHADERS)
	@mkdir -p bin
	$(C++) $(DEMO_OBJECTS) $(FLAGS) $(VULKAN_CFLAGS) $(SDL_CFLAGS) $(VULKAN_LIBS) $(SDL_LIBS) -o $@

shaders: $(COMPILED_SHADERS)
	
all: bin/demo

clean:
	rm -rf build
	rm -rf bin
