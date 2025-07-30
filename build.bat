@echo off
setlocal enabledelayedexpansion
(
    call compile_shaders.bat
    clang -c src/*.c -I/include -m64
    clang *.o -o main.exe -I/include -lSDL3 -lSDL3_ttf -lSDL3_image -lSDL3_mixer -L/lib -m64 "-Wl,/SUBSYSTEM:WINDOWS,/ENTRY:mainCRTStartup"
    del *.o
    main.exe
)