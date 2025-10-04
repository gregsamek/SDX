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
    float3 _L = lightPosVS - input.PositionVS;
    float distance_to_light = length(_L);
    float3 L = normalize(_L);
    float3 V = normalize(-input.PositionVS); // in view space, camera is at origin; (0,0,0 - posVS) = -posVS
    float3 H = normalize(L + V);

    // these constants are for a light with a range of ~50 units (see table at end of file)
    float attenuation = 1.0f / (1.0f + 0.09f * distance_to_light + 0.032f * distance_to_light * distance_to_light);
    
    // Simple lambertian
    // float NdotL = saturate(dot(N, L));

    // Valve's Half Lambert
    // less harsh; looks better than boosting the ambient term way up
    // https://developer.valvesoftware.com/wiki/Half_Lambert
    float NdotL = pow(dot(N, L) * 0.5 + 0.5, 2);
    float spec = (NdotL > 0.0f) ? pow(saturate(dot(H, N)), shininess) : 0.0f;

    float3 ambient  = attenuation * ambientStrength * lightColor * albedo.rgb;
    float3 diffuse  = attenuation * lightColor * NdotL * albedo.rgb;
    float3 specular = attenuation * lightColor * spec * specSample;

    float3 color = ambient + diffuse + specular;

    output.Color = float4(color, albedo.a);
    return output;
}

/*
Light Attenuation Constants
https://learnopengl.com/Lighting/Light-casters
Range Linear Quadratic
3250, 0.0014, 0.000007
600, 0.007, 0.0002
325, 0.014, 0.0007
200, 0.022, 0.0019
160, 0.027, 0.0028
100, 0.045, 0.0075
65, 0.07, 0.017
50, 0.09, 0.032
32, 0.14, 0.07
20, 0.22, 0.20
13, 0.35, 0.44
7, 0.7, 1.8
*/