Texture2D texture_color           : register(t0, space2);
Texture2D texture_prepass         : register(t1, space2);
SamplerState sampler_texture      : register(s0, space2);
SamplerState sampler_data_texture : register(s1, space2);

cbuffer UBO_Fog : register(b0, space3)
{
    float3 FogColor;          // linear space
    float  FogDensity;        // for exp/exp2; e.g. 0.02

    float  FogStart;          // for linear fog
    float  FogEnd;            // for linear fog
    int    FogMode;           // 0=Linear, 1=Exp, 2=Exp2
    int    DepthIsViewZ;      // 1 if texture stores camera-space z (positive = -z_view), 0 if it stores Euclidean distance

    // Height fog (optional)
    float  HeightFogEnable;   // 0 or 1
    float  FogHeight;         // world-space height of fog layer
    float  HeightFalloff;     // e.g. 0.1–2.0
    float  padding0;

    float4x4 InvProj;         // inverse projection (for view-ray)
    float4x4 InvView;         // inverse view (view->world), for height fog
};

float3 GetViewRay(float2 uv)
{
    // Returns a view-space point on the far plane; use for direction
    float2 ndc = uv * 2.0f - 1.0f;
    float4 p   = mul(InvProj, float4(ndc, 1.0f, 1.0f));
    return p.xyz / p.w; // not normalized
}

float ComputeDistance(float viewDepth, float2 uv)
{
    // If depth is camera-space z (positive = -z_view), convert to Euclidean distance
    if (DepthIsViewZ != 0)
    {
        float3 vr = GetViewRay(uv);     // view-ray to far plane (view space)
        float t   = viewDepth / max(-vr.z, 1e-6); // scale along ray so z matches +viewDepth
        float3 vp = vr * t;             // view-space position of the pixel
        return length(vp);              // Euclidean distance
    }
    else
    {
        // Already Euclidean distance
        return max(viewDepth, 0.0f);
    }
}

float FogByMode(float d)
{
    if (FogMode == 0) // Linear
    {
        float denom = max(FogEnd - FogStart, 1e-6);
        return saturate((d - FogStart) / denom);
    }
    else if (FogMode == 1) // Exp
    {
        return 1.0f - exp(-FogDensity * d);
    }
    else // Exp2
    {
        float k = FogDensity * d;
        return 1.0f - exp2(-(k * k));
    }
}

float HeightAttenuation(float3 viewPos)
{
    if (HeightFogEnable == 0.0f)
        return 1.0f;

    // Convert to world to read world Y
    float3 worldPos = mul(float4(viewPos, 1.0f), InvView).xyz;
    float h = worldPos.y - FogHeight;
    // Simple attenuation with height (denser below FogHeight)
    // You can clamp/saturate as desired; keep positive
    return exp(-HeightFalloff * max(h, 0.0f));
}

struct Fragment_Input 
{
    float4 position : SV_Position;
    float2 uv : TEXCOORD0;
};

float4 main(Fragment_Input fragment) : SV_TARGET
{
    float4 scene = texture_color.Sample(sampler_texture, fragment.uv);

    float vd = texture_prepass.Sample(sampler_data_texture, fragment.uv);

    // If depth is 0 (sky) decide how you want to fog it. Here we apply max fog.
    if (vd <= 0.0f)
    {
        float fogAmtSky = (FogMode == 0) ? 1.0f : saturate(1.0f); // full fog
        float3 rgb = lerp(scene.rgb, FogColor, fogAmtSky);
        return float4(rgb, scene.a);
    }

    float d = ComputeDistance(vd, fragment.uv);

    // Base fog amount by distance
    float fogAmt = FogByMode(d);

    // Optional height attenuation
    if (HeightFogEnable != 0.0f)
    {
        // Reuse the view-space position computed from the ray (avoid recomputing if desired)
        float3 vr = GetViewRay(fragment.uv);
        float t = (DepthIsViewZ != 0) ? (vd / max(-vr.z, 1e-6)) : (d / max(length(vr), 1e-6)); // crude mapping if d is Euclidean
        float3 vp = vr * t;
        float heightAtten = HeightAttenuation(vp);
        // Combine: reduce fog above the fog layer
        fogAmt *= heightAtten;
    }

    float3 outRGB = lerp(scene.rgb, FogColor, saturate(fogAmt));
    return float4(outRGB, scene.a);
}