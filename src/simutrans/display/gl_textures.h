#ifndef DISPLAY_GL_TEXTURES_H
#define DISPLAY_GL_TEXTURES_H

class gl_texture_t
{
public:

    uint32_t tex_id;
    uint32_t width;
    uint32_t height;
    uint8_t  *data;

    gl_texture_t(uint32_t id, uint32_t w, uint32_t h, uint8_t * data) {
        tex_id = id;
        width = w;
        height = h;
        this->data = data;
    }

    static gl_texture_t * create_texture(int width, int height, uint8_t * data);
    static void bind(uint32_t tex_id);

    void update_texture(uint8_t * data);
    void update_region(int x, int y, int w, int h, uint8_t *data);
};

#endif
