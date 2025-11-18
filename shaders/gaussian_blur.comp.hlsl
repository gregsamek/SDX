Texture2D<float4> texture_in    : register(t0, space0);
SamplerState      sampler_linear: register(s0, space0);

[[vk::image_format("rgba16f")]]
RWTexture2D<float4> texture_out : register(u0, space1);

cbuffer GaussianBlurParams : register(b0, space2)
{
    uint horizontal; // 0 = vertical, 1 = horizontal
    float stride;    // not used in this implementation, but could be for adjustable blur radius
    float2 _pad;     // keep cbuffer aligned (optional)
}

static const float weight[5] = { 0.2270270270, 0.1945945946, 0.1216216216, 0.0540540541, 0.0162162162 };

[numthreads(8, 8, 1)]
void main(uint3 gid : SV_DispatchThreadID)
{
    uint2 pix = gid.xy;

    uint w, h;
    texture_in.GetDimensions(w, h);

    if (pix.x >= w || pix.y >= h) return;

    float2 texel = 1.0 / float2(w, h);
    float2 uv = (float2(pix) + 0.5) * texel;

    float3 result = texture_in.SampleLevel(sampler_linear, uv, 0).rgb * weight[0];

    if (horizontal != 0)
    {
        [unroll]
        for (int i = 1; i < 5; ++i)
        {
            result += texture_in.SampleLevel(sampler_linear, uv + float2(texel.x * i * stride, 0.0), 0).rgb * weight[i];
            result += texture_in.SampleLevel(sampler_linear, uv - float2(texel.x * i * stride, 0.0), 0).rgb * weight[i];
        }
    }
    else
    {
        [unroll]
        for (int i = 1; i < 5; ++i)
        {
            result += texture_in.SampleLevel(sampler_linear, uv + float2(0.0, texel.y * i * stride), 0).rgb * weight[i];
            result += texture_in.SampleLevel(sampler_linear, uv - float2(0.0, texel.y * i * stride), 0).rgb * weight[i];
        }
    }

    texture_out[pix] = float4(result, 1.0);
}