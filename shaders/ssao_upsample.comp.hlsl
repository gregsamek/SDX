// Inputs:
//  - AO_Low:       half-resolution SSAO (r16f)
//  - Guide_Low:    half-resolution guide (RGBA16F: RGB = normal, A = view-space z) produced by your min-depth downsample
//  - Guide_Full:   full-resolution guide (RGBA16F: RGB = normal, A = view-space z) from your prepass
// Output:
//  - AO_High:      full-resolution upsampled SSAO (r16f), edge-aware filtered

Texture2D<float>     AO_Low      : register(t0, space0);
Texture2D<float4>    Guide_Low   : register(t1, space0);
Texture2D<float4>    Guide_Full  : register(t2, space0);

[[vk::image_format("r16f")]]
RWTexture2D<float>   AO_High     : register(u0, space1);

cbuffer UBO_Upsample : register(b0, space2)
{
    float sigmaSpatial; // in low-res pixel units (e.g. 1.0)
    float sigmaDepth;   // in view-space z units (e.g. 1.0 .. 3.0 depending on your scale)
    float sigmaNormal;  // angular softness via (1 - dot(n)) scaling (e.g. 0.1 .. 0.3)
    float normalPower;  // additional sharpening via pow(dot, normalPower) (e.g. 8 .. 32). Set to 1 to disable.
}

static const float kEps = 1e-6f;

float3 SafeNormalize(float3 v)
{
    float l2 = max(dot(v, v), kEps);
    return v * rsqrt(l2);
}

float SpatialWeight(int dx, int dy, float sigmaS)
{
    float d2 = float(dx * dx + dy * dy);
    float inv2Sig2 = 0.5f / max(sigmaS * sigmaS, 1e-6f);
    return exp(-d2 * inv2Sig2);
}

float DepthWeight(float zFull, float zLow, float sigmaZ)
{
    // Use absolute view-space distance along the view ray.
    float dz = abs(abs(zFull) - abs(zLow));
    float invSig = 1.0f / max(sigmaZ, 1e-6f);
    return exp(-dz * invSig);
}

float NormalWeight(float3 nFull, float3 nLow, float sigmaN, float nPow)
{
    float nd = saturate(dot(nFull, nLow));
    // Two components:
    //  - exponential term in (1 - dot) to softly gate across edges
    //  - power term to increase selectivity near edges
    float wExp = exp(-(1.0f - nd) / max(sigmaN, 1e-6f));
    float wPow = (nPow > 1.0f) ? pow(nd, nPow) : nd;
    return wExp * wPow;
}

float BilinearAO(Texture2D<float> tex, float2 lowCoord, uint2 lowDim)
{
    int2 p00 = int2(floor(lowCoord));
    float2 f = lowCoord - float2(p00);

    int2 p10 = p00 + int2(1, 0);
    int2 p01 = p00 + int2(0, 1);
    int2 p11 = p00 + int2(1, 1);

    p00 = clamp(p00, int2(0, 0), int2(lowDim) - 1);
    p10 = clamp(p10, int2(0, 0), int2(lowDim) - 1);
    p01 = clamp(p01, int2(0, 0), int2(lowDim) - 1);
    p11 = clamp(p11, int2(0, 0), int2(lowDim) - 1);

    float a00 = tex.Load(int3(p00, 0));
    float a10 = tex.Load(int3(p10, 0));
    float a01 = tex.Load(int3(p01, 0));
    float a11 = tex.Load(int3(p11, 0));

    float a0 = lerp(a00, a10, f.x);
    float a1 = lerp(a01, a11, f.x);
    return lerp(a0, a1, f.y);
}

[numthreads(8, 8, 1)]
void main(uint3 GlobalInvocationID : SV_DispatchThreadID)
{
    uint2 outDim;
    AO_High.GetDimensions(outDim.x, outDim.y);

    uint2 dst = GlobalInvocationID.xy;
    if (dst.x >= outDim.x || dst.y >= outDim.y)
        return;

    uint2 lowDim;
    AO_Low.GetDimensions(lowDim.x, lowDim.y);

    // Full-res guide at the output pixel
    float4 gFull4 = Guide_Full.Load(int3(int2(dst), 0));
    float3 nFull  = SafeNormalize(gFull4.rgb);
    float  zFull  = gFull4.a;

    // Map full-res pixel center to low-res pixel space
    float2 uv       = (float2(dst) + 0.5f) / float2(outDim);
    float2 lowCoord = uv * float2(lowDim) - 0.5f;
    int2   base     = int2(floor(lowCoord));

    // 3x3 cross-bilateral in low-res space centered at the mapped coordinate
    float aoSum = 0.0f;
    float wSum  = 0.0f;

    [unroll]
    for (int dy = -1; dy <= 1; ++dy)
    {
        [unroll]
        for (int dx = -1; dx <= 1; ++dx)
        {
            int2 p = clamp(base + int2(dx, dy), int2(0, 0), int2(lowDim) - 1);

            float  ao    = AO_Low.Load(int3(p, 0));
            float4 gLow4 = Guide_Low.Load(int3(p, 0));
            float3 nLow  = SafeNormalize(gLow4.rgb);
            float  zLow  = gLow4.a;

            float wS = SpatialWeight(dx, dy, sigmaSpatial);
            float wD = DepthWeight(zFull, zLow, sigmaDepth);
            float wN = NormalWeight(nFull, nLow, sigmaNormal, normalPower);

            float w = wS * wD * wN;

            aoSum += w * ao;
            wSum  += w;
        }
    }

    float aoOut;
    if (wSum < 1e-4f)
    {
        // Fallback: bilinear sample when guidance rejects all neighbors
        aoOut = BilinearAO(AO_Low, lowCoord, lowDim);
    }
    else
    {
        aoOut = aoSum / wSum;
    }

    // Optional: clamp to a sane range
    AO_High[dst] = saturate(aoOut);
}