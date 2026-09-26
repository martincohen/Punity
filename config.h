#define PUN_WINDOW_TITLE "Punity"
#define PUN_CANVAS_WIDTH  480
#define PUN_CANVAS_HEIGHT 300
#define PUN_CANVAS_SCALE  3

#define PUN_PLATFORM_WINDOWS_D3D 1

#define PUN_STACK_CAPACITY megabytes(16)
#define PUN_STORAGE_CAPACITY megabytes(16)

#define PUN_USE_STB_IMAGE 1
#define PUN_USE_STB_VORBIS 1

struct Shader
{
    float color_split;
    float cube;
};

#define PUN_SHADER_TYPE Shader