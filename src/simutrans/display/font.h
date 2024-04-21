/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef DISPLAY_FONT_H
#define DISPLAY_FONT_H


#include "../simtypes.h"
#include "../utils/unicode.h"

#include <stdio.h>
#include <vector>

/**
 * Terminology:
 *  - glyph:        display data of a single character
 *  - font:         a collection of glyphs (usually called a font face)
 *  - width,height: size of the glyph
 *  - advance:      number of pixels between the start of a glyph and the next glyph
 *                  (not necessarily equal to width)
 */
class font_t
{
public:
    /// glyph data is stored dense in an array
    /// 23 rows of bytes, second column for widths between 8 and 16
    struct glyph_t
    {
        glyph_t();

        uint8* bitmap;

        sint16 height;
        sint16 width;
        sint16 advance;
        sint16 top;
        sint16 left;
    };

public:
    font_t();

    // returns true if this character is contained in the font
    static bool is_char_in_freetype_font(const char* fname, utf16 testchar);

public:
    /// @returns true on success
    bool load_from_file(const char *fname, int size);
    bool is_loaded() const { return !glyphs.empty(); }

    const char *get_fname() const { return fname; }
    sint16 get_linespace() const { return linespace; }
    sint16 get_ascent() const { return ascent; }

    /// @returns true if this is a valid (defined) glyph
    bool is_valid_glyph(utf32 c) const { return  is_loaded()  &&  c < get_glyph_count()  &&  glyphs[c].advance != 0xFF;  }

    /// @returns size in pixels between the start of this glyph and the next glyph
    sint16 get_glyph_advance(utf32 c) const;

    /// @returns width in pixels of the glyph of a character
    sint16 get_glyph_width(utf32 c) const;

    /// @returns height in pixels of the glyph of a character
    sint16 get_glyph_height(utf32 c) const;

    /// @returns yoffset in pixels of the glyph of a character
    sint16 get_glyph_top(uint32 c) const;

    /// @returns glyph data of a character
    /// @sa font_t::glyph_t::bitmap
    const uint8 *get_glyph_bitmap(utf32 c) const;

    /// @returns glyph data for this utf32 char
    const glyph_t& get_glyph(utf32 c) const;

    uint32 get_glyph_count() const { return glyphs.size(); }

private:
    /// Load a freetype font
    bool load_from_freetype(const char *fname, int pixel_height);

    void print_debug() const;

private:
    char fname[PATH_MAX];
    sint16 linespace;
    sint16 ascent;
    sint16 descent;
    std::vector<glyph_t> glyphs;
};

#endif
