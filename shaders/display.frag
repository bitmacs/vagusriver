#version 330 core

out vec4 FragColor;

uniform sampler2D distance_tex;
uniform sampler2D line_tex;
uniform float max_distance;

in vec2 v_uv;

void main() {
    {
        FragColor = texture(distance_tex, v_uv);
        return;
    }
    float distance_value = texture(distance_tex, v_uv).r;
    float normalized = distance_value / max_distance;
    float shade = 1.0 - clamp(normalized, 0.0, 1.0);
    vec3 sdf_color = vec3(shade);

    vec4 line_color = texture(line_tex, v_uv);
    if (line_color.a > 0.0) {
        FragColor = vec4(1.0, 0.0, 0.0, 1.0);
    } else {
        FragColor = vec4(sdf_color, 1.0);
    }
}
