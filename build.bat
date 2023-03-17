@echo off

if "%VULKAN_SDK%"=="" set VULKAN_SDK=.
if "%SDL_INCLUDE%"=="" set SDL_INCLUDE=.
if "%SDL_LIB%"=="" set SDL_LIB=.
if "%CL_EXE%"=="" set CL_EXE=cl.exe
if "%LINK_EXE%"=="" set LINK_EXE=link.exe
if "%GLSLC_EXE%"=="" set GLSLC=glslc.exe

set CFLAGS=/I"%VULKAN_SDK%\Include" /I"%SDL_INCLUDE%" /W4
set LFLAGS=/LIBPATH:"%VULKAN_SDK%\Lib" /LIBPATH:"%SDL_LIB%" vulkan-1.lib SDL2.lib

@echo on

if not exist bin mkdir bin
if not exist build mkdir build


%CL_EXE% %CFLAGS% /TC /c src\main.c /Fo:build\
%CL_EXE% %CFLAGS% /TP /c /I"%SDL_INCLUDE%" src\imgui_wrapper.cpp /Fo:build\
%CL_EXE% %CFLAGS% /TP /c imgui\*.cpp /Fo:build\
%LINK_EXE% %LFLAGS% /out:"bin\main.exe" build\*.obj
