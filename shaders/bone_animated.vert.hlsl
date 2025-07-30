// Uniform buffer bound at space1, binding 0
// Contains the Model-View-Projection matrix
cbuffer TransformUBO : register(b0, space1)
{
    float4x4 mvp; // Model-View-Projection matrix
};

// Uniform buffer for per-draw data (skinning offset)
// Bound at vertex shader uniform slot 1
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
    float2 TexCoord : TEXCOORD1;
    uint BoneIndices : TEXCOORD2; // 4 bone indices packed into a single uint
    float4 BoneWeights : TEXCOORD3;
};

// Output structure passed to the fragment shader
struct VertexOutput
{
    float4 Position : SV_Position; // Clip space position (mandatory)
    float2 TexCoord : TEXCOORD0;   // Pass texture coordinate to fragment shader
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

    // --- 3. Apply Transformations ---
    // First, apply the skinning matrix to the local-space vertex position.
    float4 skinnedPosition = mul(skinMatrix, float4(input.Position, 1.0f));

    // Then, transform the skinned position to clip space using the MVP matrix.
    output.Position = mul(mvp, skinnedPosition);

    // Pass texture coordinate through to the fragment shader
    output.TexCoord = input.TexCoord;

    return output;
}
