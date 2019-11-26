struct PixelInput
{
	float4 position_out: SV_POSITION;
    float2 texcoord_out: TEXCOORD;
};

Texture3D tex : register(t0);
Texture2D noise_tex : register(t5);
SamplerState tex_sampler : register(s0);

cbuffer ConfigBuffer : register(b1)
{
    float4x4 projection;
    float4x4 view;
    float3 camera_pos;
    int window_width;
    int window_height;
	float sample_weight;
};

float ray_box(float3 rd, float3 rs, float3 box_size) {
    float3 m = 1.0 / rd; // can precompute if traversing a set of aligned boxes
    float3 n = m * rs;   // can precompute if traversing a set of aligned boxes
    float3 k = abs(m) * box_size;
    float3 t1 = -n - k;
    float3 t2 = -n + k;
    float tN = max(max(t1.x, t1.y), t1.z);
    float tF = min(min(t2.x, t2.y), t2.z);
    if (tN > tF || tF < 0.0) return -1;//vec2(-1.0); // no intersection
    return tN;
}

float4 f(float3 p) {
	p = p * 0.5f + 0.5f;
	//p.yz = 1 - p.yz;
	return tex.Sample(tex_sampler, p);
}

float4 trace_tex(float3 rd, float3 rs, float t) {
	int NUM_STEPS = 512;//256;
	float step_size = sqrt(16.0f) / NUM_STEPS;
    float4 color = 0;
	// Initial step, so we're sure we're inside the texture box.
	t += step_size;
	[loop]
	//[unroll(512)]
	for (int i = 0; i < NUM_STEPS; ++i) {
		float3 p = rd * t + rs;
		if(p.x > 1 || p.y > 1 || p.z > 1 || p.x < -1 || p.y < - 1 || p.z < -1) {
			break;
		}
		float4 val = f(p);
		
		if(val.w > 0.0f) {
			val.xyz = normalize(val.xyz) * 0.5f + 0.5f;
			val.w *= sample_weight;
			color.xyz += val.xyz * val.w;
			color.w += val.w;
			if(color.w > 1.0f) {
				break;
			}
		}
		t += step_size;
	}
	return color;
}

float3x3 get_view_matrix(float3 cam_pos) {
	float3 y = float3(0,1,0);
	float3 z = normalize(cam_pos);
	float3 x = normalize(cross(y, z));
	y = normalize(cross(z, x));

	return float3x3(
		x.x, y.x, z.x,
		x.y, y.y, z.y,
		x.z, y.z, z.z);
}

float4 trace_ray(float3 rs, float3 rd) {
	float t = ray_box(rd, rs, float3(1, 1, 1));
	bool is_in_box = rs.x < 1 && rs.x > -1 && rs.y < 1 && rs.y > -1 && rs.z < 1 && rs.z > -1;
	if (is_in_box) {
		t = 0.0f;
	}
	if (t > 0 || is_in_box) {
		float4 h = trace_tex(rd, rs, t);
        return h;
    }
	return float4(-1, -1, -1, -1);
}

static const float PI = 3.14159265359;

float4 main(PixelInput input) : SV_TARGET
{
	float4 color = float4(0, 0, 0, 1);
	for (int y = 0; y < 1; ++y)
	for (int x = 0; x < 1; ++x) {
		//float2 offset = float2((x - 0.5f)/ 1400.0f, (y - 0.5f) / 900.0f);
		float2 offset = float2(0,0);
		float2 pixel_pos = input.texcoord_out.xy * 2.0 - 1.0f;
		pixel_pos.y *= float(window_height) / float(window_width);
		pixel_pos += offset;
		float3x3 v = get_view_matrix(camera_pos);
		float3 rs = camera_pos;
		float3 rd = normalize(float3(pixel_pos, -1.0f));
		rd = mul(v, rd);

    	float4 c = trace_ray(rs, rd);
        if(c.w > 0) {
		    color.xyz += float3(c.xyz);
            color.w += c.w;
        }

	}
	color = saturate(color);
	//color /= 4.0f;
	return color;
}