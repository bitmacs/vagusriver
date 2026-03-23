#include <GLFW/glfw3.h>
#include <glad/gl.h>
#include <glm/glm.hpp>
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <string>

std::string read_file(const char *path) {
    std::FILE *f = std::fopen(path, "rb");
    assert(f && "read_file: open failed");
    std::fseek(f, 0, SEEK_END);
    const long n = std::ftell(f);
    std::fseek(f, 0, SEEK_SET);
    std::string s;
    s.resize(n);
    const size_t nr = std::fread(s.data(), 1, n, f);
    std::fclose(f);
    assert(nr == n);
    return s;
}

void create_texture(const GladGLContext &gl, int width, int height, GLint internal_format, GLenum format, GLenum type, GLuint &texture) {
    gl.GenTextures(1, &texture);
    gl.BindTexture(GL_TEXTURE_2D, texture);
    gl.TexImage2D(GL_TEXTURE_2D, 0, internal_format, width, height, 0, format, type, nullptr);
    gl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    gl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    gl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    gl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    gl.BindTexture(GL_TEXTURE_2D, 0);
}

void create_framebuffer(const GladGLContext &gl, GLuint color_tex, GLuint &fbo) {
    gl.GenFramebuffers(1, &fbo);
    gl.BindFramebuffer(GL_FRAMEBUFFER, fbo);
    gl.FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, color_tex, 0);
    assert(gl.CheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
    gl.BindFramebuffer(GL_FRAMEBUFFER, 0);
}

int smallest_power_of_two_greater_or_equal(int value) {
    assert(value > 0);
    int power = 1;
    while (power < value) {
        power <<= 1;
    }
    return power;
}

void create_program(const GladGLContext &gl, const char *vs_code, const char *fs_code, GLuint &program) {
    GLuint vs = gl.CreateShader(GL_VERTEX_SHADER);
    gl.ShaderSource(vs, 1, &vs_code, nullptr);
    gl.CompileShader(vs);
    {
        GLint status;
        gl.GetShaderiv(vs, GL_COMPILE_STATUS, &status);
        if (status == GL_FALSE) {
            GLint log_len;
            gl.GetShaderiv(vs, GL_INFO_LOG_LENGTH, &log_len);
            std::string log(log_len, '\0');
            gl.GetShaderInfoLog(vs, log_len, &log_len, log.data());
            fprintf(stderr, "Vertex shader compile error: %s\n", log.c_str());
            assert(false);
        }
    }
    GLuint fs = gl.CreateShader(GL_FRAGMENT_SHADER);
    gl.ShaderSource(fs, 1, &fs_code, nullptr);
    gl.CompileShader(fs);
    {
        GLint status;
        gl.GetShaderiv(fs, GL_COMPILE_STATUS, &status);
        if (status == GL_FALSE) {
            GLint log_len;
            gl.GetShaderiv(fs, GL_INFO_LOG_LENGTH, &log_len);
            std::string log(log_len, '\0');
            gl.GetShaderInfoLog(fs, log_len, &log_len, log.data());
            fprintf(stderr, "Fragment shader compile error: %s\n", log.c_str());
            assert(false);
        }
    }
    program = gl.CreateProgram();
    gl.AttachShader(program, vs);
    gl.AttachShader(program, fs);
    gl.LinkProgram(program);
    {
        GLint status;
        gl.GetProgramiv(program, GL_LINK_STATUS, &status);
        if (status == GL_FALSE) {
            GLint log_len;
            gl.GetProgramiv(program, GL_INFO_LOG_LENGTH, &log_len);
            std::string log(log_len, '\0');
            gl.GetProgramInfoLog(program, log_len, &log_len, log.data());
            fprintf(stderr, "Program link error: %s\n", log.c_str());
            assert(false);
        }
    }
    gl.DeleteShader(vs);
    gl.DeleteShader(fs);
}

