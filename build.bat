@echo off

if "%VULKAN_SDK%"=="" set VULKAN_SDK=.
if "%SDL_INCLUDE%"=="" set SDL_INCLUDE=.
if "%SDL_LIB%"=="" set SDL_LIB=.
if "%CL_EXE%"=="" set CL_EXE=cl.exe
if "%LINK_EXE%"=="" set LINK_EXE=link.exe
if "%GLSLC_EXE%"=="" set GLSLC_EXE=glslc.exe

set CFLAGS=/D_CRT_SECURE_NO_WARNINGS /I"%VULKAN_SDK%\Include" /I"%SDL_INCLUDE%" /W4 %OPTIMIZE%
set LFLAGS=/LIBPATH:"%VULKAN_SDK%\Lib" /LIBPATH:"%SDL_LIB%" vulkan-1.lib SDL2.lib
set SOURCES=src\3d.c src\noise.c src\planet.c src\renderer.c src\transfer_buffer.c simplex\simplex.c

@echo on

if not exist bin mkdir bin
if not exist build mkdir build

%GLSLC_EXE% shaders\planet.vert -o build\planet.vert.spv
%GLSLC_EXE% shaders\planet.frag -o build\planet.frag.spv
%CL_EXE% %CFLAGS% /TC /std:c11 /c src\main.c /Fo:build\
%CL_EXE% %CFLAGS% /TC /std:c11 /c %SOURCES% /Fo:build\
%CL_EXE% %CFLAGS% /TP /c /I"%SDL_INCLUDE%" src\imgui_wrapper.cpp /Fo:build\
%CL_EXE% %CFLAGS% /TP /c imgui\*.cpp /Fo:build\
%LINK_EXE% %LFLAGS% /out:"bin\main.exe" build\*.obj
