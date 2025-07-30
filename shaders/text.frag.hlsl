Texture2D<float4> tex : register(t0, space2);
SamplerState samp : register(s0, space2);

struct PSInput {
    float4 color : TEXCOORD0;
    float2 tex_coord : TEXCOORD1;
};

struct PSOutput {
    float4 color : SV_Target;
};

// PSOutput main(PSInput input) {
//     PSOutput output;
//     output.color = input.color * tex.Sample(samp, input.tex_coord);
//     return output;
// }

// SDF Rendering
PSOutput main(PSInput input) {
    PSOutput output;
    const float smoothing = (1.0 / 8.0);
    float distance = tex.Sample(samp, input.tex_coord).a;
    float alpha = smoothstep(0.5 - smoothing, 0.5 + smoothing, distance);
    output.color = float4(input.color.rgb, input.color.a * alpha);
    return output;
}
