/**
 * Copyright (c) 2016 martincohen
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 */

#include "punity.h"

#if PLATFORM_OSX || PLATFORM_LINUX
// TODO
#else
#include <windows.h>
#include <dsound.h>
#endif

#if USE_STB_IMAGE
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#endif

#if USE_STB_VORBIS
#include <stb_vorbis.c>
#undef R
#undef L
#undef C
#endif

//#define PUNP_SOUND_DEFAULT_SOUND_VOLUME 0.8f
//#define PUNP_SOUND_DEFAULT_MASTER_VOLUME 0.8f

#define PUNP_SOUND_DEFAULT_SOUND_VOLUME 0.9f
#define PUNP_SOUND_DEFAULT_MASTER_VOLUME 0.9f

#define PUNP_WINDOW_WIDTH  (CANVAS_WIDTH  * CANVAS_SCALE)
#define PUNP_WINDOW_HEIGHT (CANVAS_HEIGHT * CANVAS_SCALE)
#define PUNP_FRAME_TIME    (1.0/30.0)

// Set to 1 to output a audio.buf file from the mixer.
#define PUNP_SOUND_DEBUG_FILE 0
#define PUNP_SOUND_CHANNELS 2
#define PUNP_SOUND_BUFFER_CHUNK_COUNT 16
#define PUNP_SOUND_BUFFER_CHUNK_SAMPLES  3000
#define PUNP_SOUND_SAMPLES_TO_BYTES(samples) ((samples) * (sizeof(i16) * PUNP_SOUND_CHANNELS))
#define PUNP_SOUND_BYTES_TO_SAMPLES(bytes) ((bytes) / (sizeof(i16) * PUNP_SOUND_CHANNELS))
#define PUNP_SOUND_BUFFER_BYTES (PUNP_SOUND_SAMPLES_TO_BYTES(PUNP_SOUND_BUFFER_CHUNK_COUNT * PUNP_SOUND_BUFFER_CHUNK_SAMPLES))

Core *CORE = 0;

//
//
//

void
panic(const char *message, const char *description, const char *function, const char *file, int line)
{
    printf("ASSERTION FAILED:\n"
                   "\t    message: %s\n"
                   "\tdescription: %s\n"
                   "\t   function: %s\n"
                   "\t       file: %s\n"
                   "\t       line: %d\n",

           message,
           description,
           function,
           file,
           line);

    assert(0);
    // *(int *)0 = 0;
}


//
// Utility
//

void
perf_from(PerfSpan *span)
{
    span->stamp = perf_get();
}

void
perf_to(PerfSpan *span)
{
    f64 now = perf_get();
    span->delta = (f32)(now - span->stamp);
    span->stamp = now;
}

f32
perf_delta(const PerfSpan *span)
{
    f64 now = perf_get();
    return maximum(0.0f, (f32)(now - span->stamp));
}

//
// File I/O
//

void *
file_read(const char *path, isize *size)
{
    FILE *f = fopen(path, "rb");
    if (f) {
        u8 *ptr;
        isize read;

        fseek(f, 0L, SEEK_END);
        *size = ftell(f);
        fseek(f, 0L, SEEK_SET);

        ptr = malloc(*size + 1);
        read = fread(ptr, 1, *size, f);
        fclose(f);
        ASSERT(read == *size);
        ptr[*size] = 0;

        return ptr;
    }
    return 0;
}

//
// Memory
//

void *
virtual_reserve(void *ptr, u32 size)
{
#if PLATFORM_WINDOWS
    ptr = VirtualAlloc(ptr, size, MEM_RESERVE, PAGE_NOACCESS);
    ASSERT(ptr);
#else
    ptr = mmap((void*)ptr, size,
               PROT_NONE,
               MAP_PRIVATE | MAP_ANON,
               -1, 0);
    ASSERT(ptr != MAP_FAILED);
    msync(ptr, size, MS_SYNC|MS_INVALIDATE);
#endif
    return ptr;
}

void *
virtual_commit(void *ptr, u32 size)
{
#if PLATFORM_WINDOWS
    ptr = VirtualAlloc(ptr, size, MEM_COMMIT, PAGE_READWRITE);
    ASSERT(ptr);
#else
    // TODO: MAP_PRIVATE or MAP_SHARED?
    ptr = mmap(ptr, size,
               PROT_READ | PROT_WRITE,
               MAP_FIXED | MAP_SHARED | MAP_ANON,
               -1, 0);
    ASSERT(ptr != MAP_FAILED);

    msync(ptr, size, MS_SYNC|MS_INVALIDATE);
#endif
    return ptr;
}

void
virtual_decommit(void *ptr, u32 size)
{
#if PLATFORM_WINDOWS
    VirtualFree(ptr, size, MEM_DECOMMIT);
#else
    // instead of unmapping the address, we're just gonna trick
    // the TLB to mark this as a new mapped area which, due to
    // demand paging, will not be committed until used.

    mmap(ptr, size,
         PROT_NONE,
         MAP_FIXED | MAP_PRIVATE | MAP_ANON,
         -1, 0);
    msync(ptr, size, MS_SYNC|MS_INVALIDATE);
#endif
}

inline void
virtual_free(void *ptr, u32 size)
{
#if PLATFORM_WINDOWS
    unused(size);
    VirtualFree((void *)ptr, 0, MEM_RELEASE);
#else
    msync(ptr, size, MS_SYNC);
    munmap(ptr, size);
#endif
}

