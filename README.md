# Punity

A tiny game framework inspired by [HandmadeHero.org](http://handmadehero.org), **rxi`s Cade** (codename; soon), [Love2D](http://love2d.org) and [PICO-8](http://www.lexaloffle.com/pico-8.php) for making low resolution retro games. Framework is built around an idea to give the developer tools to build his own tools. It provides a foundation for making games and tools only using C, but Lua bindings are also in the pipeline.

The primary motivation was to get ready for porting [@x_rxi](https://twitter.com/x_rxi)'s engine Cade (codename; soon) to [SDL2](http://libsdl.org) to support both mobile and [GCW Zero](http://gcw-zero.com). However, I liked it too much, so it's now a separate project.

The engine is build along with my tiny game called [My Little Porny](martincohen.itch.io/my-little-porny).

# Current Features

Features are in the making, but for those worth mentioning at this point:

- **Assets are pre-compiled and pre-optimized.** And all in binary and attached to main executable. The compilation/optimization process is controlled via code with several built-in default functions.
- **Built-in support for PNG, GIF, TGA, BMP, PSD through awesome [stb_image.h](https://github.com/nothings/stb/blob/master/stb_image.h).** To process your image assets.
- **Built-in support for reading Tiled's JSON format.** To process your tilemap assets.
- **Windows, Linux, Android and GCW Zero.** Plus other platforms are in the pipeline too: Mac OS X, iOS, Pandora (and anything else that is supported by SDL2, or supports SDL2).
- **No build script dependency.** There's just a tiny BAT/SH file to run the compiler. Of course you'd need SDKs for whatever target platforms you're targetting.
- **Built-in GIF recorder (on F9 key).** (based on modifications to [jo_gif.c](http://www.jonolick.com/) by [@x_rxi](https://twitter.com/x_rxi))

Future is uncertain, but there are plans for future (at least to a point of finishing [My Little Porny](martincohen.itch.io/my-little-porny)).

# Planned features

Just to keep a list somewhere:

- Audio engine inspired by retro music chip synthesis.
- Joystick(s)/Game controller support.
- Live reloading for game code or assets.
- Debugging console and API for it.

# Limitations

To gain the right retro feel, I've built the framework around some sane limitations:

- Indexed palette with maximum of 255 colors (0 reserved for transparency).
- Made to be used with retro resolutions (think GameBoy).
- Tiles are indexed with 16-bit number.

# Dependencies

Apart for tiny libraries used are distributed with the source, there's currently a dependency on SDL2. The SDL2 is kept isolated in punity\_main.c, so to provide a new platform, you only need to provide platform's own punity\_main.c.

# Building

Currently only `MinGW` build script is provided, as MSVC does not support C99 (although rumors are it's support it it's newest versions).

1. Get *Punity*.
1. Get [SDL2](https://www.libsdl.org/download-2.0.php), unzip to `lib/SDL`.
2. Run `build.bat` or `build.sh`.

Making a custom script for any other compiler is quite simple, just take a look at existing scripts.

Note: There's also a `Qt Creator` .pro file, as I'm now using it for the development, so that's an option to use as well (as opposed to using shell scripts).

Note: For building for GCW Zero, you also need to get [GCW Zero Toolchain](http://www.gcw-zero.com/develop).

# Thank you

- [@x_rxi](https://twitter.com/x_rxi) for immense support and inspiration.
- [@cmuratori](https://twitter.com/cmuratori) for handmadehero.org
- [@d7samurai](https://twitter.com/d7samurai) for the name *Punity* and a name for the game I built it around *My Little Porny*.
- [@nothings](https://twitter.com/nothings) for [stb libraries](https://github.com/nothings/stb).
- All guys from [gcw-zero.com](http://gcw-zero.com/) community at `#gcw` to help me with setting up the build.