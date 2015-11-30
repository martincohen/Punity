#include "punity_assets.h"

#define STB_IMAGE_IMPLEMENTATION
#include <lib/stb_image.h>

#include "memory.c"


#if 0

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
    case AssetType_Raw:
        WRITE_ASSET_HEADER(Asset);
        break;
    case AssetType_Palette:
        WRITE_ASSET_HEADER(Palette);
        break;
    case AssetType_Image:
        WRITE_ASSET_HEADER(Image);
        break;
    case AssetType_ImageSet:
        WRITE_ASSET_HEADER(ImageSet);
        break;
    case AssetType_TileMap:
        WRITE_ASSET_HEADER(TileMap);
        break;
    default:
        ASSERT(0 && "Not supported.");
        break;
    }

    if (data) {
        fwrite(data, 1, data_size, out->binary);
        out->binary_offset += data_size;
    }
}

#endif

Image *
image_read(char *path, Palette *palette)
{
    int w, h, comp;
    Color *pixels = (Color*)stbi_load(path, &w, &h, &comp, STBI_rgb_alpha);
    ASSERT(pixels);

    memi size = w*h;
    Image *image = calloc(sizeof(Image) + size, 1);
    image->asset.size = sizeof(Image) + size;
    image->asset.type = AssetType_Image;
    image->w = w;
    image->h = h;

    u8 ix;
    Color *pixels_end = pixels + size;
    Color *pixels_it = pixels;
    u8 *indexed_it = image_data(image);
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

        ASSERT(palette->colors_count != 0xFF);
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

    return image;
}

void
palette_read(Palette *palette, char *path)
{
    palette->asset.type = AssetType_Palette;
    palette->asset.size = sizeof(Palette);
    free(image_read(path, palette));
}

// TODO: Optimize image sets to
//       - only contain non-transparent images
//       - to only contain linear pieces with actual content (rects)

// TODO: Allow for image sets with various sprite sizes
//       - this would be needed anywhay when I introduce the above optimizations

ImageSet *
image_set_read(char *path, Palette *palette, u32 cell_w, u32 cell_h)
{
    ASSERT(cell_w <= 0xFF);
    ASSERT(cell_h <= 0xFF);

    Image *image = image_read(path, palette);

    memi rows = image->h / cell_h;
    memi cols = image->w / cell_w;
    ASSERT(rows * cols <= 0x10000);

    memi size = image->w * image->h;
    ImageSet *set = calloc(sizeof(ImageSet) + size, 1);
    set->asset.size = sizeof(ImageSet) + size;
    set->asset.type = AssetType_ImageSet;
    set->count = rows * cols;
    set->cw = cell_w;
    set->ch = cell_h;

    // data
    // o0 o1 o2 o3
    // o4 o5 o6 o7
    // o8 o9 10 11
    // 12 13 14 15

    // linear
    // o0 o4 o8 12
    // o1 o5 o9 13

    memi sprite_size = cell_w * cell_h;
    for (memi r = 0; r != rows; ++r) {
        for (memi c = 0; c != cols; ++c) {
            for (memi y = 0; y != set->ch; y++) {
                memi s_offset = ((c + (r * cols)) * sprite_size) + (y * set->cw);
                memi i_offset = (c * set->cw) + (r * sprite_size * cols) + (y * image->w);
                // printf("in %d out %d\n", d_offset, l_offset);
                memcpy(image_set_data(set) + s_offset,
                       image_data(image) + i_offset,
                       set->cw);
            }
        }
    }

    free(image);

    return set;
}

ImageSet *
image_set_read_ex(char *path, Palette *palette, u32 cell_w, u32 cell_h, i32 pivot_x, i32 pivot_y, V4i padding)
{
    ImageSet *set = image_set_read(path, palette, cell_w, cell_h);
    set->pivot_x = pivot_x;
    set->pivot_y = pivot_y;
    set->padding = padding;

    return set;
}

