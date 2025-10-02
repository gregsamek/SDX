#ifdef __HLSL_VERSION
    #define CLIP_TEST(v)  clip(v)
#elif defined(__METAL_VERSION__)
    #include <metal_stdlib>
    using namespace metal;
    #define CLIP_TEST(v)  if (any((v) < 0)) discard_fragment()
#else
    #define CLIP_TEST(v)  if (any((v) < 0)) discard
#endif

Texture2D DiffuseTex  : register(t0, space2);
Texture2D SpecularTex : register(t1, space2);
SamplerState Sampler  : register(s0, space2);

cbuffer LightingUBO : register(b0, space3)
{
    float3 lightPosVS; float ambientStrength;
    float3 lightColor; float shininess;
};

struct FragmentInput
{
    float4 PositionCS : SV_Position;
    float3 PositionVS : TEXCOORD0;
    float3 NormalVS   : TEXCOORD1;
    float2 TexCoord   : TEXCOORD2;
};

struct FragmentOutput
{
    float4 Color : SV_Target0;
};

FragmentOutput main(FragmentInput input)
{
    FragmentOutput output;

    float4 albedo = DiffuseTex.Sample(Sampler, input.TexCoord);
    CLIP_TEST(albedo.a - 0.5);

    float3 specSample = SpecularTex.Sample(Sampler, input.TexCoord).rgb;

    // View-space vectors
    float3 N = normalize(input.NormalVS);
    float3 L = normalize(lightPosVS - input.PositionVS);
    float3 V = normalize(-input.PositionVS); // camera at origin in view space
    float3 H = normalize(L + V);

    // fyi NdotL is sometimes referred to as the "lambertian"
    // float NdotL = saturate(dot(N, L));

    // this is Valve's Half Lambert trick
    // it's less harsh, and looks less flat than jacking the ambient term way up
    // see: https://developer.valvesoftware.com/wiki/Half_Lambert
    float NdotL = pow(dot(N, L) * 0.5 + 0.5, 2);
    float spec = (NdotL > 0.0f) ? pow(saturate(dot(H, N)), shininess) : 0.0f;

    float3 ambient  = ambientStrength * lightColor * albedo.rgb;
    float3 diffuse  = lightColor * NdotL * albedo.rgb;
    float3 specular = lightColor * spec * specSample;

    float3 color = ambient + diffuse + specular;

    output.Color = float4(color, albedo.a);
    return output;
}