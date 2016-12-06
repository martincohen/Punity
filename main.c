#define PUNITY_IMPLEMENTATION
#include "punity.h"

typedef struct Game_
{
    Font font;
}
Game;

static Game *GAME = 0;

// Called first to configure the window.
//
int
configure(CoreConfig *config)
{
    config->canvas_width  = 256;
    config->canvas_height = 256;
    config->canvas_scale  = 2;

    return 1;
}

// Called second to initialize the game.
//
int
init()
{
    // Push our Game struct to the storage bank.
    GAME = bank_push_t(CORE->storage, Game, 1);

    // Load and setup font.
    font_load_resource(&GAME->font, "font.png", 4, 7);
    CORE->canvas.font = &GAME->font;

    return 1;
}

// Called every frame to update & draw.
//
void
step()
{
    // Clear the canvas.
    canvas_clear(1);
    // Draw text.
    text_draw("Hello world!", 16, 16, 2);
}