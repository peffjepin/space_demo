.PHONY: all clean

CC ?= gcc
C++ ?= g++

SDL_CFLAGS ?= $$(pkg-config --cflags sdl2)
SDL_LIBS ?= $$(pkg-config --libs sdl2)
VULKAN_CFLAGS ?= $$(pkg-config --cflags vulkan)
VULKAN_LIBS ?= $$(pkg-config --libs vulkan)

IMGUI_SOURCES = $(wildcard imgui/*.cpp)
IMGUI_OBJECTS = $(patsubst imgui/%.cpp, build/%.o, $(IMGUI_SOURCES))

build/%.o: imgui/%.cpp
	@mkdir -p build
	$(C++) -c $(SDL_CFLAGS)  $< -o $@

build/%.o: src/%.c
	@mkdir -p build
	$(CC) -c $^ -o $@

build/%.o: src/%.cpp
	@mkdir -p build
	$(C++) -c $^ -o $@

bin/demo: build/main.o build/imgui_wrapper.o $(IMGUI_OBJECTS)
	@mkdir -p bin
	$(C++) $^ $(VULKAN_CFLAGS) $(SDL_CFLAGS) $(VULKAN_LIBS) $(SDL_LIBS) -o $@
	
all: bin/demo

clean:
	rm -rf build
	rm -rf bin
