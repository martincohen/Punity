#include "punity.c"
#include <dyad.c>
#include <ircclient.c>

static Font font;

#define COLOR_BLACK PUN_COLOR_BLACK
#define COLOR_WHITE PUN_COLOR_WHITE
#define COLOR_BLUE  3
#define COLOR_GREEN 4
#define COLOR_GRAY  5

#define SHELL_PADDING_LEFT (24)
#define SHELL_PADDING_TOP  (16)

typedef struct Shell_ Shell;

#define SHELL_HANDLER(name) void name(Shell *shell)
typedef SHELL_HANDLER(ShellHandlerF);

typedef struct ShellView_
{
    Shell *shell;

    bool visible;

    int x;
    int y;
    int cols;
    int rows;

    TextAttr *attr;
    char *text;

    char input[256];
    int input_length;
    int input_caret;
    int input_length_last;

    bool caret_show;
    int caret_frame;
    int caret_x;
    int caret_y;
	u8 caret_color;

    int scroll_y;
}
ShellView;

typedef struct Shell_
{
    // int width;
    // int height;
    // int cols;
    // int rows;

    int handler_caret_x;
    int handler_caret_y;
    size_t handler_frame;
    ShellHandlerF *handler_;

    ShellView *views[256];
    int views_count;
}
Shell;

SHELL_HANDLER(shell_prompt);
SHELL_HANDLER(shell_command);

void
shell_init(Shell *shell)
{
    // int width  = PUN_CANVAS_WIDTH  - (SHELL_PADDING_LEFT * 2);
    // int height = PUN_CANVAS_HEIGHT - (SHELL_PADDING_TOP * 2);
    // int cols = width  / CORE->canvas.font->char_width;
    // int rows = height / CORE->canvas.font->char_height;

    // shell->width = width;
    // shell->height = height;
    // shell->cols = cols;
    // shell->rows = rows;

    memset(shell, 0, sizeof(Shell));
    shell->handler_ = shell_prompt;
}

ShellView *
shell_view_add(Shell *shell, int x, int y, int cols, int rows)
{
    ASSERT(shell->views_count != array_count(shell->views));

    size_t cells = rows * cols;
    size_t size = sizeof(ShellView) +
         (cells * sizeof(TextAttr)) +
         (cells * sizeof(char));

    ShellView *view = (ShellView*)malloc(size);
    memset(view, 0, sizeof(ShellView));

    shell->views[shell->views_count] = view;
    shell->views_count++;

    view->shell = shell;
    view->x = x;
    view->y = y;
    view->cols = cols;
    view->rows = rows;
	view->caret_color = COLOR_WHITE;

    view->attr = (TextAttr*)(view + 1);
    TextAttr *attr = view->attr;
    for (int i = 0; i != cells; ++i, ++attr) {
        attr->fg = PUN_COLOR_WHITE;
        attr->bg = PUN_COLOR_BLACK;
    }

    view->text = (char*)(view->attr + cells);
    memset(view->text, 0, cells * sizeof(char));

    return view;
}

int
shell_view_offset(ShellView *view, int x, int y)
{
    x = clamp(x, 0, view->cols);
    y = clamp(y, 0, view->rows);
    return (x) + (y * view->cols);
}

void
shell_draw(Shell *shell)
{
    ShellView *view;
    Font *font = CORE->canvas.font;
    int offset_x, offset_y;
    for (int i = 0; i != shell->views_count; ++i)
    {
        view = shell->views[i];
		if (view->visible)
		{
			offset_x = view->x * font->char_width;
			offset_y = view->y * font->char_height;

			TextAttr *row_attr = view->attr;
			char *row_text = view->text;
			for (int y = 0; y != view->rows; ++y)
			{
				text_draw_attr(row_text, offset_x, offset_y + (y * font->char_height), row_attr, view->cols);
				row_text += view->cols;
				row_attr += view->cols;
			}
		}
    }
}