GLuint create_program_from_files(const GladGLContext &gl, const char *vs_path, const char *fs_path) {
    std::string vs_code = read_file(vs_path);
    std::string fs_code = read_file(fs_path);
    GLuint program;
    create_program(gl, vs_code.c_str(), fs_code.c_str(), program);
    return program;
}

struct Vertex {
    float position[2];
    float uv[2];
};

void generate_quad_geometry_data(float size, Vertex *vertices, uint32_t *indices) {
    float half_size = size * 0.5f;
    vertices[0] = {-half_size, -half_size, 0.0f, 0.0f};
    vertices[1] = {half_size, -half_size, 1.0f, 0.0f};
    vertices[2] = {-half_size, half_size, 0.0f, 1.0f};
    vertices[3] = {half_size, half_size, 1.0f, 1.0f};
    indices[0] = 0; indices[1] = 1; indices[2] = 2;
    indices[3] = 2; indices[4] = 1; indices[5] = 3;
}

struct MouseState {
    glm::vec2 pos = {0.0f, 0.0f};
    glm::vec2 prev_pos = {0.0f, 0.0f};
    bool down[2] = {false, false};
};

static MouseState mouse_state = {};

int main() {
    int status = glfwInit();
    assert(status == GLFW_TRUE);

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE); // macOS 上 Core Profile 必须
    glfwWindowHint(GLFW_COCOA_RETINA_FRAMEBUFFER, GLFW_FALSE); // 关闭 Retina 高分辨率，framebuffer 与窗口像素 1:1
