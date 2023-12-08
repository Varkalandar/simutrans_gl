#ifndef DISPLAY_GL_TEXTURES_H
#define DISPLAY_GL_TEXTURES_H

class gl_texture_t
{
public:

    uint32_t tex_id;
    uint32_t width;
    uint32_t height;

    gl_texture_t(uint32_t id, uint32_t w, uint32_t h) {
        tex_id = id;
        width = w;
        height = h;
    }

    static gl_texture_t * create_texture(int width, int height, uint8_t * data);
};

#endif
