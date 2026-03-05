#include "gl.h"
#include <GLFW/glfw3.h>
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

    const float clear_color[4] = {0.0f, 0.0f, 0.0f, 0.0f};

    GLuint fbo0, color_tex0;
    create_texture(gl, width, height, color_tex0);
    create_framebuffer(gl, color_tex0, fbo0);
    clear_framebuffer(gl, fbo0, width, height, clear_color);

    GLuint fbo1, color_tex1;
    create_texture(gl, width, height, color_tex1);
    create_framebuffer(gl, color_tex1, fbo1);
    clear_framebuffer(gl, fbo1, width, height, clear_color);

    //
    GLuint naive_gi_fbo, naive_gi_tex;
    create_texture(gl, width, height, naive_gi_tex);
    create_framebuffer(gl, naive_gi_tex, naive_gi_fbo);

    GLuint line_program;
    GLuint naive_gi_program;
    {
        std::string vs_code = read_file("shaders/shader.vert");
        std::string fs_code = read_file("shaders/shader.frag");
        create_program(gl, vs_code.c_str(), fs_code.c_str(), line_program);
    }
    {
        std::string vs_code = read_file("shaders/naive-gi.vert");
        std::string fs_code = read_file("shaders/naive-gi.frag");
        create_program(gl, vs_code.c_str(), fs_code.c_str(), naive_gi_program);
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
    }

    assert(gl.GetError() == GL_NO_ERROR);

    int ping_pong = 0;  // 0: 画到 fbo0，采样 tex1；1: 画到 fbo1，采样 tex0

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        GLuint rt = (ping_pong == 0) ? fbo0 : fbo1;
        GLuint read_tex = (ping_pong == 0) ? color_tex1 : color_tex0;
        GLuint write_tex = (ping_pong == 0) ? color_tex0 : color_tex1;

        gl.BindFramebuffer(GL_FRAMEBUFFER, rt);
        gl.Viewport(0, 0, width, height);

        gl.UseProgram(line_program); // 先定 pipeline（合批按 program 分，program 切换成本高）
        gl.Uniform2f(gl.GetUniformLocation(line_program, "resolution"), (float) width, (float) height);
        gl.Uniform2f(gl.GetUniformLocation(line_program, "mouse_pos"), mouse_state.pos.x, mouse_state.pos.y);
        gl.Uniform2f(gl.GetUniformLocation(line_program, "prev_mouse_pos"), mouse_state.prev_pos.x, mouse_state.prev_pos.y);
        gl.Uniform1i(gl.GetUniformLocation(line_program, "mouse_down"), mouse_state.down[0]);
        gl.Uniform1i(gl.GetUniformLocation(line_program, "color_tex"), 0);
        gl.ActiveTexture(GL_TEXTURE0);
        gl.BindTexture(GL_TEXTURE_2D, read_tex);
        // gl.PolygonMode(GL_FRONT_AND_BACK, GL_LINE); // 再定光栅化状态（同 program 下可只改此项）
        gl.BindVertexArray(quad_vao);
        gl.DrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);

        ping_pong = 1 - ping_pong;  // 下一帧交换

        assert(gl.GetError() == GL_NO_ERROR);

        // naive GI pass
        {
            gl.BindFramebuffer(GL_FRAMEBUFFER, naive_gi_fbo);
            gl.Viewport(0, 0, width, height);
            gl.UseProgram(naive_gi_program);
            gl.Uniform2f(gl.GetUniformLocation(naive_gi_program, "resolution"), (float) width, (float) height);
            gl.Uniform1i(gl.GetUniformLocation(naive_gi_program, "color_tex"), 0);
            gl.ActiveTexture(GL_TEXTURE0);
            gl.BindTexture(GL_TEXTURE_2D, write_tex);
            gl.BindVertexArray(quad_vao);
            gl.DrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
            assert(gl.GetError() == GL_NO_ERROR);
        }

        gl.BindFramebuffer(GL_READ_FRAMEBUFFER, naive_gi_fbo);
        gl.BindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        gl.BlitFramebuffer(0, 0, width, height, 0, 0, width, height, GL_COLOR_BUFFER_BIT, GL_NEAREST);

        gl.BindFramebuffer(GL_FRAMEBUFFER, 0);
        glfwSwapBuffers(window);
    }

    gl.DeleteVertexArrays(1, &quad_vao);
    gl.DeleteBuffers(1, &quad_ebo);
    gl.DeleteBuffers(1, &quad_vbo);
    gl.DeleteProgram(naive_gi_program);
    gl.DeleteProgram(line_program);
    gl.DeleteFramebuffers(1, &fbo1);
    gl.DeleteTextures(1, &color_tex1);
    gl.DeleteFramebuffers(1, &fbo0);
    gl.DeleteTextures(1, &color_tex0);
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
