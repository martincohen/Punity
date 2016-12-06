# Punity

A tiny game engine with zero dependencies in C that I made for myself to make tiny games. It's great for game jams. It is also my kōhai course in simplicity and a tribute to my senpai and friend [rxi](https://twitter.com/x_rxi).

- Single file: `punity.h` all ready for you to start working. (maybe also grab `build.bat` when you're at it)
- Approx. 6000 lines of C code.
- No dependencies.
- Produces a single executable with all resources baked to it.
- Images in PNG, JPG, PSD, TGA, GIF, BMP, HDR, PIC, PNM when used with `stb_image`. (see **Integration** below)
- Sounds in OGG when used with `stb_vorbis`. (see **Integration** below)
- Drawing bitmaps, texts and standard primitives all in software, this is *feature*, shut up!
- Deferred drawing through draw lists.
- Sounds, fonts and bitmap API available.
- Built in GIF recorder (outputs `record.gif` file).
- Example included (see `example-platformer.c`).
- Experimental integration with SDL2 (build with `build sdl`, see `lib/punity-sdl.c`).

## Speed

On an average machine, a game [33 grams](https://martincohen.itch.io/33-grams) utilizing previous version of Punity runs in 240x240 at ~20μs per frame (equivalent of ~50000 FPS). On [GCW Zero](http://www.gcw-zero.com/specifications), the same game in 320x240 runs between 0.7ms to 2ms per frame (equivalent of ~500-1500 FPS).

## Limitations

- Perfect for use with 320x240 resolution. No one will ever need more, anyway.
- Perfect for simple pixel art with only 255 colors (0 reserved for transparency)!
- No need to worry about 8 bits of transparency, because you just get one!
- Optionaly limited to 30 frames per second that gives you them retro feels!
- Fuck other platforms, **Windows** is where it's at! (see **Planned features** for more platforms)

## Goals

- **Ready-to-go instant package.** No tedious external dependencies or fussing around with linkers: download, code and build from a single package.
- **Simplicity and minimalism.** Rather go for limitations than complexities.
- **Portability** Only get absolute minimum from the platform (window, input and drawing/audio buffers).
- **Optional additional features.** Plug & play addons for additional asset file formats, platforms or even scripting.
- **No arbitrary limitations.** All the limitations in the system have their reasons (performance, API simplicity). You might be writing a retro game, but the devices today have a lot of memory and computational power to use.
- **One file.** Even though it's supposed to be a ready-to-go package, the essence is in a single file that you can grab and use.

One of the most important parts of Punity is that it's made to be changed and edited as you need. That way the code can be really simple only giving you basic features and doesn't need to worry about customizations and abstractions.

## Planned features

**Note that the project is still in development.** More documentation, features, examples and fixes is underway.

- **More drawing functions** very soon (`draw_frame()`).
- **Even more drawing functions** a bit later (`draw_line()`, `draw_circle()`, etc.).
- **Tile maps** (with support of loading simple Tiled format and with customizable Tile elements).
- **SDL platform layer** in an optional separate file `punity_sdl.c` with additional pre-made build scripts (Windows, Linux, Mac OS X, iOS, Android, Raspberry Pi, [Dingux](http://wiki.dingoonity.org/index.php?title=Dingux:About), etc.). This would introduce a dependency on SDL2, though.

# Documenation

Please, see the [documentation in Wiki](https://github.com/martincohen/Punity/wiki).

## Files

- `mingw/_mingw_unicode.h` & `mingw/dsound.h` - Provided for your convenience to be able to build with default MinGW installation. This file is taken from MinGW-W64 project.
- `lib/stb_image.h` - Optional library to load images.
- `lib/stb_vorbis.c` - Optional library to load ogg audio files.
- `lib/gifw.h` - Optional library to record and save GIFs.
- `build.bat` - MSVC and MinGW build batch file.
- `config.h` - Example configuration file that customizes Punity. Can be switched off with using `#define PUN_CONFIG 0`
- `main.c` - Minimal application using Punity.
- `punity.h` - Punity's header/implementation file.

# Contributors

- [@d7samurai](https://twitter.com/d7samurai)
- [@x_rxi](https://twitter.com/x_rxi)

# Thank you

To everyone who gives Punity it a try, plus these awesome guys:
 
 - [@j_vanrijn](https://twitter.com/J_vanRijn) for his tall support 24/7.
 - [@cmuratori](https://twitter.com/cmuratori) for handmadehero.org
 - [@nothings](https://twitter.com/nothings) for [stb](https://github.com/nothings/stb).
