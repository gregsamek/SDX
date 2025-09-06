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

/*
    This is the hard-coded result of `glm_ortho(0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, projection_matrix_ortho)`
    * note the switch between row/col major!
*/
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

    Output output;

    output.Position = mul(ViewProjectionMatrix, vertexPos[vert]);
    output.Texcoord = vertexUVs[vert];

    return output;
}