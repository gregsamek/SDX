Texture2D<float4> InImage : register(t0, space0);
[[vk::image_format("rgba16f")]]
RWTexture2D<float4> OutImage : register(u0, space1);

// Picks the closest sample along the camera's forward axis.
// If your view-space z is positive distance, abs() is harmless.
// If your view-space z is negative (typical RH view, camera looks -Z),
// abs() makes the comparison choose the nearest (least |z|).
static float DepthKey(float viewZ)
{
    // Treat NaN/Inf as far.
    float k = abs(viewZ);
    return (isnan(k) || !isfinite(k)) ? 3.402823e38f : k;
}

[numthreads(8, 8, 1)]
void main(uint3 GlobalInvocationID : SV_DispatchThreadID)
{
    uint2 outDim;
    OutImage.GetDimensions(outDim.x, outDim.y);

    uint2 dst = GlobalInvocationID.xy;
    if (dst.x >= outDim.x || dst.y >= outDim.y)
        return;

    uint2 inDim;
    InImage.GetDimensions(inDim.x, inDim.y);

    // 2x downsample: each output pixel corresponds to a 2x2 block in the input.
    uint2 base = dst * 2;

    float bestKey = 3.402823e38f;
    float4 best   = float4(0, 0, 0, 0);

    // Gather the 2x2 neighborhood and pick the sample with the minimum depth along view direction.
    [unroll] for (uint dy = 0; dy < 2; ++dy)
    {
        [unroll] for (uint dx = 0; dx < 2; ++dx)
        {
            uint2 p = base + uint2(dx, dy);
            // Clamp for odd input sizes.
            p = min(p, inDim - 1);

            float4 s = InImage.Load(int3(p, 0));
            float k = DepthKey(s.a);

            if (k < bestKey)
            {
                bestKey = k;
                best = s; // Keep the normal from the closest sample.
            }
        }
    }

    // Write out: RGB = normal from closest sample, A = its depth (view-space z).
    OutImage[dst] = best;
}