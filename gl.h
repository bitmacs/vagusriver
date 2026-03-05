#ifndef GL_H
#define GL_H

#include <glad/gl.h>

void create_texture(const GladGLContext &gl, int width, int height, GLuint &texture);
void create_framebuffer(const GladGLContext &gl, GLuint color_tex, GLuint &fbo);
void clear_framebuffer(const GladGLContext &gl, GLuint fbo, int width, int height, const float clear_color[4]);
void create_program(const GladGLContext &gl, const char *vs_code, const char *fs_code, GLuint &program);

#endif
