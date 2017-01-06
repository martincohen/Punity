/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2016 Martin Cohen
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */ 

/*

Library to write animated GIF supporting all GIF features. It's made to not do
any fancy stuff outside of raw GIF writing, so don't expect any palette
quantizations or dithering (those might appear later as separate libraries).

I made the library to have fast GIF recording in my tiny game engine Punity,
as other choices were really slow because of extra features I didn't actually
need.

The library takes frame data (raw pixels, palette, frame properties) and does
a buffered stream output through a callback function. The buffer size is 8k by default,
but you can change it with:

#define GIFW_WRITE_BUFFER_SIZE (...)

Usage:

// header only
#include "gifw.h"

// header+implementation
#define GIFW_IMPLEMENTATION
#include "gifw.h"

*/

#ifndef GIFW_H
#define GIFW_H

#ifndef GIFW_WRITE_BUFFER_SIZE
#define GIFW_WRITE_BUFFER_SIZE (8192)
#endif

#include <stdint.h>

typedef struct GIFWColor_
{
    uint8_t r, g, b;
}
GIFWColor;

typedef struct GIFWColorTable_
{
    GIFWColor colors[256];
    int count;
}
GIFWColorTable;

#define GIF_LZW_HASH_SIZE 5003

typedef struct GIFWLZWState_
{
    // LZW dictionary.
    int16_t entries[GIF_LZW_HASH_SIZE];
    // LZW dictionary cache.
    int32_t hash[GIF_LZW_HASH_SIZE];

    uint8_t buffer[256];
    uint8_t buffer_size;
    
    // Bits are being added here.
    uint32_t bits;
    // Number of bits in `bits`.
    uint32_t bits_count;
    // Current size of code being stored.
    uint32_t bits_size;
}
GIFWLZWState;

typedef struct GIFW GIFW;

#define GIFW_WRITE_CALLBACK(name) int name(GIFW *gif, uint8_t *begin, uint8_t *end, void *user)
typedef GIFW_WRITE_CALLBACK(GIFWWriteCallbackF);

// GIFW storage.
struct GIFW
{
    int width;
    int height;
    int repeat;

    int frames_count;
    GIFWLZWState state;

    uint8_t buffer[GIFW_WRITE_BUFFER_SIZE];
    int buffer_size;
    GIFWWriteCallbackF *callback;
    void *callback_user;
};

enum {
    // No disposal specified. The decoder is
    // not required to take any action.
    // Use this as default.
    GIFWFrameDispose_NotSpecified = 0,

    // The graphic is to be left in place.
    GIFWFrameDispose_DoNotDispose = 1,

    // Restore to background color. The area used by the
    // graphic must be restored to the background color.
    GIFWFrameDispose_RestoreBackground = 2,

    // Restore to previous. The decoder is required to
    // restore the area overwritten by the graphic with
    // what was there prior to rendering the graphic.
    GIFWFrameDispose_RestorePrevious = 3,
};

// Writes GIF header.
//
void gifw_begin
(
    GIFW *G,

    // Dimensions of the GIF.
    int width, int height,

    // Number of repeats
    // 0 - repeat forever
    // 1 - repeat once
    // 2 - repeat twice
    // ...
    int repeat,

    // Optional global color table
    // If set to 0, then you must provide a color_table
    // for each frame (see gifw_frame).
    GIFWColorTable *color_table,

    // Index to color_table for a background color.
    uint8_t background_color_index,

    // Stream writing callback and optional user data passed to the callback.
    GIFWWriteCallbackF *callback, void *callback_user);

// Compresses and stores the frame passed.
//
void gifw_frame
(
    GIFW *G,
    
    // Pixels for the frame.
    uint8_t *pixels,
    
    // Optional frame color table.
    // If set to 0, then you must provide global color table via gifw_begin.
    GIFWColorTable *color_table,
    
    // Subframe position and dimensions.
    // This is useful if you only want to store portions that have
    // changed since previous frame. If that's what you're looking for,
    // then you also must set the dispose correctly (see enum above).
    uint16_t left,  uint16_t top,
    uint16_t width, uint16_t height,
    
    // Delay is set in centiseconds.
    // 1 = 10 ms
    uint16_t delay,

    // Disposal method, see GIFWFrameDispose_* constants in enum above.
    uint8_t dispose,

    // Set to 1 if you want to provide a transparent color index,
    int transparent_color_index_flag,

    // Index of a color in color_table that is treated as transparent.
    // Note that `transparent_color_index_flag` has to be set to 1.
    uint8_t transparent_color_index
);


// Finalizes saving the GIF image and flushes the buffer.
//
void gifw_end(GIFW *G);


#endif


//
//
//
#ifdef GIFW_IMPLEMENTATION
#undef GIFW_IMPLEMENTATION
//
//
//

#include <string.h>

static unsigned int
gifw_log2_(unsigned int v)
{
    register unsigned int r;
    register unsigned int shift;

    r =     (v > 0xFFFF) << 4; v >>= r;
    shift = (v > 0xFF  ) << 3; v >>= shift; r |= shift;
    shift = (v > 0xF   ) << 2; v >>= shift; r |= shift;
    shift = (v > 0x3   ) << 1; v >>= shift; r |= shift;
                                            r |= (v >> 1);

    return r;
}