void
shell_view_clear(ShellView *view, int count, TextAttr attr)
{
    if (count == 0) {
        return;
    }

    Shell *shell = view->shell;
    int x = view->caret_x;
    int y = view->caret_y - view->scroll_y;
    int offset = shell_view_offset(view, 0, y);

    char *text_it = view->text + offset;
    TextAttr *attr_it = view->attr + offset;
    for(; y < view->rows; ++y)
    {
        memset(text_it + x, 0, view->cols - x);
        for(; x < view->cols; ++x) {
            attr_it[x] = attr;
        }
        attr_it += view->cols;
        text_it += view->cols;
        x = 0;
    }
}

void
shell_new_line(ShellView *view, TextAttr attr)
{
    view->caret_x = 0;
    view->caret_y++;

    int view_y = view->caret_y - view->scroll_y;
    if (view_y >= view->rows)
    {
        char *row_text = view->text;
        TextAttr *row_attr = view->attr;
		for (int y = 1; y != view->rows; ++y)
        {
            memmove(row_text,
                row_text + view->cols,
                view->cols);
            memmove(row_attr,
                row_attr + view->cols,
                view->cols * sizeof(TextAttr));

			row_text += view->cols;
			row_attr += view->cols;
		}

        view->scroll_y++;
    }
	shell_view_clear(view, view->cols, attr);
}

void
shell_print(ShellView *view, const char *text, int length, TextAttr attr)
{
	if (text == 0 ||  *text == 0) {
		return;
	}

    if (length == -1) {
        length = strlen(text);
    }

    char *line = (char*)text;
    int offset, size;
    for (;;)
    {
        if (*text == '\n' || length == 0)
        {
            while (text - line)
            {
                size = min(view->cols - view->caret_x, text - line);
                // TODO: Scan back on line to find a nearest good wrap.
                offset = shell_view_offset(view,
                        view->caret_x,
                        view->caret_y - view->scroll_y);
                memcpy(view->text + offset, line, size);
                for (int i = 0; i != size; ++i) {
                    view->attr[offset + i] = attr;
                }

                line += size;

                view->caret_x += size;
                if (view->caret_x == view->cols && line) {
                    shell_new_line(view, attr);
                }
            }
            
            if (*text == '\n') {
                shell_new_line(view, attr);
                line = (char*)text + 1;
            }

            if (length == 0) {
                break;
            }
        }
        text++;
        length--;
    }
}

void
shell_view_input_blit(ShellView *V, int x, int y, TextAttr attr)
{
    Shell *S = V->shell;
    V->caret_x = x;
    V->caret_y = y;
    if (V->input_length_last > V->input_length) {
       shell_view_clear(V, V->input_length_last, attr);
    }
    shell_print(V, V->input, V->input_length, attr);
    V->input_length_last = V->input_length;
}

int
shell_view_input_insert(ShellView *view, char *text, size_t text_length)
{
    text_length = minimum(text_length, array_count(view->input) - view->input_length);
    if (text_length == 0) {
        return 0;
    }
    
    if (view->input_caret < view->input_length)
    {
        // .....
        //   ^^^vvv
        // ..xxx...
        memmove(view->input + view->input_caret + text_length,
            view->input + view->input_caret,
            view->input_length - view->input_caret);
    }

    memcpy(view->input + view->input_caret, text, text_length);
    view->input_length += text_length;
    view->input_caret += text_length;

    return 1;
}

int
shell_view_input_remove(ShellView *view, int length)
{
    ASSERT(view->input_caret >= 0);
    
    if (length == 0) {
        return 0;
    } else if (length < 0) {
        length = -length;
        view->input_caret -= length;
    }

    if (view->input_caret < 0) {
        view->input_caret = 0;
        return 0;
    }

    // ..xx..
    //   vv^^
    // ....
    memmove(view->input + view->input_caret,
        view->input + view->input_caret + length,
        view->input_length - view->input_caret - length);
    
    view->input_length -= length;

    return 1;
}

int
shell_view_input(ShellView *view, int x, int y, TextAttr attr)
{
    int ret = 0;

    if (CORE->key_text_length) {
        ret = shell_view_input_insert(view, CORE->key_text, CORE->key_text_length);
    }

    if (key_pressed(KEY_RIGHT)) {
        view->input_caret = minimum(view->input_caret + 1, view->input_length);
    }

    if (key_pressed(KEY_LEFT)) {
        view->input_caret = maximum(view->input_caret - 1, 0);
    }

    if (key_pressed(KEY_BACKSPACE)) {
        if (view->input_caret != 0) {
            ret |= shell_view_input_remove(view, -1);
        }
    }

    if (key_pressed(KEY_DELETE)) {
        ret |= shell_view_input_remove(view, 1);
    }

    if (ret) {
        shell_view_input_blit(view, x, y, attr);
    }

    int input_caret = x + view->input_caret;
    view->caret_x = (input_caret) % view->cols;
    view->caret_y = y + (input_caret / view->cols);

    return ret;
}

