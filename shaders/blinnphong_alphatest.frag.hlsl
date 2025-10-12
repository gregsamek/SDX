#ifdef __HLSL_VERSION
    #define CLIP_TEST(v)  clip(v)
#elif defined(__METAL_VERSION__)
    #include <metal_stdlib>
    using namespace metal;
    #define CLIP_TEST(v)  if (any((v) < 0)) discard_fragment()
#else
    #define CLIP_TEST(v)  if (any((v) < 0)) discard
#endif

Texture2D texture_diffuse  : register(t0, space2);
Texture2D texture_specular : register(t1, space2);
SamplerState sampler_texture  : register(s0, space2);

struct Light_Spotlight
{
    float3 position;
    float attenuation_constant_linear; 
    float3 color;
    float attenuation_constant_quadratic;
    float3 direction;
    float cutoff_inner;  // don't pass angle; pass SDL_cosf(glm_rad(angle))
    float cutoff_outer;
    float padding[3]; // pad to float4 size
};

StructuredBuffer<Light_Spotlight> buffer_spotlights : register(t2, space2);

cbuffer Light_Directional_Uniform : register(b0, space3)
{
    float3 light_directional_direction; float ambientStrength;
    float3 light_directional_color; float padding1;
};

struct Fragment_Input
{
    float4 position_clipspace : SV_Position;
    float3 position_viewspace : TEXCOORD0;
    float3 normal_viewspace   : TEXCOORD1;
    float2 texture_coordinate   : TEXCOORD2;
};

struct Fragment_Output
{
    float4 color : SV_Target0;
};

Fragment_Output main(Fragment_Input fragment)
{
    Fragment_Output output;

    float4 albedo = texture_diffuse.Sample(sampler_texture, fragment.texture_coordinate);
    CLIP_TEST(albedo.a - 0.5);

    float3 ambient_color = float3(1.0f, 1.0f, 1.0f);
    float3 color_out = ambientStrength * ambient_color * albedo.rgb;

    float shininess = 32.0f; // adjustable parameter; technically supposed to be per-material
    float3 specSample = texture_specular.Sample(sampler_texture, fragment.texture_coordinate).rgb;

    float3 N = normalize(fragment.normal_viewspace);

    float3 V = normalize(-fragment.position_viewspace); // in view space, camera is at origin; (0,0,0 - posVS) = -posVS

    // directional light uniform

    float3 L_directional_light = -light_directional_direction; // assume we are given the normalized direction the light is coming FROM

    float NdotL_directional = pow(dot(N, L_directional_light) * 0.5 + 0.5, 2);

    // diffuse contribution
    color_out  +=  light_directional_color * NdotL_directional * albedo.rgb;

    float3 H_directional = normalize(L_directional_light + V);
    float spec_directional = (NdotL_directional > 0.0f) ? pow(saturate(dot(H_directional, N)), shininess) : 0.0f;

    // specular contribution
    color_out += light_directional_color * spec_directional * specSample;

    // handle all spotlights in storage buffer
    
    for (int i = 0; i < 1; i++) // TODO replace 1 with actual number of active lights; pass as uniform?
    {
        Light_Spotlight light = buffer_spotlights[i];

        float3 L_unnormalized = light.position - fragment.position_viewspace;
        float distance_to_light = length(L_unnormalized);
        float attenuation = 1.0f / (1.0f + light.attenuation_constant_linear * distance_to_light + light.attenuation_constant_quadratic * (distance_to_light * distance_to_light));

        float3 L = normalize(L_unnormalized);

        float theta = dot(L, -light.direction); // assume we are given the normalized direction the light is coming FROM

        float epsilon = light.cutoff_inner - light.cutoff_outer;
        float intensity = saturate((theta - light.cutoff_outer) / epsilon);

        // diffuse and specular contribution from spotlight
        float NdotL = saturate(dot(N, L));
        color_out += intensity * attenuation * light.color * NdotL * albedo.rgb;

        float3 H = normalize(L + V);
        float spec = (NdotL > 0.0f) ? pow(saturate(dot(H, N)), shininess) : 0.0f;
        color_out += intensity * attenuation * light.color * spec * specSample;
    }
    
    output.color = float4(color_out, albedo.a);
    return output;
}

/*
NOTES

Simple lambertian
float NdotL = saturate(dot(N, L));

Valve's Half Lambert
less harsh; looks better than boosting the ambient term way up
https://developer.valvesoftware.com/wiki/Half_Lambert
float NdotL = pow(dot(N, L) * 0.5 + 0.5, 2);

Specular uses Blinn-Phong model, hence H.N instead of R.V

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