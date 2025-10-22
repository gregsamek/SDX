#!/bin/sh

set -e

(
    # find SDL3_shadercross-3.0.0-darwin-arm64-x64 -type f -exec xattr -d com.apple.quarantine {} \; 2>/dev/null
    SDL3_shadercross-3.0.0-darwin-arm64-x64/bin/shadercross shaders/unlit_unanimated.vert.hlsl      -o MyApp.app/Contents/Resources/shaders/unlit_unanimated.vert.msl
    SDL3_shadercross-3.0.0-darwin-arm64-x64/bin/shadercross shaders/blinnphong_unanimated.vert.hlsl -o MyApp.app/Contents/Resources/shaders/blinnphong_unanimated.vert.msl
    SDL3_shadercross-3.0.0-darwin-arm64-x64/bin/shadercross shaders/swapchain.frag.hlsl             -o MyApp.app/Contents/Resources/shaders/swapchain.frag.msl
    SDL3_shadercross-3.0.0-darwin-arm64-x64/bin/shadercross shaders/unlit_alphatest.frag.hlsl       -o MyApp.app/Contents/Resources/shaders/unlit_alphatest.frag.msl
    SDL3_shadercross-3.0.0-darwin-arm64-x64/bin/shadercross shaders/pbr_alphatest.frag.hlsl         -o MyApp.app/Contents/Resources/shaders/pbr_alphatest.frag.msl
    SDL3_shadercross-3.0.0-darwin-arm64-x64/bin/shadercross shaders/blinnphong_alphatest.frag.hlsl  -o MyApp.app/Contents/Resources/shaders/blinnphong_alphatest.frag.msl
    SDL3_shadercross-3.0.0-darwin-arm64-x64/bin/shadercross shaders/pbr_unanimated.vert.hlsl        -o MyApp.app/Contents/Resources/shaders/pbr_unanimated.vert.msl
    SDL3_shadercross-3.0.0-darwin-arm64-x64/bin/shadercross shaders/pbr_animated.vert.hlsl          -o MyApp.app/Contents/Resources/shaders/pbr_animated.vert.msl
    SDL3_shadercross-3.0.0-darwin-arm64-x64/bin/shadercross shaders/text.vert.hlsl                  -o MyApp.app/Contents/Resources/shaders/text.vert.msl
    SDL3_shadercross-3.0.0-darwin-arm64-x64/bin/shadercross shaders/text.frag.hlsl                  -o MyApp.app/Contents/Resources/shaders/text.frag.msl
    SDL3_shadercross-3.0.0-darwin-arm64-x64/bin/shadercross shaders/fullscreen_quad.vert.hlsl       -o MyApp.app/Contents/Resources/shaders/fullscreen_quad.vert.msl
    SDL3_shadercross-3.0.0-darwin-arm64-x64/bin/shadercross shaders/sprite.vert.hlsl                -o MyApp.app/Contents/Resources/shaders/sprite.vert.msl

    # if for some reason, you can't use shadercross, you *theoretically* can use the underlying tools
    # HOWEVER, they DO NOT work with structured buffers, so good luck with that!
    # https://github.com/libsdl-org/SDL/issues/12200

    # glslc -x hlsl -fshader-stage=vert shader.vert.hlsl -o MyApp.app/Contents/Resources/shaders/shader.vert.spv
    # spirv-cross MyApp.app/Contents/Resources/shaders/shader.vert.spv --msl --stage vert --output MyApp.app/Contents/Resources/shaders/shader.vert.msl
    # spirv-cross MyApp.app/Contents/Resources/shaders/shader.vert.spv --reflect --stage vert --output MyApp.app/Contents/Resources/shaders/shader.vert.json
    # glslc -x hlsl -fshader-stage=frag shader.frag.hlsl -o MyApp.app/Contents/Resources/shaders/shader.frag.spv
    # spirv-cross MyApp.app/Contents/Resources/shaders/shader.frag.spv --msl --stage frag --output MyApp.app/Contents/Resources/shaders/shader.frag.msl
    # spirv-cross MyApp.app/Contents/Resources/shaders/shader.frag.spv --reflect --stage frag --output MyApp.app/Contents/Resources/shaders/shader.frag.json
)