void
shell_view_input_reset(ShellView *view)
{
    view->input_length = 0;
    view->input_length_last = 0;
    view->input_caret = 0;
}

void
shell_set_handler(Shell *shell, ShellHandlerF *handler)
{
    shell->handler_frame = 0;
    shell->handler_ = handler;
}

void
shell_step(Shell *shell)
{
    // int caret_x = shell->caret_x;
    // int caret_y = shell->caret_y;
    ShellHandlerF *handler = shell->handler_;
    shell->handler_(shell);
    if (handler == shell->handler_) {
        shell->handler_frame++;
    }

    // if ((caret_x != shell->caret_x) || (caret_y != shell->caret_y)) {
    //     shell->caret_frame = -2;
    // }

    Canvas canvas = CORE->canvas;
    //CORE->canvas.translate_x = SHELL_PADDING_LEFT;
    //CORE->canvas.translate_y = SHELL_PADDING_TOP;

    shell_draw(shell);

    ShellView *view;
    for (int i = 0; i != shell->views_count; ++i)
    {
        view = shell->views[i];
        if (view->caret_show && (view->caret_frame < 0 || (view->caret_frame % 4) < 2))
        {
            int screen_y = view->y + (view->caret_y - view->scroll_y);
            Rect caret_r = rect_make_size(
                view->x + view->caret_x * CORE->canvas.font->char_width,
                screen_y * CORE->canvas.font->char_height,
                CORE->canvas.font->char_width,
                CORE->canvas.font->char_height
            );
            rect_draw(caret_r, view->caret_color);
        }
        view->caret_frame++;
    }


    // char buf[256];
    // sprintf(buf, "%d %d", shell->caret_x, shell->caret_y);
    // text_draw(buf, 0, -CORE->canvas.font->char_height, 2);

    CORE->canvas = canvas;
}


SHELL_HANDLER(shell_prompt)
{
    // if (shell->handler_frame == 0) {
    //     shell_view_input_reset(shell);
    //     shell_print(shell, " > ", -1);
    //     shell->handler_caret_x = shell->caret_x;
    //     shell->handler_caret_y = shell->caret_y;
    // }

    // if (shell_view_input(shell)) {
    //     shell_view_input_blit(shell, shell->handler_caret_x, shell->handler_caret_y);
    // }

    // if (key_pressed(KEY_RETURN)) {
    //     shell_new_line(shell);
    //     shell_set_handler(shell, shell_command);
    // }
}

SHELL_HANDLER(shell_command)
{
    // TODO: Decide by the input what command to launch.
    // shell_print(shell, shell->input, shell->input_length);
    // shell_new_line(shell);
    // shell_set_handler(shell, shell_prompt);
}

//
//
//

typedef struct
{
    ShellView *view;
    IRCChannel *channel;
    bool dirty;
}
ShellIRCChannel;

typedef struct
{
    Shell *shell;
    IRCClient client;
    ShellIRCChannel *log;
    ShellIRCChannel *channel;
    ShellIRCChannel channels[16];
    int channels_count;

    ShellView *view_input;
    ShellView *view_channels;
}
ShellIRC;

ShellIRC irc = {0};

