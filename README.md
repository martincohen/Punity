# Punity

A tiny game framework inspired by HandmadeHero.org, Love2D and PICO-8 for making low resolution retro games. Framework is built around an idea to give the developer tools to build his own tools. It provides a foundation for making games and tools only using C.

The primary motivation was to get ready for porting [@x_rxi](https://twitter.com/x_rxi)'s engine to [SDL2](http://libsdl.org) to support [GCW Zero](http://gcw-zero.com) and mobile. But it all went a bit different way, as you can see.

I'm building it along a tiny game called [My Little Porny](martincohen.itch.io/my-little-porny).

# Features

Features are in the making, but for those worth mentioning at this point:

- Assets are compiled, optimized to binary and attached to main executable. The process is controlled via code.
- Built-in support for PNG, GIF, TGA, BMP, PSD through awesome [stb_image.h](https://github.com/nothings/stb/blob/master/stb_image.h). Details here.
- Windows now, but multiple platforms are in my heart: Linux, Mac OS X, Android, iOS, GCW Zero, Pandora (and anything else that is supported by SDL2).
- No build script dependency. There's just a tiny BAT file to run the compiler.
- Built-in GIF recorder (based on modifications to [jo_gif.c](http://www.jonolick.com/) by [@x_rxi](https://twitter.com/x_rxi))

Future is uncertain, but all obvious features will be added along with some ingenious ideas from the [HandmadeHero](http://handmadehero.org/) project.

# Limitations

To gain the right retro feel, I've built the framework around some sane limitations:

- Indexed palette with maximum of 255 colors (0 reserved for transparency).
- Made to be used with retro resolutions (think GameBoy).
- Tiles are indexed with 16-bit number.

# Dependencies

Apart for tiny libraries used are distributed with the source, there's currently a dependency on SDL2. The SDL2 is kept isolated in program.c, so to provide a new platform, you only need to provide platform's own program.c.

# Building

Currently only `MinGW` build script is provided.

1. Get *Punity*.
1. Get [SDL2](https://www.libsdl.org/download-2.0.php), unzip to `lib/SDL`.
2. Run `build.bat`.

Note: There's also a `Qt Creator` .pro file, as I'm now using it for the development, so that's an option to use as well (as opposed to using `build.bat`).

# Thank you

- [@x_rxi](https://twitter.com/x_rxi) for immense support and inspiration.
- [@cmuratori](https://twitter.com/cmuratori) for handmadehero.org
- [@d7samurai](https://twitter.com/d7samurai) for the name *Punity* and a name for the game I built it around *My Little Porny*.
- [@nothings](https://twitter.com/nothings) for [stb libraries](https://github.com/nothings/stb).