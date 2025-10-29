#ifdef __HLSL_VERSION
    #define CLIP_TEST(v)  clip(v)
#elif defined(__METAL_VERSION__)
    #include <metal_stdlib>
    using namespace metal;
    #define CLIP_TEST(v)  if (any((v) < 0)) discard_fragment()
#else
    #define CLIP_TEST(v)  if (any((v) < 0)) discard
#endif

Texture2D texture_diffuse    : register(t0, space2);
SamplerState sampler_texture : register(s0, space2);

struct Fragment_Input
{
    float4 position_clipspace : SV_Position;
    float3 position_viewspace : TEXCOORD0;
    float3 normal_viewspace   : TEXCOORD1;
    float2 texture_coordinate : TEXCOORD2;
};

struct Fragment_Output
{
    float4 data : SV_Target0;
};

Fragment_Output main(Fragment_Input fragment)
{
    float4 albedo = texture_diffuse.Sample(sampler_texture, fragment.texture_coordinate);
    CLIP_TEST(albedo.a - 0.5);
    
    Fragment_Output output;

    float3 N_vs = normalize(fragment.normal_viewspace);
    float depth = fragment.position_viewspace.z;
    output.data = float4(N_vs, depth);

    return output;
}