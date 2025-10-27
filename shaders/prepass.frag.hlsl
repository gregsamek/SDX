#ifdef __HLSL_VERSION
    #define CLIP_TEST(v)  clip(v)
#elif defined(__METAL_VERSION__)
    #include <metal_stdlib>
    using namespace metal;
    #define CLIP_TEST(v)  if (any((v) < 0)) discard_fragment()
#else
    #define CLIP_TEST(v)  if (any((v) < 0)) discard
#endif

struct Fragment_Input
{
    float4 position_clipspace_camera : SV_Position;
    float3 position_viewspace        : TEXCOORD0;
    float3 normal_viewspace          : TEXCOORD1;
};

struct Fragment_Output
{
    float4 normal : SV_Target0;
};

Fragment_Output main(Fragment_Input fragment)
{
    Fragment_Output output;

    // Encode normal in view space to [0,1] range
    float3 N_vs = normalize(fragment.normal_viewspace);
    N_vs = N_vs * 0.5f + 0.5f;
    output.normal = float4(N_vs, 1.0f);

    return output;
}