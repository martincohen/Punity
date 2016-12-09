# Version 2.2

- Added tiled format support (still experimental; see example-tiled).
- Added `Tile` and `TileMap` structs to join Tiled format and collision detection in scene.
- Removed tilemap from scene.
- OpenGL canvas now centers canvas correctly in fullscreen.
- Getting system keys now works (Alt+F4 is still reserved for system).
- Fixed MinGW/GCC support.
- Added `sound_stop` to stop all instances of playing sound.
- Changed signature of `tile_draw` (index is now second argument).
- Added support for Visual Studio 2013 (build with 64-bit version of compiler).

# Version 2.1

- Major cleanup of stuff not essential, or rarely used, or just plain stupid.
- Removed `file_*` functions.
- Removed `string_*` functions.
- Removed `V2f` and `V2`, as those are not essential.
- Removed `perf_from()`, `perf_to()`, `perf_delta()`
- Removed `Font` struct (superseded by Bitmap as it now stores tile_width and tile_height).

# Version 2.0

- Switched to single file `punity.h`.
- Removed the need for `config.h`.
- Added GIF recording.
  - Depends on `lib/gifw.h`.
  - Forced to 30fps. (even if Punity runs on 60fps)
  - Call `record_begin()` to start recording.
  - Call `record_end()` to end recording and save the recording to `record.gif`.
    - Saving the `gif` might take longer time so the frame in which the record_end() is called
      will take longer than 30 or 60fps (depending on your settings).
- Added SDL runtime. (still work-in-progress)
  - Depends on `lib/punity-sdl.c`.
  - `SDL2.dll` has to be distributed with the executable.
  - Build with `build sdl`
   - Reorganized `main()`, better platform code separation.
- Added entity and collision system.
  - Customizable `SceneEntity` struct with `PUN_SCENE_ENTITY_CUSTOM`.
  - Uses `SpatialHash` for entities.
  - Uses fixed tile map for static colliders.
    - Much faster than adding each tile to `SpatialHash`.
  - Integer-only without need to fiddle with float epsilons.
  - Uses `Deque` to store "unlimited amount" of entities.
  - Use `entity_add` / `entity_remove` to add or remove entities.
  - Use `entity_move_x`, `entity_move_y` to move your entities.
- Reworked `build.bat`.
   - Now able to build different targets than `main.c` (i.e. build examples/example-platformer)
   - Arguments can be in any order.
- Added PUNITY_OPENGL_30FPS to disable forcing framerate to 30fps.
- Removed DirectX support (moved to a separate module that will be optional in the future).
  - This change has been made to keep the runtime layer in `punity.h` file as simple as possible.
- Changed `init` signature to return `int` to be able to exit in case of error.
- Changed `Bank` now reserves given capacity and commits when needed.
- Changed `file_read` now takes optional `bank` pointer to store the file, if it's 0 then it'll use malloc()
- Changed COLOR_CHANNELS to PUNITY_COLOR_CHANNELS
- Added `configure` callback now added to do runtime configuration for some options (see CoreConfig)
- Added `window_title(title)` to set the title of the window dynamically.
- Added `window_fullscreen_toggle()` to maximize the window to fullscreen and back.
- Added `Deque` for dynamic allocations.
- Added `pixel_draw`
- Added `line_draw`
- Added `color_set` to set color at given index in the palette.
- Added `frame_draw` to draw rectangle outlines.
- Added `CORE->time`.
- Moved `color_lerp` is now in header file.
- Added `CORE->key_text` and `CORE->key_text_length` to be able to work with text input.
- Fixed bug with drawing bitmaps higher than the screen.
- Removed `PUN_COLOR_BLACK` and `PUN_COLOR_WHITE`.

# Version 1.6

- Version actually used for Ludum Dare and Low-Rez jams.
- "Cherchez" has been made with this version: https://martincohen.itch.io/cherchez
- All the configuration macros are now prefixed with PUN_
- Fixed bug in sound mixing.
- All rendering properties are now stored in dedicated `Canvas` struct.
- Added simple randomization functions (see the documentation).
- Added `file_write` function to accompany `file_read`.
- Added basic `V2f` struct and functions.
- Added vertical flipping for `bitmap_draw` function.
- Reworked `bitmap_draw` function.
- Added drawing list support (see the documentation).
- Added `*_push` functions for pushing draw operations to draw list (see the documentation)

# Version 1.5

- Renamed configuration macros to use PUN_* prefix
- PUN_MAIN can now be used to not define the main entry function.
- Forced PUN_COLOR_BLACK to 1 and PUN_COLOR_WHITE to 2.
- Changed bitmap_draw signature to have Bitmap* at the beginning.
- Changed text_draw signature to have const char* at the beginning.
- Translation direction changed in bitmap_draw() and text_draw().

# Version 1.4

- Fixed audio clipping problems by providing a soft-clip.
- Added master and per-sound volume controls.

# Version 1.3

- Fixed RGB in bitmap loader on Windows.

# Version 1.2

- Replaced usage of `_` prefixes with `PUNP_` prefixes to not violate reserved identifiers.

# Version 1.1

- Fixed timing issues and frame counting.
- Fixed KEY_* constants being not correct for Windows platform.
- Added draw_rect().

# Version 1.0

- Initial release

