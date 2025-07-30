// Texture bound at space2, texture register t0
Texture2D CubeTexture : register(t0, space2);
// Sampler bound at space2, sampler register s0
SamplerState CubeSampler : register(s0, space2);

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
    output.Color = CubeTexture.Sample(CubeSampler, input.TexCoord);

    // Optional: Apply lighting or other effects here
    // output.Color *= float4(1.0, 1.0, 1.0, 1.0); // Example: No lighting

    // Ensure alpha is set (e.g., fully opaque) if the texture doesn't have it
    // or if blending is enabled
    // output.Color.a = 1.0f;

    return output;
}
