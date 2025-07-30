#!/bin/sh

set -e

(
    ./compile_shaders.sh

    mkdir -p obj

    # Determine the number of available CPU cores
    if command -v nproc >/dev/null; then
        JOBS=$(nproc)
    elif command -v sysctl >/dev/null; then
        JOBS=$(sysctl -n hw.ncpu)
    else
        JOBS=2
    fi

    find src -name "*.c" | xargs -P "$JOBS" -I {} sh -c '
        source_file="$1"
        echo "Compiling $source_file"
        clang -c "$source_file" -o "obj/$(basename "$source_file" .c).o" \
            -I/usr/local/include \
            -fcolor-diagnostics -fansi-escape-codes \
            -g -O0 \
            -fno-omit-frame-pointer
    ' _ {}

    echo "Linking..."
    clang obj/*.o -o \
        MyApp.app/Contents/MacOS/main \
        -L./MyApp.app/Contents/Frameworks \
        -lSDL3 -lSDL3_ttf -lSDL3_image -lSDL3_mixer \
        "-Wl,-rpath,@executable_path/../Frameworks" \
        -g # Keep -g on the link line to tell the linker to preserve symbols

    echo "Generating dSYM file..."
    dsymutil MyApp.app/Contents/MacOS/main

    rm -rf obj

    echo "Build complete."

    # ./MyApp.app/Contents/MacOS/main
)

# OTHER SCRIPTS THAT OCCASIONALLY NEED TO BE RUN:

########################

# run this when shadercross is updated to fix Apple quarantine nonsense

# find SDL3_shadercross-3.0.0-darwin-arm64-x64 -type f -exec xattr -d com.apple.quarantine {} \; 2>/dev/null

########################

# SDL has started using .framework folders instead of .dylibs 
# to keep compiling as .dylibs, use the option to turn frameworks off 
# once all of the libraries (mixer, net) have precompiled official binaries released, I may switch. 
# But until then, I need to compile those helper libraries from source; 
# I can't get them to compile using the frameworks, but I can compile with the dylibs installed to usr/local

# check dependencies with otools:
# otool -L MyApp.app/Contents/Frameworks/libSDL3_ttf.0.dylib

# cmake -S . -B ./build -DSDL_SHARED=ON -DSDL3_FRAMEWORK=OFF -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"

# these dylibs end of being broken because they are still linked to the framework versions of SDL3,
# so I need to fix the install names and change the framework references to the dylibs

# cd MyApp.app/Contents/Frameworks

# install_name_tool -id @rpath/libSDL3.0.dylib libSDL3.0.dylib
# install_name_tool -change @rpath/SDL3.framework/Versions/A/SDL3 @rpath/libSDL3.0.dylib libSDL3_ttf.0.dylib
# install_name_tool -change @rpath/SDL3.framework/Versions/A/SDL3 @rpath/libSDL3.0.dylib libSDL3_image.0.dylib

# because I'm compiling SDL_ttf from source, I also need to copy over libfreetype and libpng,
# and fix their install names to point to the rpath, so that they can be found

# otool -L libSDL3_ttf.0.dylib

# cp /opt/homebrew/opt/freetype/lib/libfreetype.6.dylib .
# install_name_tool -id @rpath/libfreetype.6.dylib libfreetype.6.dylib
# install_name_tool -change /opt/homebrew/opt/freetype/lib/libfreetype.6.dylib @rpath/libfreetype.6.dylib libSDL3_ttf.0.dylib
# otool -L libfreetype.6.dylib

# cp /opt/homebrew/opt/libpng/lib/libpng16.16.dylib .
# install_name_tool -id @rpath/libpng16.16.dylib libpng16.16.dylib
# install_name_tool -change /opt/homebrew/opt/libpng/lib/libpng16.16.dylib @rpath/libpng16.16.dylib libfreetype.6.dylib
