RWStructuredBuffer<float> birds_x: register(u0);
RWStructuredBuffer<float> birds_y: register(u1);
RWStructuredBuffer<float> birds_z: register(u2);

RWStructuredBuffer<float> birds_vx: register(u3);
RWStructuredBuffer<float> birds_vy: register(u4);
RWStructuredBuffer<float> birds_vz: register(u5);

RWTexture3D<float4> data_texture: register(u6);
RWTexture3D<float4> volume_texture: register(u7);

static const float PI2 = 3.1415 * 2.0f;

[numthreads(1000, 1, 1)]
void main(uint3 threadIDInGroup : SV_GroupThreadID, uint3 groupID : SV_GroupID,
          uint3 dispatchThreadId : SV_DispatchThreadID, uint gidx : SV_GroupIndex){
    int idx = dispatchThreadId.x + dispatchThreadId.y * 10000 + dispatchThreadId.z * 100000;
    float x = birds_x[idx];
    float y = birds_y[idx];
    float z = birds_z[idx];
    float vx = birds_vx[idx];
    float vy = birds_vy[idx];
    float vz = birds_vz[idx];
    uint3 pos = floor(float3(x, y, z));
    float4 result = float4(vx, vy, vz, 1);
    data_texture[pos] += result;
    volume_texture[pos] += float4(vx, vy, vz, 1);
}