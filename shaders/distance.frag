#version 330 core

layout(location = 0) out float FragDistance;

uniform sampler2D seed_tex;

in vec2 v_uv;

void main() {
    vec2 seed = texture(seed_tex, v_uv).xy;
    FragDistance = length(v_uv - seed);
}
