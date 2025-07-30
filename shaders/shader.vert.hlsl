// Uniform buffer bound at space1, binding 0
// Contains the Model-View-Projection matrix
cbuffer TransformUBO : register(b0, space1)
{
    float4x4 mvp; // Model-View-Projection matrix
};

// Input vertex structure from the application
// Matches SDL_GPUVertexAttribute layout
struct VertexInput
{
    float3 Position : TEXCOORD0; // Location 0
    float2 TexCoord : TEXCOORD1; // Location 1
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

    // Transform vertex position to clip space
    output.Position = mul(mvp, float4(input.Position, 1.0f));

    // Pass texture coordinate through
    output.TexCoord = input.TexCoord;

    return output;
}
