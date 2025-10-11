cbuffer TransformUBO : register(b0, space1)
{
    float4x4 mvp; // VP * M
    float4x4 mv;  // V * M
#ifdef LIGHTING_HANDLES_NON_UNIFORM_SCALING
    float4x4 normalMat4; // upper-left 3x3 = inverse-transpose of (V*M).xyz
#endif
};

cbuffer SkinningUBO : register(b1, space1)
{
    uint baseJointOffsetBytes; // The offset into the global joint matrix buffer
};

// Storage buffer containing all joint matrices for all models for this frame.
// This is a read-only resource.
// Bound at vertex shader storage buffer slot 0.
StructuredBuffer<float4x4> JointMatrixBuffer : register(t0, space0);

// Input vertex structure from the application
// Matches SDL_GPUVertexAttribute layout
struct VertexInput
{
    float3 Position : TEXCOORD0;
    float3 Normal : TEXCOORD1;
    float2 TexCoord : TEXCOORD2;
    uint BoneIndices : TEXCOORD3; // 4 bone indices packed into a single uint
    float4 BoneWeights : TEXCOORD4;
};

struct VertexOutput
{
    float4 PositionCS : SV_Position; // clip-space position
    float3 PositionVS : TEXCOORD0;   // view-space position
    float3 NormalVS   : TEXCOORD1;   // view-space normal
    float2 TexCoord   : TEXCOORD2;
};

// Vertex Shader Main Function
VertexOutput main(VertexInput input)
{
    VertexOutput output;

    // --- 1. Unpack Bone Indices ---
    // The 4 bone indices (8-bit each) are packed into a single 32-bit uint.
    // We unpack them using bitwise shifts and masks.
    uint index0 = (input.BoneIndices >> 0) & 0xFF;
    uint index1 = (input.BoneIndices >> 8) & 0xFF;
    uint index2 = (input.BoneIndices >> 16) & 0xFF;
    uint index3 = (input.BoneIndices >> 24) & 0xFF;

    // --- 2. Calculate the Skinning Matrix ---
    // Initialize a matrix to accumulate the bone influences.
    float4x4 skinMatrix = (float4x4)0;

    // Add the influence of each bone, weighted by its corresponding weight.
    // The final index is calculated by adding the model's base offset to the vertex's bone index.
    uint baseOffset = baseJointOffsetBytes / sizeof(float4x4); // Convert byte offset to matrix index
    index0 += baseOffset;
    index1 += baseOffset;
    index2 += baseOffset;
    index3 += baseOffset;
    skinMatrix += JointMatrixBuffer[index0] * input.BoneWeights.x;
    skinMatrix += JointMatrixBuffer[index1] * input.BoneWeights.y;
    skinMatrix += JointMatrixBuffer[index2] * input.BoneWeights.z;
    skinMatrix += JointMatrixBuffer[index3] * input.BoneWeights.w;

    // apply the skinning matrix to the local-space vertex position.
    float4 skinnedPosition = mul(skinMatrix, float4(input.Position, 1.0f));

    // transform the skinned position to clip space using the MVP matrix.
    output.PositionCS = mul(mvp, skinnedPosition);

    float4 position_viewspace = mul(mv, skinnedPosition);
    output.PositionVS = position_viewspace.xyz;
    
#ifdef LIGHTING_HANDLES_NON_UNIFORM_SCALING
    float3x3 normalMat = (float3x3)normalMat4;
    output.NormalVS = normalize(mul(normalMat, input.Normal));
#else
    output.NormalVS = normalize(mul((float3x3)mv, input.Normal));
#endif

    output.TexCoord = input.TexCoord;

    return output;
}
