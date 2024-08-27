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

#include "../dataobj/environment.h"

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
	dbg->message("font_t::load_from_freetype", "trying to load '%s' in size %d", fname, pixel_height);

	FT_Library ft_library = NULL;
	if(  FT_Init_FreeType(&ft_library) != FT_Err_Ok  ) {
		dbg->error( "font_t::load_from_freetype", "Freetype initialization failed" );
		return false;
	}

	// Ok, we guessed something about the filename, now actually load it
	FT_Face face;

	if(  FT_New_Face( ft_library, fname, 0, &face ) != FT_Err_Ok  ) {
		dbg->warning( "font_t::load_from_freetype", "Cannot load %s", fname );
		FT_Done_FreeType( ft_library );
		return false;
	}

	if(  FT_Set_Pixel_Sizes( face, 0, pixel_height ) != FT_Err_Ok  ) {
		dbg->warning( "font_t::load_from_freetype", "Cannot set pixel size %d for %s", pixel_height, fname);

		// try to find closest available pixel_height
		int best = -1;
		for (int i=0; i<face->num_fixed_sizes; i++) {
			int h = (face->available_sizes[i].y_ppem + 32) >> 6;
			if (best == -1  ||  abs(best - pixel_height) > abs(h - pixel_height)) {
				best = h;
			}
		}

		if (best == -1) {
			// failed
			FT_Done_Face(face);
			FT_Done_FreeType(ft_library);
			return false;
		}
		if(  FT_Set_Pixel_Sizes( face, 0, best) != FT_Err_Ok  ) {
			dbg->warning( "font_t::load_from_freetype", "Cannot set pixel size %d for %s", best, fname);
		}
		// continue anyway
	}

	glyphs.resize(0x10000);

	ascent    = face->size->metrics.ascender/64;
	linespace = face->size->metrics.height/64;
	descent   = face->size->metrics.descender/64;

	tstrncpy( this->fname, fname, lengthof(this->fname) );

	uint32 num_glyphs = 0;

	for(uint32 glyph_nr=0;  glyph_nr<0xFFFF;  glyph_nr++) {

		uint32 idx = FT_Get_Char_Index(face, glyph_nr);
		if(idx==0 && glyph_nr!=0) {
			// glyph not there, we need to render glyph 0 instead
			glyphs[glyph_nr].advance = 0xFF;
			continue;
		}

		// load glyph image into the slot (erase previous one)
		if(FT_Load_Glyph(face, idx, FT_LOAD_RENDER) != FT_Err_Ok) {
			// glyph not there ...
			glyphs[glyph_nr].advance = 0xFF;
			continue;
		}

		const FT_Error error = FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL);
		if(error != FT_Err_Ok || face->glyph->bitmap.pitch == 0) {
			// glyph not there ...
			glyphs[glyph_nr].advance = 0xFF;
			continue;
		}

		// use only needed amount
		num_glyphs = glyph_nr+1;

		// now render into cache
		// the bitmap is at slot->bitmap
		// the glyph base is at slot->bitmap_left, CELL_HEIGHT - slot->bitmap_top
		
		glyph_t & glyph = glyphs[glyph_nr];
		
		// set glyph size
		glyph.height  = face->glyph->bitmap.rows;
		glyph.width   = face->glyph->bitmap.width;
		glyph.advance = face->glyph->bitmap.width+1;				
		
		// Hajo: the bitmaps are all top aligned. Bitmap top is the ascent
		// above the base line
		// to find the real top position, we must take the font ascent
		// and reduce it by the glyph ascent 
		glyph.top = ascent - face->glyph->bitmap_top - 1;
		
		// transform glyph to Simutrans bitmap
		
		glyph.bitmap = (uint8*)calloc(glyph.height * glyph.width, 1);
		uint8 * const bitmap = glyph.bitmap;
		
		for(int y = 0; y < glyph.height; y++) {
			for(int x = 0; x < face->glyph->bitmap.pitch; x++) {
			
				const uint8 alpha = face->glyph->bitmap.buffer[y * face->glyph->bitmap.pitch + x];
				// simgraph blend routines want 5 bit alpha i.e. 0 .. 31
				
				bitmap[y * glyph.width + x] = (uint8)(pow((alpha / 255.0), 0.75) * 32);
			}		
		}
	}

	if(num_glyphs < 0x80) {
		FT_Done_Face( face );
		FT_Done_FreeType( ft_library );
		return false;
	}

	// hack for not rendered full width space
	if(  glyphs[0x3000].advance == 0xFF  &&  glyphs[0x3001].advance != 0xFF  ) {
		glyphs[0x3000].advance = glyphs[0x3001].advance;
		glyphs[0x3000].width = 0;
		glyphs[0x3000].top = 0;
    }
    
	// Hajo: how to get proper space width?
	if (glyphs[' '].advance == 0xFF) {
		glyphs[' '].advance = glyphs['n'].advance;
	}

	// Use only needed amount
	glyphs.resize(num_glyphs);

	FT_Done_Face( face );
	FT_Done_FreeType( ft_library );
	return true;
}


void font_t::print_debug() const
{
	dbg->debug("font_t::print_debug", "Loaded font %s with %i glyphs\n", get_fname(), get_glyph_count());
	dbg->debug("font_t::print_debug", "height: %i, descent: %i", linespace, descent );
}


bool font_t::load_from_file(const char *srcfilename, int size)
{
	tstrncpy( fname, srcfilename, lengthof(fname) );

	bool ok = load_from_freetype( fname, size );

#if MSG_LEVEL>=4
	if(  ok  ) {
		print_debug();
	}
#endif
	return ok;
}

sint16 font_t::get_glyph_advance(utf32 c) const
{
	if(!is_loaded()) {
		return 0;
	}
	else if(c >= get_glyph_count()  ||  glyphs[c].advance == 0xFF) {
		return glyphs[0].advance;
	}

	return glyphs[c].advance;
}


const font_t::glyph_t& font_t::get_glyph(utf32 c) const
{
	static glyph_t dummy;
	
    if (is_loaded()) {
		if (c < get_glyph_count()  &&  glyphs[c].advance < 0xFF) {
			return glyphs[c];
		}
		else {
			return glyphs[0];
		}
	}
    
    return dummy;
}


sint16 font_t::get_glyph_width(utf32 c) const
{
	if(!is_loaded()) {
		return 0;
	}
	else if(c >= get_glyph_count()) {
		c = 0;
	}

	return glyphs[c].width;
}


sint16 font_t::get_glyph_height(utf32 c) const
{
	if(!is_loaded()) {
		return 0;
	}
	else if(c >= get_glyph_count()) {
		c = 0;
	}

	return glyphs[c].height;
}


sint16 font_t::get_glyph_top(uint32 c) const
{
	if(!is_loaded()) {
		return 0;
	}
	else if(c >= get_glyph_count()) {
		c = 0;
	}

	return glyphs[c].top;
}


const uint8 *font_t::get_glyph_bitmap(utf32 c) const
{
	if(!is_loaded()) {
		return NULL;
	}
	else if(c >= get_glyph_count()) {
		c = 0;
	}

	return glyphs[c].bitmap;
}
