
RWTexture3D<float4> render_texture: register(u0);

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
    float decay;
};

[numthreads(10, 10, 10)]
void main(uint3 threadIDInGroup : SV_GroupThreadID, uint3 groupID : SV_GroupID,
          uint3 dispatchThreadId : SV_DispatchThreadID, uint gidx : SV_GroupIndex){
    float4 val = render_texture[dispatchThreadId.xyz];
    val.w *= decay;
    render_texture[dispatchThreadId.xyz] = val;
}
