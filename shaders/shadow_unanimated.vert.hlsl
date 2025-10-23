cbuffer TransformUBO : register(b0, space1)
{
    float4x4 mvp_light;
};

struct Vertex_Input
{
    float3 position : TEXCOORD0;
    float3 normal   : TEXCOORD1;
    float2 texture_coordinate : TEXCOORD2;
    float4 tangent : TEXCOORD3; // .w = handedness
};

struct Vertex_Output
{
    float4 position_clipspace : SV_Position;
};

Vertex_Output main(Vertex_Input vertex)
{
    Vertex_Output output;

    float4 position_worldspace = float4(vertex.position, 1.0f);
    output.position_clipspace = mul(mvp_light, position_worldspace);

    return output;
}