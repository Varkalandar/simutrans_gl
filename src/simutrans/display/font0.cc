/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "font.h"

#include "../macros.h"
#include "../simdebug.h"
#include "../sys/simsys.h"
#include "../simtypes.h"
#include "../utils/simstring.h"

#include <math.h>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>


font_t::glyph_t::glyph_t() :
	height(0),
	width(0),
	advance(0xFF),
	top(0)
{
	bitmap = 0;
}


font_t::font_t() :
	linespace (0),
	descent(0)
{
	fname[0] = 0;
}


#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H
#include FT_TRUETYPE_TABLES_H


bool font_t::load_from_freetype(const char *fname, int pixel_height)
{
	return true;
}


void font_t::print_debug() const
{
}


bool font_t::load_from_file(const char *srcfilename, int size)
{
	return true;
}


uint8 font_t::get_glyph_advance(utf32 c) const
{
	return 0;
}


uint8 font_t::get_glyph_width(utf32 c) const
{
	return 0;
}


uint8 font_t::get_glyph_height(utf32 c) const
{
	return 0;
}


uint8 font_t::get_glyph_top(uint32 c) const
{
	return 0;
}


const uint8 *font_t::get_glyph_bitmap(utf32 c) const
{
	return NULL;
}

