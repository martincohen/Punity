/** 
 * Copyright (c) 2016 martincohen
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 */ 

#include "punity.h"

#if PUN_PLATFORM_OSX || PUN_PLATFORM_LINUX
// TODO
#else
#include <windows.h>
#include <dsound.h>
#endif

#if PUN_USE_STB_IMAGE
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#endif

#if PUN_USE_STB_VORBIS
#include <stb_vorbis.c>
#undef R
#undef L
#undef C
#endif

//#define PUNP_SOUND_DEFAULT_SOUND_VOLUME 0.8f
//#define PUNP_SOUND_DEFAULT_MASTER_VOLUME 0.8f

#define PUNP_SOUND_DEFAULT_SOUND_VOLUME 0.9f
#define PUNP_SOUND_DEFAULT_MASTER_VOLUME 0.9f

#define PUNP_WINDOW_WIDTH  (PUN_CANVAS_WIDTH  * PUN_CANVAS_SCALE)
#define PUNP_WINDOW_HEIGHT (PUN_CANVAS_HEIGHT * PUN_CANVAS_SCALE)
#define PUNP_FRAME_TIME    (1.0/30.0)

// Set to 1 to output a audio.buf file from the mixer.
#define PUNP_SOUND_DEBUG_FILE 0
#define PUNP_SOUND_CHANNELS 2
#define PUNP_SOUND_BUFFER_CHUNK_COUNT 16
#define PUNP_SOUND_BUFFER_CHUNK_SAMPLES  3000
#define PUNP_SOUND_SAMPLES_TO_BYTES(samples, channels) ((samples) * (sizeof(i16) * channels))
#define PUNP_SOUND_BYTES_TO_SAMPLES(bytes, channels) ((bytes) / (sizeof(i16) * channels))
#define PUNP_SOUND_BUFFER_BYTES (PUNP_SOUND_SAMPLES_TO_BYTES(PUNP_SOUND_BUFFER_CHUNK_COUNT * PUNP_SOUND_BUFFER_CHUNK_SAMPLES, PUNP_SOUND_CHANNELS))

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

#define RAND32_MAX 0xffffffff

u32
rand_u(u32 *x)
{
    if (*x == 0) *x = 314159265;
    *x ^= *x << 13;
    *x ^= *x >> 17;
    *x ^= *x << 5;
    return *x;
}

f32
rand_f(u32 *x)
{
    return (f32)(rand_u(x) % 1000) / 1000.0f;
}

f32
rand_fr(u32 *x, f32 min, f32 max)
{
    f32 f = rand_f(x);
    return min + (f * (max-min));
}

i32
rand_ir(u32 *x, i32 min, i32 max)
{
    i32 f = abs((i32)rand_u(x));
    return min + (f % ((max-min) + 1));
}

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
file_read(const char *path, size_t *size)
{
    FILE *f = fopen(path, "rb");
    if (f) {
        fseek(f, 0L, SEEK_END);
        *size = ftell(f);
        fseek(f, 0L, SEEK_SET);

        u8 *ptr = malloc(*size + 1);
        size_t read = fread(ptr, 1, *size, f);
        fclose(f);
        ASSERT(read == *size);
        ptr[*size] = 0;

        return ptr;
    }
    return 0;
}

bool
file_write(const char *path, void *ptr, size_t size)
{
    FILE *f = fopen(path, "wb+");
    if (f) {
        fwrite(ptr, size, 1, f);
        fclose(f);
        return true;
    }
    return false;
}

//
// Memory
//