void *
virtual_alloc(void *ptr, u32 size)
{
    return virtual_commit(virtual_reserve(ptr, size), size);
}

//
//
//

inline void
bank_init(Bank *stack, u32 capacity)
{
    ASSERT(stack);
    stack->begin = stack->it = virtual_alloc(0, capacity);
    ASSERT(stack->begin);
    stack->end = stack->begin + capacity;
}

void *
bank_push(Bank *stack, u32 size)
{
    ASSERT(stack);
    ASSERT(stack->begin);
    if ((stack->end - stack->it) < size) {
        printf("Not enought memory in bank (%d required, %d available).\n", (int)(stack->end - stack->it), size);
        ASSERT(0);
        return 0;
    } else {
        void *ptr = stack->it;
        stack->it += size;
        return ptr;
    }
}

inline void
bank_pop(Bank *stack, void *ptr)
{
    ASSERT(stack);
    ASSERT(stack->begin);
    ASSERT((u8 *)ptr >= stack->begin && (u8 *)ptr <= stack->it);
    stack->it = ptr;
}

inline BankState
bank_begin(Bank *bank)
{
    BankState state;
    state.bank = bank;
    state.state = *bank;
    return state;
}

inline void
bank_end(BankState *state)
{
    *state->bank = state->state;
}

//
// Graphics types
//

inline Color
color_make(u8 r, u8 g, u8 b, u8 a)
{
    Color c = {{COLOR_CHANNELS}};
    return c;
}

extern inline Rect
rect_make(i32 min_x, i32 min_y, i32 max_x, i32 max_y)
{
    Rect v;
    v.min_x = min_x;
    v.min_y = min_y;
    v.max_x = max_x;
    v.max_y = max_y;
    return v;
}

extern inline Rect
rect_make_size(i32 x, i32 y, i32 w, i32 h)
{
    Rect v = rect_make(x, y, x + w, y + h);
    return v;
}

inline b32
rect_check_limits(Rect *rect, i32 min_x, i32 min_y, i32 max_x, i32 max_y)
{
    return (rect->min_x >= min_x) && (rect->min_y >= min_y) &&
           (rect->max_x <= max_x) && (rect->max_y <= max_y) &&
           (rect->min_x <= rect->max_x) &&
           (rect->min_y <= rect->max_y);
}

//
// Canvas
//

inline void
rect_intersect(Rect *r, Rect *c)
{
    r->min_x = minimum(c->max_x, maximum(r->min_x, c->min_x));
    r->min_y = minimum(c->max_y, maximum(r->min_y, c->min_y));
    r->max_x = maximum(c->min_x, minimum(r->max_x, c->max_x));
    r->max_y = maximum(c->min_y, minimum(r->max_y, c->max_y));
}

// Returns 0 if R does not intersect with C (is fully invisible).
// Returns 1 if R has NOT been clipped (is fully visible).
// Returns 2 if R has been clipped.
i32
clip_rect_with_offsets(Rect *r, Rect *c, i32 *ox, i32 *oy)
{
    i32 res = 1;

    if (r->max_x < c->min_x || r->min_x >= c->max_x ||
        r->max_y < c->min_y || r->min_y >= c->max_y) {
        return 0;
    }


    if (r->min_x < c->min_x) {
        *ox = c->min_x - r->min_x;
        r->min_x = c->min_x;
        res = 2;
    } else if (r->max_x > c->max_x) {
        r->max_x = c->max_x;
        res = 2;
    }

    if (r->min_y < c->min_y) {
        *oy = c->min_y - r->min_y;
        r->min_y = c->min_y;
        res = 2;
    } else if (r->max_y > c->max_y) {
        r->max_y = c->max_y;
        res = 2;
    }

    return res;
}

inline void
clip_set(Rect rect)
{
    Rect canvas_rect = rect_make_size(0, 0, CORE->canvas->width, CORE->canvas->height);
    rect_intersect(&rect, &canvas_rect);
    CORE->clip = rect;
}

inline void
clip_reset(void)
{
    CORE->clip = rect_make_size(0, 0, CORE->canvas->width, CORE->canvas->height);
}

inline b32
clip_check(void)
{
    return (CORE->clip.min_x >= 0) &&
           (CORE->clip.min_y >= 0) &&
           (CORE->clip.max_x <= CORE->canvas->width) &&
           (CORE->clip.max_y <= CORE->canvas->height) &&
           (CORE->clip.min_x <= CORE->clip.max_x) &&
           (CORE->clip.min_y <= CORE->clip.max_y);
}

void
shift_colors(Bitmap *bitmap)
{

}

void
canvas_clear(u8 color)
{
    // TODO: This should clear only within the clip, so
    //       essentially we're drawing a rectangle here.
    memset(CORE->canvas->pixels, color, CORE->canvas->width * CORE->canvas->height);
}

void
rect_draw(Rect r, u8 color)
{
    rect_tr(&r, CORE->translate_x, CORE->translate_y);
    rect_intersect(&r, &CORE->clip);
    if (r.max_x > r.min_x && r.max_y > r.min_y) {
        u32 canvas_pitch = CORE->canvas->width;
        u8 *row = CORE->canvas->pixels + r.min_x + (r.min_y * canvas_pitch);
        i32 w = r.max_x - r.min_x;
        while (r.max_y != r.min_y) {
            memset(row, color, w);
            row += canvas_pitch;
            r.max_y--;
        }
    }
}

