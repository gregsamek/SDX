#ifdef __HLSL_VERSION
    #define CLIP_TEST(v)  clip(v)
#elif defined(__METAL_VERSION__)
    #include <metal_stdlib>
    using namespace metal;
    #define CLIP_TEST(v)  if (any((v) < 0)) discard_fragment()
#else
    #define CLIP_TEST(v)  if (any((v) < 0)) discard
#endif

#define MAGIC_DEBUG_SHADOW_DEPTH_TEXTURE 1

cbuffer Settings_Uniform : register(b0, space3)
{
    uint magic_debug;
};

Texture2D hdrTex : register(t0, space2);
SamplerState Sampler : register(s0, space2);

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

float LinearizeDepth(float depth, float near, float far)
{
    return (2.0 * near) / (far + near - depth * (far - near));
}

float4 main(FragmentInput input): SV_Target0
{
    if (magic_debug & MAGIC_DEBUG_SHADOW_DEPTH_TEXTURE)
    {
        float3 color = hdrTex.Sample(Sampler, input.TexCoord).r;
        color = LinearizeDepth(color, 0.001, 15.0); // near/far plane values
        return float4(color, 1.0);
    }
    else
    {
        const float exposure = 1.0; // Adjust exposure as needed
        float3 hdr = hdrTex.Sample(Sampler, input.TexCoord).rgb;
        float3 color = hdr * exposure;
        // color = reinhardTonemap(color);
        color = saturate(color);
        color = linear_to_srgb(color); // can skip if SDL_GPU_SWAPCHAINCOMPOSITION_SDR_LINEAR
        return float4(color, 1.0);
    }
    
}
