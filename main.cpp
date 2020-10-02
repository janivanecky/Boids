#include "platform.h"
#include "graphics.h"
#include "file_system.h"
#include "maths.h"
#include "memory.h"
#include "ui.h"
#include "font.h"
#include "input.h"
#include <cassert>
#include <mmsystem.h>

float quad_vertices[] = {
    -1.0f, -1.0f, 0.0f, 1.0f,
    0.0f, 0.0f,
    1.0f, 1.0f, 0.0f, 1.0f,
    1.0f, 1.0f,
    -1.0f, 1.0f, 0.0f, 1.0f,
    0.0f, 1.0f,

    -1.0f, -1.0f, 0.0f, 1.0f,
    0.0f, 0.0f,
    1.0f, -1.0f, 0.0f, 1.0f,
    1.0f, 0.0f,
    1.0f, 1.0f, 0.0f, 1.0f,
    1.0f, 1.0f,
};

uint32_t quad_vertices_stride = sizeof(float) * 6;
uint32_t quad_vertices_count = 6;

ComputeShader get_compute_shader(char *file_path) {
    File shader_file = file_system::read_file(file_path);
    ComputeShader shader = graphics::get_compute_shader_from_code((char *)shader_file.data, shader_file.size);
    file_system::release_file(shader_file);
    assert(graphics::is_ready(&shader));
    return shader;
}

