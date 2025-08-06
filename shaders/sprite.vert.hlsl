struct Output
{
    float4 Position : SV_Position;
    float2 Texcoord : TEXCOORD0;
};

cbuffer UniformBlock : register(b0, space1)
{
    float4 vertexPos[4];
};

static const uint triangleIndices[6] = {0, 1, 2, 3, 2, 1};
static const float2 vertexUVs[4] = 
{
    {0.0f, 0.0f},
    {1.0f, 0.0f},
    {0.0f, 1.0f},
    {1.0f, 1.0f}
};
static const float4x4 ViewProjectionMatrix =
{
    {2.0f, 0.0f, 0.0f, -1.0f},
    {0.0f, -2.0f, 0.0f, 1.0f},
    {0.0f, 0.0f, 1.0f, 0.0f},
    {0.0f, 0.0f, 0.0f, 1.0f}
};

Output main(uint id : SV_VertexID)
{
    uint vert = triangleIndices[id % 6];

    float3 coordWithDepth = float3(vertexPos[vert].xy, 0.5);

    Output output;

    output.Position = mul(ViewProjectionMatrix, float4(coordWithDepth, 1.0f));
    output.Texcoord = vertexUVs[vert];

    return output;
}