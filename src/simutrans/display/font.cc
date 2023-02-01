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

#ifdef USE_FREETYPE
#include "../dataobj/environment.h"
#endif

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


#ifdef USE_FREETYPE

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H
#include FT_TRUETYPE_TABLES_H


bool font_t::load_from_freetype(const char *fname, int pixel_height)
{
	dbg->message( "font_t::load_from_freetype", "trying to load '%s' in size %d", fname, pixel_height);

	FT_Library ft_library = NULL;
	if(  FT_Init_FreeType(&ft_library) != FT_Err_Ok  ) {
		dbg->error( "font_t::load_from_freetype", "Freetype initialization failed" );
		return false;
	}

	// Ok, we guessed something about the filename, now actually load it
	FT_Face face;

	if(  FT_New_Face( ft_library, fname, 0, &face ) != FT_Err_Ok  ) {
		dbg->error( "font_t::load_from_freetype", "Cannot load %s", fname );
		FT_Done_FreeType( ft_library );
		return false;
	}

	if(  FT_Set_Pixel_Sizes( face, 0, pixel_height ) != FT_Err_Ok  ) {
		dbg->error( "font_t::load_from_freetype", "Cannot load %s", fname);
		FT_Done_Face(face);
		FT_Done_FreeType(ft_library);
		return false;
	}

	glyphs.resize(0x10000);

	const sint16 ascent = face->size->metrics.ascender/64;
	linespace           = face->size->metrics.height/64;
	descent             = face->size->metrics.descender/64;
	
	tstrncpy( this->fname, fname, lengthof(this->fname) );

	uint32 num_glyphs = 0;

	for(  uint32 glyph_nr=0;  glyph_nr<0xFFFF;  glyph_nr++  ) {

		uint32 idx = FT_Get_Char_Index( face, glyph_nr );
		if(  idx==0  &&  glyph_nr!=0  ) {
			// glyph not there, we need to render glyph 0 instead
			glyphs[glyph_nr].advance = 0xFF;
			continue;
		}

		/* load glyph image into the slot (erase previous one) */
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

		/* now render into cache
		 * the bitmap is at slot->bitmap
		 * the glyph base is at slot->bitmap_left, CELL_HEIGHT - slot->bitmap_top
		 */
		
		glyph_t & glyph = glyphs[glyph_nr];
		
		// set glyph size
		glyph.height  = face->glyph->bitmap.rows;
		glyph.width   = face->glyph->bitmap.width;
		glyph.advance = face->glyph->bitmap.width+1;
		
		
		// the bitmaps are all top aligned. Bitmap top is the ascent
		// above the base line
		// to find the real top position, we must take the font ascent
		// and reduce it by the glyph ascent 
		glyph.top = ascent - face->glyph->bitmap_top - 1;
		
		// transform glyph to Simutrans bitmap
		
		glyph.bitmap = (uint8*)calloc(glyph.height * glyph.width, 1);
		uint8 * bitmap = glyph.bitmap;
		
		for(int y = 0; y < glyph.height; y++) {
			for(int x = 0; x < face->glyph->bitmap.pitch; x++) {
			
				uint8 alpha = face->glyph->bitmap.buffer[y * face->glyph->bitmap.pitch + x];
				// simgraph blend routines want alpha in percent
				
				bitmap[y * glyph.width + x] = (uint8)(pow((alpha / 255.0), 0.75) * 100);
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

	// Use only needed amount
	glyphs.resize(num_glyphs);
	glyphs[(uint32)' '].advance = glyphs[(uint32)'n'].advance;

	FT_Done_Face( face );
	FT_Done_FreeType( ft_library );
	return true;
}

#endif


void font_t::print_debug() const
{
	dbg->debug("font_t::print_debug", "Loaded font %s with %i glyphs\n", get_fname(), get_num_glyphs());
	dbg->debug("font_t::print_debug", "height: %i, descent: %i", linespace, descent );

	/*
	for(uint8 glyph_nr = ' ';  glyph_nr<128; glyph_nr ++) {
		char msg[128 + GLYPH_BITMAP_HEIGHT * (GLYPH_BITMAP_WIDTH+1)]; // +1 for trailing newline

		char *c = msg + sprintf(msg, "glyph %c: width %i, top %i\n", glyph_nr, get_glyph_width(glyph_nr), get_glyph_yoffset(glyph_nr) );

		for(  uint32 y = 0;  y < GLYPH_BITMAP_HEIGHT;  y++  ) {
			for(  uint32 x = 0;  x < (uint32)min(GLYPH_BITMAP_WIDTH, get_glyph_width(glyph_nr));  x++  ) {
				const uint8 data = get_glyph_bitmap(glyph_nr)[y+(x/CHAR_BIT)*GLYPH_BITMAP_HEIGHT];
				const bool bit_set = (data & (0x80>>(x%CHAR_BIT))) != 0;

				*c++ = bit_set ? '*' : ' ';
			}
			*c++ = '\n';
		}
		*c++ = 0;
		dbg->debug("font_t::print_debug", "glyph data: %s", msg );
	}
	*/
}


bool font_t::load_from_file(const char *srcfilename)
{
	tstrncpy( fname, srcfilename, lengthof(fname) );

#ifdef USE_FREETYPE
	bool ok = load_from_freetype( fname, env_t::fontsize );
#endif

#if MSG_LEVEL>=4
	if(  ok  ) {
		print_debug();
	}
#endif
	return ok;
}


uint8 font_t::get_glyph_advance(utf32 c) const
{
	if(  !is_loaded()  ) {
		return 0;
	}
	else if(  c >= get_num_glyphs()  ||  glyphs[c].advance == 0xFF  ) {
		return glyphs[0].advance;
	}

	return glyphs[c].advance;
}


uint8 font_t::get_glyph_width(utf32 c) const
{
	if(  !is_loaded()  ) {
		return 0;
	}
	else if(  c >= get_num_glyphs()  ) {
		c = 0;
	}

	return glyphs[c].width;
}


uint8 font_t::get_glyph_height(utf32 c) const
{
	if(  !is_loaded()  ) {
		return 0;
	}
	else if(  c >= get_num_glyphs()  ) {
		c = 0;
	}

	return glyphs[c].height;
}


uint8 font_t::get_glyph_top(uint32 c) const
{
	if(  !is_loaded()  ) {
		return 0;
	}
	else if(  c >= get_num_glyphs()  ) {
		c = 0;
	}

	return glyphs[c].top;
}


const uint8 *font_t::get_glyph_bitmap(utf32 c) const
{
	if(  !is_loaded()  ) {
		return NULL;
	}
	else if(  c >= get_num_glyphs()  ) {
		c = 0;
	}

	return glyphs[c].bitmap;
}
