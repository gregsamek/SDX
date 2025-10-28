// Blank SSAO shader for testing //////////////////////////////////////////////////////////////////

// Texture2D    prepass_texture : register(t0, space2);
// SamplerState texture_sampler : register(s0, space2);

// struct Fragment_Input
// {
//     float4 position : SV_Position;
//     float2 texcoord : TEXCOORD0;
// };

// struct Fragment_Output
// {
//     float4 color : SV_Target0;
// };

// Fragment_Output main(Fragment_Input fragment)
// {
//     float3 normal_viewspace = prepass_texture.Sample(texture_sampler, fragment.texcoord).rgb;
//     float depth_viewspace = prepass_texture.Sample(texture_sampler, fragment.texcoord).a;
    
//     Fragment_Output output;
//     output.color = depth_viewspace;
//     return output;
// }

// SSAO implementation based on common techniques /////////////////////////////////////////////////

Texture2D    prepass_texture : register(t0, space2);
SamplerState texture_sampler : register(s0, space2);

cbuffer ProjectionMatrix : register(b0, space3)
{
    float4x4 projection;     // View -> Clip. LH, depth 0..1
}

cbuffer SSAO_UBO : register(b1, space3)
{
    float2 screen_size;
    float noise_size;
    float radius; // TODO scale radius with depth?
    float bias;
    float intensity;
    float power;
    float kernel_size;
};

#define SETTINGS_RENDER_DISABLE_AO 4
cbuffer Settings_Uniform : register(b2, space3)
{
    uint settings_render;
};

struct Fragment_Input
{
    float4 position : SV_Position;
    float2 texcoord : TEXCOORD0;
};

struct Fragment_Output
{
    float4 color : SV_Target0;
};

// Hash utilities for per-pixel randomization (deterministic, no noise texture)
float2 hash22(float2 p)
{
    // Simple, cheap 2D hash to [0,1)
    float2 h = float2(
        dot(p, float2(127.1, 311.7)),
        dot(p, float2(269.5, 183.3))
    );
    return frac(sin(h) * 43758.5453);
}

// Radical inverse for Hammersley sequence
float radicalInverse_VdC(uint bits)
{
    bits = (bits << 16) | (bits >> 16);
    bits = ((bits & 0x55555555u) << 1) | ((bits & 0xAAAAAAAAu) >> 1);
    bits = ((bits & 0x33333333u) << 2) | ((bits & 0xCCCCCCCCu) >> 2);
    bits = ((bits & 0x0F0F0F0Fu) << 4) | ((bits & 0xF0F0F0F0u) >> 4);
    bits = ((bits & 0x00FF00FFu) << 8) | ((bits & 0xFF00FF00u) >> 8);
    return float(bits) * 2.3283064365386963e-10; // 1/2^32
}

float2 hammersley(uint i, uint N)
{
    return float2(float(i) / float(N), radicalInverse_VdC(i));
}

// Cosine-weighted hemisphere sampling in tangent space (z+ is the normal)
float3 sampleHemisphereCosine(float2 Xi)
{
    // Xi.x ~ [0,1), Xi.y ~ [0,1)
    float phi = 6.28318530718 * Xi.y; // 2*pi
    float cosTheta = sqrt(1.0 - Xi.x);
    float sinTheta = sqrt(Xi.x);
    float x = cos(phi) * sinTheta;
    float y = sin(phi) * sinTheta;
    float z = cosTheta;
    return float3(x, y, z);
}

// Reconstruct view-space position from UV and view-space depth, using projection._11/_22
float3 reconstructViewPosition(float2 uv, float depthVS, float2 projFocal)
{
    // Map UV (0..1, +Y down) to NDC (-1..1, +Y up) for D3D/Metal LH
    float2 ndc;
    ndc.x = uv.x * 2.0 - 1.0;
    ndc.y = 1.0 - uv.y * 2.0;

    // For LH projection with w = z:
    // ndc.x = (x * P00) / z  => x = ndc.x * z / P00
    // ndc.y = (y * P11) / z  => y = ndc.y * z / P11
    float3 posVS;
    posVS.z = max(depthVS, 1e-4); // avoid division by zero
    posVS.x = ndc.x * posVS.z / max(projFocal.x, 1e-6);
    posVS.y = ndc.y * posVS.z / max(projFocal.y, 1e-6);
    return posVS;
}

