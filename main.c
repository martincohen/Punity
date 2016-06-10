#include "punity.c"

static Bitmap splash;
static Font font;
static Sound sound1;
static Sound sound2;

#define COLOR_BLACK (1)
#define COLOR_WHITE (2)

void
init()
{
    CORE->canvas.palette.colors[0] = color_make(0x00, 0x00, 0x00, 0x00);
    CORE->canvas.palette.colors[1] = color_make(0x00, 0x00, 0x00, 0xff);
    CORE->canvas.palette.colors[2] = color_make(0xff, 0xff, 0xff, 0xff);
    CORE->canvas.palette.colors[3] = color_make(0x63, 0x9b, 0xff, 0xff);

    CORE->canvas.palette.colors_count = 4;
    canvas_clear(1);

	bitmap_load_resource(&splash, "splash.png");

    bitmap_load_resource(&font.bitmap, "font.png");
    font.char_width = 4;
    font.char_height = 7;

    sound_load_resource(&sound1, "sound1.ogg");
	sound_load_resource(&sound2, "sound2.ogg");

    CORE->canvas.font = &font;
}

void
step()
{
	if (key_pressed(KEY_SPACE)) {
		sound_play(&sound1);
	}

	if (key_down(KEY_RIGHT)) {
		CORE->canvas.translate_x++;
	}

	if (key_down(KEY_LEFT)) {
		CORE->canvas.translate_x--;
	}

	if (key_down(KEY_DOWN)) {
		CORE->canvas.translate_y++;
	}

	if (key_down(KEY_UP)) {
		CORE->canvas.translate_y--;
	}

	if (key_pressed(KEY_ESCAPE)) {
		CORE->running = 0;
	}


    canvas_clear(0);

    bitmap_draw(&splash, 0, 0, 0, 0, 0, 0, 0);
    text_draw("\n\nHELLO\nWORLD", 0, 0, 3);
    rect_draw(rect_make_size(32, 32, 8, 8), 2);

    static char buf[256];
    sprintf(buf, "%03f %05d", CORE->perf_step.delta, (i32)CORE->frame);
    text_draw(buf, 0, 0, 2);

	CORE->canvas.bitmap->pixels[0] = COLOR_WHITE;
}