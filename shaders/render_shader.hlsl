
RWStructuredBuffer<float> birds_x: register(u0);
RWStructuredBuffer<float> birds_y: register(u1);
RWStructuredBuffer<float> birds_z: register(u2);
RWStructuredBuffer<float> birds_vx: register(u4);
RWStructuredBuffer<float> birds_vy: register(u5);
RWStructuredBuffer<float> birds_vz: register(u6);

RWTexture2D<float4> render_texture: register(u3);

cbuffer ConfigBuffer : register(b0)
{
    int world_width;
    int world_height;
    int world_depth;
};

cbuffer ConfigBuffer : register(b1)
{
    float4x4 projection;
    float4x4 view;
    float3 camera_pos;
    int window_width;
    int window_height;
    float sample_weight;
    float m;
    float e;
    float g;
    float f;
};

uint wang_hash(uint seed)
{
    seed = (seed ^ 61) ^ (seed >> 16);
    seed *= 9;
    seed = seed ^ (seed >> 4);
    seed *= 0x27d4eb2d;
    seed = seed ^ (seed >> 15);
    return seed;
}

static const float PI2 = 3.1415 * 2.0f;

float random(int seed) {
    return float(wang_hash(seed) % 1000) / 1000.0f;
}

float3 random_sphere(uint hash)
{
	float a = random(hash);
    float b = random(hash + 3);
    float azimuth = a * 2 * 3.14159265;
    float polar = acos(2 * b - 1);

    float3 result;
	result.x = sin(polar) * cos(azimuth);
	result.y = cos(polar);
	result.z = sin(polar) * sin(azimuth);

	return result;
}

[numthreads(1000, 1, 1)]
void main(uint3 threadIDInGroup : SV_GroupThreadID, uint3 groupID : SV_GroupID,
          uint3 dispatchThreadId : SV_DispatchThreadID, uint gidx : SV_GroupIndex){
    int idx = dispatchThreadId.x + dispatchThreadId.y * 10000 + dispatchThreadId.z * 100000;
    float x = birds_x[idx];
    float y = birds_y[idx];
    float z = birds_z[idx];
    float4 pos = float4(x, y, z, 1);
    float vx = birds_vx[idx];
    float vy = birds_vy[idx];
    float vz = birds_vz[idx];
    float3 v = float3(vx, vy, vz);
    pos.xyz /= float3(world_width, world_height, world_depth);
    pos.xyz = pos.xyz * 2.0f - 1.0f;
    pos = mul(view, pos);

    float d = length(pos.xyz);
    float r = m * pow(abs(f - d) / g, e);
    for (int i = 0; i < 32; ++i) {
        float3 sample_pos = pos;
        sample_pos.xyz += random_sphere(idx * 33 + i) * r;

        // Project from 3D to 2D.
        float4 out_posf = mul(projection, sample_pos);
        out_posf /= out_posf.w;
        out_posf = out_posf * 0.5 + 0.5;
        out_posf.xy *= float2(window_width, window_height);
        
        // Store sample to output texture.
        uint2 out_pos = uint2(out_posf.xy);
        render_texture[out_pos] += sample_weight / 32 * float4(normalize(v) * 0.5f + 0.5f, 1.0f);
    }
}
