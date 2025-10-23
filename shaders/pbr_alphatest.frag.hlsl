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
Texture2D texture_metallic_roughness : register(t1, space2);
Texture2D texture_normal : register(t2, space2);
Texture2D<float> shadow_map : register(t3, space2);

SamplerState sampler_texture  : register(s0, space2);
SamplerState sampler_shadow : register(s1, space2);

struct Light_Spotlight
{
    float3 position;
    float attenuation_constant_linear; 
    float3 color;
    float attenuation_constant_quadratic;
    float3 direction;
    float cutoff_inner; // passed as SDL_cosf(glm_rad(angle))
    float cutoff_outer;
    float padding[3]; // storage buffers need to pad to float4 size
};

// WARNING: StructuredBuffers are not natively supported by SDL's GPU API.
// They will work with SDL_shadercross because it does special processing to
// support them, but not with direct compilation via dxc.
// See https://github.com/libsdl-org/SDL/issues/12200 for details.
StructuredBuffer<Light_Spotlight> buffer_spotlights : register(t4, space2);

cbuffer Light_Directional_Uniform : register(b0, space3)
{
    float3 light_directional_direction; float light_directional_strength;
    float3 light_directional_color; float padding1;
};

cbuffer Light_Hemisphere_Uniform : register(b1, space3)
{
    float3 up_viewspace;
    float _padding;
    float3 color_sky;
    float _padding2;
    float3 color_ground;
    float _padding3;
};

cbuffer Shadow_Uniform : register(b2, space3)
{
    float2 shadow_texel_size;
    float shadow_bias;
    float shadow_pcf_radius; // in texels
};

struct Fragment_Input
{
    float4 position_clipspace_camera : SV_Position;
    float3 position_viewspace : TEXCOORD0;
    float3 normal_viewspace   : TEXCOORD1;
    float2 texture_coordinate : TEXCOORD2;
    float3 tangent_viewspace  : TEXCOORD3;
    float3 bitangent_viewspace: TEXCOORD4;
    float4 position_clipspace_light : TEXCOORD5;
};

struct Fragment_Output
{
    float4 color : SV_Target0;
};

float Distribution_GGX(float NdotH, float roughness)
{
    float a = roughness * roughness * roughness * roughness;
    float denom = (NdotH * NdotH) * (a - 1.0f) + 1.0f;
    return a / (3.14159265f * denom * denom + 1e-7f);
}

float Geometry_SchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0f);
    float k = (r * r) / 8.0f;
    return NdotV / (NdotV * (1.0f - k) + k + 1e-7f);
}

float Geometry_Smith(float NdotV, float NdotL, float roughness)
{
    float ggx1 = Geometry_SchlickGGX(NdotV, roughness);
    float ggx2 = Geometry_SchlickGGX(NdotL, roughness);
    return ggx1 * ggx2;
}

float3 Fresnel_Schlick(float VdotH, float3 F0)
{
    return F0 + (1.0f - F0) * pow(1.0f - VdotH, 5.0f);
}

float3 Fresnel_Schlick_Roughness(float NdotV, float3 F0, float roughness)
{
    return F0 + (max(float3((1.0 - roughness).xxx), F0) - F0) * pow(saturate(1.0 - NdotV), 5.0);
}

float ShadowFactor(float4 position_clipspace_light)
{
    float3 ndc = position_clipspace_light.xyz / max(position_clipspace_light.w, 1e-7f);
    float2 uv  = ndc.xy * 0.5f + 0.5f;
    uv.y = 1.0f - uv.y;
    float fragment_depth = ndc.z;

    // If outside shadow map or behind far plane, consider fully lit
    if (uv.x < 0.0f || uv.x > 1.0f || uv.y < 0.0f || uv.y > 1.0f || fragment_depth > 1.0f)
        return 1.0f;

    // 3x3 PCF
    float sum = 0.0f;
    int radius = (int)shadow_pcf_radius; // use floor
    [unroll]
    for (int y = -radius; y <= radius; y++)
    {
        [unroll]
        for (int x = -radius; x <= radius; x++)
        {
            float2 offset = float2((float)x, (float)y) * shadow_texel_size;
            float map_depth = shadow_map.Sample(sampler_shadow, uv + offset).r; // read depth
            sum += (fragment_depth <= (map_depth + shadow_bias)) * 1.0f;
        }
    }

    float kernel = (2 * radius + 1);
    return sum / (kernel * kernel);
}

