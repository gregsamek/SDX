// Kawase Upsample (Dual-Kawase) compute shader
// - Reads from the smaller level (small_level) and upsamples to the current level size.
// - Optionally adds the current level's downsampled image (base_level) for energy preservation.
// - Applies a small Kawase blur while upsampling.

Texture2D<float4> small_level : register(t0, space0); // previous (smaller) level
// Texture2D<float4> base_level  : register(t1, space0); // current level from down pass (same size as dst)
SamplerState      linearClamp : register(s0, space0);

[[vk::image_format("rgba16f")]]
RWTexture2D<float4> dst_level : register(u0, space1);

cbuffer KawaseUpParams : register(b0, space2)
{
    // Kawase kernel controls
    float radius;        
    float tap_bias;      // ~0.5 is common; 0.0 disables offset bias

    // Blend weights
    // float small_weight;  // contribution of upsampled blur (typically 1.0)
    // float base_weight;   // contribution of base level (typically 1.0)
}

float3 Kawase4Small(float2 uv, float2 texelSmall)
{
    float2 o = (radius + tap_bias) * texelSmall;

    float3 c = 0.0.xxx;
    c += small_level.SampleLevel(linearClamp, uv + float2( o.x,  o.y), 0).rgb;
    c += small_level.SampleLevel(linearClamp, uv + float2(-o.x,  o.y), 0).rgb;
    c += small_level.SampleLevel(linearClamp, uv + float2( o.x, -o.y), 0).rgb;
    c += small_level.SampleLevel(linearClamp, uv + float2(-o.x, -o.y), 0).rgb;

    return c * 0.25;
}

[numthreads(8, 8, 1)]
void main(uint3 Gid : SV_DispatchThreadID)
{
    uint2 dst = Gid.xy;

    // Bounds check
    uint outW, outH;
    dst_level.GetDimensions(outW, outH);
    if (dst.x >= outW || dst.y >= outH) return;

    // Normalized UV of the destination pixel center
    float2 uv = (float2(dst) + 0.5) / float2(outW, outH);

    // Small-level texel size for Kawase offsets
    uint sw, sh;
    small_level.GetDimensions(sw, sh);
    float2 texelSmall = 1.0 / float2(sw, sh);

    // Blur from the small level (upsampled via sampling in normalized UV space)
    float3 up = Kawase4Small(uv, texelSmall);

    // Base (current level) contribution
    // float3 base = (base_weight != 0.0) ? base_level.SampleLevel(linearClamp, uv, 0).rgb : 0.0.xxx;

    // Combine
    // float3 outColor = small_weight * up + base_weight * base;

    dst_level[dst] = float4(up, 1.0);
}