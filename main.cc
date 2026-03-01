#include <GLFW/glfw3.h>
#include <glad/gl.h>
#include <glm/glm.hpp>
#include <cassert>

int main() {
    int status = glfwInit();
    assert(status == GLFW_TRUE);

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);  // macOS 上 Core Profile 必须
    glfwWindowHint(GLFW_COCOA_RETINA_FRAMEBUFFER, GLFW_FALSE);  // 关闭 Retina 高分辨率，framebuffer 与窗口像素 1:1
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

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        gl.ClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        gl.Clear(GL_COLOR_BUFFER_BIT);
        glfwSwapBuffers(window);
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