void
shirc_update_channels(ShellIRC *irc)
{
    char buf[32];
    int tab_width = irc->view_channels->cols / array_count(irc->channels);
    int length;
    TextAttr attr;
    attr.bg = COLOR_GRAY;
    attr.fg = COLOR_WHITE;
    ShellIRCChannel *channel;
    irc->view_channels->caret_x = 0;
    irc->view_channels->caret_y = 0;
    shell_view_clear(irc->view_channels, irc->view_channels->cols, attr);
    for (int i = 0; i != irc->channels_count; ++i) {
        channel = &irc->channels[i];
        if (channel == irc->channel) {
			attr.bg = COLOR_WHITE;
			attr.fg = COLOR_BLACK;
        } else {
			attr.bg = COLOR_GRAY;
			attr.fg = COLOR_WHITE;
		}
        if (channel->dirty) {
            attr.fg = COLOR_GREEN;
        }
		if (channel->channel) {
			sprintf(buf, " %.8s ", channel->channel->name);
		} else {
			sprintf(buf, " %.8s ", "Log");
		}
        shell_print(irc->view_channels, buf, -1, attr);
    }
}

ShellIRCChannel *
shirc_add_channel(ShellIRC *irc, IRCChannel *channel)
{
    ShellIRCChannel *channel_view = &irc->channels[irc->channels_count];
    irc->channels_count++;
    channel_view->dirty = 0;
    if (channel) {
        channel->udata = channel_view;
    }
    return channel_view;
}

int
shirc_get_current(ShellIRC *irc)
{
    return irc->channel - irc->channels;
}

void
shirc_set_current(ShellIRC *irc, int index)
{
    if (index < 0) {
        index = maximum(0, irc->channels_count - index);
    } else {
        index = index % irc->channels_count;
    }

    ShellIRCChannel *new_current = &irc->channels[index];
    if (new_current == irc->channel) {
        return;
    }
    if (irc->channel) {
        irc->channel->view->visible = false;
    }
    irc->channel = new_current;
    if (irc->channel) {
        irc->channel->view->visible = true;
        irc->channel->dirty = false;
    }

    shirc_update_channels(irc);
}

SHELL_HANDLER(shirc_handler)
{
    ShellView *view = irc.view_input;
    rect_draw(rect_make_size(
        0, SHELL_PADDING_TOP + view->y * CORE->canvas.font->char_height,
        PUN_CANVAS_WIDTH, CORE->canvas.font->char_height),
        PUN_COLOR_WHITE);

    TextAttr attr;
    attr.fg = PUN_COLOR_BLACK;
    attr.bg = PUN_COLOR_WHITE;
    if (shell_view_input(view, 0, 0, attr)) {
        // Input updated.
    }

    if (key_pressed(KEY_RETURN))
	{
        ircc_send_message(irc.channel->channel, view->input, view->input_length);
        shell_view_input_reset(view);
        view->caret_x = 0;
        view->caret_y = 0;
        view->scroll_y = 0;
        shell_view_clear(view, view->cols, attr);
    }

    if (key_pressed(KEY_TAB) && CORE->key_modifiers & KEY_MOD_CONTROL) {
        shirc_set_current(&irc, shirc_get_current(&irc) + 1);
    }
}

IRCC_ON_MESSAGE(irc_on_message)
{
    const char *channel_name;
    TextAttr attr;
    attr.bg = COLOR_BLACK;
    attr.fg = COLOR_WHITE;

    ShellIRCChannel *channel_view;
    switch (message->type)
    {
    case IRCMessageType_Registered:
        ircc_channel_join(C, "#random", -1);
        break;
    
    case IRCMessageType_ChannelAdded:
        if (channel != irc.client.channels)
        {
            channel_view = shirc_add_channel(&irc, channel);
			channel_view->view = shell_view_add(irc.shell,
				irc.log->view->x,
				irc.log->view->y,
				irc.log->view->cols,
				irc.log->view->rows);
			channel_view->channel = channel;
            shirc_update_channels(&irc);
        } else {
            irc.log->channel = channel;
            channel->udata = irc.log;
        }
        break;

    case IRCMessageType_Message:
        attr.fg = COLOR_GREEN;
        channel_view = (ShellIRCChannel*)channel->udata;
        if (!channel_view || channel == irc.client.channels) {
            channel_view = irc.log;
        }
        shell_new_line(channel_view->view, attr);
        shell_print(channel_view->view, "[", 1, attr);
        shell_print(channel_view->view, channel->name, -1, attr);
        shell_print(channel_view->view, "] ", 2, attr);

        if (message->nick.begin) {
            attr.fg = COLOR_BLUE;
            shell_print(channel_view->view, message->nick.begin, token_length(&message->nick), attr);
            attr.fg = COLOR_WHITE;
            shell_print(channel_view->view, ": ", 2, attr);
        }
        attr.fg = COLOR_WHITE;
        shell_print(channel_view->view, message->body.begin, token_length(&message->body), attr);

        if (channel_view != irc.channel) {
            channel_view->dirty = true;
            shirc_update_channels(&irc);
        }
        break;
    case IRCMessageType_Error:
        break;
    }
}

