#include "../simdebug.h"
#include "gl_textures.h"

#include <GLFW/glfw3.h>


gl_texture_t * gl_texture_t::create_texture(int width, int height, uint8_t * data)
{
    // Create a new texture object in memory and bind it
    GLuint texId;
    glGenTextures(1, &texId);
    glBindTexture(GL_TEXTURE_2D, texId);

    dbg->message("create_texture()", "id=%d error=%x", texId, glGetError());

    // All RGB bytes are aligned to each other and each component is 1 byte
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    dbg->message("create_texture()", "error=%x", glGetError());

    // Upload the texture data
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, data);
    dbg->message("create_texture()", "error=%x", glGetError());

    // Setup the ST coordinate system
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    dbg->message("create_texture()", "error=%x", glGetError());

    // Setup what to do when the texture has to be scaled
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    dbg->message("create_texture()", "error=%x", glGetError());

    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    dbg->message("create_texture()", "error=%x", glGetError());

    return new gl_texture_t(texId, width, height);
}