int main(int argc, char **argv) {
    // Set up window
    uint32_t window_width = 1600, window_height = 900;
 	Window window = platform::get_window("Boids", window_width, window_height);
    uint32_t size = 200;
    uint32_t world_width = size, world_height = size, world_depth = size;
    assert(platform::is_window_valid(&window));

    const int NUM_BIRDS = 1000000;

    // Init graphics
    graphics::init();
    graphics::init_swap_chain(&window);

    font::init();
    ui::init((float)window_width, (float)window_height);
    ui::set_input_responsive(true);

    // Create window render target
	RenderTarget render_target_window = graphics::get_render_target_window(true);
    assert(graphics::is_ready(&render_target_window));
    DepthBuffer depth_buffer = graphics::get_depth_buffer(window_width, window_height);
    assert(graphics::is_ready(&depth_buffer));
    graphics::set_render_targets_viewport(&render_target_window, &depth_buffer);

    // Compute shaders init.
    ComputeShader diffuse_shader_x = get_compute_shader("diffuse_shader_x.hlsl");
    ComputeShader diffuse_shader_y = get_compute_shader("diffuse_shader_y.hlsl");
    ComputeShader diffuse_shader_z = get_compute_shader("diffuse_shader_z.hlsl");
    ComputeShader update_shader = get_compute_shader("update_shader.hlsl");
    ComputeShader storage_shader = get_compute_shader("storage_shader.hlsl");
    ComputeShader decay_shader = get_compute_shader("decay_shader.hlsl");
    ComputeShader render_shader = get_compute_shader("render_shader.hlsl");

    // Vertex shader for displaying textures.
    File vertex_shader_file = file_system::read_file("vertex_shader.hlsl"); 
    VertexShader vertex_shader_2d = graphics::get_vertex_shader_from_code((char *)vertex_shader_file.data, vertex_shader_file.size);
    file_system::release_file(vertex_shader_file);
    assert(graphics::is_ready(&vertex_shader_2d));

    // Pixel shader for displaying textures.
    File pixel_shader_file = file_system::read_file("pixel_shader.hlsl"); 
    PixelShader pixel_shader_2d = graphics::get_pixel_shader_from_code((char *)pixel_shader_file.data, pixel_shader_file.size);
    file_system::release_file(pixel_shader_file);
    assert(graphics::is_ready(&pixel_shader_2d));

    pixel_shader_file = file_system::read_file("raytrace_shader.hlsl"); 
    PixelShader raytrace_shader = graphics::get_pixel_shader_from_code((char *)pixel_shader_file.data, pixel_shader_file.size);
    file_system::release_file(pixel_shader_file);
    assert(graphics::is_ready(&raytrace_shader));
    
    // Textures
    //Texture2D render_texture = graphics::get_texture2D(NULL, window_width, window_height, DXGI_FORMAT_R32_FLOAT, 4);
    Texture2D render_texture = graphics::get_texture2D(NULL, window_width, window_height, DXGI_FORMAT_R32G32B32A32_FLOAT, 16);
    Texture3D data_texture_a = graphics::get_texture3D(NULL, world_width, world_height, world_depth, DXGI_FORMAT_R32G32B32A32_FLOAT, 16);
    Texture3D data_texture_b = graphics::get_texture3D(NULL, world_width, world_height, world_depth, DXGI_FORMAT_R32G32B32A32_FLOAT, 16);
    Texture3D volume_texture = graphics::get_texture3D(NULL, world_width, world_height, world_depth, DXGI_FORMAT_R32G32B32A32_FLOAT, 16);

    float *birds_x_data = memory::alloc_heap<float>(NUM_BIRDS);
    float *birds_y_data = memory::alloc_heap<float>(NUM_BIRDS);
    float *birds_z_data = memory::alloc_heap<float>(NUM_BIRDS);
    float *birds_vx_data = memory::alloc_heap<float>(NUM_BIRDS);
    float *birds_vy_data = memory::alloc_heap<float>(NUM_BIRDS);
    float *birds_vz_data = memory::alloc_heap<float>(NUM_BIRDS);
    
    for(int i = 0; i < NUM_BIRDS; ++i) {
        birds_x_data[i] = math::random_uniform(0.0f, float(world_width));
        birds_y_data[i] = math::random_uniform(0.0f, float(world_height));
        birds_z_data[i] = math::random_uniform(0.0f, float(world_height));

        float random_azimuth = math::random_uniform(0, math::PI2);
        float random_polar = math::random_uniform(0, math::PI);
        birds_vx_data[i] = math::sin(random_azimuth) * math::sin(random_polar);
        birds_vy_data[i] = math::cos(random_polar);
        birds_vz_data[i] = math::cos(random_azimuth) * math::sin(random_polar);
    }

    StructuredBuffer birds_x = graphics::get_structured_buffer(sizeof(float), NUM_BIRDS);
    graphics::update_structured_buffer(&birds_x, birds_x_data);
    StructuredBuffer birds_y = graphics::get_structured_buffer(sizeof(float), NUM_BIRDS);
    graphics::update_structured_buffer(&birds_y, birds_y_data);
    StructuredBuffer birds_z = graphics::get_structured_buffer(sizeof(float), NUM_BIRDS);
    graphics::update_structured_buffer(&birds_z, birds_z_data);
    StructuredBuffer birds_vx = graphics::get_structured_buffer(sizeof(float), NUM_BIRDS);
    graphics::update_structured_buffer(&birds_vx, birds_vx_data);
    StructuredBuffer birds_vy = graphics::get_structured_buffer(sizeof(float), NUM_BIRDS);
    graphics::update_structured_buffer(&birds_vy, birds_vy_data);
    StructuredBuffer birds_vz = graphics::get_structured_buffer(sizeof(float), NUM_BIRDS);
    graphics::update_structured_buffer(&birds_vz, birds_vz_data);

	graphics::set_blend_state(BlendType::ALPHA);
    TextureSampler tex_sampler = graphics::get_texture_sampler(SampleMode::WRAP);

    // Quad mesh for rendering the resulting texture.
    Mesh quad_mesh = graphics::get_mesh(quad_vertices, quad_vertices_count, quad_vertices_stride, NULL, 0, 0);

    // Constant buffers.
    struct Config {
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
        int filler2;
    };
    Config config = {
        world_width,
        world_height,
        world_depth,
        1.0f,
        1.0f,
        1.0f,
        0.25f,
        1.0f, 
        1.0f,
        8,
        0.5f,
    };
    ConstantBuffer config_buffer = graphics::get_constant_buffer(sizeof(Config));

    // Set up 3D rendering matrices buffer.
    float aspect_ratio = float(window_width) / float(window_height);
    Matrix4x4 projection_matrix = math::get_perspective_projection_dx_rh(math::deg2rad(60.0f), aspect_ratio, 0.01f, 10.0f);
    float azimuth = 0.0f;
    float polar = math::PIHALF;
    float radius = 2.0f;
    Vector3 eye_pos = Vector3(math::cos(azimuth) * math::sin(polar), math::cos(polar), math::sin(azimuth) * math::sin(polar)) * radius;
    Matrix4x4 view_matrix = math::get_look_at(eye_pos, Vector3(0,0,0), Vector3(0,1,0));
    
    struct RenderSettings{
        Matrix4x4 projection_matrix;
        Matrix4x4 view_matrix;
        Vector3 camera_pos;
        int window_width;

        int window_height;
        float sample_weight;
        float dof_size;
        float dof_distribution;

        float dof_distance;
        float dof_depth;
        float decay;
        int filler1;
    };
    RenderSettings render_settings = {
        projection_matrix,
        view_matrix,
        eye_pos,
        window_width,
        window_height,
        0.1f,
        0.0f,
        3.0f,
        1.0f,
        1.0f,
        0.0f,
    };
    ConstantBuffer render_buffer = graphics::get_constant_buffer(sizeof(RenderSettings));

    Timer timer = timer::get();
    timer::start(&timer);

    // Render loop
    bool is_running = true;
    bool show_ui = false;
    bool pause = false;
    bool ray_trace = false;
    int boid_count = 1000;
    
    while(is_running)
    {
        printf("%f\n", timer::checkpoint(&timer));
        input::reset();
    
        // Event loop
        Event event;
        while(platform::get_event(&event))
        {
            input::register_event(&event);

            // Check if close button pressed
            switch(event.type)
            {
                case EventType::EXIT:
                {
                    is_running = false;
                }
                break;
            }
        }
        // React to inputs
        {
            if (input::key_pressed(KeyCode::ESC)) is_running = false; 
            if (input::key_pressed(KeyCode::F1)) show_ui = !show_ui;
            if (input::key_pressed(KeyCode::F4)) pause = !pause;

            if(!ui::is_registering_input()) {
                radius -= input::mouse_scroll_delta() * 0.1f;

                if (input::mouse_left_button_down()) {
                    Vector2 dm = input::mouse_delta_position();
                    azimuth += dm.x * 0.003f;
                    polar -= dm.y * 0.003f;
                    polar = math::clamp(polar, 0.02f, math::PI);
                }
            }
            Vector3 eye_pos = Vector3(
                math::cos(azimuth) * math::sin(polar),
                math::cos(polar),
                math::sin(azimuth) * math::sin(polar)
            ) * radius;
            render_settings.view_matrix = math::get_look_at(eye_pos, Vector3(0,0,0), Vector3(0,1,0));;
            render_settings.camera_pos = eye_pos;
        }

        float clear_tex[4] = {0, 0, 0, 0};
        graphics_context->context->ClearUnorderedAccessViewFloat(render_texture.ua_view, clear_tex);
        graphics_context->context->ClearUnorderedAccessViewFloat(data_texture_a.ua_view, clear_tex);

        // Set constant buffers.
        graphics::set_constant_buffer(&config_buffer, 0);
        graphics::update_constant_buffer(&config_buffer, &config);

        graphics::set_constant_buffer(&render_buffer, 1);
        graphics::update_constant_buffer(&render_buffer, &render_settings);

        if(!pause) {
            graphics::set_compute_shader(&decay_shader);
            graphics::set_texture_compute(&volume_texture, 0);
            graphics::run_compute(world_width / 10, world_height / 10, world_depth / 10);

            graphics::set_compute_shader(&storage_shader);
            graphics::set_structured_buffer(&birds_x, 0);
            graphics::set_structured_buffer(&birds_y, 1);
            graphics::set_structured_buffer(&birds_z, 2);
            graphics::set_structured_buffer(&birds_vx, 3);
            graphics::set_structured_buffer(&birds_vy, 4);
            graphics::set_structured_buffer(&birds_vz, 5);
            graphics::set_texture_compute(&data_texture_a, 6);
            graphics::set_texture_compute(&volume_texture, 7);
            graphics::run_compute(boid_count, 1, 1);

            graphics::set_compute_shader(&diffuse_shader_x);
            graphics::set_texture_compute(&data_texture_a, 0);
            graphics::set_texture_compute(&data_texture_b, 1);
            graphics::run_compute(world_width / 10, world_height / 10, world_depth / 10);
            
            graphics::set_compute_shader(&diffuse_shader_y);
            graphics::set_texture_compute(&data_texture_b, 0);
            graphics::set_texture_compute(&data_texture_a, 1);
            graphics::run_compute(world_width / 10, world_height / 10, world_depth / 10);

            graphics::set_compute_shader(&diffuse_shader_z);
            graphics::set_texture_compute(&data_texture_a, 0);
            graphics::set_texture_compute(&data_texture_b, 1);
            graphics::run_compute(world_width / 10, world_height / 10, world_depth / 10);

            graphics::set_compute_shader(&update_shader);
            graphics::set_structured_buffer(&birds_x, 0);
            graphics::set_structured_buffer(&birds_y, 1);
            graphics::set_structured_buffer(&birds_z, 2);
            graphics::set_structured_buffer(&birds_vx, 3);
            graphics::set_structured_buffer(&birds_vy, 4);
            graphics::set_structured_buffer(&birds_vz, 5);
            graphics::set_texture_compute(&data_texture_b, 6);
            graphics::run_compute(boid_count, 1, 1);
        }

        graphics::set_compute_shader(&render_shader);
        graphics::set_structured_buffer(&birds_x, 0);
        graphics::set_structured_buffer(&birds_y, 1);
        graphics::set_structured_buffer(&birds_z, 2);
        graphics::set_texture_compute(&render_texture, 3);
        graphics::set_structured_buffer(&birds_vx, 4);
        graphics::set_structured_buffer(&birds_vy, 5);
        graphics::set_structured_buffer(&birds_vz, 6);
        graphics::run_compute(boid_count, 1, 1);

        graphics::unset_texture_compute(0);
        graphics::unset_texture_compute(3);
        graphics::unset_texture_compute(6);
        graphics::unset_texture_compute(7);

        // Clear screen.
        graphics::set_render_targets_viewport(&render_target_window);
        graphics::clear_render_target(&render_target_window, 0.0f, 0.0f, 0.0f, 1);
            
        // Draw.
        graphics::set_vertex_shader(&vertex_shader_2d);
        graphics::set_texture_sampler(&tex_sampler, 0);
        if(ray_trace) {
            graphics::set_pixel_shader(&raytrace_shader);
            graphics::set_texture(&volume_texture, 0);
        } else {
            graphics::set_pixel_shader(&pixel_shader_2d);
            graphics::set_texture(&render_texture, 0);
        }
        graphics::draw_mesh(&quad_mesh);

        graphics::unset_texture(0);
        graphics::unset_texture(1);

        if(show_ui) {
            Panel panel = ui::start_panel("", Vector2(10, 10.0f), 420.0f);
            ui::add_slider(&panel, "ALIGNMENT", &config.alignment_weight, 0, 2);
            ui::add_slider(&panel, "AVOIDANCE", &config.avoidance_weight, 0, 2);
            ui::add_slider(&panel, "COHESION", &config.cohesion_weight, 0, 2);
            ui::add_slider(&panel, "CENTER ATTRACTION", &config.center_attraction, 0, 2);
            ui::add_slider(&panel, "MAX SPEED", &config.max_speed, 0, 5);
            ui::add_slider(&panel, "MAX FORCE", &config.max_force, 0, 5);
            ui::add_slider(&panel, "SENSE DISTANCE", &config.sense_distance, 0, 64);
            ui::add_slider(&panel, "VELOCITY UPDATE", &config.velocity_update_step, 0, 1.0f);
            ui::add_slider(&panel, "SAMPLE WEIGHT", &render_settings.sample_weight, 0.001, 1.0f);
            ui::add_slider(&panel, "BOID COUNT", &boid_count, 1, 1000);
            ui::add_toggle(&panel, "RAY TRACE", &ray_trace);

            if(!ray_trace) {
                ui::add_slider(&panel, "DOF SIZE", &render_settings.dof_size, 0.0f, 2.0f);
                ui::add_slider(&panel, "DOF DIST", &render_settings.dof_distribution, 0.0f, 4.0f);
                ui::add_slider(&panel, "DOF DISTANCE", &render_settings.dof_distance, 0.0f, 10.0f);
                ui::add_slider(&panel, "DOF DEPTH", &render_settings.dof_depth, 0.0f, 5.0f);
            } 

            ui::add_slider(&panel, "DECAY", &render_settings.decay, 0.0f, 1.0f);

            ui::end_panel(&panel);
            ui::end();
        }

        graphics::swap_frames();
        
        //Texture *temp = data_texture_in;
        //data_texture_in = data_texture_out;
        //data_texture_out = temp;
    }

    ui::release();

    graphics::release();

    return 0;
}