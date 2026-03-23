#version 330 core

// 计算点 p 到线段 ab 的距离的平方
float sdf_line_squared(vec2 p, vec2 a, vec2 b) {
    vec2 ba = b - a;
    vec2 pa = p - a;
    float t = clamp(dot(pa, ba) / dot(ba, ba), 0.0, 1.0);
    return dot(pa - ba * t, pa - ba * t);
}

out vec4 FragColor;

uniform vec2 resolution;
uniform vec2 mouse_pos;
uniform vec2 prev_mouse_pos;
uniform bool mouse_down;

uniform sampler2D color_tex;

in vec2 v_uv;

void main() {
    vec4 color = texture(color_tex, v_uv);
    if (mouse_down) {
        vec2 a = prev_mouse_pos;
        a.y = resolution.y - a.y;
        vec2 b = mouse_pos;
        b.y = resolution.y - b.y;
        float radius_squared = 10.0f * 10.0f;
        if (sdf_line_squared(v_uv * resolution, a, b) <= radius_squared) {
            color = vec4(1.0, 0.0, 0.0, 1.0);
        }
    }
    FragColor = color;
}
