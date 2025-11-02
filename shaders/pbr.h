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