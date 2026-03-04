#include <GLFW/glfw3.h>
#include <glad/gl.h>
#include <glm/glm.hpp>
#include <cassert>
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

void create_texture(const GladGLContext &gl, int width, int height, GLuint &texture) {
    gl.GenTextures(1, &texture);
    gl.BindTexture(GL_TEXTURE_2D, texture);
    gl.TexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    gl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    gl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    gl.BindTexture(GL_TEXTURE_2D, 0);
}

void create_framebuffer(const GladGLContext &gl, GLuint color_tex, GLuint &fbo) {
    gl.GenFramebuffers(1, &fbo);
    gl.BindFramebuffer(GL_FRAMEBUFFER, fbo);
    gl.FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, color_tex, 0);
    assert(gl.CheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
    gl.BindFramebuffer(GL_FRAMEBUFFER, 0);
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
    create_texture(gl, width, height, color_tex0);
    create_framebuffer(gl, color_tex0, fbo0);

    GLuint fbo1, color_tex1;
    create_texture(gl, width, height, color_tex1);
    create_framebuffer(gl, color_tex1, fbo1);

    const float clear_color[4] = {0.0f, 0.0f, 0.0f, 1.0f};

    gl.BindFramebuffer(GL_FRAMEBUFFER, fbo0);
    gl.Viewport(0, 0, width, height);
    gl.ClearColor(clear_color[0], clear_color[1], clear_color[2], clear_color[3]);
    gl.Clear(GL_COLOR_BUFFER_BIT);

    gl.BindFramebuffer(GL_FRAMEBUFFER, fbo1);
    gl.Viewport(0, 0, width, height);
    gl.ClearColor(clear_color[0], clear_color[1], clear_color[2], clear_color[3]);
    gl.Clear(GL_COLOR_BUFFER_BIT);

    gl.BindFramebuffer(GL_FRAMEBUFFER, 0);

    GLuint program;
    {
        std::string vs_code = read_file("shaders/shader.vert");
        std::string fs_code = read_file("shaders/shader.frag");
        create_program(gl, vs_code.c_str(), fs_code.c_str(), program);
    }

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

    int ping_pong = 0;  // 0: 画到 fbo0，采样 tex1；1: 画到 fbo1，采样 tex0

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        GLuint rt = (ping_pong == 0) ? fbo0 : fbo1;
        GLuint read_tex = (ping_pong == 0) ? color_tex1 : color_tex0;

        gl.BindFramebuffer(GL_FRAMEBUFFER, rt);
        gl.Viewport(0, 0, width, height);

        gl.UseProgram(program); // 先定 pipeline（合批按 program 分，program 切换成本高）
        gl.Uniform2f(gl.GetUniformLocation(program, "resolution"), (float) width, (float) height);
        gl.Uniform2f(gl.GetUniformLocation(program, "mouse_pos"), mouse_state.pos.x, mouse_state.pos.y);
        gl.Uniform2f(gl.GetUniformLocation(program, "prev_mouse_pos"), mouse_state.prev_pos.x, mouse_state.prev_pos.y);
        gl.Uniform1i(gl.GetUniformLocation(program, "mouse_down"), mouse_state.down[0]);
        gl.Uniform1i(gl.GetUniformLocation(program, "color_tex"), 0);
        gl.ActiveTexture(GL_TEXTURE0);
        gl.BindTexture(GL_TEXTURE_2D, read_tex);
        // gl.PolygonMode(GL_FRONT_AND_BACK, GL_LINE); // 再定光栅化状态（同 program 下可只改此项）
        gl.BindVertexArray(quad_vao);
        gl.DrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);

        ping_pong = 1 - ping_pong;  // 下一帧交换

        assert(gl.GetError() == GL_NO_ERROR);

        // 当前帧结果在 rt 里，blit 到窗口
        gl.BindFramebuffer(GL_READ_FRAMEBUFFER, rt);
        gl.BindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        gl.BlitFramebuffer(0, 0, width, height, 0, 0, width, height, GL_COLOR_BUFFER_BIT, GL_NEAREST);

        gl.BindFramebuffer(GL_FRAMEBUFFER, 0);
        glfwSwapBuffers(window);
    }

    gl.DeleteVertexArrays(1, &quad_vao);
    gl.DeleteBuffers(1, &quad_ebo);
    gl.DeleteBuffers(1, &quad_vbo);
    gl.DeleteProgram(program);
    gl.DeleteFramebuffers(1, &fbo1);
    gl.DeleteTextures(1, &color_tex1);
    gl.DeleteFramebuffers(1, &fbo0);
    gl.DeleteTextures(1, &color_tex0);
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
