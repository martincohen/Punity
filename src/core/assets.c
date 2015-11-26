#include <stdio.h>
#include <string.h>

#include "core/core.h"

#define STB_IMAGE_IMPLEMENTATION
#include "lib/stb_image.h"

internal char *
strpush(char *dest, char *src)
{
    while (*src) {
        *dest++ = *src++;
    }
    return dest;
}

typedef struct Buffer
{
    u8 *ptr;
    memi count;
    memi capacity;
}
Buffer;

void
buffer_realloc(Buffer *buffer, memi capacity)
{
    buffer->ptr = realloc(buffer->ptr, capacity);
    buffer->capacity = capacity;
}

void *
_buffer_push(Buffer *buffer, memi count, memi capacity)
{
    memi c = buffer->count + count;
    if (c > buffer->capacity) {
        assert(capacity >= count);
        buffer_realloc(buffer, buffer->capacity + capacity);
    }

    void *ptr = buffer->ptr + buffer->count;
    buffer->count = c;
    return ptr;
}

#define buffer_push(buffer, type, count, capacity) \
    ((type *)_buffer_push(buffer, count * sizeof(type), capacity * sizeof(type)))

#define WRITE_ASSET_HEADER(Type) \
{\
    Type *asset = (Type *)head; \
    ((Asset*)asset)->size = data_size; \
    ((Asset*)asset)->type = type; \
    \
    u32 type_string_length = strlen(#Type); \
    fwrite(#Type, 1, type_string_length, out->binary); \
    out->binary_offset += type_string_length; \
    \
    fwrite(asset, 1, sizeof(Type), out->binary); \
    fprintf(out->header, #Type "* %s = ((" #Type " *)(_res + 0x%.8x));\n", id, out->binary_offset); \
    fprintf(out->header, "#define _%s ((" #Type " *)(_res + 0x%.8x))\n", id, out->binary_offset); \
    \
    out->binary_offset += sizeof(Type); \
}



internal void
write(AssetWrite *out, AssetType type, char *id, void *head, void *data, u32 data_size)
{
    switch (type)
    {
    case Asset_Raw:
        WRITE_ASSET_HEADER(Asset);
        break;
    case Asset_Palette:
        WRITE_ASSET_HEADER(Palette);
        break;
    case Asset_Image:
        WRITE_ASSET_HEADER(Image);
        break;
    case Asset_SpriteList:
        WRITE_ASSET_HEADER(ImageSet);
        break;
    case Asset_TileMap:
        WRITE_ASSET_HEADER(TileMap);
        break;
    default:
        assert(0 && "Not supported.");
        break;
    }

    if (data) {
        fwrite(data, 1, data_size, out->binary);
        out->binary_offset += data_size;
    }
}

typedef struct
{
    Image image;
    u8 *data;
} ReadImage;

internal u8 *
read_image(char *path, Image *image, Palette *palette)
{
    int w, h, comp;
    Color *pixels = (Color*)stbi_load(path, &w, &h, &comp, STBI_rgb_alpha);
    assert(pixels);

    memi size = w*h;
    u8 *indexed = malloc(size);
    memset(indexed, 0, size);

    u8 ix;
    Color *pixels_end = pixels + size;
    Color *pixels_it = pixels;
    u8 *indexed_it = indexed;
    for (;
         pixels_it != pixels_end;
         ++pixels_it)
    {
        ix = 0;
        if (pixels_it->a < 0x7F) {
            pixels_it->a = 0;
            goto next;
        }

        pixels_it->a = 0xFF;

        for (ix = 1; ix < palette->colors_count; ix++) {
            if (palette->colors[ix].rgba == pixels_it->rgba)
                goto next;
        }

        // Add the color.

        assert(palette->colors_count != 0xFF);
        if (palette->colors_count != 0xFF) {
            ix = palette->colors_count;
            palette->colors[ix] = *pixels_it;
            palette->colors_count++;
            printf("[palette] + #%0.2x %0.2x%0.2x%0.2x%0.2x\n",
                   ix,
                   pixels_it->r,
                   pixels_it->g,
                   pixels_it->b,
                   pixels_it->a);
        }
next:
        // Write the indexed color.

        *indexed_it++ = ix;
    }
    free(pixels);

    image->w = w;
    image->h = h;

    return indexed;
}

internal void
_write_image(AssetWrite *out, char *id, char *path, u32 flags, u8 palette_only)
{
    Image image = {0};
    u8 *data = read_image(path, &image, &out->palette);
    if (palette_only) {
        printf("  palette only\n");
    } else {
        printf("  width %d, height %d\n", image.w, image.h);
        write(out, Asset_Image, id, &image, data, image.w * image.h);
    }
    free(data);
}

internal void
_write_image_set(AssetWrite *out, char *id, char *path, u32 cw, u32 ch, i32 pivot_x, i32 pivot_y, V4i padding)
{
    assert(cw <= 0xFF);
    assert(ch <= 0xFF);

    ImageSet set = {0};
    Image image = {0};
    u8 *data = read_image(path, &image, &out->palette);
    set.padding = padding;

    set.pivot_x = pivot_x;
    set.pivot_y = pivot_y;
    set.cw = cw;
    set.ch = ch;
    memi rows = image.h / set.ch;
    memi cols = image.w / set.cw;
    assert(rows * cols <= 0x10000);
    set.count = rows * cols;

    memi size = image.w * image.h;
    u8 *linear = malloc(size);

    // data
    // o0 o1 o2 o3
    // o4 o5 o6 o7
    // o8 o9 10 11
    // 12 13 14 15

    // linear
    // o0 o4 o8 12
    // o1 o5 o9 13

    memi sprite_size = set.cw * set.ch;
    for (memi r = 0; r != rows; ++r) {
        for (memi c = 0; c != cols; ++c) {
            for (memi y = 0; y != set.ch; y++) {
                memi l_offset = ((c + (r * cols)) * sprite_size) + (y * set.cw);
                memi d_offset = (c * set.cw) + (r * sprite_size * cols) + (y * image.w);
                // printf("in %d out %d\n", d_offset, l_offset);
                memcpy(linear + l_offset,
                       data + d_offset,
                       set.cw);
            }
        }
    }

    write(out, Asset_SpriteList, id, &set, linear, size);

    free(data);
    free(linear);
}

void
_write_c(AssetWrite *out, char *id, char *path)
{
    IOReadResult f = io_read(path);
    assert(f.data);
    fwrite(f.data, 1, f.size, out->header);
    fprintf(out->header, "\n\n");
    free(f.data);
}

void
_write_raw(AssetWrite *out, char *id, char *path)
{
    IOReadResult f = io_read(path);
    assert(f.data);
    write(out, Asset_Raw, id, NULL, f.data, f.size);
    free(f.data);
}

void
_write_tile_map(AssetWrite *out, char *id, char *path)
{
    IOReadResult f = io_read(path);
    assert(f.data);

    Buffer data = {0};

    TileMap map = {0};

    u32 n = 0;
    i32 m = 1;
    char *it = f.data;
    while (*it)
    {
        switch (*it)
        {
        case ',':
        case '\r':
            break;

        case '\n':
            if (*it == '\n') {
                if (map.h == 0) {
                    map.w = data.count / sizeof(u16);
                }
                map.h++;
            }
            break;

        default:
            m = 1;
            if (*it == '-')
            {
                m = -1;
                it++;
            }

            assert(is_number(*it));
            do {
                n = (n * 10) + (*it - '0');
                it++;
            } while (is_number(*it));

            *buffer_push(&data, u16, 1, 1024) = (u16)(m == -1 ? 0xFFFF : n);
            n = 0;
            goto next;
            break;
        }
        it++;
next:;
    }
    assert(map.w);
    assert(map.h);

    write(out, Asset_TileMap, id, &map, data.ptr, data.count);

    free(data.ptr);
}

//
//
//

void
make_path(char *name, char *path)
{
    path = strpush(path, ASSETS_DIR "res/");
    path = strpush(path, name);
    *path = 0;
}

void
make_id(char *name, char *id)
{
    *id = 0;
    id = strpush(id, "asset_");

    while (*name)
    {
        switch (*name)
        {
        case '.':
            goto finish;

        default:
            *id++ = *name;
            break;
        }

        name++;
    }

finish:
    *id = 0;
}

#define _ASSET_PATH_AND_ID \
    char path[1024];\
    char id[256];\
    make_path(name, path);\
    make_id(name, id);\
    printf("writing `%s` (%s)\n", id, path);


void assets_add_image(AssetWrite *out, char *name, u32 flags)
{
    _ASSET_PATH_AND_ID;
    _write_image(out, id, path, flags, 0);
}

void assets_add_image_palette(AssetWrite *out, char *name)
{
    _ASSET_PATH_AND_ID;
    _write_image(out, id, path, 0, 1);
}

void assets_add_image_set(AssetWrite *out, char *name, u32 cell_w, u32 cell_h, i32 pivot_x, i32 pivot_y, V4i padding)
{
    _ASSET_PATH_AND_ID;
    _write_image_set(out, id, path,
                     cell_w, cell_h,
                     pivot_x, pivot_y,
                     padding);
}

void assets_add_tile_map(AssetWrite *out, char *name)
{
    _ASSET_PATH_AND_ID;
    _write_tile_map(out, id, path);
}

#undef _ASSET_PATH_AND_ID

int
main(int argc, char**argv)
{
    AssetWrite out = {0};
    out.binary = fopen(ASSETS_DIR "res.bin", "wb+");
    assert(out.binary);
    out.header = fopen(ASSETS_DIR "res.h", "w+");
    assert(out.header);

    out.palette.colors_count = 1;

    fprintf(out.header, "#ifndef RES_H\n#define RES_H\n\n");
    fprintf(out.header, "#include \"res.c\"\n\n");

    assets(&out);
    write(&out, Asset_Palette, "asset_palette", &out.palette, 0, 0);

    fprintf(out.header, "\n");
    fprintf(out.header, "#endif // RES_H\n");

    fclose(out.binary);
    fclose(out.header);


    FILE *res_c = fopen(ASSETS_DIR "res.c", "wb+");
    IOReadResult res = io_read(ASSETS_DIR "res.bin");
    assert(res.data);
    fprintf(res_c, "u8 _res[] = {");
    for (memi i = 0; i != res.size; i++) {
        if (i % 16 == 0) {
            fprintf(res_c, "\n    ");
        }
        fprintf(res_c, "0x%0.2x, ", *res.data++);
    }
    fprintf(res_c, "\n};\n");
    fclose(res_c);
}
