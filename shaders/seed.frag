#version 330 core

out vec2 seed;

uniform sampler2D line_tex;

in vec2 v_uv;

void main() {
    float a = texture(line_tex, v_uv).a;
    seed = (a > 0.0) ? v_uv : vec2(-1.0);
}
