#pragma once

#include "base.h"

struct Framebuffer {
    GLuint id;
    GLuint colorTexture;
    GLuint depthTexture;
};

/***
 * @brief Creates custom Framebuffer with color and depth texture
 * @return fbo
 */
Framebuffer createFramebuffer(int width, int height);

/**
 * @brief Deletes Framebuffer (fbo and bound textures)
 * @param framebuffer
 */
void deleteFramebuffer(Framebuffer framebuffer);
