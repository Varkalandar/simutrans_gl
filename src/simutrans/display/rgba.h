#ifndef DISPLAY_RGBA_H
#define DISPLAY_RGBA_H


class rgb888_t
{
public:
	uint8_t r, g, b;

	rgb888_t()
    {
		this->r = 0;
		this->g = 0;
		this->b = 0;
	}

	rgb888_t(uint8_t r, uint8_t g, uint8_t b)
    {
		this->r = r;
		this->g = g;
		this->b = b;
	}

    rgb888_t & operator =(const uint8_t & c)
    {
		r = c;
		g = c;
		b = c;
		return *this;
	}

    bool operator ==(const uint8_t & c) const
    {
		return r == c && g == c && b == c;
	}

};


class rgba_t
{
public:

    float red, green, blue, alpha;

    rgba_t()
    {
        red = green = blue = 0.0f;
        alpha = 1.0f;
    }

    rgba_t(float red, float green, float blue, float alpha=1.0f)
    {
        this->red = red;
        this->green = green;
        this->blue = blue;
        this->alpha = alpha;
    }

    rgba_t(rgb888_t color, float alpha=1.0f)
    {
        this->red = color.r / 255.0f;
        this->green = color.g / 255.0f;
        this->blue = color.b / 255.0f;
        this->alpha = alpha;
    }

    rgba_t(rgba_t color, float alpha)
    {
        this->red = color.red / 255.0f;
        this->green = color.green / 255.0f;
        this->blue = color.blue / 255.0f;
        this->alpha = alpha;
    }

    uint32_t asUint32()
    {
        return ((int)(alpha * 255) << 24) + ((int)(red * 255) << 16) + ((int)(green * 255) << 8) + ((int)(blue * 255));
    }

    void fromUint32(uint32_t c)
    {
        alpha = ((c >> 24) & 255) / 255.0f;
        red = ((c >> 16) & 255) / 255.0f;
        green = ((c >> 8) & 255) / 255.0f;
        blue = (c & 255) / 255.0f;
    }

    bool operator ==(const rgba_t& c) const
    {
		return red == c.red && green == c.green && blue == c.blue && alpha == c.alpha;
	}

    bool operator !=(const rgba_t& c) const
    {
		return red != c.red || green != c.green || blue != c.blue || alpha != c.alpha;
	}
};

#define RGBA_CLEAR rgba_t(0.0f, 0.0f, 0.0f, 0.0f)
#define RGBA_BLACK rgba_t(0.0f, 0.0f, 0.0f)
#define RGBA_WHITE rgba_t(1.0f, 1.0f, 1.0f)

#endif


