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
    vec2 frag_pos = v_uv * resolution;
    vec2 best_seed = sample_seed(v_uv);
    float best_dist = (best_seed.x >= 0.0) ? distance_squared(best_seed, frag_pos) : 1e30;

    // 线稿上的像素 best_dist≈0，3×3 循环不会更新，直接跳过
    if (best_dist < 1e-6) {
        seed = best_seed;
        return;
    }

    vec2 texel_offset = vec2(step_pixels) / resolution;
    for (int dy = -1; dy <= 1; ++dy) {
        for (int dx = -1; dx <= 1; ++dx) {
            vec2 offset = texel_offset * vec2(dx, dy);
            vec2 candidate_seed = sample_seed(v_uv + offset);
            if (candidate_seed.x < 0.0) {
                continue;
            }
            float candidate_dist = distance_squared(candidate_seed, frag_pos);
            if (candidate_dist < best_dist) {
                best_dist = candidate_dist;
                best_seed = candidate_seed;
            }
        }
    }
    seed = best_seed;
}
