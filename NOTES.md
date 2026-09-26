# Notes

A few notes to be kept safe.

- Use `gifsicle` to scale up and optimize gifs.
- Also take a look at LICEcap

# GCW Zero

- RAM speed allows for 1500 fps
[15:14:18] <MikeDX> is that sdl_flip() limits?
[15:14:35] <MikeDX> if you jus had a single loop with sdl_flip;frame++:
[15:14:59] <pcercuei> no, in that case you can go much faster
[15:15:18] <pcercuei> *if* you're using SDL_HWSURFACE without double-buffering
- http://www.gamedev.net/topic/426589-sdl_hwsurface/
