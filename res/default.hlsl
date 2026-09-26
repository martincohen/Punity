SamplerState state;
Texture2D texture2d;

cbuffer frame_constants
{
    float time;
    float scale;
    float2 size;

    float channel_split;
    float cube;
};

struct PixelShaderInput
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD;
};

// Hardness of scanline.
//  -8.0 = soft
// -16.0 = medium
static float HARD_SCAN = -8.0;

// Hardness of pixels in scanline.
// -2.0 = soft
// -4.0 = hard
static float HARD_PIX = -2.0;


// Amount of shadow mask.
static float MASK_DARK =  0.5;
static float MASK_LIGHT = 1.5;

static float2 resolution = size.xy / scale;
static float pixel = (scale/size.x);

// sRGB to Linear.
// http://chilliant.blogspot.cz/2012/08/srgb-approximations-for-hlsl.html
// Assuming using sRGB typed textures this should not be needed.
float to_linear_(float c) {
    float ret;
    if (c <= 0.04045) {
        ret = c/12.92;
    } else {
        ret = pow(abs((c + 0.055) / 1.055), 2.4);
    }
    return ret;
}

float3 to_linear(float3 c) {
    return float3(
        to_linear_(c.r),
        to_linear_(c.g),
        to_linear_(c.b)
    );
    // return c;
}

// Linear to sRGB.
// Assuming using sRGB typed textures this should not be needed.
float to_srgb_(float c) {
    float ret;
    if (c < 0.0031308) {
        ret = c * 12.92;
    } else {
        ret = 1.055 * pow(abs(c), 0.41666) - 0.055;
    }
    return ret;
}

float3 to_srgb(float3 c) {
    return float3(
        to_srgb_(c.r),
        to_srgb_(c.g),
        to_srgb_(c.b)
    );
    // return c;
}

// Distance in emulated pixels to nearest texel.
float2 dist(float2 pos) {
    pos = (pos * resolution);
    return float2(-((pos - floor(pos)) - float2(0.5, 0.5)));
}

// float rand_1_05(in float2 uv) {
//     float2 noise = (frac(sin(dot(uv ,float2(12.9898,78.233)*2.0)) * 43758.5453 * time));
//     return abs(noise.x + noise.y) * 0.5;
// }

float rand(float2 n) {
    return frac(sin(dot(n.xy, float2(12.9898, 78.233)))* 43758.5453 * time);
}

float3 test(float3 c) {
    // return c*(1.0/64.0)+c*c*c;
    return c;
}

// Nearest emulated sample given floating point position and texel offset.
// Also zero's off screen.
float3 fetch(float2 pos, float2 off)
{
    // float3 ret;
    // float2 uv = floor(pos * resolution + off) / resolution;
    // // Deal with off-screen pixels.
    // if (max(abs(uv.x-0.5), abs(uv.y-0.5)) > 0.5) {
    //     // TODO: This might probably need our `CORE->background` property to be available.
    //     ret = float3(0.0, 0.0, 0.0);
    // } else {
    //     ret = test(to_linear(texture2d.SampleBias(state, uv, -16.0).rgb));
    // }
    // return ret;
    return to_linear(texture2d.SampleBias(state, pos + (off * pixel), -16.0).rgb);
}

// Gaussian.
float gauss(float pos, float scale) {
    return exp2(scale * pos * pos);
}

float3 gauss_sample_3(float2 pos, float off)
{
    // float3 b = fetch(pos, float2(-1.0, off));
    // float3 c = fetch(pos, float2( 0.0, off));
    // float3 d = fetch(pos, float2( 1.0, off));
    float3 b = fetch(pos, float2(-0.3, off));
    float3 c = fetch(pos, float2( 0.0, off));
    float3 d = fetch(pos, float2( 0.3, off));

    // Convert distance to weight.
    float dst = dist(pos).x;
    float scale = HARD_PIX;
    // float wb = 0.17; // gauss(dst - 1.0, scale);
    // float wc = 0.19; // gauss(dst + 0.0, scale);
    // float wd = 0.17; // gauss(dst + 1.0, scale);
    float wb = gauss(dst - 1.0, scale);
    float wc = gauss(dst + 0.0, scale);
    float wd = gauss(dst + 1.0, scale);

    // Return filtered sample.
    // return ((b * wb) + (c * wc) + (d * wd)) / (wb + wc + wd);

    float3 ret = ((b * wb) + (c * wc) + (d * wd)) / (wb + wc + wd);
    ret.r = b.r;
    ret.b = d.b;
    return ret;
}