TileMap *
tile_map_read_csv(char *path)
{
    IOReadResult f = io_read(path);
    ASSERT(f.data);

    Memory data = {0};

    i32 w = 0;
    i32 h = 0;

    u32 n = 0;
    i32 m = 1;
    char *it = (char*)f.data;
    while (*it)
    {
        switch (*it)
        {
        case ',':
        case '\r':
            break;

        case '\n':
            if (*it == '\n') {
                if (h == 0) {
                    w = data.count / sizeof(u16);
                }
                h++;
            }
            break;

        default:
            m = 1;
            if (*it == '-')
            {
                m = -1;
                it++;
            }

            ASSERT(is_number(*it));
            do {
                n = (n * 10) + (*it - '0');
                it++;
            } while (is_number(*it));

            *memory_push(&data, u16, 1, 1024) = (u16)(m == -1 ? 0xFFFF : n);
            n = 0;
            goto next;
            break;
        }
        it++;
next:;
    }
    ASSERT(w);
    ASSERT(h);

    TileMap *map = calloc(tile_map_size(w, h), 1);
    map->asset.size = tile_map_size(w, h);
    map->asset.type = AssetType_TileMap;
    map->w = w;
    map->h = h;
    memcpy(tile_map_data(map), data.ptr, w*h * sizeof(u16));

    free(data.ptr);

    return map;
}

#define _DEFINE_TYPE(Type) { AssetType_##Type, #Type }
typedef struct { u32 type; char *name; } AssetTypeName;
static AssetTypeName ASSET_TYPE_NAMES[] = {
    _DEFINE_TYPE(TileMap),
    _DEFINE_TYPE(Palette),
    _DEFINE_TYPE(Image),
    _DEFINE_TYPE(ImageSet),
    {0, 0}
};
#undef _DEFINE_TYPE

void
assets_write(AssetWrite *out, char *name, void *_asset)
{
    Asset *asset = _asset;
    AssetTypeName *type = ASSET_TYPE_NAMES;
    while (type->name) {
        if (type->type == asset->type) {
            break;
        }
        type++;
    }
    ASSERT(type->name);

    fwrite(asset, 1, asset->size, out->binary);
//    fprintf(out->header, "const %s* const %s = ((%s *)(_res + 0x%.8x));\n",
//            type->name, name, type->name, out->binary_offset);
    fprintf(out->header, "#define %s ((%s *)(_res + 0x%.8x))\n",
            name, type->name, out->binary_offset);
    out->binary_offset += asset->size;
}

char *
assets_path(AssetWrite *out, char *path)
{
    static char buffer[1024];
    *buffer = 0;

    char *it = buffer;
    it = strpush(it, out->path);
    it = strpush(it, path);

    return buffer;
}

int
assets_main(int argc, char**argv)
{
    AssetWrite out = {0};

    memi length = 0;
    if (argc > 1)
    {
        length = strlen(argv[1]);
        ASSERT(length < 1023);
        strcpy(out.path, argv[1]);
    }
    else
    {
#ifdef ASSETS_DIR
        length = strlen(ASSETS_DIR);
        ASSERT(length < 1023);
        strcpy(out.path, ASSETS_DIR);
#endif
    }

    // TODO: Windows/Unix paths.
    if (length != 0) {
        if (out.path[length - 1] != '/' && out.path[length - 1] != '\\') {
            out.path[length++] = '/';
        }
    }

    out.binary = fopen(assets_path(&out, "res.bin"), "wb+");
    ASSERT(out.binary);
    out.header = fopen(assets_path(&out, "res.h"), "w+");
    ASSERT(out.header);

    // out.palette.colors_count = 1;

    fprintf(out.header, "#ifndef RES_H\n#define RES_H\n\n");
    fprintf(out.header, "#include \"res.c\"\n\n");

    assets(&out);

    fprintf(out.header, "\n");
    fprintf(out.header, "#endif // RES_H\n");

    fclose(out.binary);
    fclose(out.header);


    FILE *res_c = fopen(assets_path(&out, "res.c"), "wb+");
    IOReadResult res = io_read(assets_path(&out, "res.bin"));
    ASSERT(res.data);
    fprintf(res_c, "u8 _res[] = {");
    for (memi i = 0; i != res.size; i++) {
        if (i % 16 == 0) {
            fprintf(res_c, "\n    ");
        }
        fprintf(res_c, "0x%0.2x, ", *res.data++);
    }
    fprintf(res_c, "\n};\n");
    fclose(res_c);

    return 0;
}
