#include "punity.c"

static Bitmap splash;
static Font font;
static Sound sound1;
static Sound sound2;

typedef enum {
	PC_TRANSPARENT,
	PC_BLACK,
	PC_WHITE,
	PC_SKY_BLUE,

	PC_COUNT,
} PaletteColor;

void
init(void)
{
    CORE->palette.colors[PC_TRANSPARENT] = color_make(0x00, 0x00, 0x00, 0x00);
    CORE->palette.colors[PC_BLACK]       = color_make(0x00, 0x00, 0x00, 0xff);
    CORE->palette.colors[PC_WHITE]       = color_make(0xff, 0xff, 0xff, 0xff);
    CORE->palette.colors[PC_SKY_BLUE]    = color_make(0x63, 0x9b, 0xff, 0xff);

    CORE->palette.colors_count = PC_COUNT;
    canvas_clear(1);

	bitmap_load_resource(&splash, "splash.png");

    bitmap_load_resource(&font.bitmap, "font.png");
    font.char_width = 4;
    font.char_height = 7;

    sound_load_resource(&sound1, "sound1.ogg");
	sound_load_resource(&sound2, "sound2.ogg");
	// sound2.rate = 48000;

    CORE->font = &font;
}

void
step(void)
{
	if (key_pressed(KEY_SPACE)) {
		sound_play(&sound1);
	}

	if (key_down(KEY_RIGHT)) {
		CORE->translate_x++;
	}

	if (key_down(KEY_LEFT)) {
		CORE->translate_x--;
	}

	if (key_down(KEY_DOWN)) {
		CORE->translate_y++;
	}

	if (key_down(KEY_UP)) {
		CORE->translate_y--;
	}

	if (key_pressed(KEY_ESCAPE)) {
		CORE->running = 0;
	}


    canvas_clear(0);

    bitmap_draw(0, 0, 0, 0, &splash, 0, 0, 0);

    text_draw(0, 0, "\n\nHELLO\nWORLD", 3);

    rect_draw(rect_make_size(32, 32, 8, 8), 2);

    {
	    static char buf[256];
	    sprintf(buf, "%03f %05d", CORE->perf_step.delta, (i32)CORE->frame);
	    text_draw(0, 0, buf, 2);
	}

	CORE->canvas->pixels[0] = PC_WHITE;
}