static void
gifw_flush_(GIFW *gif)
{
    if (gif->buffer_size != 0) {
        gif->callback(gif, gif->buffer, gif->buffer + gif->buffer_size, gif->callback_user);
        gif->buffer_size = 0;
    }
}

static void
gifw_write_(GIFW *gif, void *data, int size)
{
    int remaining;
    while (size)
    {
        remaining = GIFW_WRITE_BUFFER_SIZE - gif->buffer_size;
        if (!remaining) {
            gifw_flush_(gif);
            remaining = GIFW_WRITE_BUFFER_SIZE;
        }

        remaining = remaining > size ? size : remaining;
        memmove(gif->buffer + gif->buffer_size, data, remaining);

        gif->buffer_size += remaining;
        size -= remaining;
        data = ((uint8_t*)data) + remaining;
        if (size == 0) {
            break;
        }
    }
}

static void
gifw_write_string_(GIFW *gif, const char *string, int length)
{
    gifw_write_(gif, (void*)string, length);
}

#define GIFW_WRITE_(Name, Type) \
    static inline void gifw_write_##Name##_(GIFW *G, Type v) \
    { gifw_write_(G, &v, sizeof(Type)); }

GIFW_WRITE_(u8, uint8_t);
GIFW_WRITE_(i8, int8_t);
GIFW_WRITE_(i16, int16_t);
GIFW_WRITE_(u16, uint16_t);
GIFW_WRITE_(i32, int32_t);
GIFW_WRITE_(u32, uint32_t);

uint32_t
gifw_next_pow2_(uint32_t n)
{
    // NOTE: Returns 0 for n == 0.
    n--;
    n |= n >> 1;   // Divide by 2^k for consecutive doublings of k up to 32,
    n |= n >> 2;   // and then or the results.
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;
    n++;
    return n;
}

void
gifw_begin(GIFW *G, int width, int height, int repeat,
    GIFWColorTable *color_table,
    uint8_t background_color_index,
    GIFWWriteCallbackF *callback, void *callback_user)
{
    memset(G, 0, sizeof(GIFW));

    G->frames_count = 0;
    G->width = width;
    G->height = height;
    G->repeat = repeat;
    G->callback = callback;
    G->callback_user = callback_user;

    gifw_write_string_(G, "GIF89a", 6);

    gifw_write_u16_(G, G->width);
    gifw_write_u16_(G, G->height);

    // uint8_t color_table_present : 1;
    // uint8_t color_resolution : 3;
    // uint8_t sort_flag : 1;
    // uint8_t color_table_size : 3;
    uint8_t packed = 0;

    // Color resolution.
    // 8 bits per channel minus one.
    packed |= (8-1) << 4;
    
    // Color table size.
    // Calculated from this value with: 2^(1+n).
    uint32_t colors_count = 0;
    if (color_table)
    {
        colors_count = color_table->count;
        if (colors_count < 2) {
            colors_count = 2;
        } else if (colors_count > 256) {
            colors_count = 256;
        }
        colors_count = gifw_next_pow2_(colors_count);

        // Color table present.
        packed |= (uint32_t)1 << 7;
        // Color table size.
        packed |= ((uint32_t)gifw_log2_(colors_count) - 1) & 0x7;
    }
    
    gifw_write_u8_(G, packed); 

    // Background color index.
    // This is only meaningful if global color table is present.
    gifw_write_u8_(G, background_color_index); 
    // Pixel aspect ratio
    // Set to 0 for no aspect information is given.
    // See "viii) Pixel Aspect Ratio" in https://www.w3.org/Graphics/GIFW/spec-gif89a.txt.
    gifw_write_u8_(G, 0); 

    // Global color table.
    if (color_table) {
        gifw_write_(G, color_table->colors, colors_count * 3);
    }

    // Application extension to store repeat value.
    if (G->repeat >= 0) {
        gifw_write_string_(G, "\x21\xff\x0bNETSCAPE2.0\x03\x01", 16);
        gifw_write_u16_(G, G->repeat);
        gifw_write_u8_(G, 0);
    }
}

void
gifw_bits_flush_(GIFW *G)
{
    if (G->state.buffer_size != 0)
    {
        // Write count of bytes in the block.
        gifw_write_u8_(G, G->state.buffer_size);
        // Write bytes in the block. 
        gifw_write_(G, G->state.buffer, G->state.buffer_size);
        
        G->state.buffer_size = 0;
    }
}

void
gifw_bits_push_(GIFW *G, uint32_t bits)
{
    GIFWLZWState *state = &G->state;
    state->bits |= bits << state->bits_count;
    state->bits_count += state->bits_size;
    while (state->bits_count >= 8)
    {
        state->buffer[state->buffer_size++] = state->bits & 0xFF;
        state->bits >>= 8;
        state->bits_count -= 8;
        if (state->buffer_size == 0xFF) {
            gifw_bits_flush_(G);
        }
    }
}

