#ifdef __HLSL_VERSION            // compiled as HLSL → DXIL/SPIR-V
    #define CLIP_TEST(v)  clip(v)
#elif defined(__METAL_VERSION__) // compiled as Metal Shading Language
    #include <metal_stdlib>
    using namespace metal;
    #define CLIP_TEST(v)  if (any((v) < 0)) discard_fragment()
#else                           // assume GLSL
    #define CLIP_TEST(v)  if (any((v) < 0)) discard
#endif


Texture2D    texture_unlit : register(t0, space2);
SamplerState sampler_unlit : register(s0, space2);

struct FragmentInput
{
    float4 ScreenPosition : SV_Position;
    float2 TexCoord       : TEXCOORD0;
};

struct FragmentOutput
{
    float4 Color : SV_Target0;
};

FragmentOutput main(FragmentInput input)
{
    FragmentOutput output;

    output.Color = texture_unlit.Sample(sampler_unlit, input.TexCoord);

    CLIP_TEST(output.Color.a - 0.5);

    return output;
}
