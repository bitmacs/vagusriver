#version 330 core

#define M_2_PI 6.28318530717958647692
#define epsilon 0.0001

float rand(vec2 p) {
    vec3 h = fract(vec3(p.xyx) * vec3(0.1031, 0.1030, 0.0973));
    h += dot(h, h.yzx + 33.33);
    return fract((h.x + h.y) * h.z);
}

out vec4 FragColor;

uniform sampler2D scene_tex;
uniform sampler2D distance_tex;
uniform float rand_seed;

in vec2 v_uv;

void main() {
    vec4 scene_color = texture(scene_tex, v_uv);

    if (scene_color.a > 0.0) {
        FragColor = scene_color;
        return;
    }

    const int ray_count = 64;
    const int max_steps = 16;

    float one_over_ray_count = 1.0 / float(ray_count);
    float ray_angle_step = M_2_PI * one_over_ray_count;

    float offset = rand(gl_FragCoord.xy + rand_seed);

    vec4 radiance = vec4(0.0);

    vec4 sky = vec4(0.02, 0.08, 0.2, 1.0);

    for (int i = 0; i < ray_count; ++i) {
        float angle = ray_angle_step * (float(i) + offset);
        vec2 ray_direction = vec2(cos(angle), -sin(angle));

        vec2 sample_uv = v_uv;
        vec4 radiance_delta = vec4(0.0);
        bool hit = false;

        for (int step = 0; step < max_steps; ++step) {
            float d = texture(distance_tex, sample_uv).r;
            sample_uv += ray_direction * d;
            if (sample_uv.x < 0.0 || sample_uv.x > 1.0 || sample_uv.y < 0.0 || sample_uv.y > 1.0) {
                break;
            }

            if (d < epsilon) {
                radiance_delta = texture(scene_tex, sample_uv);
                hit = true;
                break;
            }
        }

        if (!hit) { radiance_delta = sky; }

        radiance += radiance_delta;
    }

    FragColor = vec4(radiance.rgb * one_over_ray_count, 1.0);
}
