# Punity

A tiny game engine with zero dependencies in C that I made for myself to make tiny games. It's great for game jams. It is also my kōhai course in simplicity and a tribute to my senpai and friend [rxi](https://twitter.com/x_rxi).

- Two files: `punity.c`, `punity.h` all ready for you to start working. (maybe also grab `build.bat` when you're at it)
- Approx. 1900 lines of C code.
- No dependencies.
- Produces a single executable with all resources baked to it.
- Images in PNG, JPG, PSD, TGA, GIF, BMP, HDR, PIC, PNM when used with `stb_image`. (see **Integration** below)
- Sounds in OGG when used with `stb_vorbis`. (see **Integration** below)
- Drawing bitmaps, texts and standard primitives all in software, this is *feature*, shut up!
- Deferred drawing through draw lists.
- Sounds, fonts and bitmap API available.

## Speed

On an average machine, a game [33 grams](https://martincohen.itch.io/33-grams) utilizing previous version of Punity runs in 240x240 at ~20μs per frame (equivalent of ~50000 FPS). On [GCW Zero](http://www.gcw-zero.com/specifications), the same game in 320x240 runs between 0.7ms to 2ms per frame (equivalent of ~500-1500 FPS).

## Limitations

- Perfect for use with 320x240 resolution. No one will ever need more, anyway.
- Perfect for simple pixel art with only 255 colors (0 reserved for transparency)!
- No need to worry about 8 bits of transparency, because you just get one!
- Limited to 30 frames per second that gives you them retro feels!
- Fuck other platforms, **Windows** is where it's at! (see **Planned features** for more platforms)

There is another project I'm working on called **Blander** which is also quite small game engine that allows a lot of things that *Punity* does not. It's more bland, though. It's much bigger than Punity, multiple files aaaaand it renders in hardware,... shut up!

## Goals

- **Ready-to-go instant package.** No tedious external dependencies or fussing around with linkers: download, code and build from a single package.
- **Simplicity and minimalism.** Rather go for limitations than complexities.
- **Portability** Only get absolute minimum from the platform (window, input and drawing/audio buffers).
- **Optional additional features.** Plug & play addons for additional asset file formats, platforms or even scripting.
- **No arbitrary limitations.** All the limitations in the system have their reasons (performance, API simplicity). You might be writing a retro game, but the devices today have a lot of memory and computational power to use.
- **Two files.** Even though it's supposed to be a ready-to-go package, the essence is in two files that you can grab and use.

## Planned features

**Note that the project is still in development.** More documentation, features, examples and fixes is underway.

- **More drawing functions** very soon (`draw_frame()`).
- **Even more drawing functions** a bit later (`draw_line()`, `draw_circle()`, etc.).
- **Tile maps** (with support of loading simple Tiled format and with customizable Tile elements).
- **GIF recorder** to record the screen to animated gifs. Optional, because it brings a dependency on rxi's modified version of [`jo_gif.c`](http://www.jonolick.com/home/gif-writer).
- **Draw lists** to be able to push drawing operations in arbitrary order, but to get them sorted by *z* coordinate before rendering.
- **Live asset reloading** in *debug* mode while keeping the same possibility of baking assets to executable as it has now in *release* mode.
- **SDL platform layer** in an optional separate file `punity_sdl.c` with additional pre-made build scripts (Windows, Linux, Mac OS X, iOS, Android, Raspberry Pi, [Dingux](http://wiki.dingoonity.org/index.php?title=Dingux:About), etc.). This would introduce a dependency on SDL2, though.
- **Optional integration with Mason** (my other project) that provides single-file build system for C and C++ to replace tedious `.bat`/`.sh` maintenance with awesomely beautiful Lisp dialect from [rxi](https://twitter.com/x_rxi).

# Usage

Build is **working with MSVC or MinGW**.

1. [Download](https://github.com/martincohen/Punity/archive/master.zip).
2. Edit `main.c`.
3. Run `build`.

Build script is setup to compile `main.c` as single-translation unit, so if you need another C files, include them in `main.c` directly (same as I've include `punity.c` there). If you need something else, modify `build.bat` to you likings.

You can customize some aspects of **Punity** by changing macros in `config.h`, which is included automatically from `punity.h`. If you don't want this, use `#define PUN_CONFIG 0` and specify the macros in any way you feel is better for you.

To use your own building script and project structure, just grab `punity.h` and `punity.c` and use them as you see fit.

Punity defines the `main()` function for you, but it gives you two other functions for you to implement as compensation:

- `init()` is called to initialize your own data, load assets, etc.
- `step()` that is called every frame; you update and draw here.

## Core

Punity also gives you a pointer to `CORE` (see `Core` struct in `punity.h`) to access canvas, inputs, timers, audio and memory.

The `CORE` is used also within the Punity's functions to make it a bit easier on passing arguments. For example, `text_draw()` expects a font to be set in `CORE->canvas.font`. 

## Memory

Punity provides two fixed-size basic memory banks:

- `CORE->stack` is used to store larger amounts of temporary memory. Typically it's used store dynamic in-function buffers that are thrown away when the function ends.
- `CORE->storage` is used for long term memory (like bitmaps, tilemaps, audio, etc.)

This way, no complex memory management is needed at all. Taking memory from the banks is just a matter of one pointer change (and a few optional assertions for you convenience). Should you need more memory, just change the `PUN_STACK_CAPACITY` or `PUN_STORAGE_CAPACITY` macros in `config.h` to use more memory.

Remember, you always know in advance how many entities, assets and buffers, so generally there's no need to use dynamic memory management.

Get `BankState` with `bank_begin()` to store current state of the bank. This is useful if you want to do more `bank_push()` calls and free the whole memory with just one call to `bank_end()`.

```c
BankState bank_state = bank_begin(CORE->stack);

f32 *channel0 = bank_push(CORE->stack, samples_count * sizeof(f32));
f32 *channel1 = bank_push(CORE->stack, samples_count * sizeof(f32));

// TODO: Do nasty things to `channel0` and `channel1`.

bank_end(&bank_state);
```

For simpler allocations, you can just use `bank_push()` and `bank_pop()`:

```c
u8 *canvas = bank_push(CORE->stack, CANVAS_WIDTH * CANVAS_HEIGHT);

// TODO: Draw naked women on `canvas`.
// TODO: Blend naked women in `canvas` to the `CORE->canvas`.

bank_pop(CORE->stack, canvas);
```

Asset loading functions like `bitmap_load()` store the bitmap data to `CORE->storage` memory bank automatically. You are free to change pointer to `CORE->storage` to any other bank you have. For example it might be handy to keep asset and state memory separate, so you can write and load them to file system separately.

## Assets

For tilemaps, images, audio, or other additional files, you can either choose to access the files directly, or to pack them with the executable. To do that, take a peek at `main.rc`. It looks like this:

```
Format:
<ResourceIdentifier> RESOURCE "<ResourcePath>"

Example:
font.png RESOURCE "res\\font.png"
```

Please, make sure you keep the first line `icon.ico ICON "res\\icon.ico"` in there, so your application and the main window will have a nice icon (that you can customize too!).

In the code, I usually define a `Game` struct that holds all the state and asset data like this:

```c
typedef struct
{
    Font font;
    Bitmap background;
    // ---
}
Game;

static Game game;

void init() {
    // To load from a file.
    bitmap_load(&game.font.bitmap, "res/font.png");
    // To load from resource.
    bitmap_load_resource(&game.font.bitmap, "font.png");

    // To use the font.
    CORE->canvas.font = &game.font;
}

// See the main.c for more examples.
```

## Drawing

To customize size and scale of the canvas, change `PUN_CANVAS_WIDTH`, `PUN_CANVAS_HEIGHT` and `PUN_CANVAS_SCALE` macros in `config.h`.

`CORE->canvas.bitmap` is `Bitmap` struct that allows you to access the frame buffer at any time. You can either change the buffer manually, or use Punity's functions. See `punity.h` for detailed information.

- `rect_draw()` - to draw a filled rectangle.
- `frame_draw()` - to draw just an frame of a rectangle.
- `bitmap_draw()` - to draw whole or a piece of bitmap.
- `text_draw()` - to draw text using `CORE->canvas.font`.

These functions use painter's algorithm (last drawn is on top), however you can use drawing list to draw in any order you want.

### Drawing lists

Functions `draw_list_` add support for deferring and sorting drawing operations by z-key.

```c
DrawList list;
// Init with space for 128 drawing operations.
draw_list_init(&list, 128);

while (running)
{
    draw_list_begin(&list);
    // Call `*_push` drawing functions such as `rect_draw_push(...)`.
    draw_list_end(&list);
}
```

Punity automatically initializes, begins and ends a default drawing list available in `CORE->draw_list`. It's initial reserve parameter can be customized via `PUN_DRAW_LIST_RESERVE` macro (number of items). So you can directly call `*_push` drawing functions without worrying about it too much.

The push functions automatically draw to the `CORE->draw_list` so you don't have to pass it each time. Their signature is the same as their non-push counterparts, only a z-key is needed as last parameter.

In case you need it, you can call `draw_list_end` earlier (in your `step` function), but make sure you call `draw_list_begin` if you need to resume drawing to the list in the same frame (Punity calls `draw_list_begin` itself before the `step` function is called).

The memory needed for the drawing list is taken from the `CORE->stack` bank, so make sure there's enough space, or just adjust it in case it'll fail during the busiest frames. Maximum number of items drawn is not limited to the initial reserve parameter, Punity increases the space for the next frame in case more space is needed (up to the limits of `CORE->stack` bank). This behavior might change in the future. For more details see `draw_list_push_` and `draw_list_begin` source code.

## Audio

Loading the sounds is currently only supported when `PUN_USE_STB_VORBIS` is enabled and the `stb_vorbis.c` is available. The API currently allows for loading and playing a sound (there's no pause, nor stop, nor rewind). Example:

```c
Sound track, explosion;
sound_load_resource(&track, "track.ogg");
sound_load_resource(&explosion, "explosion.ogg");
// Enable looping of the background track.
track.flags |= SoundFlag_Loop;

// Loops forever.
sound_play(&track); 
// One shot.
sound_play(&explosion);
```

## Integration

Punity is prepared for use with [stb_image](https://github.com/nothings/stb/blob/master/stb_image.h) or [stb_vorbis](https://github.com/nothings/stb/blob/master/stb_vorbis.c) to load images and audio files. The versions that I'm using are available in `lib/` directory.

- In `config.h` enable `PUN_USE_STB_IMAGE` macro. It'll allow you to use `bitmap_load` and `bitmap_load_resource` to load PNG, GIS, PSD and more.
- In `config.h` enable `PUN_USE_STB_VORBIS` macro. It'll allow you to use `sound_load` and `sound_load_resource` to load or stream OGG files.

## Debugging

You can run `devenv bin\main.exe` in case you're running from *Visual Studio Command Prompt* (or with `vcvarsall.bat` environment) to debug with Visual Studio. Note that you'll need to build with `build debug` to be able to debug.

## Building

- `build` - Runs build with defaults (debug version of MSVC).
- `build debug|release` - Runs build with MSVC, so you'll need to run `vcvarsall.bat` or Visual Studio Command Prompt.
- `build debug|release gcc` - Runs build with GCC (note that you need MinGW-W64 to compile successfully).

## Files

- `mingw/_mingw_unicode.h` & `mingw/dsound.h` - Provided for your convenience to be able to build with default MinGW installation. This file is taken from MinGW-W64 project.
- `lib/stb_image.h` - Optional library to load images.
- `lib/stb_vorbis.c` - Optional library to load ogg audio files.
- `build.bat` - MSVC and MinGW build batch file.
- `config.h` - Example configuration file that customizes Punity. Can be switched off with using `#define PUN_CONFIG 0`
- `main.c` - Example application using Punity.
- `punity.h` - Punity's header file.
- `punity.c` - Punity's source file.

# TODO

A list of tasks I keep with important changes planned to appear in upcoming releases.

- Tilemaps.
- `draw_frame()`
- `draw_circle()`
- `draw_line()`
- Limit the number of audio voices.
- Palette rotations, shifting, etc.
- Experiment with a reasonable replacement of `StretchDIBits` to gain more performance.
- Animated GIF recording. Will need to solve the previous issues with it being too slow. Probably first record the frames to memory, then process the gif in a separate thread.

# Outstanding (involuntary) contributions

- [@d7samurai](https://twitter.com/d7samurai) - I use a lot of his ideas and pieces of code reworked from C++ to C.

# Thank you
 
 - [@x_rxi](https://twitter.com/x_rxi) for immense support and inspiration.
 - [@d7samurai](https://twitter.com/d7samurai) for the name *Punity* and a lot of wisdom donations.
 - [@j_vanrijn](https://twitter.com/J_vanRijn) for his tall support 24/7.
 - [@cmuratori](https://twitter.com/cmuratori) for handmadehero.org
 - [@nothings](https://twitter.com/nothings) for [stb](https://github.com/nothings/stb).