void
frame_draw(Rect r, u8 color)
{

}

//
// Bitmap
//

void
bitmap_init(Bitmap *bitmap, i32 width, i32 height, void *pixels, int bpp)
{
    Palette *palette = &CORE->palette;
    u32 size = width * height;

    bitmap->width = width;
    bitmap->height = height;

    bitmap->pixels = bank_push(CORE->storage, size);
    if (pixels) {
        if (bpp == BITMAP_32) {
            Color pixel;
            Color *pixels_end = ((Color *)pixels) + size;
            Color *pixels_it = pixels;

            u8 *it = bitmap->pixels;
            u8 ix;

            for (; pixels_it != pixels_end; ++pixels_it) {
                if (pixels_it->a < 0x7F) {
                    ix = COLOR_TRANSPARENT;
                    // pixels_it->a = 0;
                    goto next;
                }

                pixel = *pixels_it;
                // TODO: This is just for Windows, need to be done a bit better.
                pixel.b = pixels_it->r;
                pixel.r = pixels_it->b;
                pixel.a = 0xFF;
                for (ix = 1; ix < palette->colors_count; ix++) {
                    if (palette->colors[ix].rgba == pixel.rgba)
                        goto next;
                }

                // Add the color.

                ASSERT(palette->colors_count != 0xFF);
                if (palette->colors_count != 0xFF) {
                    ix = palette->colors_count;
                    palette->colors[ix] = pixel;
                    palette->colors_count++;
                    // printf("[palette] + #%0.2x %0.2x%0.2x%0.2x%0.2x\n",
                    //        ix, pixel.r, pixel.g, pixel.b, pixel.a);
                }
                next:;
                // Write the indexed color.

                *it++ = ix;
            }

        } else if (bpp == BITMAP_8)  {
            memcpy(bitmap->pixels, pixels, size);
        } else  {
            ASSERT_MESSAGE(0, "bitmap_init: Invalid bpp specified.");
        }
    } // if pixels
}

void
bitmap_clear(Bitmap *bitmap, u8 color)
{
    memset(bitmap->pixels, color, bitmap->width * bitmap->height);
}

#if USE_STB_IMAGE

void
bitmap_load(Bitmap *bitmap, const char *path)
{
    // TODO: Treat path as relative to executable, not the CWD!
    int width, height, comp;
    Color *pixels = (Color *)stbi_load(path, &width, &height, &comp, STBI_rgb_alpha);
    ASSERT(pixels);
    ASSERT(comp == 4);

    bitmap_init(bitmap, (i32)width, (i32)height, pixels, BITMAP_32);
    free(pixels);
}

void
bitmap_load_resource(Bitmap *bitmap, const char *resource_name)
{
    int width, height, comp;
    isize size;
    void *ptr;
    Color *pixels;

    ptr = resource_get(resource_name, &size);

    ASSERT(ptr);
    pixels = (Color *)stbi_load_from_memory(ptr, (int)size, &width, &height, &comp, STBI_rgb_alpha);
    ASSERT(pixels);
    ASSERT(comp == 4);

    bitmap_init(bitmap, (i32)width, (i32)height, pixels, BITMAP_32);
    free(pixels);
}

#endif

#define PUNP_BLIT(color, source_increment)                       \
    for (punp_blit_y = 0; punp_blit_y != h; ++punp_blit_y) {     \
        for (punp_blit_x = 0; punp_blit_x != w; ++punp_blit_x) { \
            dst[punp_blit_x] = *src ? color : dst[punp_blit_x];  \
            source_increment;                                    \
        }                                                        \
        dst += dst_fill;                                         \
        src += src_fill;                                         \
    }

// void image_set_draw(i32 x, i32 y, ImageSet *set, u16 index, u8 mode, u8 mask, V4i *clip)

void
bitmap_draw(i32 x, i32 y, i32 pivot_x, i32 pivot_y, Bitmap *bitmap, Rect *bitmap_rect, u32 flags, u8 color)
{
    Rect p_bitmap_rect, rect;
    i32 sx = 0, sy = 0;

    ASSERT(bitmap);
    ASSERT(clip_check());

    if (bitmap_rect) {
        ASSERT(rect_check_limits(bitmap_rect, 0, 0, bitmap->width, bitmap->height));
        p_bitmap_rect = *bitmap_rect;
    } else {
        p_bitmap_rect = rect_make_size(0, 0, bitmap->width, bitmap->height);
    }

    rect = rect_make_size(x - pivot_x, y - pivot_y,
                          p_bitmap_rect.max_x - p_bitmap_rect.min_x,
                          p_bitmap_rect.max_y - p_bitmap_rect.min_y);

    rect_tr(&rect, CORE->translate_x, CORE->translate_y);
    if (clip_rect_with_offsets(&rect, &CORE->clip, &sx, &sy)) {
        i32 w = rect.max_x - rect.min_x;
        i32 h = rect.max_y - rect.min_y;
        u32 dst_fill = CORE->canvas->width;
        u8 *dst = CORE->canvas->pixels + rect.min_x + (rect.min_y * dst_fill);
        i32 punp_blit_x, punp_blit_y;

        // i32 sw = (p_bitmap_rect.max_x - p_bitmap_rect.min_x) - sx;
        // i32 sh = (p_bitmap_rect.max_y - p_bitmap_rect.min_y) - sy;
        sx += p_bitmap_rect.left;
        sy += p_bitmap_rect.top;


        if ((flags & DrawFlags_FlipH) == 0) {
            u32 src_fill = bitmap->width - w;
            u8 *src = bitmap->pixels
                      // + (index * (sw * sh))
                      + (sx + (sy * bitmap->width));
            if (flags & DrawFlags_Mask) {
                PUNP_BLIT(color, src++);
            } else {
                PUNP_BLIT(*src, src++);
            }
        } else {
            u32 src_fill = bitmap->width + w;
            u8 *src = bitmap->pixels
                      // + (index * (sw * sh))
                      + (sx + (sy * bitmap->width))
                      + (w - 1);

            if (flags & DrawFlags_Mask) {
                PUNP_BLIT(color, src--);
            } else {
                PUNP_BLIT(*src, src--);
            }
        }
    }
}