void
gifw_write_frame_raster_(GIFW *G, uint8_t *pixels, int width, int height)
{
    GIFWLZWState *state = &G->state;
    memset(state, 0, sizeof(GIFWLZWState));
    memset(state->hash, 0xFF, sizeof(state->hash));

    // LZW Minimum Code Size
    gifw_write_u8_(G, 8);
    state->bits_size = 9;

    // Clear.
    gifw_bits_push_(G, 0x100);

    uint8_t *it  = pixels;
    uint8_t *end = pixels + (width * height);

    uint32_t entry_max = 511;
    uint32_t entry_count = 0x102;
    int32_t  hash_value, hash_key;

    uint32_t entry = *it++;
    uint32_t entry_next;

repeat:
    while (it != end)
    { 
        entry_next = *it++;
        // We store hash value as a pair of 2 entries (entry and entry_next).
        // Each entry can be of maximum 12-bits.
        hash_value  = ((entry_next << 12) + entry);
        hash_key    = ((entry_next <<  4) ^ entry) % GIF_LZW_HASH_SIZE;
        while (state->hash[hash_key] >= 0) {
            if (state->hash[hash_key] == hash_value) {
                entry = state->entries[hash_key];
                goto repeat;
            }
            hash_key = (hash_key + 1) % GIF_LZW_HASH_SIZE;
        }

        gifw_bits_push_(G, entry);
        entry = entry_next;

        if (entry_count < 4096)
        {
            if (entry_count > entry_max) {
                // Increase the code size.
                ++state->bits_size;
                if (state->bits_size == 12) {
                    entry_max = 4096;
                } else {
                    entry_max = (1 << state->bits_size) - 1;
                }
            }

            // Write entry index to entries.
            state->entries[hash_key] = entry_count++;
            // Write the pair of entry and entry_next to the hash.
            // under the same key.
            state->hash[hash_key]  = hash_value;
        }
        else
        {
            // Clear if we reached max size of dictionary.
            memset(state->hash, 0xFF, sizeof(state->hash));
            gifw_bits_push_(G, 0x100);
            state->bits_size = 9;
            entry_count = 0x102;
            entry_max = 511;
        }
    }
    // Push last entry.
    gifw_bits_push_(G, entry);
    // Push STOP.
    gifw_bits_push_(G, 0x101);
    gifw_bits_push_(G, 0);

    // Flush remaining bits.
    gifw_bits_flush_(G);
}

void
gifw_frame(GIFW *G,
    uint8_t *pixels,
    GIFWColorTable *color_table,
    uint16_t left,  uint16_t top,
    uint16_t width, uint16_t height,
    uint16_t delay, uint8_t dispose,
    int transparent_color_index_flag,
    uint8_t transparent_color_index)
{
    G->frames_count++;

    uint8_t packed = 0;

    // GCE Introducer.
    gifw_write_u8_(G, 0x21);
    // GCE Label
    gifw_write_u8_(G, 0xF9);
    // GCE Block size.
    gifw_write_u8_(G, 0x04);
    
    // GCE Packed
    // uint8_t  gce_reserved : 3;
    // uint8_t  gce_disposal_method : 3;
    // uint8_t  gce_user_input_flag : 1;
    // uint8_t  gce_transparent_color_flag : 1;
    // header.gce_disposal_method = GIFDisposal_None;
    
    // Disposal method
    packed |= (dispose & 0x7) << 2;
    // Transparent color flag
    packed |= transparent_color_index_flag & 1;

    gifw_write_u8_(G, packed);

    // GCE Delay time.
    gifw_write_u16_(G, delay);
    // GCE Transparent color index.
    gifw_write_u8_(G, transparent_color_index);
    // GCE Block terminator
    gifw_write_u8_(G, 0);

    // Image separator
    gifw_write_u8_(G, 0x2C);
    // Left
    gifw_write_u16_(G, left);
    // Right
    gifw_write_u16_(G, top);
    // Width
    gifw_write_u16_(G, width);
    // Height
    gifw_write_u16_(G, height);

    // uint8_t  color_table_present : 1;
    // uint8_t  interlace_flag : 1;
    // uint8_t  sort_flag : 1;
    // uint8_t  reserved : 2;
    // uint8_t  color_table_size : 3;
    
    packed = 0;

    uint32_t colors_count = 0;
    if (color_table)
    {
        colors_count = color_table->count;
        if (colors_count < 2) {
            colors_count = 2;
        } else if (colors_count > 256) {
            colors_count = 256;
        }
        colors_count = gifw_next_pow2_(colors_count);

        // Color table present.
        packed |= (uint32_t)1 << 7;
        // Color table size.
        packed |= ((uint32_t)gifw_log2_(colors_count) - 1) & 0x7;
    }

    gifw_write_u8_(G, packed);

    if (color_table) {
        gifw_write_(G, color_table->colors, colors_count * 3);
    }

    gifw_write_frame_raster_(G, pixels, width, height);

    // Frame terminator.
    gifw_write_u8_(G, 0);
}

void
gifw_end(GIFW *G)
{
    gifw_write_u8_(G, 0x3B);
    gifw_flush_(G);
}

#endif