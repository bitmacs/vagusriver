#version 330 core

layout(location = 0) out float FragDistance;

uniform sampler2D seed_tex;
uniform vec2 resolution;

in vec2 v_uv;

void main() {
    vec2 frag_pos = v_uv * resolution;
    vec2 seed = texture(seed_tex, v_uv).xy;
    if (seed.x < 0.0) {
        FragDistance = length(resolution);
    } else {
        FragDistance = length(seed - frag_pos);
    }
}
