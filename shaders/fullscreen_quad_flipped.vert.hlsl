struct Vertex_Output 
{
    float4 position : SV_Position;
    float2 texcoord : TEXCOORD0;
};

Vertex_Output main(uint vid : SV_VertexID)
{
    static const uint   idx[6]  = { 0, 1, 2, 3, 2, 1 };
    static const float2 pos[4]  = 
    {
        float2(-1.0,  1.0),
        float2( 1.0,  1.0),
        float2(-1.0, -1.0),
        float2( 1.0, -1.0)
    };
    static const float2 uv [4]  = 
    {
        float2(0.0, 0.0),
        float2(1.0, 0.0),
        float2(0.0, 1.0),
        float2(1.0, 1.0)
    };

    uint i = idx[vid % 6];

    Vertex_Output output;
    output.position = float4(pos[i], 0.0, 1.0); // clip space
    output.texcoord = float2(1.0 - uv[i].x, uv[i].y);
    return output;
}

// // Full-screen triangle
// struct VSOut
// {
//     float4 pos : SV_POSITION;
//     float2 uv  : TEXCOORD0;
// };

// VSOut VS_Fullscreen(uint vid : SV_VertexID)
// {
//     VSOut o;
//     // Fullscreen triangle positions
//     float2 p = float2((vid << 1) & 2, vid & 2);
//     o.pos = float4(p * float2(2, -2) + float2(-1, 1), 0, 1);
//     o.uv  = p;
//     return o;
// }