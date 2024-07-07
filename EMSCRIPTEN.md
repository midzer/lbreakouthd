# Emscripten

## Build

```
emmake make
```

## Link

```
em++ -flto -O3 -fno-exceptions *.o ../libgame/libgame.a -o index.html -sUSE_SDL=2 -sUSE_SDL_MIXER=2 -sSDL2_MIXER_FORMATS=wav -sUSE_SDL_IMAGE=2 -sSDL2_IMAGE_FORMATS='["jpg","png"]' -sUSE_SDL_TTF=2 -sASYNCIFY -sASYNCIFY_IGNORE_INDIRECT -sASYNCIFY_ONLY=@../funcs.txt -sASYNCIFY_STACK_SIZE=81920 -sINITIAL_MEMORY=64mb -sENVIRONMENT=web --preload-file levels/ --preload-file themes/ -Wl,-u,fileno --closure 1 -sEXPORTED_RUNTIME_METHODS=['allocate']
```