// Project a view-space position back to texture UVs
float2 projectToUV(float3 posVS, float2 projFocal)
{
    // ndc.x = (x * P00) / z, ndc.y = (y * P11) / z
    float invZ = 1.0 / max(posVS.z, 1e-4);
    float ndcX = posVS.x * projFocal.x * invZ;
    float ndcY = posVS.y * projFocal.y * invZ;

    // Convert NDC (+Y up) to texture UV (+Y down)
    float2 uv;
    uv.x = ndcX * 0.5 + 0.5;
    uv.y = (-ndcY) * 0.5 + 0.5;
    return uv;
}

Fragment_Output main(Fragment_Input fragment)
{
    if (settings_render & SETTINGS_RENDER_DISABLE_AO)
    {
        Fragment_Output out0;
        out0.color = 1.0; // no occlusion
        return out0;
    }
    float4 gbuffer = prepass_texture.SampleLevel(texture_sampler, fragment.texcoord, 0.0);
    float3 normal = normalize(gbuffer.rgb);
    float  depth_viewspace  = gbuffer.a;

    // Early out if no geometry (optional, depending on how your prepass encodes depth)
    if (depth_viewspace <= 0.0)
    {
        Fragment_Output out0;
        out0.color = 1.0; // no occlusion
        return out0;
    }

    // Pull focal lengths from projection matrix (P00, P11)
    // HLSL matrix indexing: _11 is first row/col (P00), _22 is P11
    float2 projFocal = float2(projection._11, projection._22);

    // Reconstruct current pixel's view-space position
    float3 position= reconstructViewPosition(fragment.texcoord, depth_viewspace, projFocal);

    // Build TBN from view-space normal and a per-pixel random vector
    float2 rand2 = hash22(fragment.texcoord * 8192.0); // high frequency seed in UV space
    float3 randVec = normalize(float3(rand2 * 2.0 - 1.0, 0.0)); // random direction in XY plane
    float3 tangent = normalize(randVec - normal * dot(randVec, normal));        // orthonormalize
    float3 bitangent = cross(normal, tangent);

    float3x3 TBN = float3x3(tangent, bitangent, normal);

    float occlusion = 0.0;

    // Hemisphere sampling and accumulation
    [loop]
    for (uint i = 0u; i < kernel_size; ++i)
    {
        float2 Xi = hammersley(i, kernel_size);
        float3 hemi = sampleHemisphereCosine(Xi);

        // Optional scale so more samples are closer to the center (improves stability)
        // Scale factor: s ~ [0.1, 1.0]
        float s = lerp(0.1, 1.0, (float(i) / float(kernel_size)));

        // Sample position in view space
        float3 sampleVS = position + mul(hemi, TBN) * (radius * s);

        // Project to screen UVs to fetch depth
        float2 sampleUV = projectToUV(sampleVS, projFocal);

        // Skip samples outside the screen
        [branch]
        if (any(sampleUV < 0.0) || any(sampleUV > 1.0))
            continue;

        // Fetch view-space depth at the sample's screen position
        float sampleDepthVS = prepass_texture.SampleLevel(texture_sampler, sampleUV, 0.0).a;

        // If scene depth is closer than the sample position, it's occluded.
        // LH: larger z => farther. Occlusion if (scene depth) < (sample z - bias).
        float occluded = (float)(sampleDepthVS < (sampleVS.z - bias));

        // Range attenuation to limit false positives across depth discontinuities
        float dist = abs(sampleDepthVS - position.z);
        float rangeAtten = saturate(radius / (dist + 1e-4)); // ~1 when close, ->0 when far
        // Smooth it a bit
        rangeAtten = smoothstep(0.0, 1.0, rangeAtten);

        occlusion += occluded * rangeAtten;
    }

    // Normalize and apply shaping/power/intensity
    float ao = 1.0 - (occlusion / max(float(kernel_size), 1.0));
    ao = pow(saturate(ao), power);
    ao *= intensity;
    ao = saturate(ao);

    Fragment_Output output;
    // Important: assign the scalar to the float4, per your note
    output.color = ao;
    return output;
}

/*
NOTES

there seems to be a HLSL or SDL_gpu quirk where I need to return a float4 even if I'm only using one channel
if I try to return a single float, the texture data is corrupted

I also need to assign the final output as output.color = occlusion
NOT output.color.r = occlusion
otherwise the output is corrupted in the same way

*/