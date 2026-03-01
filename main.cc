#include <GLFW/glfw3.h>
#include <glad/gl.h>
#include <glm/glm.hpp>
#include <cassert>

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
    GLFWwindow *window = glfwCreateWindow(800, 600, "Demo", nullptr, nullptr);
    assert(window);

    // 将上下文设为当前
    glfwMakeContextCurrent(window);

    // 为当前 context 分配 GladGLContext 并加载函数指针
    GladGLContext gl;
    int version = gladLoadGLContext(&gl, (GLADloadfunc) glfwGetProcAddress);
    assert(version != 0);

    // ESC 键关闭窗口
    glfwSetKeyCallback(window, [](GLFWwindow *w, int key, int, int action, int) {
        if (key == GLFW_KEY_ESCAPE && action == GLFW_RELEASE)
            glfwSetWindowShouldClose(w, GLFW_TRUE);
    });

    // 离屏渲染：FBO + 颜色纹理附件
    GLuint fbo, color_tex;
    create_texture(gl, 800, 600, color_tex);
    create_framebuffer(gl, color_tex, fbo);

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        // 渲染到离屏 FBO 并 clear
        gl.BindFramebuffer(GL_FRAMEBUFFER, fbo);
        gl.Viewport(0, 0, 800, 600);
        gl.ClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        gl.Clear(GL_COLOR_BUFFER_BIT);

        // 将离屏颜色附件拷贝到窗口默认 framebuffer
        gl.BindFramebuffer(GL_READ_FRAMEBUFFER, fbo);
        gl.BindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        gl.BlitFramebuffer(0, 0, 800, 600, 0, 0, 800, 600, GL_COLOR_BUFFER_BIT, GL_NEAREST);

        gl.BindFramebuffer(GL_FRAMEBUFFER, 0);
        glfwSwapBuffers(window);
    }

    gl.DeleteFramebuffers(1, &fbo);
    gl.DeleteTextures(1, &color_tex);
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