Fragment_Output main(Fragment_Input fragment)
{
    Fragment_Output output;

    float4 albedo = texture_diffuse.Sample(sampler_texture, fragment.texture_coordinate);
    CLIP_TEST(albedo.a - 0.5);

    float3 mr_sample = texture_metallic_roughness.Sample(sampler_texture, fragment.texture_coordinate).rgb;
    float metallic = mr_sample.b;
    float roughness = mr_sample.g;
    float ao = mr_sample.r;

    // Normal Mapping

    float3 N_ts = texture_normal.Sample(sampler_texture, fragment.texture_coordinate).xyz * 2.0f - 1.0f;
    // If normal map uses OpenGL convention (green down), uncomment:
    // N_ts.y = -N_ts.y;

    float3 T = normalize(fragment.tangent_viewspace);
    float3 B = normalize(fragment.bitangent_viewspace);
    float3 N_geom = normalize(fragment.normal_viewspace);
    float3x3 TBN = float3x3(T, B, N_geom); // rows: T, B, N

    float3 N = normalize(mul(N_ts, TBN));

    // uncomment to override normal mapping
    // N = normalize(fragment.normal_viewspace);

    float3 V = normalize(-fragment.position_viewspace); // view space camera at origin

    float3 F0 = float3(0.04f, 0.04f, 0.04f);
    F0 = lerp(F0, albedo.rgb, metallic);

    float3 Lo = float3(0.0f, 0.0f, 0.0f);

    // Directional light
    
    // assume we are given the normalized direction the light is coming FROM
    float3 L_directional_light = -light_directional_direction;
    float3 H_directional = normalize(L_directional_light + V);
    float3 radiance = light_directional_color * light_directional_strength;

    float NdotH_directional = saturate(dot(N, H_directional));
    float NdotV = saturate(dot(N, V));
    float NdotL_directional = saturate(dot(N, L_directional_light));
    float VdotH_directional = saturate(dot(V, H_directional));

    float D = Distribution_GGX(NdotH_directional, roughness);
    float G = Geometry_Smith(NdotV, NdotL_directional, roughness);
    float3 F = Fresnel_Schlick(VdotH_directional, F0);

    float3 numerator = D * G * F;
    float denominator = 4.0f * NdotV * NdotL_directional + 1e-7f;
    float3 specular = numerator / denominator;

    float3 kS = F;
    float3 kD = (1.0f.xxx - kS) * (1.0f - metallic);

    float3 direct_dir = (kD * albedo.rgb / 3.14159265f + specular) * radiance * NdotL_directional;

    float shadow = ShadowFactor(fragment.position_clipspace_light);
    Lo += direct_dir * shadow;

    // Spotlights

    for (int i = 0; i < 0; i++) // TODO: number of active lights as uniform
    {
        Light_Spotlight light = buffer_spotlights[i];

        float3 L_unnormalized = light.position - fragment.position_viewspace;
        float distance_to_light = length(L_unnormalized);
        float attenuation = 1.0f / (1.0f + light.attenuation_constant_linear * distance_to_light + light.attenuation_constant_quadratic * (distance_to_light * distance_to_light));

        float3 L = normalize(L_unnormalized);
        float3 H = normalize(L + V);

        // assume we are given the normalized direction the light is coming FROM
        float theta = dot(L, -light.direction);
        float epsilon = light.cutoff_inner - light.cutoff_outer;
        float intensity = saturate((theta - light.cutoff_outer) / epsilon);

        float3 radiance_spot = light.color * intensity * attenuation;

        float NdotH = saturate(dot(N, H));
        float NdotL = saturate(dot(N, L));
        float VdotH = saturate(dot(V, H));

        float D_spot = Distribution_GGX(NdotH, roughness);
        float G_spot = Geometry_Smith(NdotV, NdotL, roughness);
        float3 F_spot = Fresnel_Schlick(VdotH, F0);

        float3 numerator_spot = D_spot * G_spot * F_spot;
        float denominator_spot = 4.0f * NdotV * NdotL + 1e-7f;
        float3 specular_spot = numerator_spot / denominator_spot;

        float3 kS_spot = F_spot;
        float3 kD_spot = (1.0f.xxx - kS_spot) * (1.0f - metallic);

        Lo += (kD_spot * albedo.rgb / 3.14159265f + specular_spot) * radiance_spot * NdotL;
    }
    
    // Hemispheric diffuse (indirect diffuse)
    float NdotUP = dot(N, up_viewspace);
    float N_weight = saturate(0.5f * (NdotUP + 1.0f));
    float3 diffuse_environment_color = lerp(color_ground, color_sky, N_weight);

    // energy-conserving
    float3 kS_ambient = Fresnel_Schlick_Roughness(NdotV, F0, roughness);
    float3 kD_ambient = (1.0f.xxx - kS_ambient) * (1.0f - metallic);

    float3 diffuse_ambient = kD_ambient * (albedo.rgb / 3.14159265f) * diffuse_environment_color * ao;

    // Hemispheric specular (indirect specular)
    float3 R = reflect(-V, N);
    float RdotUP = dot(R, up_viewspace);
    float R_weight = saturate(0.5f * (RdotUP + 1.0f));
    float3 specular_environment_color = lerp(color_ground, color_sky, R_weight);

    float3 specular_ambient = specular_environment_color * kS_ambient * (1.0f - roughness);

    // float RdotSUN = saturate(dot(R, -light_directional_direction));
    // float shininess = lerp(16.0f, 1024.0f, (1.0f - roughness) * (1.0f - roughness));
    // float specular_sun = pow(RdotSUN, shininess);
    // // fade if sun is below horizon relative to N:
    // specular_sun *= step(0.0f, dot(N, -light_directional_direction));
    // float3 sun_color = float3(1.0f, 1.0f, 1.0f); // tweak to taste
    // specular_ambient += sun_color * kS_ambient * specular_sun;
    
    Lo += diffuse_ambient + specular_ambient;

    output.color = float4(Lo, albedo.a);
    return output;
}

/*
NOTES

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