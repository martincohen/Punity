# Punity

A tiny game engine with zero dependencies in C that I made for myself to make tiny games. It's great for game jams. It is also my kōhai course in simplicity and a tribute to my senpai and friend [rxi](https://twitter.com/x_rxi).

- Three simple files: `punity.c`, `punity.h` and `build.bat` all ready for you to start working.
- Approx. 1500 lines of C code.
- Produces a single executable with all resources baked to it.
- Images PNG, JPG, PSD, TGA, GIF, BMP, HDR, PIC, PNM when used with `stb_image`. (see **Integration** below)
- Images OGG when used with `stb_vorbis`. (see **Integration** below)
- Renders all in software, this is *feature*, shut up!
- Sounds, fonts and bitmap API available.

## Limitations

- Perfect for use with 128x128. Not for full HD games!
- Perfect for simple pixel art with only 255 colors (0 reserved for transparency)!
- No need to worry about 8 bits of transparency, because you just get one!

There is another project I'm working on called **Blander** which is aslo a minimal game engine that allows a lot of things that *Punity* does not. It's more bland, though: it renders in hardware,... shut up!

## Planned features

**Note that the project is still in development.** By default only supports Windows, but more platforms will be possible in the future (probably first with a separate platform file for SDL2). More documentation, features, examples and fixes is underway.

- More drawing functions very soon (`draw_rect()`, `draw_frame()`).
- Even more drawing functions a bit later (`draw_line()`, `draw_circle()`, etc.).
- Live asset reloading in **debug** mode while keeping the same possibility of baking assets to executable as it has now in **release** mode.

# Usage

Build is **compatible with MSVC**. **MinGW** is also available, though you'll need MinGW-W64 or TDM because of some Windows headers (notably `dsound.h`). I'll take a look at what can be done to avoid this in the future.

1. [Download Punity](https://github.com/martincohen/Punity/archive/master.zip)
2. Customize `src/main.c` and the example assets.
3. Run `build debug` from command line to compile.

Build script is setup to compile `src/main.c` as single-translation unit, so if you need another C files, include them in `src/main.c` directly (same as I've include `punity.c` there). If you need something else, modify `build.bat` to you likings.

You can customize some aspects of **Punity** by changing macros in `config.h`.

## Memory

You can either choose to use `malloc()` to manage your memory, or you can use Punity's own facilities. Punity provides two fixed-size basic memory banks:

**CORE->stack** is used to store larger amounts of temporary memory. Typically it's used store dynamic in-function buffers that are thrown away when the function ends.

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

bank_pop(CORE->stack, canvas);
```

## Assets

For tilemaps, images, audio, or other additional files, you can either choose to access the files directly, or to pack them with the executable. To do that, take a peek at `src/main.rc`. It looks like this:

```
Format:
<ResourceIdentifier> RESOURCE "<ResourcePath>"

Example:
font.png RESOURCE "src\\font.png"
```

Please, make sure you keep the first line `icon.ico ICON "src\\icon.ico"` in there, so your application and the main window will have a nice icon (that you can customize too!).

Then, in the code you load `font.png` like this:

```
Bitmap bitmap;
bitmap_load_resource(&font_bitmap, "font.png")`
```

Punity is prepared for use with [stb_image](https://github.com/nothings/stb/blob/master/stb_image.h) or [stb_vorbis](https://github.com/nothings/stb/blob/master/stb_vorbis.c) to load images and audio files. The versions that I'm using are available in `src/` directory.

- In `config.h` enable `USE_STB_IMAGE` macro. It'll allow you to use `bitmap_load` and `bitmap_load_resource` to load PNG, GIS, PSD and more.
- In `config.h` enable `USE_STB_VORBIS` macro. It'll allow you to use `sound_load` and `sound_load_resource` to load or stream OGG files.

## Debugging

You can run `devenv bin\main.exe` in case you're running from *Visual Studio Command Prompt* (or with `vcvarsall.bat` environment) to debug with Visual Studio. Note that you'll need to build with `build debug` to be able to debug.

## Building

- `build debug|release` - Runs build with MSVC, so you'll need to run `vcvarsall.bat` or Visual Studio Command Prompt.
- `build debug|release gcc` - Runs build with GCC (note that you need MinGW-W64 to compile successfully).

# Thank you
 
 - [@x_rxi](https://twitter.com/x_rxi) for immense support and inspiration.
 - [@d7samurai](https://twitter.com/d7samurai) for the name *Punity* and a lot of wisdom donations.
 - [@j_vanrijn](https://twitter.com/J_vanRijn) for his tall support 24/7.
 - [@cmuratori](https://twitter.com/cmuratori) for handmadehero.org
 - [@nothings](https://twitter.com/nothings) for [stb](https://github.com/nothings/stb).