//
//
//

Shell *shell;

void
init()
{
    CORE->canvas.palette.colors[COLOR_BLACK] = color_make(0x20, 0x22, 0x23, 0xff);
    CORE->canvas.palette.colors[COLOR_BLUE]  = color_make(0x63, 0x9b, 0xff, 0xff);
    CORE->canvas.palette.colors[COLOR_GREEN] = color_make(0x63, 0xff, 0x9b, 0xff);
    CORE->canvas.palette.colors[COLOR_GRAY]  = color_make(0x60, 0x62, 0x63, 0xff);
    CORE->canvas.palette.colors_count = 6;

    CORE->background = CORE->canvas.palette.colors[0];

    bitmap_load_resource(&font.bitmap, "font.png");
    font.char_width = 4;
    font.char_height = 7;

    CORE->canvas.font = &font;

    CORE->shader->color_split = 0.6;
    CORE->shader->cube = 0.5;

    static Shell shell_;
    shell_init(&shell_);
    shell = &shell_;

    int cols = PUN_CANVAS_WIDTH / font.char_width;
    int rows = PUN_CANVAS_HEIGHT / font.char_height;

    dyad_init();
    dyad_setUpdateTimeout(0);
    irc.shell = shell;

    irc.log = shirc_add_channel(&irc, 0);
    irc.log->channel = 0;

    irc.log->view     = shell_view_add(shell, 0, 0, cols, rows - 2);
    irc.view_input    = shell_view_add(shell, 0, rows - 2, cols, 1);
    irc.view_channels = shell_view_add(shell, 0, rows - 1, cols, 1);

    irc.view_input->caret_show = 1;
	irc.view_input->caret_color = COLOR_BLACK;
    irc.view_input->visible = true;
    irc.view_channels->visible = true;

    irc.log->view->caret_y = irc.log->view->rows - 1;
    TextAttr attr;
    attr.bg = COLOR_WHITE;
    attr.fg = COLOR_BLACK;
    shell_view_clear(irc.view_input, irc.view_input->cols, attr);
    shirc_update_channels(&irc);

#if 0
    // ircc.on_message = irc_on_message;
#else
    ircc_init(&irc.client, "punity", 0, irc_on_message);
    shirc_set_current(&irc, 0);
#endif

    // shell_print(irc.log->view, "One two three", -1);
    // shell_new_line(irc.log->view);
    // shell_new_line(irc.log->view);
    // shell_print(irc.log->view, "Four five six seven", -1);
    // shell_new_line(irc.log->view);
    // shell_print(irc.log->view, "One two three", -1);
    // shell_new_line(irc.log->view);

    // shell_print(shell, "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Nulla id tristique ipsum. Donec eu augue et purus tempus dignissim. Sed efficitur tortor tortor, in ultricies neque varius in. Nunc id iaculis risus, sed porta lorem. Curabitur ut justo dapibus, porta justo non, ultricies nisl. Aenean urna nunc, maximus sed libero a, viverra consequat mi. Donec ex enim, porta sit amet lectus id, rhoncus commodo dui.", -1);
    // shell_print(irc.log->view, "Shell v.23\nType HELP for help\n\n", -1, attr);
    shell_set_handler(shell, shirc_handler);
}


void
step()
{
    if (dyad_getStreamCount() > 0) {
        dyad_update();
    }

    canvas_clear(COLOR_BLACK);

    // if (key_pressed(KEY_RETURN)) {
    //     window_fullscreen_toggle();
    // }

    if (key_pressed(KEY_ESCAPE)) {
        CORE->running = 0;
    }

    shell_step(shell);

    static char buf[256];
    sprintf(buf, "%03f %05d", CORE->perf_step.delta, (i32)CORE->frame);
    text_draw(buf, 32, 32, COLOR_BLUE);
}
