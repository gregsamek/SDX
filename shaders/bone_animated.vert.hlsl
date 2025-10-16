cbuffer Transforms : register(b0, space1)
{
    float4x4 mvp;
    float4x4 mv;
#ifdef LIGHTING_HANDLES_NON_UNIFORM_SCALING
    float4x4 mv_inverse_transpose; // upper-left 3x3 = inverse-transpose of (V*M).xyz
#endif
};

cbuffer Skinning : register(b1, space1)
{
    uint base_joint_offset_bytes; // offset into the joint matrix storage buffer
};

// WARNING: StructuredBuffers are not natively supported by SDL's GPU API.
// They will work with SDL_shadercross because it does special processing to
// support them, but not with direct compilation via dxc.
// See https://github.com/libsdl-org/SDL/issues/12200 for details.

// Storage buffer containing all joint matrices for all models for this frame.
StructuredBuffer<float4x4> joint_matrix_buffer : register(t0, space0);

struct Vertex_Input
{
    float3 position : TEXCOORD0;
    float3 normal : TEXCOORD1;
    float2 texture_coordinate : TEXCOORD2;
    uint bone_indices : TEXCOORD3; // 4 8-bit bone indices packed into a single 32-bit uint
    float4 bone_weights : TEXCOORD4;
};

struct Vertex_Output
{
    float4 skinned_position_clipspace : SV_Position;
    float3 skinned_position_viewspace : TEXCOORD0;
    float3 normal_viewspace   : TEXCOORD1;
    float2 texture_coordinate   : TEXCOORD2;
};

Vertex_Output main(Vertex_Input vertex)
{
    Vertex_Output output;

    // 4 8-bit bone indices packed into a single 32-bit uint
    uint index0 = (vertex.bone_indices >> 0) & 0xFF;
    uint index1 = (vertex.bone_indices >> 8) & 0xFF;
    uint index2 = (vertex.bone_indices >> 16) & 0xFF;
    uint index3 = (vertex.bone_indices >> 24) & 0xFF;

    float4x4 skin_matrix = (float4x4)0;
    uint base_offset = base_joint_offset_bytes / sizeof(float4x4); // Convert byte offset to matrix index
    index0 += base_offset;
    index1 += base_offset;
    index2 += base_offset;
    index3 += base_offset;
    skin_matrix += joint_matrix_buffer[index0] * vertex.bone_weights.x;
    skin_matrix += joint_matrix_buffer[index1] * vertex.bone_weights.y;
    skin_matrix += joint_matrix_buffer[index2] * vertex.bone_weights.z;
    skin_matrix += joint_matrix_buffer[index3] * vertex.bone_weights.w;

    float4 skinned_position_worldspace = mul(skin_matrix, float4(vertex.position, 1.0f));

    output.skinned_position_clipspace = mul(mvp, skinned_position_worldspace);

    float4 skinned_position_viewspace = mul(mv, skinned_position_worldspace);
    output.skinned_position_viewspace = skinned_position_viewspace.xyz;
    
    // assumes the skin matrix does not have non-uniform scaling
    float3x3 skin_matrix_3x3 = (float3x3)skin_matrix;
    float3 normal_worldspace = mul(skin_matrix_3x3, vertex.normal);
    normal_worldspace = normalize(normal_worldspace);

#ifdef LIGHTING_HANDLES_NON_UNIFORM_SCALING
    float3x3 normalMat = (float3x3)mv_inverse_transpose;
    output.normal_viewspace = normalize(mul(normalMat, normal_worldspace));
#else
    output.normal_viewspace = normalize(mul((float3x3)mv, normal_worldspace));
#endif

    output.texture_coordinate = vertex.texture_coordinate;

    return output;
}