#undef PUNP_BLIT

void
text_draw(i32 x, i32 y, const char *text, u8 color)
{
    ASSERT(CORE->font);
    {
        Font *font = CORE->font;
        i32 columns = font->bitmap.width / font->char_width;

        i32 dx = x;
        i32 dy = y;

        Rect cr;
        char c;
        while (*text) {
            c = *text;
            switch (c) {
                case '\n':
                    dx = x;
                    dy += font->char_height;
                    break;
                default:
                    cr.left = (c % columns) * font->char_width;
                    cr.top = (c / columns) * font->char_height;
                    cr.right = cr.left + font->char_width;
                    cr.bottom = cr.top + font->char_height;
                    bitmap_draw(dx, dy, 0, 0, &font->bitmap, &cr, DrawFlags_Mask, color);
                    dx += font->char_width;
                    break;
            }
            text++;
        }
    }
}

//
// Sound
//

#if USE_STB_VORBIS

static void
punp_sound_load_stbv(Sound *sound, stb_vorbis *stream)
{
    stb_vorbis_info info = stb_vorbis_get_info(stream);
    sound->volume = PUNP_SOUND_DEFAULT_SOUND_VOLUME;
    sound->rate = info.sample_rate;
    sound->samples_count = stb_vorbis_stream_length_in_samples(stream);
    sound->samples = bank_push(CORE->storage, PUNP_SOUND_SAMPLES_TO_BYTES(sound->samples_count));

    {
        static i16 buffer[1024];
        i16 *it = sound->samples;
        int samples_read_per_channel;
        for (; ;) {
            int samples_read_per_channel =
                    stb_vorbis_get_samples_short_interleaved(stream, PUNP_SOUND_CHANNELS, buffer, 1024);

            if (samples_read_per_channel == 0) {
                break;
            }

            // 2 channels, 16 bits per sample.
            memcpy(it, buffer, PUNP_SOUND_SAMPLES_TO_BYTES(samples_read_per_channel));
            it += samples_read_per_channel * PUNP_SOUND_CHANNELS;
        }
            }
    stb_vorbis_close(stream);
}

void
sound_load(Sound *sound, const char *path)
{
    int error = 0;
    stb_vorbis *stream;

    memset(sound, 0, sizeof(Sound));
    stream = stb_vorbis_open_filename(path, &error, 0);
    ASSERT(!error && stream);

    punp_sound_load_stbv(sound, stream);
}

void
sound_load_resource(Sound *sound, const char *resource_name)
{
    isize resource_size;
    void *resource_data;
    int error = 0;
    stb_vorbis *stream;

    memset(sound, 0, sizeof(Sound));

    resource_data = resource_get(resource_name, &resource_size);
    ASSERT(resource_data);

    stream = stb_vorbis_open_memory(resource_data, resource_size,
                                    &error, 0);

    ASSERT(!error && stream);

    punp_sound_load_stbv(sound, stream);
}

#endif // USE_STB_VORBIS

typedef struct PunPAudioSource
{
    Sound *sound;
    f32 rate;
    isize position;
    struct PunPAudioSource *next;
} PunPAudioSource;

static PunPAudioSource *punp_audio_source_pool = NULL;
static PunPAudioSource *punp_audio_source_playback = NULL;

void
sound_play(Sound *sound)
{
    PunPAudioSource *source = NULL;
    if (punp_audio_source_pool == 0) {
        // Pool is empty, therefore we must allocate a new source.
        source = bank_push(CORE->storage, sizeof(PunPAudioSource));
        // printf("Allocating audio source.\n");
    } else {
        // We have something in the pool.
        source = punp_audio_source_pool;
        punp_audio_source_pool = source->next;
        // printf("Pooling audio source.\n");
    }

    if (source) {
        memset(source, 0, sizeof(PunPAudioSource));
        source->sound = sound;
        source->rate = (f32)sound->rate / (f32)SOUND_SAMPLE_RATE;
        source->next = punp_audio_source_playback;
        punp_audio_source_playback = source;
    }
}

//static inline i16 *
//punp_audio_source_sample(PunPAudioSource *source, isize position)
//{
//    isize resampled_position = (isize)((f32)position * source->rate);
//    return source->sound->samples + ((source->position + resampled_position) * 2);
//}

#if PUNP_SOUND_DEBUG_FILE
static FILE *punp_audio_buf_file = 0;
static void
punp_win32_write_audio_buf(char *note, i16 *ptr, isize size)
{
    if (punp_audio_buf_file == 0) {
        punp_audio_buf_file = fopen("audio.buf", "wb+");
        ASSERT(punp_audio_buf_file);
    }
    // fprintf(punp_audio_buf_file, "%s %d", note, size);
    fwrite(ptr, size, 1, punp_audio_buf_file);
}
#endif