// Return scanline weight.
float scan(float2 pos, float off) {
    float d = dist(pos).y;
    return gauss(d + off, HARD_SCAN);
}

float3 tri(float2 pos)
{
    float3 a = gauss_sample_3(pos, -1.0);
    float3 b = gauss_sample_3(pos,  0.0); // gauss_sample_5
    float3 c = gauss_sample_3(pos,  1.0);
    float wa = scan(pos, -1.0);
    float wb = scan(pos,  0.0);
    float wc = scan(pos,  1.0);

    // float3 ret = float3(
    //     a.r * wa,
    //     0,
    //     c.b * wc
    // );
    // return ret + (b * wb);
    return (a * wa) + (b * wb) + (c * wc);
}

// VGA style shadow mask.
float3 mask(float2 pos)
{
    pos.xy = floor(pos.xy * float2(1.0,0.5));
    pos.x += pos.y*3.0;
    float3 mask = float3(MASK_DARK, MASK_DARK, MASK_DARK);
    pos.x = frac(pos.x/6.0);
    if (pos.x < 0.333) {
        mask.r = MASK_LIGHT;
    } else if (pos.x < 0.666) {
        mask.g = MASK_LIGHT;
    } else {
        mask.b = MASK_LIGHT;
    }
    return mask;
}

float3 scanline(float2 coord, float3 screen)
{
    screen.rgb -= sin((coord.y + (time * 72.0))) * 0.02;
    return screen;
}

float3 split_channels(float2 pos)
{
    float3 p1 = fetch(pos, float2(+channel_split, 0));
    float3 p2 = fetch(pos, float2(0, 0));
    float3 p3 = fetch(pos, float2(-channel_split, 0));
    float3 ret = float3(
        p1.r,
        p2.g + p3.b * 0.2,
        p3.b + p2.g * 0.2
    );
    return ret;
}

float movement(float y)
{
    return (1 - (sin(time) * 0.05)) * y;
}

float2 cubic_distort(float2 uv)
{
    // lens distortion coefficient (between
    // float k = -0.15;
    float k = -abs(cube); // 0.15;
    // Cubic distortion value
    // float kcube = 0.5;
    float kcube = 0.5;

    // k = -0.6; kcube = 0.5;

    // uv = floor(uv * resolution + off) / resolution;

    float r2 = (uv.x-0.5) * (uv.x-0.5) + (uv.y-0.5) * (uv.y-0.5);       
    float f = 0;
    // Only compute the cubic distortion if necessary.
    if (kcube == 0.0) {
        f = 1 + r2 * k;
    } else {
        f = 1 + r2 * (k + kcube * sqrt(r2));
    }

    // Get the right pixel for the current position.
    return float2(f*(uv.x-0.5)+0.5, f*(uv.y-0.5)+0.5);
}

float4 main(PixelShaderInput input) : SV_TARGET
{
    float add = 0.08;
    float2 pos = cubic_distort(input.texcoord.xy) * (1+add) - (add/2); // / size.xy;

    float2 pixel_pos = floor(input.texcoord / pixel) * pixel;

    float2 pos2 = pos;
    pos2.x += cos((input.texcoord.x + time) * 10) * 0.0005;
    pos2.y += sin((input.texcoord.y + time) * 10) * 0.0005;
    // pos.y += cos((input.texcoord.y + time)) * 0.01;
    // float2 pos = input.texcoord.xy;
    // // pos.y  = movement(pos.y);
    // // pos.y -= (movement(1) - 1) / 2;


    float4 ret;
    // // ret.rgb = fetch(pos, float2(0.0, 0.0), resolution); // *Mask(fragCoord.xy);
    // // ret.rgb = gauss_sample_3(pos, 0.0); // *Mask(fragCoord.xy);
    // // ret.rgb = tri(pos) * mask(input.position.xy);
    ret.rgb = tri(pos2) * mask(input.position.xy);
    // ret.rgb = tri(pos2) * 0.5 + split_channels(pos2) * 0.5;
    // ret.rgb = ret.rgb * 0.2 + (ret.rgb * 0.8 * rand(floor(pos/pixel)*pixel));
    // ret.rgb = split_channels(pos);
    // // ret.rgb = cubic_distort(pos);
    // // ret.rgb = scanline(input.position.xy, ret.rgb);

    // ret.rgb = texture2d.Sample(state, input.texcoord).rgb;
    
    ret.a = 1.0;
    ret.rgb = to_srgb(ret.rgb);
    return ret;

    // return texture2d.Sample(state, input.texcoord);
}