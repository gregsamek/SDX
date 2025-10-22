#define USE_NORMAL_MAP

cbuffer TransformUBO : register(b0, space1)
{
    float4x4 mvp;
    float4x4 mv;
#ifdef LIGHTING_HANDLES_NON_UNIFORM_SCALING
    float4x4 mv_inverse_transpose; // upper-left 3x3 = inverse-transpose of (V*M).xyz
#endif
};

struct Vertex_Input
{
    float3 position : TEXCOORD0;
    float3 normal   : TEXCOORD1;
    float2 texture_coordinate : TEXCOORD2;
#ifdef USE_NORMAL_MAP
    float4 tangent : TEXCOORD3; // .w = handedness
#endif
};

struct Vertex_Output
{
    float4 position_clipspace : SV_Position;
    float3 position_viewspace : TEXCOORD0;
    float3 normal_viewspace   : TEXCOORD1;
    float2 texture_coordinate   : TEXCOORD2;
#ifdef USE_NORMAL_MAP
    float3 tangent_viewspace  : TEXCOORD3;
    float3 bitangent_viewspace: TEXCOORD4;
#endif
};

Vertex_Output main(Vertex_Input vertex)
{
    Vertex_Output output;

    float4 position_worldspace = float4(vertex.position, 1.0f);
    output.position_clipspace = mul(mvp, position_worldspace);

    float4 position_viewspace = mul(mv, position_worldspace);
    output.position_viewspace = position_viewspace.xyz;
    
    // Transform normal to view space
#ifdef LIGHTING_HANDLES_NON_UNIFORM_SCALING
    float3x3 normalMat = (float3x3)mv_inverse_transpose;
    float3 N_vs = normalize(mul(normalMat, vertex.normal));
#else
    float3 N_vs = normalize(mul((float3x3)mv, vertex.normal));
#endif
    output.normal_viewspace = N_vs;

#ifdef USE_NORMAL_MAP
    // Transform tangent to view space and build an orthonormal TBN
    float3 T_vs = normalize(mul((float3x3)mv, vertex.tangent.xyz));
    // Orthonormalize T against N
    T_vs = normalize(T_vs - N_vs * dot(T_vs, N_vs));
    float3 B_vs = normalize(cross(N_vs, T_vs)) * vertex.tangent.w; // handedness in .w

    output.tangent_viewspace = T_vs;
    output.bitangent_viewspace = B_vs;
#endif

    output.texture_coordinate = vertex.texture_coordinate;

    return output;
}