// Called from platform to fill in the audio buffer.
//
static void
punp_sound_mix(i16 *buffer, isize samples_count)
{
    BankState bank_state = bank_begin(CORE->stack);

    isize size =  samples_count * sizeof(f32);
    f32 *channel0 = bank_push(CORE->stack, size);
    f32 *channel1 = bank_push(CORE->stack, size);

    f32 *it0, *it1;
    isize i;

    isize sound_samples, sound_samples_remaining;
    Sound *sound;
    PunPAudioSource **it, *source;

    //
    // Zero the channels.
    //

    it0 = channel0;
    it1 = channel1;

    memset(it0, 0, size);
    memset(it1, 0, size);

    //
    // Mix the sources.
    //

    it = &punp_audio_source_playback;

    while (*it) {
        source = *it;
        ASSERT(source->sound);
        sound = source->sound;
        sound_samples = samples_count;
        sound_samples_remaining = sound->samples_count - source->position;
        if (sound_samples > sound_samples_remaining) {
            sound_samples = sound_samples_remaining;
        }

        it0 = channel0;
        it1 = channel1;
        for (i = 0; i < sound_samples; ++i) {
            i16 *sample = &sound->samples[(source->position + (isize)((f32)i * source->rate)) * 2];
            //sample = punp_audio_source_sample(source, i);
            *it0++ += sample[0] * sound->volume;
            *it1++ += sample[1] * sound->volume;
        }

        source->position += sound_samples * source->rate;

        if (source->position == source->sound->samples_count) {
            *it = source->next;
            source->next = punp_audio_source_pool;
            punp_audio_source_pool = source;
        } else {
            it = &source->next;
        }
    }

    //
    // Put to output buffer, clamp and convert to 16-bit.
    //

// #define MIX_CLIP_(in, clip) (0.5 * (abs(in + clip) - abs(in - clip)) * CORE->audio_volume)

    // Make sure sound buffer is not empty
    if (sound) {
        i16 *it_buffer = buffer;
        it0 = channel0;
        it1 = channel1;
        for (i = 0; i < samples_count; ++i) {
            f32 s1 = *it0++ * sound->volume * CORE->audio_volume;
            f32 s2 = *it1++ * sound->volume * CORE->audio_volume;
            // *it_buffer++ = (i16)(MIX_CLIP_(s1, 32767));
            // *it_buffer++ = (i16)(MIX_CLIP_(s2, 32767));
            // *it_buffer++ = (i16)(s1 + 0.5f);
            // *it_buffer++ = (i16)(s2 + 0.5f);
            *it_buffer++ = (i16)clamp(s1, -32768, 32767);
            *it_buffer++ = (i16)clamp(s2, -32768, 32767);
        }
    }
// #undef MIX_CLIP_

    bank_end(&bank_state);
}


//
// Windows
//
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
#if PLATFORM_WINDOWS
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

static HINSTANCE punp_win32_instance = 0;
static HWND punp_win32_window = 0;
static i64 punp_win32_perf_counter_frequency;

//
// Input
//

static u32
win32_app_key_mods(void)
{
    u32 mods = 0;

    if (GetKeyState(VK_SHIFT) & (1 << 31))
        mods |= KEY_MOD_SHIFT;
    if (GetKeyState(VK_CONTROL) & (1 << 31))
        mods |= KEY_MOD_CONTROL;
    if (GetKeyState(VK_MENU) & (1 << 31))
        mods |= KEY_MOD_ALT;
    if ((GetKeyState(VK_LWIN) | GetKeyState(VK_RWIN)) & (1 << 31))
        mods |= KEY_MOD_SUPER;

    return mods;
}

static LRESULT CALLBACK
win32_window_callback(HWND window, UINT message, WPARAM wp, LPARAM lp)
{
    switch (message) {
        case WM_KEYDOWN: {
            CORE->key_modifiers = win32_app_key_mods();
            if (wp < KEYS_MAX) {
                CORE->key_states[wp] = 1;
                CORE->key_deltas[wp] = 1;
                // printf("key pressed: %d\n", wp);
            }
            return 0;
        }
            break;

        case WM_KEYUP: {
            CORE->key_modifiers = win32_app_key_mods();
            if (wp < KEYS_MAX) {
                CORE->key_states[wp] = 0;
                CORE->key_deltas[wp] = 1;
                // printf("key released: %d\n", wp);
            }
            return 0;
        }
            break;

        case WM_CLOSE: {
            PostQuitMessage(0);
            CORE->running = 0;
        }
            break;
    }

    return DefWindowProcA(window, message, wp, lp);
}

f64
perf_get(void)
{
    i64 counter;
    QueryPerformanceCounter((LARGE_INTEGER *)&counter);
    return (f64)((f64)counter / (f64)punp_win32_perf_counter_frequency); // *(1e3);
}

void *
resource_get(const char *name, isize *size)
{
    HRSRC handle;
    HGLOBAL data;
    void *ptr;
    DWORD t_size;

    handle = FindResource(0, name, "RESOURCE");
    if (!handle) {
        printf("FindResource failed.");
        return 0;
    }

    data = LoadResource(0, handle);
    if (!data) {
        printf("LoadResource failed.");
        return 0;
    }

    ptr = LockResource(data);
    if (!ptr) {
        printf("LockResource failed.");
        return 0;
    }

    t_size = SizeofResource(0, handle);
    if (!t_size) {
        printf("SizeofResource failed.");
        return 0;
    }

    *size = t_size;
    return ptr;
}

