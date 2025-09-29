#ifdef __HLSL_VERSION            // compiled as HLSL → DXIL/SPIR-V
    #define CLIP_TEST(v)  clip(v)
#elif defined(__METAL_VERSION__) // compiled as Metal Shading Language
    #include <metal_stdlib>
    using namespace metal;
    #define CLIP_TEST(v)  if (any((v) < 0)) discard_fragment()
#else                           // assume GLSL
    #define CLIP_TEST(v)  if (any((v) < 0)) discard
#endif

// Texture bound at space2, texture register t0
Texture2D Texture : register(t0, space2);
// Sampler bound at space2, sampler register s0
SamplerState Sampler : register(s0, space2);

// Input structure received from the vertex shader
// TEXCOORD0 matches the semantic used for TexCoord in VertexOutput
struct FragmentInput
{
    float4 ScreenPosition : SV_Position; // Screen position (not typically used directly for texturing)
    float2 TexCoord       : TEXCOORD0;   // Interpolated texture coordinate
};

// Output structure (final pixel color)
// SV_Target0 corresponds to the first color target in the pipeline
struct FragmentOutput
{
    float4 Color : SV_Target0;
};

// Fragment Shader Main Function
FragmentOutput main(FragmentInput input)
{
    FragmentOutput output;

    // Sample the texture using the interpolated texture coordinate and the bound sampler
    output.Color = Texture.Sample(Sampler, input.TexCoord);

    CLIP_TEST(output.Color.a - 0.5);

    return output;
}
