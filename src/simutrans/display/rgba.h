#ifndef DISPLAY_RGBA_H
#define DISPLAY_RGBA_H


class rgb888_t
{
public:
	unsigned char r, g, b;
};


class rgba_t
{
public:

    float red, green, blue, alpha;

    rgba_t(float red, float green, float blue, float alpha=1.0f)
    {
        this->red = red;
        this->green = green;
        this->blue = blue;
        this->alpha = alpha;
    }

    rgba_t(rgb888_t color)
    {
        this->red = color.r / 255.0f;
        this->green = color.g / 255.0f;
        this->blue = color.b / 255.0f;
        this->alpha = 1.0f;
    }
};

#define RGBA_BLACK rgba_t(0.0f, 0.0f, 0.0f)
#define RGBA_WHITE rgba_t(1.0f, 1.0f, 1.0f)

#endif


