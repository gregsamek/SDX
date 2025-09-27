#!/usr/bin/env bash
set -euo pipefail

./compile_shaders.sh

APP="MyApp.app"
APP_MACOS="$APP/Contents/MacOS"
APP_FRAMEWORKS="$APP/Contents/Frameworks"
mkdir -p "$APP_MACOS" "$APP_FRAMEWORKS" obj

# Where the xcframeworks live
DEPS_DIR="$(pwd)/external"
SDL3_XC="$DEPS_DIR/SDL3.xcframework"
SDL3_TTF_XC="$DEPS_DIR/SDL3_ttf.xcframework"
SDL3_IMG_XC="$DEPS_DIR/SDL3_image.xcframework"

# macOS slice in each xcframework
SDL3_SLICE="$SDL3_XC/macos-arm64_x86_64"
SDL3_TTF_SLICE="$SDL3_TTF_XC/macos-arm64_x86_64"
SDL3_IMG_SLICE="$SDL3_IMG_XC/macos-arm64_x86_64"

# Framework search flags (used for both compile and link)
FW_CFLAGS=(
  -F "$SDL3_SLICE"
  -F "$SDL3_TTF_SLICE"
  -F "$SDL3_IMG_SLICE"
)

# If you still include as #include <SDL.h> etc., uncomment the headers line(s):
# FW_CFLAGS+=( -I "$SDL3_SLICE/SDL3.framework/Headers" )
# FW_CFLAGS+=( -I "$SDL3_TTF_SLICE/SDL3_ttf.framework/Headers" )
# FW_CFLAGS+=( -I "$SDL3_IMG_SLICE/SDL3_image.framework/Headers" )

# Determine parallelism
if command -v nproc >/dev/null 2>&1; then
  JOBS=$(nproc)
elif command -v sysctl >/dev/null 2>&1; then
  JOBS=$(sysctl -n hw.ncpu)
else
  JOBS=2
fi

# Compile all C sources in parallel using xargs correctly
export LC_ALL=C
find src -name "*.c" -print0 | xargs -0 -P "$JOBS" -I {} sh -c '
  src="$1"; shift
  obj="obj/${src##*/}"; obj="${obj%.c}.o"
  echo "Compiling $src -> $obj"
  clang -c "$src" -o "$obj" "$@" \
    -fcolor-diagnostics -fansi-escape-codes -g -O0 -fno-omit-frame-pointer
' _ {} "${FW_CFLAGS[@]}"

echo "Linking..."
clang obj/*.o -o "$APP_MACOS/main" \
  "${FW_CFLAGS[@]}" \
  -framework SDL3 \
  -framework SDL3_ttf \
  -framework SDL3_image \
  -Wl,-rpath,@executable_path/../Frameworks \
  -Wl,-rpath,@loader_path/../Frameworks \
  -g

echo "Embedding frameworks..."
cp -R "$SDL3_SLICE/SDL3.framework"         "$APP_FRAMEWORKS/" 2>/dev/null || true
cp -R "$SDL3_TTF_SLICE/SDL3_ttf.framework" "$APP_FRAMEWORKS/" 2>/dev/null || true
cp -R "$SDL3_IMG_SLICE/SDL3_image.framework" "$APP_FRAMEWORKS/" 2>/dev/null || true

echo "Codesigning (ad-hoc for local runs)..."
for FW in SDL3 SDL3_ttf SDL3_image; do
  if [ -d "$APP_FRAMEWORKS/$FW.framework" ]; then
    codesign --force --sign - --timestamp=none "$APP_FRAMEWORKS/$FW.framework"
  fi
done
codesign --force --sign - --timestamp=none "$APP"

echo "Generating dSYM file..."
dsymutil "$APP_MACOS/main"

rm -rf obj
echo "Build complete."

echo "Verify linkage:"
otool -L "$APP_MACOS/main" || true

########################

# run this when shadercross is updated to fix Apple quarantine nonsense

# find SDL3_shadercross-3.0.0-darwin-arm64-x64 -type f -exec xattr -d com.apple.quarantine {} \; 2>/dev/null

########################