//
// Sound
//

static LPDIRECTSOUND8 punp_win32_direct_sound = 0;
static LPDIRECTSOUNDBUFFER punp_win32_audio_buffer = 0;
static WAVEFORMATEX punp_win32_audio_format = {0};
static DSBUFFERDESC punp_win32_audio_buffer_description = {0};

typedef HRESULT(WINAPI*DirectSoundCreate8F)(LPGUID,LPDIRECTSOUND8*,LPUNKNOWN);
//#define DIRECT_SOUND_CREATE(name) \
//    HRESULT name(LPCGUID pcGuidDevice, LPDIRECTSOUND8 *ppDS, LPUNKNOWN pUnkOuter)
//typedef DIRECT_SOUND_CREATE(DirectSoundCreateF);

static b32
punp_win32_sound_init(void)
{
    HRESULT hr;
    HMODULE dsound_lib;
    DirectSoundCreate8F DirectSoundCreate8;
    DSBUFFERDESC primary_buffer_description = {sizeof(primary_buffer_description)};
    LPDIRECTSOUNDBUFFER primary_buffer;


    dsound_lib = LoadLibraryA("dsound.dll");
    if (!dsound_lib) {
        printf("LoadLibrary(dsound.dll) failed.\n");
        return 0;
    }

    DirectSoundCreate8 = (DirectSoundCreate8F)GetProcAddress(dsound_lib, "DirectSoundCreate8");
    if (!DirectSoundCreate8) {
        printf("GetProcAddress(DirectSoundCreate8) failed.\n");
        return 0;
    }

    hr = DirectSoundCreate8(0, &punp_win32_direct_sound, 0);
    if (hr != DS_OK) {
        printf("DirectSoundCreate failed.\n");
        return 0;
    }

    hr = IDirectSound8_SetCooperativeLevel(punp_win32_direct_sound, punp_win32_window, DSSCL_PRIORITY);
    if (hr != DS_OK) {
        printf("punp_win32_direct_sound->SetCooperativeLevel failed.\n");
        return 0;
    }

    primary_buffer_description.dwFlags = DSBCAPS_PRIMARYBUFFER;

    hr = IDirectSound8_CreateSoundBuffer(punp_win32_direct_sound, &primary_buffer_description, &primary_buffer, 0);
    if (hr != DS_OK) {
        printf("punp_win32_direct_sound->CreateSoundBuffer for primary buffer failed.\n");
        return 0;
    }

    punp_win32_audio_format.wFormatTag = WAVE_FORMAT_PCM;
    punp_win32_audio_format.nChannels = PUNP_SOUND_CHANNELS;
    punp_win32_audio_format.nSamplesPerSec = SOUND_SAMPLE_RATE;
    punp_win32_audio_format.wBitsPerSample = 16;
    punp_win32_audio_format.nBlockAlign = (punp_win32_audio_format.nChannels * punp_win32_audio_format.wBitsPerSample) / 8;
    punp_win32_audio_format.nAvgBytesPerSec = punp_win32_audio_format.nSamplesPerSec * punp_win32_audio_format.nBlockAlign;
    punp_win32_audio_format.cbSize = 0;

    hr = IDirectSoundBuffer8_SetFormat(primary_buffer, &punp_win32_audio_format);
    if (hr != DS_OK) {
        printf("primary_buffer->SetFormat failed.");
        return 0;
    }

    // DSBSIZE_MIN DSBSIZE_MAX

    punp_win32_audio_buffer_description.dwSize = sizeof(punp_win32_audio_buffer_description);
    // 2 seconds.
    punp_win32_audio_buffer_description.dwBufferBytes = PUNP_SOUND_BUFFER_BYTES;
    punp_win32_audio_buffer_description.lpwfxFormat = &punp_win32_audio_format;
    // dicates that IDirectSoundBuffer::GetCurrentPosition should use the new behavior of the play cursor.
    // In DirectSound in DirectX 1, the play cursor was significantly ahead of the actual playing sound on
    // emulated sound cards; it was directly behind the write cursor.
    // Now, if the DSBCAPS_GETCURRENTPOSITION2 flag is specified, the application can get a more accurate
    // play position. If this flag is not specified, the old behavior is preserved for compatibility.
    // Note that this flag affects only emulated sound cards; if a DirectSound driver is present, the play
    // cursor is accurate for DirectSound in all versions of DirectX.
    punp_win32_audio_buffer_description.dwFlags = DSBCAPS_GETCURRENTPOSITION2;

    hr = IDirectSound8_CreateSoundBuffer(punp_win32_direct_sound,
                                         &punp_win32_audio_buffer_description,
                                         &punp_win32_audio_buffer,
                                         0);
    if (hr != DS_OK)
    {
        printf("punp_win32_direct_sound->CreateSoundBuffer for secondary buffer failed.\n");
        return 0;
    }

    // IDirectSoundBuffer8_SetFormat(punp_win32_audio_buffer, &punp_win32_audio_format);

    // Clear the initial buffer.

    {
        LPVOID region1, region2;
        DWORD region1_size, region2_size;

        hr = IDirectSoundBuffer8_Lock(punp_win32_audio_buffer,
                                      0, punp_win32_audio_buffer_description.dwBufferBytes,
                                      (LPVOID *)&region1, &region1_size,
                                      (LPVOID *)&region2, &region2_size,
                                      DSBLOCK_ENTIREBUFFER);

        if (hr == DS_OK)
        {
            memset(region1, 0, region1_size);
            IDirectSoundBuffer8_Unlock(punp_win32_audio_buffer,
                                       region1, region1_size,
                                       region2, region2_size);
        }
    }

    hr = IDirectSoundBuffer8_Play(punp_win32_audio_buffer, 0, 0, DSBPLAY_LOOPING);

    if (hr != DS_OK)
    {
        ASSERT(0);
    }
    return 1;
}

