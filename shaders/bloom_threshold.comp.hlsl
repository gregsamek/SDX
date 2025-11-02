Texture2D<float4> color_in : register(t0, space0);
[[vk::image_format("rgba16f")]]
RWTexture2D<float4> bright_out : register(u0, space1);

cbuffer BloomParams : register(b0, space2)
{
    float threshold;    // e.g., 1.0–2.0 in HDR linear (after exposure)
    float soft_knee;    // [0..1], 0 = hard, ~0.5 is common
    uint  use_maxRGB;   // 1 = maxRGB metric, 0 = luminance
    float exposure;     // linear scale applied before thresholding
}

// Brightness metric: maxRGB is crisp for glints, luminance is smoother.
float Brightness(float3 c, bool useMaxRGB)
{
    if (useMaxRGB) return max(c.r, max(c.g, c.b));
    return dot(c, float3(0.2126, 0.7152, 0.0722)); // Rec.709/sRGB luminance coefficients
}

// Soft-knee bright pass producing a hue-preserving weighted color contribution.
float3 BrightPass_SoftKnee(float3 hdrColor, float threshold, float softKnee, bool useMaxRGB)
{
    float x = Brightness(hdrColor, useMaxRGB);

    // Knee width K = T * softKnee (ensure > 0 to avoid div-by-zero)
    float K = max(threshold * softKnee, 1e-5);

    // Hard part: how far above threshold
    float hard = max(x - threshold, 0.0);

    // Soft ramp over [T-K, T+K] using a quadratic curve
    float s = saturate((x - threshold + K) / (2.0 * K));
    float soft = K * s * s;

    // Total weight (in brightness domain)
    float w = hard + soft;

    // Distribute the weight back to color while preserving hue
    float denom = max(x, 1e-4);
    float3 outColor = hdrColor * (w / denom);

    return outColor;
}

[numthreads(8, 8, 1)]
void main(uint3 GlobalInvocationID : SV_DispatchThreadID)
{
    // Output coordinate (half-res target)
    uint2 dst = GlobalInvocationID.xy;

    // Bounds check for arbitrary dispatch sizes
    uint outW, outH;
    bright_out.GetDimensions(outW, outH);
    if (dst.x >= outW || dst.y >= outH)
        return;

    // Map this half-res pixel to a 2x2 block in the full-res input
    // Base (top-left) of the source block
    uint2 base = dst * 2;

    // Get input dimensions for safe clamping on odd sizes
    uint inW, inH;
    color_in.GetDimensions(inW, inH);

    // Sample four texels (box downsample 2x2) and apply bright pass to each
    bool useMax = (use_maxRGB != 0);

    int2 tl = int2(min(base.x + 0, inW - 1), min(base.y + 0, inH - 1));
    int2 tr = int2(min(base.x + 1, inW - 1), min(base.y + 0, inH - 1));
    int2 bl = int2(min(base.x + 0, inW - 1), min(base.y + 1, inH - 1));
    int2 br = int2(min(base.x + 1, inW - 1), min(base.y + 1, inH - 1));

    float3 c0 = color_in.Load(int3(tl, 0)).rgb * exposure;
    float3 c1 = color_in.Load(int3(tr, 0)).rgb * exposure;
    float3 c2 = color_in.Load(int3(bl, 0)).rgb * exposure;
    float3 c3 = color_in.Load(int3(br, 0)).rgb * exposure;

    float3 b0 = BrightPass_SoftKnee(c0, threshold, soft_knee, useMax);
    float3 b1 = BrightPass_SoftKnee(c1, threshold, soft_knee, useMax);
    float3 b2 = BrightPass_SoftKnee(c2, threshold, soft_knee, useMax);
    float3 b3 = BrightPass_SoftKnee(c3, threshold, soft_knee, useMax);

    // Average to produce the half-res prefiltered output
    float3 outColor = 0.25 * (b0 + b1 + b2 + b3);

    // Write to the half-resolution bloom level 0
    bright_out[dst] = float4(outColor, 1.0);
}