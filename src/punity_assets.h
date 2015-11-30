#ifndef PUNITY_ASSETS_H
#define PUNITY_ASSETS_H

#include "punity_core.h"

//
// Assets
//

typedef struct AssetWrite
{
    FILE *header;
    FILE *binary;
    memi binary_offset;
    char path[1024];
    // Palette palette;
}
AssetWrite;

typedef enum AssetWriteFlags
{
    AssetWriteFlag_ImageMatchPalette = 0x01,
}
AssetWriteFlags;

void assets_write(AssetWrite *out, char *name, void *_asset);
char *assets_path(AssetWrite *out, char *path);

Image *image_read(char *path, Palette *palette);
void palette_read(Palette *palette, char *path);
ImageSet *image_set_read(char *path, Palette *palette, u32 cell_w, u32 cell_h);
ImageSet *image_set_read_ex(char *path, Palette *palette, u32 cell_w, u32 cell_h, i32 pivot_x, i32 pivot_y, V4i padding);

void assets(AssetWrite *out);
int assets_main(int argc, char**argv);

#endif // PUNITY_ASSETS_H

