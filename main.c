#define PUNITY_IMPLEMENTATION
#include "punity.h"

typedef struct Game_
{
    Font font;
}
Game;

static Game *GAME = 0;

int
init()
{
    GAME = bank_push_t(CORE->storage, Game, 1);

    font_load_resource(&GAME->font, "font.png", 4, 7);
    CORE->canvas.font = &GAME->font;

    return 1;
}

void
step()
{
    canvas_clear(1);

    if (key_down(KEY_ESCAPE)) {
        CORE->running = 0;
    }
    
    char buf[256];
    sprintf(buf, "%.3fms", CORE->time_delta * 1e3);
    text_draw(buf, 0, 0, 2);

}