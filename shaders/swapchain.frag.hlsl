#define SETTINGS_RENDER_ENABLE_BLOOM (1 << 7)

cbuffer Settings_Uniform : register(b0, space3)
{
    uint settings_render;
};

Texture2D    texture_hdr   : register(t0, space2);
SamplerState sampler_hdr   : register(s0, space2);
Texture2D    texture_bloom : register(t1, space2);
SamplerState sampler_bloom : register(s1, space2);

struct FragmentInput
{
    float4 ScreenPosition : SV_Position;
    float2 TexCoord       : TEXCOORD0;
};

float3 tonemapACES(float3 x) 
{
    const float a=2.51, b=0.03, c=2.43, d=0.59, e=0.14;
    return saturate((x*(a*x+b)) / (x*(c*x+d)+e));
}

float3 reinhardTonemap(float3 color) 
{
    return color / (color + float3(1.0, 1.0, 1.0));
}

float3 linear_to_srgb(float3 c) 
{
    float3 lo = 12.92 * c;
    float3 hi = 1.055 * pow(max(c, 0.0.xxx), 1.0/2.4) - 0.055;
    return lerp(lo, hi, step(0.0031308.xxx, c));
}

// // cheap approximation
// float3 linear_to_srgb(float3 c) 
// {
//     return pow(abs(color), float(1.0f/2.2f).xxx);
// }

float LinearizeDepth(float depth, float near, float far)
{
    return (2.0 * near) / (far + near - depth * (far - near));
}

float4 main(FragmentInput input): SV_Target0
{
    float exposure = 1.0; // Adjust as needed
    float4 hdr = texture_hdr.Sample(sampler_hdr, input.TexCoord);
    float alpha = hdr.a;
    float3 color = hdr.rgb * exposure;
    if (settings_render & SETTINGS_RENDER_ENABLE_BLOOM)
    {
        float3 bloom = texture_bloom.Sample(sampler_bloom, input.TexCoord).rgb;
        color += bloom;
    }

    // Tonemap here if desired
    // color = reinhardTonemap(color);
    // color = tonemapACES(color);
    
    color = saturate(color);
    color = linear_to_srgb(color); // can skip if SDL_GPU_SWAPCHAINCOMPOSITION_SDR_LINEAR (not currently trying to support this)
    return float4(color, alpha);    
}
