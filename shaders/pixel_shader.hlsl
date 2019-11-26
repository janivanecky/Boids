struct PixelInput
{
	float4 position_out: SV_POSITION;
    float2 texcoord_out: TEXCOORD;
};

Texture2D tex : register(t0);
SamplerState tex_sampler : register(s0);

float4 main(PixelInput input) : SV_TARGET
{
    float4 color = tex.Sample(tex_sampler, input.texcoord_out);
    color.w = 1.0f;
    return color;
}