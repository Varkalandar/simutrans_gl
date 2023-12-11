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
    if(glGetError()) dbg->message("create_texture()", "error=%x", glGetError());

    // Upload the texture data
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, data);
    if(glGetError()) dbg->message("create_texture()", "error=%x", glGetError());

    // Setup the ST coordinate system
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    if(glGetError()) dbg->message("create_texture()", "error=%x", glGetError());

    // Setup what to do when the texture has to be scaled
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    if(glGetError()) dbg->message("create_texture()", "error=%x", glGetError());

    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    if(glGetError()) dbg->message("create_texture()", "error=%x", glGetError());

    return new gl_texture_t(texId, width, height, data);
}


void gl_texture_t::update_texture(uint8_t * data)
{
    // dbg->message("update_texture()", "id=%d", tex_id);

    this->data = data;
    glBindTexture(GL_TEXTURE_2D, tex_id);

    // All RGB bytes are aligned to each other and each component is 1 byte
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    if(glGetError()) dbg->message("update_texture()", "error=%x", glGetError());

    // Upload the texture data
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, data);
    if(glGetError()) dbg->message("update_texture()", "error=%x", glGetError());

    // Setup the ST coordinate system
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    if(glGetError()) dbg->message("update_texture()", "error=%x", glGetError());

    // Setup what to do when the texture has to be scaled
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    if(glGetError()) dbg->message("update_texture()", "error=%x", glGetError());

    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    if(glGetError()) dbg->message("update_texture()", "error=%x", glGetError());
}



void gl_texture_t::update_region(int x, int y, int w, int h, uint8_t *data)
{
    glBindTexture(GL_TEXTURE_2D, tex_id);

	glTexSubImage2D(GL_TEXTURE_2D,
						0, // level,
						x, y, w, h,
						GL_RGBA, // GLenum format,
						GL_UNSIGNED_BYTE, // GLenum type,
						data);

    if(glGetError()) dbg->message("update_region()", "error=%x", glGetError());
}