#endif

    // 创建窗口和 OpenGL 上下文
    int width = 800;
    int height = 600;
    GLFWwindow *window = glfwCreateWindow(width, height, "Demo", nullptr, nullptr);
    assert(window);

    // 将上下文设为当前
    glfwMakeContextCurrent(window);

    // 为当前 context 分配 GladGLContext 并加载函数指针
    GladGLContext gl;
    int version = gladLoadGLContext(&gl, (GLADloadfunc) glfwGetProcAddress);
    assert(version != 0);

    glfwSwapInterval(1);

    // ESC 键关闭窗口
    glfwSetKeyCallback(window, [](GLFWwindow *w, int key, int, int action, int) {
        if (key == GLFW_KEY_ESCAPE && action == GLFW_RELEASE)
            glfwSetWindowShouldClose(w, GLFW_TRUE);
    });

    glfwSetCursorPosCallback(window, [](GLFWwindow *w, double xpos, double ypos) {
        mouse_state.prev_pos = mouse_state.pos;
        mouse_state.pos.x = xpos;
        mouse_state.pos.y = ypos;
    });

    glfwSetMouseButtonCallback(window, [](GLFWwindow *w, int button, int action, int mods) {
        if (action == GLFW_PRESS) {
            mouse_state.down[button] = true;
        } else if (action == GLFW_RELEASE) {
            mouse_state.down[button] = false;
        }
    });

    GLuint fbo0, color_tex0;
    create_texture(gl, width, height, GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE, color_tex0);
    create_framebuffer(gl, color_tex0, fbo0);

    GLuint fbo1, color_tex1;
    create_texture(gl, width, height, GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE, color_tex1);
    create_framebuffer(gl, color_tex1, fbo1);

    const float clear_color[4] = {0.0f, 0.0f, 0.0f, 0.0f};

    gl.BindFramebuffer(GL_FRAMEBUFFER, fbo0);
    gl.Viewport(0, 0, width, height);
    gl.ClearColor(clear_color[0], clear_color[1], clear_color[2], clear_color[3]);
    gl.Clear(GL_COLOR_BUFFER_BIT);

    gl.BindFramebuffer(GL_FRAMEBUFFER, fbo1);
    gl.Viewport(0, 0, width, height);
    gl.ClearColor(clear_color[0], clear_color[1], clear_color[2], clear_color[3]);
    gl.Clear(GL_COLOR_BUFFER_BIT);

    gl.BindFramebuffer(GL_FRAMEBUFFER, 0);

    GLuint line_program = create_program_from_files(gl, "shaders/shader.vert", "shaders/line.frag");
    GLuint seed_program = create_program_from_files(gl, "shaders/shader.vert", "shaders/seed.frag");
    GLuint jfa_program = create_program_from_files(gl, "shaders/shader.vert", "shaders/jfa.frag");
    GLuint distance_program = create_program_from_files(gl, "shaders/shader.vert", "shaders/distance.frag");
    GLuint display_program = create_program_from_files(gl, "shaders/shader.vert", "shaders/display.frag");

    GLuint quad_vao, quad_vbo, quad_ebo;
    gl.GenVertexArrays(1, &quad_vao);
    gl.GenBuffers(1, &quad_vbo);
    gl.GenBuffers(1, &quad_ebo);
    {
        Vertex vertices[4];
        uint32_t indices[6];
        generate_quad_geometry_data(2.0f, vertices, indices);

        gl.BindVertexArray(quad_vao);
        gl.BindBuffer(GL_ARRAY_BUFFER, quad_vbo);
        gl.BufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * 4, vertices, GL_STATIC_DRAW);
        gl.BindBuffer(GL_ELEMENT_ARRAY_BUFFER, quad_ebo);
        gl.BufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint32_t) * 6, indices, GL_STATIC_DRAW);
        gl.EnableVertexAttribArray(0);
        gl.VertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), nullptr);
        gl.EnableVertexAttribArray(1);
        gl.VertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *) offsetof(Vertex, uv));
        gl.BindVertexArray(0);
    }

    assert(gl.GetError() == GL_NO_ERROR);

    GLuint seed_tex[2];
    GLuint seed_fbo[2];
    for (int i = 0; i < 2; ++i) {
        create_texture(gl, width, height, GL_RG32F, GL_RG, GL_FLOAT, seed_tex[i]);
        create_framebuffer(gl, seed_tex[i], seed_fbo[i]);
    }

    GLuint distance_tex;
    GLuint distance_fbo;
    create_texture(gl, width, height, GL_R32F, GL_RED, GL_FLOAT, distance_tex);
    create_framebuffer(gl, distance_tex, distance_fbo);

    int ping_pong = 0;  // 0: 画到 fbo0，采样 tex1；1: 画到 fbo1，采样 tex0
    float max_distance = std::sqrt(static_cast<float>(width * width + height * height));

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        int write_index = ping_pong;
        int read_index = 1 - ping_pong;
        GLuint line_fbo = (write_index == 0) ? fbo0 : fbo1;
        GLuint line_target_tex = (write_index == 0) ? color_tex0 : color_tex1;
        GLuint history_tex = (read_index == 0) ? color_tex0 : color_tex1;

        gl.BindFramebuffer(GL_FRAMEBUFFER, line_fbo);
        gl.Viewport(0, 0, width, height);

        gl.UseProgram(line_program);
        gl.Uniform2f(gl.GetUniformLocation(line_program, "resolution"), (float) width, (float) height);
        gl.Uniform2f(gl.GetUniformLocation(line_program, "mouse_pos"), mouse_state.pos.x, mouse_state.pos.y);
        gl.Uniform2f(gl.GetUniformLocation(line_program, "prev_mouse_pos"), mouse_state.prev_pos.x, mouse_state.prev_pos.y);
        gl.Uniform1i(gl.GetUniformLocation(line_program, "mouse_down"), mouse_state.down[0]);
        gl.Uniform1i(gl.GetUniformLocation(line_program, "color_tex"), 0);
        gl.ActiveTexture(GL_TEXTURE0);
        gl.BindTexture(GL_TEXTURE_2D, history_tex);
        gl.BindVertexArray(quad_vao);
        gl.DrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
        assert(gl.GetError() == GL_NO_ERROR);

        ping_pong = read_index;

        gl.BindFramebuffer(GL_FRAMEBUFFER, seed_fbo[0]);
        gl.Viewport(0, 0, width, height);
        gl.UseProgram(seed_program);
        gl.Uniform2f(gl.GetUniformLocation(seed_program, "resolution"), (float) width, (float) height);
        gl.Uniform1i(gl.GetUniformLocation(seed_program, "line_tex"), 0);
        gl.ActiveTexture(GL_TEXTURE0);
        gl.BindTexture(GL_TEXTURE_2D, line_target_tex);
        gl.BindVertexArray(quad_vao);
        gl.DrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
        assert(gl.GetError() == GL_NO_ERROR);

        int seed_index = 0;
        int next_seed_index = 1;
        int max_dim = std::max(width, height);
        int step_bound = smallest_power_of_two_greater_or_equal(max_dim);
        for (int step = step_bound / 2; step >= 1; step /= 2) {
            gl.BindFramebuffer(GL_FRAMEBUFFER, seed_fbo[next_seed_index]);
            gl.Viewport(0, 0, width, height);
            gl.UseProgram(jfa_program);
            gl.Uniform2f(gl.GetUniformLocation(jfa_program, "resolution"), (float) width, (float) height);
            gl.Uniform1f(gl.GetUniformLocation(jfa_program, "step_pixels"), static_cast<float>(step));
            gl.Uniform1i(gl.GetUniformLocation(jfa_program, "seed_tex"), 0);
            gl.ActiveTexture(GL_TEXTURE0);
            gl.BindTexture(GL_TEXTURE_2D, seed_tex[seed_index]);
            gl.BindVertexArray(quad_vao);
            gl.DrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
            assert(gl.GetError() == GL_NO_ERROR);
            std::swap(seed_index, next_seed_index);
        }

        gl.BindFramebuffer(GL_FRAMEBUFFER, distance_fbo);
        gl.Viewport(0, 0, width, height);
        gl.UseProgram(distance_program);
        gl.Uniform2f(gl.GetUniformLocation(distance_program, "resolution"), (float) width, (float) height);
        gl.Uniform1i(gl.GetUniformLocation(distance_program, "seed_tex"), 0);
        gl.ActiveTexture(GL_TEXTURE0);
        gl.BindTexture(GL_TEXTURE_2D, seed_tex[seed_index]);
        gl.BindVertexArray(quad_vao);
        gl.DrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
        assert(gl.GetError() == GL_NO_ERROR);

        gl.BindFramebuffer(GL_FRAMEBUFFER, 0);
        gl.Viewport(0, 0, width, height);
        gl.UseProgram(display_program);
        gl.Uniform1f(gl.GetUniformLocation(display_program, "max_distance"), max_distance);
        gl.Uniform1i(gl.GetUniformLocation(display_program, "distance_tex"), 0);
        gl.Uniform1i(gl.GetUniformLocation(display_program, "line_tex"), 1);
        gl.ActiveTexture(GL_TEXTURE0);
        gl.BindTexture(GL_TEXTURE_2D, distance_tex);
        gl.ActiveTexture(GL_TEXTURE1);
        gl.BindTexture(GL_TEXTURE_2D, line_target_tex);
        gl.BindVertexArray(quad_vao);
        gl.DrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
        assert(gl.GetError() == GL_NO_ERROR);

        glfwSwapBuffers(window);
    }

    gl.DeleteFramebuffers(1, &distance_fbo);
    gl.DeleteTextures(1, &distance_tex);
    for (int i = 1; i >= 0; --i) {
        gl.DeleteFramebuffers(1, &seed_fbo[i]);
        gl.DeleteTextures(1, &seed_tex[i]);
    }
    gl.DeleteVertexArrays(1, &quad_vao);
    gl.DeleteBuffers(1, &quad_ebo);
    gl.DeleteBuffers(1, &quad_vbo);
    gl.DeleteProgram(display_program);
    gl.DeleteProgram(distance_program);
    gl.DeleteProgram(jfa_program);
    gl.DeleteProgram(seed_program);
    gl.DeleteProgram(line_program);
    gl.DeleteFramebuffers(1, &fbo1);
    gl.DeleteTextures(1, &color_tex1);
    gl.DeleteFramebuffers(1, &fbo0);
    gl.DeleteTextures(1, &color_tex0);
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
