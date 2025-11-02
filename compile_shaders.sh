#!/usr/bin/env bash

set -euo pipefail

# Config (override via environment variables if desired)
SHADERCROSS_BIN="${SHADERCROSS_BIN:-SDL3_shadercross-3.0.0-darwin-arm64-x64/bin/shadercross}"
SHADER_SRC_DIR="${SHADER_SRC_DIR:-shaders}"
OUT_DIR="${OUT_DIR:-MyApp.app/Contents/Resources/shaders}"

# Ensure shadercross exists
if [[ ! -x "$SHADERCROSS_BIN" ]]; then
  echo "Error: shadercross not found or not executable at: $SHADERCROSS_BIN" >&2
  exit 1
fi

# Clean output directory
rm -rf "$OUT_DIR"
mkdir -p "$OUT_DIR"

# Compile each .hlsl file to .msl, .json, .spv, .dxil
# Preserves relative subdirectories from SHADER_SRC_DIR
echo "Scanning '$SHADER_SRC_DIR' for .hlsl files..."
found_any=false
while IFS= read -r -d '' src; do
  found_any=true

  # Compute relative path and strip .hlsl suffix
  rel="${src#$SHADER_SRC_DIR/}"
  base_no_ext="${rel%.hlsl}"

  # Where outputs will go (ensure directory exists)
  out_base="$OUT_DIR/$base_no_ext"
  mkdir -p "$(dirname "$out_base")"

  echo "Compiling: $rel"
  "$SHADERCROSS_BIN" "$src" -o "$out_base.msl"
  "$SHADERCROSS_BIN" "$src" -o "$out_base.json"
  "$SHADERCROSS_BIN" "$src" -o "$out_base.spv"
  "$SHADERCROSS_BIN" "$src" -o "$out_base.dxil"
done < <(find "$SHADER_SRC_DIR" -type f -name "*.hlsl" -print0)

if [[ "$found_any" == false ]]; then
  echo "No .hlsl files found in '$SHADER_SRC_DIR'."
fi

echo "Done. Outputs written to: $OUT_DIR"

# run this when a new SDL_shadercross binary is downloaded on macOS (and update SHADERCROSS_BIN)
    
    # find SDL3_shadercross-3.0.0-darwin-arm64-x64 -type f -exec xattr -d com.apple.quarantine {} \; 2>/dev/null

# if for some reason, you can't use shadercross, you *theoretically* can use the underlying tools
# HOWEVER, they DO NOT work with structured buffers, so good luck with that!
# https://github.com/libsdl-org/SDL/issues/12200

    # glslc -x hlsl -fshader-stage=vert shader.vert.hlsl -o MyApp.app/Contents/Resources/shaders/shader.vert.spv
    # spirv-cross MyApp.app/Contents/Resources/shaders/shader.vert.spv --msl --stage vert --output MyApp.app/Contents/Resources/shaders/shader.vert.msl
    # spirv-cross MyApp.app/Contents/Resources/shaders/shader.vert.spv --reflect --stage vert --output MyApp.app/Contents/Resources/shaders/shader.vert.json
    # glslc -x hlsl -fshader-stage=frag shader.frag.hlsl -o MyApp.app/Contents/Resources/shaders/shader.frag.spv
    # spirv-cross MyApp.app/Contents/Resources/shaders/shader.frag.spv --msl --stage frag --output MyApp.app/Contents/Resources/shaders/shader.frag.msl
    # spirv-cross MyApp.app/Contents/Resources/shaders/shader.frag.spv --reflect --stage frag --output MyApp.app/Contents/Resources/shaders/shader.frag.json