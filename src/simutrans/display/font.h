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

class gl_texture_t;

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
    struct glyph_t
    {
        glyph_t();

        uint8* bitmap;

        uint8 height;
		uint8 width;
        sint8 bearing;
		uint8 advance;
		uint8 top;

		uint32_t sheet_index;
	};

	gl_texture_t * glyph_sheet;

	font_t();


	/// @returns true on success
	bool load_from_file(const char *fname, int size);
	bool is_loaded() const { return !glyphs.empty(); }

    const char *get_fname() const { return fname; }
    sint16 get_linespace() const { return linespace; }
    sint16 get_ascent() const { return ascent; }

    /// @returns true if this is a valid (defined) glyph
    bool is_valid_glyph(utf32 c) const { return  is_loaded()  &&  c < get_glyph_count()  &&  glyphs[c].advance != 0xFF;  }

    /// @returns size in pixels between the cursor and the start of this glyph
    sint8 get_glyph_bearing(utf32 c) const;

    /// @returns size in pixels between the start of this glyph and the next glyph
    uint8 get_glyph_advance(utf32 c) const;

    /// @returns width in pixels of the glyph of a character
    uint8 get_glyph_width(utf32 c) const;

	/// @returns height in pixels of the glyph of a character
	uint8 get_glyph_height(utf32 c) const;

	/// @returns yoffset in pixels of the glyph of a character
	uint8 get_glyph_top(uint32 c) const;

    /// @returns glyph data of a character
    /// @sa font_t::glyph_t::bitmap
    const uint8 *get_glyph_bitmap(utf32 c) const;

    /// @returns glyph data for this utf32 char
    const glyph_t& get_glyph(utf32 c) const;

    uint32 get_glyph_count() const { return glyphs.size(); }

private:
#if COLOUR_DEPTH != 0
	/// Load a freetype font
	bool load_from_freetype(const char *fname, int pixel_height);
#endif

    void print_debug() const;

private:
    char fname[PATH_MAX];
    sint16 linespace;
    sint16 ascent;
    sint16 descent;

public:	// for simgraph has_character()
	std::vector<glyph_t> glyphs;
};
#endif
