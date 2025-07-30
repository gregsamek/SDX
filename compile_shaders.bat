@echo off
setlocal enabledelayedexpansion
(
    SDL3_shadercross-3.0.0-windows-VC-x64\bin\shadercross.exe shaders/unanimated.vert.hlsl -o shaders/unanimated.vert.spv -d SPIRV -t VERTEX
    SDL3_shadercross-3.0.0-windows-VC-x64\bin\shadercross.exe shaders/unanimated.frag.hlsl -o shaders/unanimated.frag.spv -d SPIRV -t FRAGMENT
)