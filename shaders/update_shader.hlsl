RWStructuredBuffer<float> birds_x: register(u0);
RWStructuredBuffer<float> birds_y: register(u1);
RWStructuredBuffer<float> birds_z: register(u2);

RWStructuredBuffer<float> birds_vx: register(u3);
RWStructuredBuffer<float> birds_vy: register(u4);
RWStructuredBuffer<float> birds_vz: register(u5);

RWTexture3D<float4> data_texture: register(u6);

cbuffer ConfigBuffer : register(b0)
{
    int world_width;
    int world_height;
    int world_depth;
    float alignment_weight;
    float avoidance_weight;
    float cohesion_weight;
    float center_attraction;
    float max_speed;
    float max_force;
    int sense_distance;
    float velocity_update_step;
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

float3 random_sphere(int seed, float max_radius) {
    float azimuth = random(seed * 314) * PI2;
    float polar = acos(2 * random(seed * 317) - 1);
    float r = random(seed * 909) * 0.95f + 0.05f;
    r = sqrt(r);
    r *= max_radius;
    float3 p = float3(sin(azimuth) * sin(polar), cos(polar), cos(azimuth) * sin(polar)) * r;
    return p;
}

[numthreads(1000, 1, 1)]
void main(uint3 threadIDInGroup : SV_GroupThreadID, uint3 groupID : SV_GroupID,
          uint3 dispatchThreadId : SV_DispatchThreadID, uint gidx : SV_GroupIndex){
    int idx = dispatchThreadId.x + dispatchThreadId.y * 10000 + dispatchThreadId.z * 100000;
    float x = birds_x[idx];
    float y = birds_y[idx];
    float z = birds_z[idx];
    float3 pos = float3(x, y, z);
    float vx = birds_vx[idx];
    float vy = birds_vy[idx];
    float vz = birds_vz[idx];
    float3 velocity = float3(vx, vy, vz);
    pos += velocity;
    pos = wrap_pos(pos);

    int3 tex_pos_in = int3(pos);

    float3 avg_velocity = 0;
    float3 avg_position = 0;
    float3 avoidance = 0;
    float counter = 0;

    // Sample environment.
    for(int i = 0; i < 32; ++i) {
        float3 dir = random_sphere(idx * i, sense_distance);
        float4 data = data_texture[wrap_pos(tex_pos_in + int3(dir))];
        float3 current_v = data.xyz;
        float current_counter = data.w;
        avg_velocity += current_v * current_counter;
        counter += current_counter;
        avg_position += dir * current_counter;
        avoidance += -normalize(dir) / length(dir) * current_counter;
    }

    // Update velocity based on boid rules
    if(counter > 0) {
        avoidance /= counter;
        avg_velocity /= counter;
        avg_position /= counter;

        float3 alignment_steer = 0;
        if(length(avg_velocity) > 0) {
            avg_velocity = normalize(avg_velocity) * max_speed;
            alignment_steer = avg_velocity - velocity;
            if(length(alignment_steer) > max_force) {
                alignment_steer = normalize(alignment_steer) * max_force;
            }
        }

        float3 avoidance_steer = 0;
        if(length(avoidance) > 0) {
            avoidance = normalize(avoidance) * max_speed;
            avoidance_steer  = avoidance - velocity;
            if(length(avoidance_steer) > max_force) {
                avoidance_steer = normalize(avoidance_steer) * max_force;
            }
        }

        float3 cohesion_steer = 0;
        if(length(avg_position) > 0) {
            avg_position = normalize(avg_position) * max_speed;
            cohesion_steer = avg_position - velocity;
            if(length(cohesion_steer) > max_force) {
                cohesion_steer = normalize(cohesion_steer) * max_force;
            }
        }

        velocity += alignment_steer * alignment_weight * velocity_update_step;
        velocity += avoidance_steer * avoidance_weight * velocity_update_step;
        velocity += cohesion_steer  * cohesion_weight * velocity_update_step;
    }

    // Update velocity based on attraction to center of the world.
    float3 relative_pos = pos - float3(world_width / 2, world_height / 2, world_depth / 2);
    float3 center_dir = -normalize(relative_pos) * max_speed;
    float3 center_steer = normalize(lerp(velocity, center_dir, 0.51f)) - velocity;
    if(length(center_steer) > max_force) center_steer = normalize(center_steer) * max_force;
    center_steer *= length(relative_pos) / 50.0f;  // This is pretty random, but sort of works

    velocity += center_attraction * center_steer * velocity_update_step;
    velocity = normalize(velocity) * max_speed;
    
    birds_x[idx] = pos.x;
    birds_y[idx] = pos.y;
    birds_z[idx] = pos.z;
    birds_vx[idx] = velocity.x;
    birds_vy[idx] = velocity.y;
    birds_vz[idx] = velocity.z;

    float4 result = float4(velocity, 1);
}