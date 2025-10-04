cbuffer TransformUBO : register(b0, space1)
{
    float4x4 mvp; // VP * M
    float4x4 mv;  // V * M
#ifdef LIGHTING_HANDLES_NON_UNIFORM_SCALING
    float4x4 normalMat4; // upper-left 3x3 = inverse-transpose of (V*M).xyz
#endif
};

struct VertexInput
{
    float3 Position : TEXCOORD0;
    float3 Normal   : TEXCOORD1;
    float2 TexCoord : TEXCOORD2;
};

struct VertexOutput
{
    float4 PositionCS : SV_Position; // clip-space position
    float3 PositionVS : TEXCOORD0;   // view-space position
    float3 NormalVS   : TEXCOORD1;   // view-space normal
    float2 TexCoord   : TEXCOORD2;
};

VertexOutput main(VertexInput input)
{
    VertexOutput output;

    float4 posWS = float4(input.Position, 1.0f);
    output.PositionCS = mul(mvp, posWS);

    float4 posVS = mul(mv, posWS);
    output.PositionVS = posVS.xyz;

#ifdef LIGHTING_HANDLES_NON_UNIFORM_SCALING
    float3x3 normalMat = (float3x3)normalMat4;
    output.NormalVS = normalize(mul(normalMat, input.Normal));
#else
    output.NormalVS = normalize(mul((float3x3)mv, input.Normal));
#endif

    output.TexCoord = input.TexCoord;

    return output;
}