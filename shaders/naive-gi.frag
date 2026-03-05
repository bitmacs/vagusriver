#version 330 core

#define PI  3.14159265358979323846
#define TAU 6.28318530717958647692 // 2 * PI

float rand(vec2 p) {
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453123);
}

out vec4 FragColor;

uniform vec2 resolution;

uniform sampler2D color_tex;

in vec2 v_uv;

void main() {
    vec4 color = texture(color_tex, v_uv);
    if (color.a == 1.0) {
        FragColor = color;
        return;
    }
    // raymarch
    int ray_count = 10;
    float one_over_ray_count = 1.0 / float(ray_count);
    float tau_over_ray_count = TAU * one_over_ray_count;
    int max_steps = 256;
    float noise = rand(v_uv);
    vec4 radiance = vec4(0.0);
    for (int i = 0; i < ray_count; ++i) {
        float angle = tau_over_ray_count * (float(i) + noise);
        vec2 ray_direction_uv = vec2(cos(angle), -sin(angle)) / resolution;

        for (int step = 0; step < max_steps; ++step) {
            vec2 sample_uv = v_uv + ray_direction_uv * float(step);
            if (sample_uv.x < 0.0 || sample_uv.x > 1.0 || sample_uv.y < 0.0 || sample_uv.y > 1.0) {
                break;
            }
            vec4 sample_light = texture(color_tex, sample_uv);
            if (sample_light.a == 1.0) {
                radiance += sample_light;
                break;
            }
        }
    }
    color = radiance * one_over_ray_count;
    FragColor = color;
}
