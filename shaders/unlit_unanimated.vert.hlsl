cbuffer TransformUBO : register(b0, space1)
{
    float4x4 mvp;
};

// SDL_GPUVertexAttribute
struct VertexInput
{
    float3 Position : TEXCOORD0;
    float2 TexCoord : TEXCOORD1;
};

struct VertexOutput
{
    float4 Position : SV_Position; // Clip space position (mandatory)
    float2 TexCoord : TEXCOORD0;   // Pass texture coordinate to fragment shader
};

VertexOutput main(VertexInput input)
{
    VertexOutput output;

    output.Position = mul(mvp, float4(input.Position, 1.0f));

    output.TexCoord = input.TexCoord;

    return output;
}
