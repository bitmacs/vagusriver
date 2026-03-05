#include "gl.h"
#include <cassert>
#include <cstdio>
#include <string>

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

void clear_framebuffer(const GladGLContext &gl, GLuint fbo, int width, int height, const float clear_color[4]) {
    gl.BindFramebuffer(GL_FRAMEBUFFER, fbo);
    gl.Viewport(0, 0, width, height);
    gl.ClearColor(clear_color[0], clear_color[1], clear_color[2], clear_color[3]);
    gl.Clear(GL_COLOR_BUFFER_BIT);
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
