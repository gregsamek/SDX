// Kawase Downsample (Dual-Kawase) compute shader
// Reduces resolution by 2x and applies a small Kawase blur in the same pass.
// Each thread writes one pixel of the next smaller bloom level.

// Source: previous bloom level (higher resolution)
Texture2D<float4> src_level   : register(t0, space0);
SamplerState      linearClamp : register(s0, space0);

// Destination: next downsampled bloom level (half resolution of src_level)
[[vk::image_format("rgba16f")]]
RWTexture2D<float4> dst_level : register(u0, space1);

cbuffer KawaseDownParams : register(b0, space2)
{
    // Blur radius in source texel units for this level (e.g., 1.0, 1.0, 2.0, 2.0, 3.0, ...)
    float radius;

    // Optional bias to slightly shift taps off the texel center to reduce ringing.
    // 0.5 is common for Kawase; set to 0.0 to disable.
    float tap_bias;

    // Unused padding to keep 16-byte alignment (or use for future params).
    float2 _pad_;
}

float3 Kawase4(float2 uv, float2 texel)
{
    // Compute offset vector for the 4 diagonal taps.
    float2 o = (radius + tap_bias) * texel;

    float3 c = 0.0.xxx;
    c += src_level.SampleLevel(linearClamp, uv + float2( o.x,  o.y), 0).rgb;
    c += src_level.SampleLevel(linearClamp, uv + float2(-o.x,  o.y), 0).rgb;
    c += src_level.SampleLevel(linearClamp, uv + float2( o.x, -o.y), 0).rgb;
    c += src_level.SampleLevel(linearClamp, uv + float2(-o.x, -o.y), 0).rgb;

    return c * 0.25;
}

[numthreads(8, 8, 1)]
void main(uint3 GlobalInvocationID : SV_DispatchThreadID)
{
    uint2 dst = GlobalInvocationID.xy;

    // Bounds check
    uint outW, outH;
    dst_level.GetDimensions(outW, outH);
    if (dst.x >= outW || dst.y >= outH)
        return;

    // Dimensions
    uint inW, inH;
    src_level.GetDimensions(inW, inH);

    // Map this half-res pixel to normalized source UV.
    // For exact 2x downsample, src is typically 2*dst in each dimension.
    // Center-of-footprint mapping keeps the chain stable:
    // uv = (dst + 0.5) / dstSize, expressed directly in source UV space.
    float2 dstSize = float2(outW, outH);
    float2 uv = (float2(dst) + 0.5) / dstSize;

    // Source texel size (for building the Kawase offsets).
    float2 texel = 1.0 / float2(inW, inH);

    float3 color = Kawase4(uv, texel);

    dst_level[dst] = float4(color, 1.0);
}