static DWORD punp_win32_audio_cursor = 0;

static void
punp_win32_sound_step(void)
{
    HRESULT hr;
    DWORD cursor_play, cursor_write;
    isize chunk_size, chunk;
    DWORD lock_cursor, lock_size;
    VOID *range1, *range2;
    DWORD range1_size, range2_size;

    hr = IDirectSoundBuffer8_GetCurrentPosition(punp_win32_audio_buffer,
                                                &cursor_play, &cursor_write);

    if (FAILED(hr)) {
        printf("IDirectSoundBuffer8_GetCurrentPosition failed.\n");
        return;
    }

    chunk_size = PUNP_SOUND_SAMPLES_TO_BYTES(PUNP_SOUND_BUFFER_CHUNK_SAMPLES);
    chunk = cursor_write / chunk_size;

    lock_cursor = ((chunk+1) * chunk_size) % PUNP_SOUND_BUFFER_BYTES;
    lock_size = chunk_size;

    if (lock_cursor == punp_win32_audio_cursor) {
        return;
    }


    hr = IDirectSoundBuffer8_Lock(punp_win32_audio_buffer,
                                  lock_cursor,
                                  lock_size,
                                  &range1, &range1_size,
                                  &range2, &range2_size,
                                  0);
    if (hr != DS_OK) {
        printf("IDirectSoundBuffer8_Lock failed.\n");
        return;
    }

    punp_sound_mix(range1, PUNP_SOUND_BYTES_TO_SAMPLES(range1_size));
#if PUNP_SOUND_DEBUG_FILE
    punp_win32_write_audio_buf("range1", range1, range1_size);
#endif
    if (range2) {
        punp_sound_mix(range2, PUNP_SOUND_BYTES_TO_SAMPLES(range2_size));
#if PUNP_SOUND_DEBUG_FILE
        punp_win32_write_audio_buf("range2", range2, range2_size);
#endif
    }

    IDirectSoundBuffer8_Unlock(punp_win32_audio_buffer,
                               range1, range1_size,
                               range2, range2_size);

    punp_win32_audio_cursor = lock_cursor;

//  static playing = 0;
//  if (playing == 0) {
//      IDirectSoundBuffer8_Play(punp_win32_audio_buffer, 0, 0, DSBPLAY_LOOPING);
//      playing = 1;
//  }
}

//
//
//

