#ifdef __HLSL_VERSION            // compiled as HLSL → DXIL/SPIR-V
    #define CLIP_TEST(v)  clip(v)
#elif defined(__METAL_VERSION__) // compiled as Metal Shading Language
    #include <metal_stdlib>
    using namespace metal;
    #define CLIP_TEST(v)  if (any((v) < 0)) discard_fragment()
#else                           // assume GLSL
    #define CLIP_TEST(v)  if (any((v) < 0)) discard
#endif

Texture2D<float4> tex : register(t0, space2);
SamplerState samp : register(s0, space2);

struct PSInput {
    float4 color : TEXCOORD0;
    float2 tex_coord : TEXCOORD1;
};

struct PSOutput {
    float4 color : SV_Target;
};

PSOutput main(PSInput input) {
    PSOutput output;
    output.color = input.color * tex.Sample(samp, input.tex_coord);
    CLIP_TEST(output.color.a - 0.5);
    return output;
}

// // SDF Rendering
// PSOutput main(PSInput input) {
//     PSOutput output;
//     const float smoothing = (1.0 / 16.0);
//     float distance = tex.Sample(samp, input.tex_coord).a;
//     float alpha = smoothstep(0.5 - smoothing, 0.5 + smoothing, distance);
//     output.color = float4(input.color.rgb, input.color.a * alpha);
//     return output;
// }