void *
virtual_reserve(void *ptr, u32 size)
{
#if PUN_PLATFORM_WINDOWS
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
#if PUN_PLATFORM_WINDOWS
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
#if PUN_PLATFORM_WINDOWS
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

void
virtual_free(void *ptr, u32 size)
{
#if PUN_PLATFORM_WINDOWS
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

void
bank_init(Bank *stack, u32 capacity)
{
    ASSERT(stack);
    stack->begin = stack->it = virtual_alloc(0, capacity);
    ASSERT(stack->begin);
    stack->end = stack->begin + capacity;
}

void
bank_clear(Bank *bank)
{
    memset(bank->begin, 0, bank->end - bank->begin);
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
    }
    void *ptr = stack->it;
    stack->it += size;
    return ptr;
}

void
bank_pop(Bank *stack, void *ptr)
{
    ASSERT(stack);
    ASSERT(stack->begin);
    ASSERT((u8 *)ptr >= stack->begin && (u8 *)ptr <= stack->it);
    stack->it = ptr;
}

BankState
bank_begin(Bank *bank)
{
    BankState state;
    state.bank = bank;
    state.state = *bank;
    return state;
}

void
bank_end(BankState *state)
{
    *state->bank = state->state;
}

//
// Graphics types
//

Color
color_make(u8 r, u8 g, u8 b, u8 a)
{
    Color c = {{COLOR_CHANNELS}};
    return c;
}

Color
color_lerp(Color from, Color to, f32 t)
{
    from.r = (i32)(255.0f * lerp((f32)from.r/255.0f, (f32)to.r/255.0f, t));
    from.g = (i32)(255.0f * lerp((f32)from.g/255.0f, (f32)to.g/255.0f, t));
    from.b = (i32)(255.0f * lerp((f32)from.b/255.0f, (f32)to.b/255.0f, t));
    return from;
}

extern inline Rect
rect_make(i32 min_x, i32 min_y, i32 max_x, i32 max_y)
{
    Rect r;
    r.min_x = min_x;
    r.min_y = min_y;
    r.max_x = max_x;
    r.max_y = max_y;
    return r;
}

extern inline Rect
rect_make_size(i32 x, i32 y, i32 w, i32 h)
{
    Rect r = rect_make(x,
                       y,
                       x + w,
                       y + h);
    return r;
}

extern inline Rect
rect_make_centered(i32 x, i32 y, i32 w, i32 h)
{
    w = w / 2;
    h = h / 2;
    Rect r = rect_make(x - w,
                       y - h,
                       x + w + 1,
                       y + h + 1);
    return r;
}

extern inline bool
rect_contains_point(Rect *rect, i32 x, i32 y)
{
    return (x >= rect->min_x && x < rect->max_x &&
            y >= rect->min_y && y < rect->max_y);
}

bool
rect_check_limits(Rect *rect, i32 min_x, i32 min_y, i32 max_x, i32 max_y)
{
    return (rect->min_x >= min_x) && (rect->min_y >= min_y) &&
           (rect->max_x <= max_x) && (rect->max_y <= max_y) &&
           (rect->min_x <= rect->max_x) &&
           (rect->min_y <= rect->max_y);
}

//
// DrawList
//

typedef enum
{
    DrawListItemType_Rect_,
    DrawListItemType_Frame_,
    DrawListItemType_Text_,
    DrawListItemType_BitmapFull_,
    DrawListItemType_BitmapPartial_
}
DrawListItemType_;

typedef struct DrawListItem
{
    u32 type;
    u32 key;
    Canvas canvas;

    union
    {
        struct {
            Rect rect;
            u8 color;
        } rect;
        
        struct {
            char *text;
            i32 x;
            i32 y;
            u8 color;
        } text;

        struct {
            Bitmap *bitmap;
            i32 x;
            i32 y;
            i32 pivot_x;
            i32 pivot_y;
            Rect bitmap_rect;
            u32 flags;
            u8 color;
        } bitmap;
        
        struct {
            void *data;
            size_t data_size;
        } custom;
    };
}
DrawListItem_;

void
draw_list_init(DrawList *list, size_t reserve)
{
    list->items = 0;
    list->items_count = 0;
    list->items_reserve = reserve;
    list->items_additional = 0;
}

void
draw_list_begin(DrawList *list)
{
    list->items_reserve += list->items_additional;
    list->items = (DrawListItem_ **)bank_push(CORE->stack, sizeof(DrawListItem_*) * list->items_reserve);
    list->items_storage = (DrawListItem_ *)bank_push(CORE->stack, sizeof(DrawListItem_) * list->items_reserve);
    list->items_count = 0;
    list->items_additional = 0;
}

static DrawListItem_ **
_sort(DrawListItem_ **entries, u32 count, DrawListItem_ **temp)
{
    DrawListItem_ **dest = temp;
    DrawListItem_ **source = entries;

    for (u32 bi = 0; bi != 4*8; bi += 4)
    {
        u32 offsets[256] = {0};

        // Count.
        for (u32 i = 0; i != count; ++i)
        {
            u32 value = (u32)source[i]->key;
            u32 piece = (value >> bi) & 0xff;
            offsets[piece]++;
        }

        // Counts to offsets.
        u32 total = 0;
        for (u32 si = 0; si != array_count(offsets); ++si)
        {
            u32 count = offsets[si];
            offsets[si] = total;
            total += count;
        }

        // Place.
        for (u32 i = 0; i != count; ++i)
        {
            u32 value = (u32)source[i]->key;
            u32 piece = (value >> bi) & 0xff;
            dest[offsets[piece]++] = source[i];
        }

        DrawListItem_ **t = dest;
        dest = source;
        source = t;
    }

    return source;
}

void
draw_list_end(DrawList *list)
{
    if (list->items_count == 0) {
        return;
    }

    Canvas canvas = CORE->canvas;
    DrawListItem_ **temp = (DrawListItem_ **)bank_push(CORE->stack,
        sizeof(DrawListItem_*) * list->items_count);
    DrawListItem_ **it = _sort(list->items, list->items_count, temp);
    DrawListItem_ *item;
    for (size_t i = 0; i != list->items_count; ++i, ++it)
    {
        item = *it;
        CORE->canvas = item->canvas;
        switch (item->type)
        {
        case DrawListItemType_Rect_:
            rect_draw(item->rect.rect, item->rect.color);
            break;
        case DrawListItemType_Frame_:
            ASSERT_MESSAGE(0, "Not implemented, yet.");
            // frame_draw(item->rect.rect, item->rect.color);
            break;
        case DrawListItemType_Text_:
            text_draw(item->text.text, item->text.x, item->text.y, item->text.color);
            break;
        case DrawListItemType_BitmapFull_:
        case DrawListItemType_BitmapPartial_:
            bitmap_draw(
                item->bitmap.bitmap,
                item->bitmap.x,
                item->bitmap.y,
                item->bitmap.pivot_x,
                item->bitmap.pivot_y,
                item->type == DrawListItemType_BitmapPartial_ ? &item->bitmap.bitmap_rect : 0,
                item->bitmap.flags,
                item->bitmap.color);
            break;
        default:
            ASSERT_MESSAGE(0, "Unknown item type.");
            break;
        }
    }
    
    list->items_count = 0;
    CORE->canvas = canvas;
}

#if 0
void *
draw_list_push(DrawList *list, i32 z, size_t size)
{
    if (list->items_count == list->items_reserve) {
        printf("Not enough space for new items in draw_list_push.\n");
        printf("Allocating more space for next frame.\n");
        list->items_reserve += 8;
        return 0;
    }

    DrawListItem_ *item = list->items[list->items_count++];
    item->data = bank_push(CORE->stack, size);
    item->data_size = size;
    item->z = z;

    return item->data;
}

void
draw_list_add(DrawList *list, i32 z, void *data, size_t size)
{
    void *dest = draw_list_push(list, z, size);
    if (dest) {
        memcpy(dest, data, size);
    }
}
#endif

//
// Pushed draw functions
//

static DrawListItem_ *
draw_list_push_(DrawList *list, i32 z, u32 type)
{
    if (list->items_count == list->items_reserve) {
        printf("Not enough space for new items in draw_list_push.\n");
        printf("Allocating more space for next frame.\n");
        list->items_additional += 4;
        return 0;
    }

    DrawListItem_ *item = &list->items_storage[list->items_count];
    item->type = type;
    // Convert from i32 to u32 by converting
    // from INT32_MIN -> INT32_MAX
    //   to 0 -> UINT32_MAX
    item->key = (u32)(z - INT32_MIN);
    item->canvas = CORE->canvas;

    list->items[list->items_count] = item;
    list->items_count++;

    return item;
}


//
// Canvas
//

void
rect_intersect(Rect *R, Rect *C)
{
    R->min_x = minimum(C->max_x, maximum(R->min_x, C->min_x));
    R->min_y = minimum(C->max_y, maximum(R->min_y, C->min_y));
    R->max_x = maximum(C->min_x, minimum(R->max_x, C->max_x));
    R->max_y = maximum(C->min_y, minimum(R->max_y, C->max_y));
}

// Returns 0 if R does not intersect with C (is fully invisible).
// Returns 1 if R has NOT been clipped (is fully visible).
// Returns 2 if R has been clipped.
i32
clip_rect_with_offsets(Rect *R, Rect *C, i32 *ox, i32 *oy)
{
    if (R->max_x < C->min_x || R->min_x >= C->max_x ||
        R->max_y < C->min_y || R->min_y >= C->max_y) {
        return 0;
    }

    i32 res = 1;

    if (R->min_x < C->min_x) {
        *ox = C->min_x - R->min_x;
        R->min_x = C->min_x;
        res = 2;
    } else if (R->max_x > C->max_x) {
        R->max_x = C->max_x;
        res = 2;
    }

    if (R->min_y < C->min_y) {
        *oy = C->min_y - R->min_y;
        R->min_y = C->min_y;
        res = 2;
    } else if (R->max_y > C->max_y) {
        R->max_y = C->max_y;
        res = 2;
    }

    return res;
}

void
clip_set(Rect rect)
{
    Rect canvas_rect = rect_make_size(0, 0, CORE->canvas.bitmap->width, CORE->canvas.bitmap->height);
    rect_intersect(&rect, &canvas_rect);
    CORE->canvas.clip = rect;
}

void
clip_reset()
{
    CORE->canvas.clip = rect_make_size(0, 0, CORE->canvas.bitmap->width, CORE->canvas.bitmap->height);
}

bool
clip_check()
{
    return (CORE->canvas.clip.min_x >= 0) &&
           (CORE->canvas.clip.min_y >= 0) &&
           (CORE->canvas.clip.max_x <= CORE->canvas.bitmap->width) &&
           (CORE->canvas.clip.max_y <= CORE->canvas.bitmap->height) &&
           (CORE->canvas.clip.min_x <= CORE->canvas.clip.max_x) &&
           (CORE->canvas.clip.min_y <= CORE->canvas.clip.max_y);
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
    memset(CORE->canvas.bitmap->pixels, color, CORE->canvas.bitmap->width * CORE->canvas.bitmap->height);
}

void
rect_draw(Rect r, u8 color)
{
    rect_tr(&r, CORE->canvas.translate_x, CORE->canvas.translate_y);
    rect_intersect(&r, &CORE->canvas.clip);
    if (r.max_x > r.min_x && r.max_y > r.min_y)
    {
        u32 canvas_pitch = CORE->canvas.bitmap->width;
        u8 *row = CORE->canvas.bitmap->pixels + r.min_x + (r.min_y * canvas_pitch);
        i32 w = r.max_x - r.min_x;
        while (r.max_y != r.min_y)
        {
            memset(row, color, w);
            row += canvas_pitch;
            r.max_y--;
        }
    }
}

void
rect_draw_push(Rect rect, u8 color, i32 z)
{
    DrawListItem_ *item = draw_list_push_(CORE->draw_list, z, DrawListItemType_Rect_);
    if (item) {
        item->rect.rect = rect;
        item->rect.color = color;
    }
}

void
frame_draw(Rect r, u8 color)
{

}

// void image_set_draw(i32 x, i32 y, ImageSet *set, u16 index, u8 mode, u8 mask, V4i *clip)

#if 1

void
bitmap_draw(Bitmap *src_bitmap, i32 x, i32 y, i32 pivot_x, i32 pivot_y, Rect *bitmap_rect, u32 flags, u8 color)
{
    ASSERT(src_bitmap);
    ASSERT(clip_check());

    Rect src_r;
    if (bitmap_rect) {
        ASSERT(rect_check_limits(bitmap_rect, 0, 0, src_bitmap->width, src_bitmap->height));
        src_r = *bitmap_rect;
    } else {
        src_r = rect_make_size(0, 0, src_bitmap->width, src_bitmap->height);
    }

    Rect dst_r = rect_make_size(x - pivot_x, y - pivot_y,
                                src_r.max_x - src_r.min_x,
                                src_r.max_y - src_r.min_y);
    rect_tr(&dst_r, CORE->canvas.translate_x, CORE->canvas.translate_y);
    i32 src_ox = 0;
    i32 src_oy = 0;
    if (clip_rect_with_offsets(&dst_r, &CORE->canvas.clip, &src_ox, &src_oy))
    {
        Bitmap *dst_bitmap = CORE->canvas.bitmap;
        i32 dst_w = dst_r.max_x - dst_r.min_x;
        i32 dst_h = dst_r.max_y - dst_r.min_y;
        i32 src_w = src_r.max_x - src_r.min_x;
        i32 src_h = src_r.max_y - src_r.min_y;

		i32 dst_step_y = dst_bitmap->width - dst_w;
		i32 dst_step_x = +1;
        u8 *dst = dst_bitmap->pixels;
		dst += dst_r.min_x + (dst_r.min_y * dst_bitmap->width);

        i32 src_step_y, src_step_x;
        u8 *src = src_bitmap->pixels;

        if (flags & DrawFlags_FlipH) {
            src_r.min_x += src_w - dst_w - src_ox;
            src_r.max_x = src_r.min_x + dst_w;
            src += src_r.max_x - 1;
            src_step_y = +dst_w;
            src_step_x = -1;
        } else {
            src_r.min_x += src_ox;
            src_r.max_x = src_r.min_x + dst_w;
            src += src_r.min_x;
			src_step_y = -dst_w;
            src_step_x = +1;
        }

        if (flags & DrawFlags_FlipV) {
            src_r.min_y += src_h - dst_h - src_oy;
            src_r.max_y = src_r.min_y + dst_h;
			src += (src_r.max_y-1) * src_bitmap->width;
			src_step_y += -src_bitmap->width;
        } else {
            src_r.min_y += src_oy;
            src_r.max_y = src_r.min_y + dst_h;
			src += src_r.min_y * src_bitmap->width;
			src_step_y += src_bitmap->width;
		}

        i32 y_, x_;

		if (flags & DrawFlags_Mask)
		{
			for (y_ = 0; y_ != dst_h; ++y_) {
				for (x_ = 0; x_ != dst_w; ++x_) {
					*dst = *src ? color : *dst;
					src += src_step_x;
					dst += dst_step_x;
				}
				dst += dst_step_y;
				src += src_step_y;
			}
		}
		else
		{
			for (y_ = 0; y_ != dst_h; ++y_) {
				for (x_ = 0; x_ != dst_w; ++x_) {
					*dst = *src ? *src : *dst;
					src += src_step_x;
					dst += dst_step_x;
				}
				dst += dst_step_y;
				src += src_step_y;
			}
		}
    }
}

#else

#define PUNP_BLIT(color, source_increment) \
    for (punp_blit_y = 0; punp_blit_y != h; ++punp_blit_y) { \
        for (punp_blit_x = 0; punp_blit_x != w; ++punp_blit_x) { \
            dst[punp_blit_x] = *src ? color : dst[punp_blit_x]; \
            source_increment; \
        } \
        dst += dst_fill; \
        src += src_fill; \
    }

void
bitmap_draw(Bitmap *bitmap, i32 x, i32 y, i32 pivot_x, i32 pivot_y, Rect *bitmap_rect, u32 flags, u8 color)
{
    ASSERT(bitmap);
    ASSERT(clip_check());

    Rect p_bitmap_rect;
    if (bitmap_rect) {
        ASSERT(rect_check_limits(bitmap_rect, 0, 0, bitmap->width, bitmap->height));
        p_bitmap_rect = *bitmap_rect;
    } else {
        p_bitmap_rect = rect_make_size(0, 0, bitmap->width, bitmap->height);
    }

    Rect rect = rect_make_size(x - pivot_x, y - pivot_y,
                               p_bitmap_rect.max_x - p_bitmap_rect.min_x,
                               p_bitmap_rect.max_y - p_bitmap_rect.min_y);

    rect_tr(&rect, CORE->canvas.translate_x, CORE->canvas.translate_y);
    i32 sx = 0;
    i32 sy = 0;
    if (clip_rect_with_offsets(&rect, &CORE->canvas.clip, &sx, &sy))
    {
        i32 w = rect.max_x - rect.min_x;
        i32 h = rect.max_y - rect.min_y;

        // i32 sw = (p_bitmap_rect.max_x - p_bitmap_rect.min_x) - sx;
        // i32 sh = (p_bitmap_rect.max_y - p_bitmap_rect.min_y) - sy;

        u32 dst_fill = CORE->canvas.bitmap->width;
        u8 *dst = CORE->canvas.bitmap->pixels + rect.min_x + (rect.min_y * dst_fill);

        i32 punp_blit_x, punp_blit_y;
        if ((flags & DrawFlags_FlipH) == 0)
        {
			sx += p_bitmap_rect.left;
			sy += p_bitmap_rect.top;
			u32 src_fill = bitmap->width - w;
            u8 *src = bitmap->pixels + (sx + (sy * bitmap->width));
            if (flags & DrawFlags_Mask) {
                PUNP_BLIT(color, src++);
            } else {
                PUNP_BLIT(*src, src++);
            }
        }
        else
        {
			i32 sw = p_bitmap_rect.max_x - p_bitmap_rect.min_x;
			sx = (sw - w - sx) + p_bitmap_rect.left;
			sy += p_bitmap_rect.top;
			u32 src_fill = bitmap->width + w;
            u8 *src = bitmap->pixels
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
#endif

void
bitmap_draw_push(
    Bitmap *bitmap,
    i32 x,
    i32 y,
    i32 pivot_x,
    i32 pivot_y,
    Rect *bitmap_rect,
    u32 flags,
    u8 color,
    i32 z)
{
    DrawListItem_ *item = draw_list_push_(CORE->draw_list, z, DrawListItemType_BitmapFull_);
    if (item) {
        item->bitmap.bitmap = bitmap;
        item->bitmap.x = x;
        item->bitmap.y = y;
        item->bitmap.pivot_x = pivot_x;
        item->bitmap.pivot_y = pivot_y;
        if (bitmap_rect) {
            ASSERT(rect_check_limits(bitmap_rect, 0, 0, bitmap->width, bitmap->height));
            item->bitmap.bitmap_rect = *bitmap_rect;
            item->type = DrawListItemType_BitmapPartial_;
        }
        item->bitmap.flags = flags;
        item->bitmap.color = color;
    }
}

void
text_draw(const char *text, i32 x, i32 y, u8 color)
{
    ASSERT(CORE->canvas.font);
    Font *font = CORE->canvas.font;
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
                bitmap_draw(&font->bitmap, dx, dy, 0, 0, &cr, DrawFlags_Mask, color);
                dx += font->char_width;
                break;
        }
        text++;
    }
}

void
text_draw_push(const char *text, i32 x, i32 y, u8 color, i32 z)
{
    DrawListItem_ *item = draw_list_push_(CORE->draw_list, z, DrawListItemType_Text_);
    if (item) {
        size_t text_length = strlen(text) + 1;
        item->text.text = (char*)bank_push(CORE->stack, text_length);
        memcpy(item->text.text, text, text_length);
        item->text.x = x;
        item->text.y = y;
        item->text.color = color;
    }
}

//
// Bitmap
//

void
bitmap_init(Bitmap *bitmap, i32 width, i32 height, void *pixels, int bpp)
{
    bitmap->width = width;
    bitmap->height = height;

    Palette *palette = &CORE->canvas.palette;

    u32 size = width * height;
    bitmap->pixels = bank_push(CORE->storage, size);
    if (pixels) {
        if (bpp == PUN_BITMAP_32) {
            Color pixel;
            Color *pixels_end = ((Color *)pixels) + size;
            Color *pixels_it = pixels;

            u8 *it = bitmap->pixels;
            u8 ix;

            for (; pixels_it != pixels_end; ++pixels_it) {
                if (pixels_it->a < 0x7F) {
                    ix = PUN_COLOR_TRANSPARENT;
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

        } else if (bpp == PUN_BITMAP_8)  {
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

#if PUN_USE_STB_IMAGE

void
bitmap_load(Bitmap *bitmap, const char *path)
{
    // TODO: Treat path as relative to executable, not the CWD!
    int width, height, comp;
    Color *pixels = (Color *)stbi_load(path, &width, &height, &comp, STBI_rgb_alpha);
    ASSERT(pixels);
    ASSERT(comp == 4);

    bitmap_init(bitmap, (i32)width, (i32)height, pixels, PUN_BITMAP_32);
    free(pixels);
}

void
bitmap_load_resource(Bitmap *bitmap, const char *resource_name)
{
    size_t size;
    void *ptr = resource_get(resource_name, &size);
    ASSERT(ptr);
    int width, height, comp;
    Color *pixels = (Color *)stbi_load_from_memory(ptr, (int)size, &width, &height, &comp, STBI_rgb_alpha);
    ASSERT(pixels);
    ASSERT(comp == 4);

    bitmap_init(bitmap, (i32)width, (i32)height, pixels, PUN_BITMAP_32);
    free(pixels);
}

#endif


//
// Sound
//

#if PUN_USE_STB_VORBIS

static void
punp_sound_load_stbv(Sound *sound, stb_vorbis *stream)
{
    stb_vorbis_info info = stb_vorbis_get_info(stream);
    sound->volume = PUNP_SOUND_DEFAULT_SOUND_VOLUME;
    sound->rate = info.sample_rate;
    sound->channels = info.channels;
    sound->samples_count = stb_vorbis_stream_length_in_samples(stream);
    sound->samples = bank_push(CORE->storage, PUNP_SOUND_SAMPLES_TO_BYTES(sound->samples_count, sound->channels));

    static i16 buffer[1024];
    i16 *it = sound->samples;
    int samples_read_per_channel;
    for (;;) {
        int samples_read_per_channel =
            stb_vorbis_get_samples_short_interleaved(stream, PUNP_SOUND_CHANNELS, buffer, 1024);

        if (samples_read_per_channel == 0) {
            break;
        }

        // 2 channels, 16 bits per sample.
        memcpy(it, buffer, PUNP_SOUND_SAMPLES_TO_BYTES(samples_read_per_channel, sound->channels));
        it += samples_read_per_channel * sound->channels;
    }

    stb_vorbis_close(stream);
}

void
sound_load(Sound *sound, const char *path)
{
    memset(sound, 0, sizeof(Sound));

    int error = 0;
    stb_vorbis *stream = stb_vorbis_open_filename(path, &error, 0);
    ASSERT(!error && stream);

    punp_sound_load_stbv(sound, stream);

    size_t size = strlen(path) + 1;
    sound->name = bank_push(CORE->storage, size);
    memcpy(sound->name, path, size);
}

void
sound_load_resource(Sound *sound, const char *resource_name)
{
    memset(sound, 0, sizeof(Sound));

    size_t resource_size;
    void *resource_data = resource_get(resource_name, &resource_size);
    ASSERT(resource_data);

    int error = 0;
    stb_vorbis *stream =
            stb_vorbis_open_memory(resource_data, resource_size,
                                   &error, 0);

    ASSERT(!error && stream);

    punp_sound_load_stbv(sound, stream);

    size_t size = strlen(resource_name) + 1;
    sound->name = bank_push(CORE->storage, size);
    memcpy(sound->name, resource_name, size);
}

#endif // PUN_USE_STB_VORBIS

typedef struct PunPAudioSource
{
    Sound *sound;
    f32 rate;
    size_t position;
    struct PunPAudioSource *next;
}
PunPAudioSource;

static PunPAudioSource *punp_audio_source_pool = 0;
static PunPAudioSource *punp_audio_source_playback = 0;

void
sound_play(Sound *sound)
{
    if (sound->sources_count > 3) {
        printf("Too many same sounds playing at once.\n");
        return;
    }
    PunPAudioSource *source = 0;
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

    if (source)
    {
        memset(source, 0, sizeof(PunPAudioSource));
        sound->sources_count++;
        source->sound = sound;
        source->rate = (f32)sound->rate / (f32)PUN_SOUND_SAMPLE_RATE;
        source->next = punp_audio_source_playback;
        punp_audio_source_playback = source;
    }
}

//static inline i16 *
//punp_audio_source_sample(PunPAudioSource *source, size_t position)
//{
//    size_t resampled_position = (size_t)((f32)position * source->rate);
//    return source->sound->samples + ((source->position + resampled_position) * 2);
//}

#if PUNP_SOUND_DEBUG_FILE
static FILE *punp_audio_buf_file = 0;
static void
punp_win32_write_audio_buf(char *note, i16 *ptr, size_t size)
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
punp_sound_mix(i16 *buffer, size_t samples_count)
{
    BankState bank_state = bank_begin(CORE->stack);

    size_t size =  samples_count * sizeof(f32);
    f32 *channel0 = bank_push(CORE->stack, size);
    f32 *channel1 = bank_push(CORE->stack, size);

    f32 *it0;
    f32 *it1;
    size_t i;

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

    size_t sound_samples;
    size_t sound_samples_remaining;
    size_t loop_samples;
	Sound *sound = 0;
    PunPAudioSource **it = &punp_audio_source_playback;
    PunPAudioSource *source;

    while (*it)
    {
        source = *it;

        i16 *sample;
        it0 = channel0;
        it1 = channel1;

        sound = source->sound;

		loop_samples = samples_count;

loop:;
        sound_samples = loop_samples;
        sound_samples_remaining = (sound->samples_count - source->position) / source->rate;
        if (sound_samples > sound_samples_remaining) {
            sound_samples = sound_samples_remaining;
        }

        if (sound->channels == 2)
        {
            for (i = 0; i != sound_samples; ++i)
            {
                sample = &sound->samples[(source->position + (size_t)((f32)i * source->rate)) * 2];
                //sample = punp_audio_source_sample(source, i);
                *it0++ += sample[0] * sound->volume;
                *it1++ += sample[1] * sound->volume;
            }
        }
        else if (sound->channels == 1)
        {
            for (i = 0; i != sound_samples; ++i)
            {
                sample = &sound->samples[(source->position + (size_t)((f32)i * source->rate))];
                //sample = punp_audio_source_sample(source, i);
                *it0++ += sample[0] * sound->volume;
                *it1++ += sample[0] * sound->volume;
            }
        }
        else
        {
            ASSERT_MESSAGE(0, "Invalid count of channels (2 or 1 are supported).");
        }

        source->position += maximum(1, sound_samples * source->rate);

        // printf("Sound %s position %d count %d played %f\n",
        //     sound->name,
        //     source->position,
        //     sound->samples_count,
        //     sound_samples * source->rate);
        
        ASSERT(source->position <= sound->samples_count);
        if (source->position == sound->samples_count)
        {
            if (sound->flags & SoundFlag_Loop) {
                source->position = 0;
                ASSERT(loop_samples >= sound_samples);
                loop_samples -= sound_samples;
                if (loop_samples == 0) {
                    // printf("Finished looping in buffer.\n");
                } else {
                    // printf("Looping in buffer. %d\n", loop_samples);
                    goto loop;
                }
            } else {
                *it = source->next;
                sound->sources_count--;
                source->next = punp_audio_source_pool;
                punp_audio_source_pool = source;
            }
            // printf("Stopping sound `%s`\n", sound->name);
        }
        else
        {
            it = &source->next;
        }
    }

    //
    // Put to output buffer, clamp and convert to 16-bit.
    //

// #define MIX_CLIP_(in, clip) (0.5 * (abs(in + clip) - abs(in - clip)) * CORE->audio_volume)

	it0 = channel0;
	it1 = channel1;
	f32 s1, s2;
	i16 *it_buffer = buffer;
	for (i = 0; i != samples_count; ++i)
    {
		s1 = *it0++ * CORE->audio_volume;
		s2 = *it1++ * CORE->audio_volume;
		*it_buffer++ = (i16)clamp(s1, -32768, 32767);
		*it_buffer++ = (i16)clamp(s2, -32768, 32767);
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
#if PUN_PLATFORM_WINDOWS
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
win32_app_key_mods()
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
            if (wp < PUN_KEYS_MAX) {
                CORE->key_states[wp] = 1;
                CORE->key_deltas[wp] = 1;
                // printf("key pressed: %d\n", wp);
            }
            return 0;
        }
            break;

        case WM_KEYUP: {
            CORE->key_modifiers = win32_app_key_mods();
            if (wp < PUN_KEYS_MAX) {
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
perf_get()
{
    i64 counter;
    QueryPerformanceCounter((LARGE_INTEGER *)&counter);
    return (f64)((f64)counter / (f64)punp_win32_perf_counter_frequency); // *(1e3);
}

void *
resource_get(const char *name, size_t *size)
{
    HRSRC handle = FindResource(0, name, "RESOURCE");
    ASSERT(handle);

    HGLOBAL data = LoadResource(0, handle);
    ASSERT(data);

    void *ptr = LockResource(data);
    ASSERT(ptr);

    DWORD t_size = SizeofResource(0, handle);
    ASSERT(t_size);

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

static bool
punp_win32_sound_init()
{
    HRESULT hr;

    HMODULE dsound_lib = LoadLibraryA("dsound.dll");
    if (!dsound_lib) {
        printf("LoadLibrary(dsound.dll) failed.\n");
        return 0;
    }

    DirectSoundCreate8F DirectSoundCreate8 =
            (DirectSoundCreate8F)GetProcAddress(dsound_lib, "DirectSoundCreate8");
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

    DSBUFFERDESC primary_buffer_description = {0};
    primary_buffer_description.dwSize = sizeof(primary_buffer_description);
    primary_buffer_description.dwFlags = DSBCAPS_PRIMARYBUFFER;

    LPDIRECTSOUNDBUFFER primary_buffer;
    hr = IDirectSound8_CreateSoundBuffer(punp_win32_direct_sound, &primary_buffer_description, &primary_buffer, 0);
    if (hr != DS_OK) {
        printf("punp_win32_direct_sound->CreateSoundBuffer for primary buffer failed.\n");
        return 0;
    }

    punp_win32_audio_format.wFormatTag = WAVE_FORMAT_PCM;
    punp_win32_audio_format.nChannels = PUNP_SOUND_CHANNELS;
    punp_win32_audio_format.nSamplesPerSec = PUN_SOUND_SAMPLE_RATE;
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

    hr = IDirectSoundBuffer8_Play(punp_win32_audio_buffer, 0, 0, DSBPLAY_LOOPING);

    if (hr != DS_OK)
    {
        ASSERT(0);
    }
    return 1;
}

static DWORD punp_win32_audio_cursor = 0;

static void
punp_win32_sound_step()
{
//  if (!punp_audio_source_playback) {
//      return;
//  }

    HRESULT hr;

    DWORD cursor_play, cursor_write;
    hr = IDirectSoundBuffer8_GetCurrentPosition(punp_win32_audio_buffer,
                                                &cursor_play, &cursor_write);

    if (FAILED(hr)) {
        printf("IDirectSoundBuffer8_GetCurrentPosition failed.\n");
        return;
    }

    u32 chunk_size = PUNP_SOUND_SAMPLES_TO_BYTES(PUNP_SOUND_BUFFER_CHUNK_SAMPLES, PUNP_SOUND_CHANNELS);
    u32 chunk = cursor_write / chunk_size;

    DWORD lock_cursor = ((chunk+1) * chunk_size) % PUNP_SOUND_BUFFER_BYTES;
    DWORD lock_size = chunk_size;

    if (lock_cursor == punp_win32_audio_cursor) {
        return;
    }

    VOID *range1, *range2;
    DWORD range1_size, range2_size;
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

    punp_sound_mix(range1, PUNP_SOUND_BYTES_TO_SAMPLES(range1_size, PUNP_SOUND_CHANNELS));
#if PUNP_SOUND_DEBUG_FILE
    punp_win32_write_audio_buf("range1", range1, range1_size);
#endif
    if (range2) {
        punp_sound_mix(range2, PUNP_SOUND_BYTES_TO_SAMPLES(range2_size, PUNP_SOUND_CHANNELS));
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

#if PUN_MAIN
int CALLBACK
WinMain(HINSTANCE instance, HINSTANCE prev_instance, LPSTR command_line, int show_code)
{
    if (align_to(PUN_CANVAS_WIDTH, 16) != PUN_CANVAS_WIDTH) {
		ASSERT_MESSAGE(0, "PUN_CANVAS_WIDTH must be aligned to 16.");
        return 1;
    }

    static Bank s_stack = {0};
    static Bank s_storage = {0};
    static Core s_core = {0};

    CORE = &s_core;
    memset(CORE, 0, sizeof(Core));

    CORE->running = 1;
    CORE->stack = &s_stack;
    CORE->storage = &s_storage;

    bank_init(CORE->stack, PUN_STACK_CAPACITY);
    bank_init(CORE->storage, PUN_STORAGE_CAPACITY);

    // TODO: Push canvas to storage? Storage is not initialized yet, so we cannot push it there.
    static Bitmap s_canvas_bitmap = {0};
    CORE->canvas.bitmap = &s_canvas_bitmap;
    bitmap_init(CORE->canvas.bitmap, PUN_CANVAS_WIDTH, PUN_CANVAS_HEIGHT, 0, 0);
    bitmap_clear(CORE->canvas.bitmap, PUN_COLOR_TRANSPARENT);
    clip_reset();

    static DrawList s_draw_list = {0};
    CORE->draw_list = &s_draw_list;
    draw_list_init(CORE->draw_list, PUN_DRAW_LIST_RESERVE);

    CORE->audio_volume = PUNP_SOUND_DEFAULT_MASTER_VOLUME;

    //
    //
    //

    punp_win32_instance = instance;
    QueryPerformanceFrequency((LARGE_INTEGER *)&punp_win32_perf_counter_frequency);

    UINT desired_sleep_ms = 1;
    bool sleep_is_granular = (timeBeginPeriod(desired_sleep_ms) == TIMERR_NOERROR);

#ifndef RELEASE_BUILD
    printf("Debug build...");
    if (AttachConsole(ATTACH_PARENT_PROCESS) || AllocConsole()) {
       freopen("CONOUT$", "w", stdout);
       freopen("CONOUT$", "w", stderr);
    }
#else
    printf("Release build...");
    // FreeConsole();
#endif

    WNDCLASSA window_class = {0};
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

    int screen_width = GetSystemMetrics(SM_CXSCREEN);
    int screen_height = GetSystemMetrics(SM_CYSCREEN);


    RECT rc;
    rc.left = (screen_width - (PUNP_WINDOW_WIDTH)) / 2;
    rc.top = (screen_height - (PUNP_WINDOW_HEIGHT)) / 2;
    rc.right = rc.left + PUNP_WINDOW_WIDTH;
    rc.bottom = rc.top + PUNP_WINDOW_HEIGHT;


    DWORD style = WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
    assert(AdjustWindowRect(&rc, style, FALSE) != 0);

    // int window_width  = rc.right - rc.left;
    // int window_height = rc.bottom - rc.top;
    // rc.left = (screen_width  - width) / 2;
    // rc.top  = (screen_height - height) / 2;

    punp_win32_window = CreateWindowExA(
            0,
            window_class.lpszClassName,
            PUN_WINDOW_TITLE,
            style,
            rc.left, rc.top,
            rc.right - rc.left, rc.bottom - rc.top,
            0, 0,
            punp_win32_instance,
            0);

    if (!punp_win32_window) {
        printf("CreateWindowExA failed.\n");
        return 1;
    }

    // Canvas

    CORE->canvas.palette.colors[PUN_COLOR_TRANSPARENT] =
        color_make(0x00, 0x00, 0x00, 0x00);
    CORE->canvas.palette.colors[PUN_COLOR_BLACK] =
        color_make(0x00, 0x00, 0x00, 0xff);
    CORE->canvas.palette.colors[PUN_COLOR_WHITE] =
        color_make(0xff, 0xff, 0xff, 0xff);
    CORE->canvas.palette.colors_count = 3;

    u32 *window_buffer;

    BITMAPINFO window_bmi = {0};
    window_bmi.bmiHeader.biSize = sizeof(window_bmi.bmiHeader);
    window_bmi.bmiHeader.biWidth = PUN_CANVAS_WIDTH;
    window_bmi.bmiHeader.biHeight = PUN_CANVAS_HEIGHT;
    window_bmi.bmiHeader.biPlanes = 1;
    window_bmi.bmiHeader.biBitCount = 32;
    window_bmi.bmiHeader.biCompression = BI_RGB;

    window_buffer = bank_push(CORE->stack, (PUN_CANVAS_WIDTH * 4) * PUN_CANVAS_HEIGHT);
    assert(window_buffer);


    // Sound

    if (punp_win32_sound_init() == 0) {
        punp_win32_audio_buffer = 0;
    }


    init();


    // TODO: Center window
    ShowWindow(punp_win32_window, SW_SHOW);

    f64 frame_time_stamp, frame_time_now, frame_time_delta;
    int x, y;
    u32 *window_row;
    u8 *canvas_it;
    BankState stack_state;
    MSG message;
    perf_from(&CORE->perf_frame);
    while (CORE->running)
    {
        perf_to(&CORE->perf_frame);
        perf_from(&CORE->perf_frame_inner);

        stack_state = bank_begin(CORE->stack);

        memset(&CORE->key_deltas, 0, PUN_KEYS_MAX);

        while (PeekMessage(&message, 0, 0, 0, PM_REMOVE)) {
            if (message.message == WM_QUIT) {
                CORE->running = 0;
            }

            TranslateMessage(&message);
            DispatchMessageA(&message);
        }

        perf_from(&CORE->perf_step);
        draw_list_begin(CORE->draw_list);
        step();
        draw_list_end(CORE->draw_list);
        perf_to(&CORE->perf_step);

        perf_from(&CORE->perf_audio);
        if (punp_win32_audio_buffer) {
            punp_win32_sound_step();
        }
        perf_to(&CORE->perf_audio);

        perf_from(&CORE->perf_blit);
        perf_from(&CORE->perf_blit_cvt);
        canvas_it = CORE->canvas.bitmap->pixels;
        for (y = PUN_CANVAS_HEIGHT; y != 0; --y) {
            window_row = window_buffer + ((y - 1) * PUN_CANVAS_WIDTH);
            for (x = 0; x != PUN_CANVAS_WIDTH; ++x) {
                *(window_row++) = CORE->canvas.palette.colors[*canvas_it++].rgba;
            }
        }
        perf_to(&CORE->perf_blit_cvt);

        perf_from(&CORE->perf_blit_gdi);
        HDC dc = GetDC(punp_win32_window);
#if 1
        // TODO: This is sadly slow (50us on my machine), need to find a faster way to do this.
        StretchDIBits(dc,
                      0, 0, PUN_CANVAS_WIDTH * PUN_CANVAS_SCALE, PUN_CANVAS_HEIGHT * PUN_CANVAS_SCALE,
                      0, 0, PUN_CANVAS_WIDTH, PUN_CANVAS_HEIGHT,
                      window_buffer,
                      &window_bmi,
                      DIB_RGB_COLORS,
                      SRCCOPY);
#else
#endif
        ReleaseDC(punp_win32_window, dc);
        perf_to(&CORE->perf_blit_gdi);
        perf_to(&CORE->perf_blit);

        perf_to(&CORE->perf_frame_inner);

#if 0
        printf("stack %d storage %d\n",
               CORE->stack->it - CORE->stack->begin,
               CORE->storage->it - CORE->storage->begin);
#endif
        // if (stack_state.state.it != CORE->stack->it) {
        //     printf("Stack memory bank was not cleared.\n");
        // }
        bank_end(&stack_state);

        f32 frame_delta = perf_delta(&CORE->perf_frame);
        if (frame_delta < PUNP_FRAME_TIME) {
            // printf("sleeping ... %.3f\n", (f32)PUNP_FRAME_TIME - frame_delta);
            Sleep((PUNP_FRAME_TIME - frame_delta) * 1e3);
        }
        CORE->frame++;
    }

#if PUNP_SOUND_DEBUG_FILE
    fclose(punp_audio_buf_file);
#endif

    return 0;
}
#endif

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