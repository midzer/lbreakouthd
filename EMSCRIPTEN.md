* `emconfigure ./confgure CPPFLAGS='-flto -O3'`
* `emmake make`
* fix Makefile of libgame: command "ar" -> "emar", then remake it. Copy resulting `libgame.a` to `src`
* emcc -flto -O3 *.o libgame.a -o index.html -sUSE_SDL=2 -sUSE_SDL_MIXER=2 -sSDL2_MIXER_FORMATS='["wav"]' -sUSE_SDL_IMAGE=2 -sSDL2_IMAGE_FORMATS='["jpg","png"]' -sUSE_SDL_TTF=2 -sASYNCIFY -sALLOW_MEMORY_GROWTH -sASYNCIFY_STACK_SIZE=5120000 --preload-file levels --preload-file themes --closure 1 -Wl,-u,fileno
