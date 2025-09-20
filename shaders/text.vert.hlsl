cbuffer TransformUBO : register(b0, space1)
{
    float4x4 mvp;
};

struct VertexInput
{
    float2 Position : TEXCOORD0;
    float2 TexCoord : TEXCOORD1;
};

struct VertexOutput
{
    float4 Color    : TEXCOORD0;
    float2 TexCoord : TEXCOORD1;   
    float4 Position : SV_Position; 
};

VertexOutput main(VertexInput input)
{
    VertexOutput output;
    output.Position = mul(mvp, float4(input.Position, 0.001, 1.0f));
    output.TexCoord = input.TexCoord;
    output.Color = float4(0.0f, 0.0f, 0.0f, 1.0f);
    return output;
}
