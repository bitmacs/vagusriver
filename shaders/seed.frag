#version 330 core

out vec2 seed;

uniform sampler2D line_tex;
uniform vec2 resolution;

in vec2 v_uv;

void main() {
    vec4 color = texture(line_tex, v_uv);
    if (color.a > 0.0) {
        seed = v_uv * resolution;
    } else {
        seed = vec2(-1.0);
    }
}
