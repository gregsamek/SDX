Texture2D<float4> texture_in   : register(t0, space0);
SamplerState      sampler_linear: register(s0, space0); // border black

[[vk::image_format("rgba16f")]]
RWTexture2D<float4> texture_out : register(u0, space1);

cbuffer KawaseDownParams : register(b0, space2)
{
    float radius;   // e.g. 0.5, 1.5, 2.5...
    float tap_bias; // ~0.5 helps; can alternate per level to reduce ringing
    float2 _pad_;
}

[numthreads(8, 8, 1)]
void main(uint3 Gid : SV_DispatchThreadID)
{
    uint2 dst = Gid.xy;

    uint outW, outH;
    texture_out.GetDimensions(outW, outH);
    if (dst.x >= outW || dst.y >= outH) return;

    uint inW, inH;
    texture_in.GetDimensions(inW, inH);

    // Map dst pixel center to source UV (exact for 2x downsample)
    float2 uv = (float2(dst) + 0.5) / float2(outW, outH);

    float2 texel = 1.0 / float2(inW, inH);
    float2 o = (radius + tap_bias) * texel;

    float3 c = 0.0.xxx;

    // center * 4
    c += texture_in.SampleLevel(sampler_linear, uv, 0).rgb * 4.0;

    // 4 diagonals * 1
    c += texture_in.SampleLevel(sampler_linear, uv + float2( o.x,  o.y), 0).rgb;
    c += texture_in.SampleLevel(sampler_linear, uv + float2(-o.x,  o.y), 0).rgb;
    c += texture_in.SampleLevel(sampler_linear, uv + float2( o.x, -o.y), 0).rgb;
    c += texture_in.SampleLevel(sampler_linear, uv + float2(-o.x, -o.y), 0).rgb;

    texture_out[dst] = float4(c / 8.0, 1.0);
}