int CALLBACK
WinMain(HINSTANCE instance, HINSTANCE prev_instance, LPSTR command_line, int show_code)
{
    static Bank s_stack = {0};
    static Bank s_storage = {0};
    static Core s_core = {0};
    static Bitmap s_canvas = {0};
    WNDCLASSA window_class = {0};
    u32 *window_buffer = NULL;
    BITMAPINFO window_bmi = {0};


    if (align_to(CANVAS_WIDTH, 16) != CANVAS_WIDTH) {
        printf("CANVAS_WIDTH must be aligned to 16.\n");
        return 1;
    }


    CORE = &s_core;
    CORE->running = 1;
    CORE->stack = &s_stack;
    CORE->storage = &s_storage;

    bank_init(CORE->stack, STACK_CAPACITY);
    bank_init(CORE->storage, STORAGE_CAPACITY);

    // TODO: Push canvas to storage? Storage is not initialized yet, so we cannot push it there.
    CORE->canvas = &s_canvas;
    bitmap_init(CORE->canvas, CANVAS_WIDTH, CANVAS_HEIGHT, 0, 0);
    bitmap_clear(CORE->canvas, COLOR_TRANSPARENT);

    clip_reset();

    CORE->audio_volume = PUNP_SOUND_DEFAULT_MASTER_VOLUME;

    //
    //
    //

    punp_win32_instance = instance;
    QueryPerformanceFrequency((LARGE_INTEGER *)&punp_win32_perf_counter_frequency);

    // b32 sleep_is_granular = (timeBeginPeriod(1 /*ms*/) == TIMERR_NOERROR);

#ifndef RELEASE_BUILD
    printf("Debug build...");
    if (AttachConsole(ATTACH_PARENT_PROCESS) || AllocConsole()) {
        freopen("CONOUT$", "w", stdout);
        freopen("CONOUT$", "w", stderr);
    }
#else
    printf("Release build...");
#endif

    window_class.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    window_class.lpfnWndProc = win32_window_callback;
    window_class.hInstance = punp_win32_instance;
    // window_class.hIcon = (HICON)LoadImage(0, "icon.ico", IMAGE_ICON, 0, 0, LR_LOADFROMFILE | LR_DEFAULTSIZE | LR_SHARED);
    window_class.hIcon = (HICON)LoadIcon(instance, "icon.ico");
    window_class.hCursor = LoadCursor(0, IDC_ARROW);
    window_class.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    window_class.lpszClassName = "Punity";

    if (!RegisterClassA(&window_class)) {
        printf("RegisterClassA failed.\n");
        return 1;
    }

    {
        int screen_width = GetSystemMetrics(SM_CXSCREEN);
        int screen_height = GetSystemMetrics(SM_CYSCREEN);
        RECT rc;
        DWORD style = WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;

        rc.left = (screen_width - (PUNP_WINDOW_WIDTH)) / 2;
        rc.top = (screen_height - (PUNP_WINDOW_HEIGHT)) / 2;
        rc.right = rc.left + PUNP_WINDOW_WIDTH;
        rc.bottom = rc.top + PUNP_WINDOW_HEIGHT;


        ASSERT(AdjustWindowRect(&rc, style, FALSE) != 0);

        // int window_width  = rc.right - rc.left;
        // int window_height = rc.bottom - rc.top;
        // rc.left = (screen_width  - width) / 2;
        // rc.top  = (screen_height - height) / 2;

        punp_win32_window = CreateWindowExA(
                0,
                window_class.lpszClassName,
                WINDOW_TITLE,
                style,
                rc.left, rc.top,
                rc.right - rc.left, rc.bottom - rc.top,
                0, 0,
                punp_win32_instance,
                0);
    }
    if (!punp_win32_window) {
        printf("CreateWindowExA failed.\n");
        return 1;
    }

    // Canvas
    window_bmi.bmiHeader.biSize = sizeof(window_bmi.bmiHeader);
    window_bmi.bmiHeader.biWidth = CANVAS_WIDTH;
    window_bmi.bmiHeader.biHeight = CANVAS_HEIGHT;
    window_bmi.bmiHeader.biPlanes = 1;
    window_bmi.bmiHeader.biBitCount = 32;
    window_bmi.bmiHeader.biCompression = BI_RGB;

    window_buffer = bank_push(CORE->stack, (CANVAS_WIDTH * 4) * CANVAS_HEIGHT);
    ASSERT(window_buffer);

    // Sound

    if (punp_win32_sound_init() == 0) {
        punp_win32_audio_buffer = 0;
    }


    init();


    // TODO: Center window
    ShowWindow(punp_win32_window, SW_SHOW);

    {
        f64 frame_time_stamp, frame_time_now, frame_time_delta;
        int x, y;
        u32 *window_row;
        u8 *canvas_it;
        MSG message;
        perf_from(&CORE->perf_frame);
        while (CORE->running) {
            perf_to(&CORE->perf_frame);
            perf_from(&CORE->perf_frame_inner);

            memset(&CORE->key_deltas, 0, KEYS_MAX);

            while (PeekMessage(&message, 0, 0, 0, PM_REMOVE)) {
                if (message.message == WM_QUIT) {
                    CORE->running = 0;
                }

                TranslateMessage(&message);
                DispatchMessageA(&message);
            }

            perf_from(&CORE->perf_step);
            step();
            perf_to(&CORE->perf_step);

            perf_from(&CORE->perf_audio);
            if (punp_win32_audio_buffer) {
                punp_win32_sound_step();
            }
            perf_to(&CORE->perf_audio);

            perf_from(&CORE->perf_blit);
            perf_from(&CORE->perf_blit_cvt);
            canvas_it = CORE->canvas->pixels;
            for (y = CANVAS_HEIGHT; y != 0; --y) {
                window_row = window_buffer + ((y - 1) * CANVAS_WIDTH);
                for (x = 0; x != CANVAS_WIDTH; ++x) {
                    *(window_row++) = CORE->palette.colors[*canvas_it++].rgba;
                }
            }
            perf_to(&CORE->perf_blit_cvt);

            perf_from(&CORE->perf_blit_gdi);
            {
                HDC dc = GetDC(punp_win32_window);
        #if 1
                // TODO: This is sadly slow (50us on my machine), need to find a faster way to do this.
                StretchDIBits(dc,
                              0, 0, CANVAS_WIDTH * CANVAS_SCALE, CANVAS_HEIGHT * CANVAS_SCALE,
                              0, 0, CANVAS_WIDTH, CANVAS_HEIGHT,
                              window_buffer,
                              &window_bmi,
                              DIB_RGB_COLORS,
                              SRCCOPY);
        #else
        #endif
                ReleaseDC(punp_win32_window, dc);
            }
            perf_to(&CORE->perf_blit_gdi);
            perf_to(&CORE->perf_blit);

            perf_to(&CORE->perf_frame_inner);

            {
                f32 frame_delta = perf_delta(&CORE->perf_frame);
                if (frame_delta < PUNP_FRAME_TIME) {
                    // printf("sleeping ... %.3f\n", (f32)PUNP_FRAME_TIME - frame_delta);
                    Sleep((PUNP_FRAME_TIME - frame_delta) * 1e3);
                }
            }
            CORE->frame++;

    #if 0
            printf("stack %d storage %d\n",
                   CORE->stack->it - CORE->stack->begin,
                   CORE->storage->it - CORE->storage->begin);
    #endif
        }
    }

#if PUNP_SOUND_DEBUG_FILE
    fclose(punp_audio_buf_file);
#endif

    return 0;
}

#endif

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------


#if 0
int
main(int argc, char **argv)
{
    return win32_main(argc, argv);
}
#endif
