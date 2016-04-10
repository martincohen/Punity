#include "punity.c"

static Font font;
static Sound sound1;
static Sound sound2;

#define COLOR_BLACK (1)
#define COLOR_WHITE (2)

void
init()
{
    CORE->palette.colors[0] = color_make(0x00, 0x00, 0x00, 0x00);
    CORE->palette.colors[1] = color_make(0x00, 0x00, 0x00, 0xff);
    CORE->palette.colors[2] = color_make(0xff, 0xff, 0xff, 0xff);
    CORE->palette.colors[3] = color_make(0x63, 0x9b, 0xff, 0xff);
    CORE->palette.colors_count = 4;
    canvas_clear(1);

    bitmap_load_resource(&font.bitmap, "font.png");
    font.char_width = 4;
    font.char_height = 7;

    sound_load_resource(&sound1, "sound1.ogg");
	sound_load_resource(&sound2, "sound2.ogg");
	sound2.rate = 48000;

    CORE->font = &font;
}

void
step()
{
	if (CORE->key_deltas[KEY_SPACE] && CORE->key_states[KEY_SPACE] == 1) {
		// sound_play(&sound1);
		sound_play(&sound2);
	}

	if (CORE->key_states[KEY_RIGHT]) {
		CORE->translate_x++;
	}

	if (CORE->key_states[KEY_LEFT]) {
		CORE->translate_x--;
	}

	if (CORE->key_states[KEY_DOWN]) {
		CORE->translate_y++;
	}

	if (CORE->key_states[KEY_UP]) {
		CORE->translate_y--;
	}

	if (CORE->key_states[KEY_ESCAPE]) {
		CORE->running = 0;
	}

    canvas_clear(0);
    // Rect r = rect_make_size(0, 0, 16, 16);
    // bitmap_draw(0, 0, 0, 0, &splash, &r, 0, 0);

    text_draw(0, 0, "\n\nHELLO\nWORLD", 3);

    rect_draw(rect_make_size(32, 32, 8, 8), 2);

    static char buf[256];
    sprintf(buf, "%03f %05d", CORE->perf_blit_gdi.delta, (i32)CORE->frame);
    text_draw(0, 0, buf, 2);
}