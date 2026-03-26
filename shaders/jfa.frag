#version 330 core

out vec2 seed;

uniform sampler2D seed_tex;
uniform vec2 resolution;
uniform float step_pixels;

in vec2 v_uv;

float distance_squared(vec2 a, vec2 b) {
    vec2 d = a - b;
    return dot(d, d);
}

vec2 sample_seed(vec2 uv) {
    return texture(seed_tex, uv).xy;
}

void main() {
    vec2 best_seed = sample_seed(v_uv);
    float best_dist = (best_seed.x >= 0.0 && best_seed.y >= 0.0) ? distance_squared(best_seed, v_uv) : 1e30;

    vec2 texel_offset = vec2(step_pixels) / resolution;
    for (int dy = -1; dy <= 1; ++dy) {
        for (int dx = -1; dx <= 1; ++dx) {
            vec2 sample_uv = v_uv + texel_offset * vec2(float(dx), float(dy));
            if (sample_uv.x < 0.0 || sample_uv.x > 1.0 || sample_uv.y < 0.0 || sample_uv.y > 1.0) {
                continue;
            }

            vec2 candidate_seed = sample_seed(sample_uv);
            if (candidate_seed.x < 0.0 || candidate_seed.y < 0.0) continue;

            float candidate_dist = distance_squared(candidate_seed, v_uv);
            if (candidate_dist < best_dist) {
                best_dist = candidate_dist;
                best_seed = candidate_seed;
            }
        }
    }
    seed = best_seed;
}
