RWTexture3D<float4> tex_in: register(u0);
RWTexture3D<float4> tex_out: register(u1);

cbuffer ConfigBuffer : register(b0)
{
    int world_width;
    int world_height;
    int world_depth;
    float alignment_weight;
    float avoidance_weight;
    float cohesion_weight;
    float max_speed;
    float max_force;
};

float3 wrap_pos(float3 pos) {
    if(pos.x < 0) {
        pos.x += world_width;
    }
    if(pos.y < 0) {
        pos.y += world_height;
    }
    if(pos.z < 0) {
        pos.z += world_depth;
    }
    if(pos.x >= world_width) {
        pos.x -= world_width;
    }
    if(pos.y >= world_height) {
        pos.y -= world_height;
    }
    if(pos.z >= world_depth) {
        pos.z -= world_depth;
    }
    return pos;
}


static const float PI2 = 3.1415 * 2.0f;

[numthreads(10, 10, 10)]
void main(uint3 threadIDInGroup : SV_GroupThreadID, uint3 groupID : SV_GroupID,
          uint3 dispatchThreadId : SV_DispatchThreadID, uint gidx : SV_GroupIndex){
    int3 idx = dispatchThreadId.xyz;
    float4 val = 0;
    for(int x = -1; x <= 1; ++x) {
        val += tex_in[wrap_pos(idx + int3(x, 0, 0))] / 3.0f;
    }
    tex_out[idx] = val;
}