#!/bin/sh

set -e

(
    # find SDL3_shadercross-3.0.0-darwin-arm64-x64 -type f -exec xattr -d com.apple.quarantine {} \; 2>/dev/null
    SDL3_shadercross-3.0.0-darwin-arm64-x64/bin/shadercross          shaders/unanimated.vert.hlsl -o MyApp.app/Contents/Resources/shaders/unanimated.vert.msl
    SDL3_shadercross-3.0.0-darwin-arm64-x64/bin/shadercross          shaders/unanimated.vert.hlsl -o MyApp.app/Contents/Resources/shaders/unanimated.vert.json
    SDL3_shadercross-3.0.0-darwin-arm64-x64/bin/shadercross          shaders/blinnphong_unanimated.vert.hlsl -o MyApp.app/Contents/Resources/shaders/blinnphong_unanimated.vert.msl
    SDL3_shadercross-3.0.0-darwin-arm64-x64/bin/shadercross          shaders/blinnphong_unanimated.vert.hlsl -o MyApp.app/Contents/Resources/shaders/blinnphong_unanimated.vert.json
    SDL3_shadercross-3.0.0-darwin-arm64-x64/bin/shadercross          shaders/swapchain.frag.hlsl -o MyApp.app/Contents/Resources/shaders/swapchain.frag.msl
    SDL3_shadercross-3.0.0-darwin-arm64-x64/bin/shadercross          shaders/swapchain.frag.hlsl -o MyApp.app/Contents/Resources/shaders/swapchain.frag.json
    SDL3_shadercross-3.0.0-darwin-arm64-x64/bin/shadercross          shaders/unlit_alphatest.frag.hlsl -o MyApp.app/Contents/Resources/shaders/unlit_alphatest.frag.msl
    SDL3_shadercross-3.0.0-darwin-arm64-x64/bin/shadercross          shaders/unlit_alphatest.frag.hlsl -o MyApp.app/Contents/Resources/shaders/unlit_alphatest.frag.json
    SDL3_shadercross-3.0.0-darwin-arm64-x64/bin/shadercross          shaders/blinnphong_alphatest.frag.hlsl -o MyApp.app/Contents/Resources/shaders/blinnphong_alphatest.frag.msl
    SDL3_shadercross-3.0.0-darwin-arm64-x64/bin/shadercross          shaders/blinnphong_alphatest.frag.hlsl -o MyApp.app/Contents/Resources/shaders/blinnphong_alphatest.frag.json
    SDL3_shadercross-3.0.0-darwin-arm64-x64/bin/shadercross          shaders/bone_animated.vert.hlsl -o MyApp.app/Contents/Resources/shaders/bone_animated.vert.msl
    SDL3_shadercross-3.0.0-darwin-arm64-x64/bin/shadercross          shaders/bone_animated.vert.hlsl -o MyApp.app/Contents/Resources/shaders/bone_animated.vert.json
    SDL3_shadercross-3.0.0-darwin-arm64-x64/bin/shadercross          shaders/text.vert.hlsl -o MyApp.app/Contents/Resources/shaders/text.vert.msl
    SDL3_shadercross-3.0.0-darwin-arm64-x64/bin/shadercross          shaders/text.vert.hlsl -o MyApp.app/Contents/Resources/shaders/text.vert.json
    SDL3_shadercross-3.0.0-darwin-arm64-x64/bin/shadercross          shaders/text.frag.hlsl -o MyApp.app/Contents/Resources/shaders/text.frag.msl
    SDL3_shadercross-3.0.0-darwin-arm64-x64/bin/shadercross          shaders/text.frag.hlsl -o MyApp.app/Contents/Resources/shaders/text.frag.json
    SDL3_shadercross-3.0.0-darwin-arm64-x64/bin/shadercross          shaders/fullscreen_quad.vert.hlsl -o MyApp.app/Contents/Resources/shaders/fullscreen_quad.vert.msl
    SDL3_shadercross-3.0.0-darwin-arm64-x64/bin/shadercross          shaders/fullscreen_quad.vert.hlsl -o MyApp.app/Contents/Resources/shaders/fullscreen_quad.vert.json
    SDL3_shadercross-3.0.0-darwin-arm64-x64/bin/shadercross          shaders/sprite.vert.hlsl -o MyApp.app/Contents/Resources/shaders/sprite.vert.msl
    SDL3_shadercross-3.0.0-darwin-arm64-x64/bin/shadercross          shaders/sprite.vert.hlsl -o MyApp.app/Contents/Resources/shaders/sprite.vert.json
    # SDL3_shadercross-3.0.0-darwin-arm64-x64/bin/shadercross entity_shader.vert.hlsl -o MyApp.app/Contents/Resources/shaders/entity_shader.vert.msl
    # SDL3_shadercross-3.0.0-darwin-arm64-x64/bin/shadercross entity_shader.vert.hlsl -o MyApp.app/Contents/Resources/shaders/entity_shader.vert.json
    # SDL3_shadercross-3.0.0-darwin-arm64-x64/bin/shadercross entity_shader.frag.hlsl -o MyApp.app/Contents/Resources/shaders/entity_shader.frag.msl
    # SDL3_shadercross-3.0.0-darwin-arm64-x64/bin/shadercross entity_shader.frag.hlsl -o MyApp.app/Contents/Resources/shaders/entity_shader.frag.json
    # SDL3_shadercross-3.0.0-darwin-arm64-x64/bin/shadercross  light_source.vert.hlsl -o MyApp.app/Contents/Resources/shaders/light_source.vert.msl
    # SDL3_shadercross-3.0.0-darwin-arm64-x64/bin/shadercross  light_source.frag.hlsl -o MyApp.app/Contents/Resources/shaders/light_source.frag.msl
    # SDL3_shadercross-3.0.0-darwin-arm64-x64/bin/shadercross        skybox.vert.hlsl -o MyApp.app/Contents/Resources/shaders/skybox.vert.msl
    # SDL3_shadercross-3.0.0-darwin-arm64-x64/bin/shadercross        skybox.frag.hlsl -o MyApp.app/Contents/Resources/shaders/skybox.frag.msl
    # glslc -x hlsl -fshader-stage=vert shader.vert.hlsl -o MyApp.app/Contents/Resources/shaders/shader.vert.spv
    # spirv-cross MyApp.app/Contents/Resources/shaders/shader.vert.spv --msl --stage vert --output MyApp.app/Contents/Resources/shaders/shader.vert.msl
    # spirv-cross MyApp.app/Contents/Resources/shaders/shader.vert.spv --reflect --stage vert --output MyApp.app/Contents/Resources/shaders/shader.vert.json
    # glslc -x hlsl -fshader-stage=frag shader.frag.hlsl -o MyApp.app/Contents/Resources/shaders/shader.frag.spv
    # spirv-cross MyApp.app/Contents/Resources/shaders/shader.frag.spv --msl --stage frag --output MyApp.app/Contents/Resources/shaders/shader.frag.msl
    # spirv-cross MyApp.app/Contents/Resources/shaders/shader.frag.spv --reflect --stage frag --output MyApp.app/Contents/Resources/shaders/shader.frag.json
)