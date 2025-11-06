Texture2D<float4> small_level : register(t0, space0); // smaller mip
// Texture2D<float4> base_level  : register(t1, space0); // same size as dst
SamplerState      linearBorder: register(s0, space0);

[[vk::image_format("rgba16f")]]
RWTexture2D<float4> dst_level : register(u0, space1);

cbuffer KawaseUpParams : register(b0, space2)
{
    float radius;    // match downpass (e.g., reverse schedule)
    float tap_bias;  // ~0.5
    // float small_w;   // typically 1.0
    // float base_w;    // typically 1.0 (try 0.8–1.2 to taste)
}

[numthreads(8, 8, 1)]
void main(uint3 Gid : SV_DispatchThreadID)
{
    uint2 dst = Gid.xy;

    uint outW, outH;
    dst_level.GetDimensions(outW, outH);
    if (dst.x >= outW || dst.y >= outH) return;

    float2 uv = (float2(dst) + 0.5) / float2(outW, outH);

    uint sw, sh;
    small_level.GetDimensions(sw, sh);

    float2 texelSmall = 1.0 / float2(sw, sh);
    float2 o = (radius + tap_bias) * texelSmall;

    float3 c = 0.0.xxx;

    // 4 edges * 1
    c += small_level.SampleLevel(linearBorder, uv + float2(-2.0*o.x, 0.0), 0).rgb;
    c += small_level.SampleLevel(linearBorder, uv + float2( 2.0*o.x, 0.0), 0).rgb;
    c += small_level.SampleLevel(linearBorder, uv + float2(0.0, -2.0*o.y), 0).rgb;
    c += small_level.SampleLevel(linearBorder, uv + float2(0.0,  2.0*o.y), 0).rgb;

    // 4 diagonals * 2
    c += small_level.SampleLevel(linearBorder, uv + float2(-o.x, -o.y), 0).rgb * 2.0;
    c += small_level.SampleLevel(linearBorder, uv + float2( o.x, -o.y), 0).rgb * 2.0;
    c += small_level.SampleLevel(linearBorder, uv + float2(-o.x,  o.y), 0).rgb * 2.0;
    c += small_level.SampleLevel(linearBorder, uv + float2( o.x,  o.y), 0).rgb * 2.0;

    float3 up = c / 12.0;

    // float3 base = base_level.SampleLevel(linearBorder, uv, 0).rgb;

    // dst_level[dst] = float4(small_w * up + base_w * base, 1.0);

    dst_level[dst] = float4(up, 1.0);
}