/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */


/*
 * Compound drawing routines, which by themselves call more basic routines from
 * simgraph??.cc
 */

#include <cstring>

#include "simgraph.h"
#include "gl_textures.h"
#include "display.h"
#include "font.h"

#include "../sys/simsys.h"
#include "../dataobj/environment.h"
#include "../dataobj/translator.h"


static font_t default_font;
static font_t headline_font;

// needed for resizing gui
int default_font_linespace = 0;


void display_bevel_box(scr_rect area,
                       rgba_t top, rgba_t left, rgba_t right, rgba_t bottom,
	                   bool dirty)
{
	display_vline_wh_clip_rgb(area.x, area.y, area.h, left, dirty);
	display_vline_wh_clip_rgb(area.x+area.w-1, area.y+1, area.h-1, right, dirty);

	display_fillbox_wh_clip_rgb(area.x, area.y, area.w, 1, top, dirty);
	display_fillbox_wh_clip_rgb(area.x+1, area.y+area.h-1, area.w-1, 1, bottom, dirty);
}


static rgba_t handle_color_sequences(utf32 code, rgba_t default_color)
{
	if(code == 'd') {
		return default_color;
	} else {
		return RGBA_WHITE;
	}
}


/**
 * len parameter added - use -1 for previous behaviour.
 * completely renovated for unicode and 10 bit width and variable height
 */
int display_text_proportional_len_clip_rgb(scr_coord_val x, scr_coord_val y,
	                                       const char* txt, control_alignment_t flags,
	                                       const rgba_t default_color, bool dirty,
	                                       sint32 len, sint32 spacing, font_size_t size)
{
	// dbg->message("display_text_proportional_len_clip_rgb()", "Text %s", txt);

	font_t * font = (size == FS_NORMAL) ? &default_font : &headline_font;
	rgba_t color = default_color;

	display_set_color(color);

	if (len < 0) {
		// don't know len yet
		len = 0x7FFF;
	}

	// adapt x-coordinate for alignment
	switch (flags & ( ALIGN_LEFT | ALIGN_CENTER_H | ALIGN_RIGHT) ) {
		case ALIGN_LEFT:
			// nothing to do
			break;

		case ALIGN_CENTER_H:
			x -= display_calc_proportional_string_len_width(txt, len, spacing, size) / 2;
			break;

		case ALIGN_RIGHT:
			x -= display_calc_proportional_string_len_width(txt, len, spacing, size);
			break;
	}


	// store the initial x (for dirty marking)
	const scr_coord_val x0 = x;

	// big loop, draw char by char
	utf8_decoder_t decoder((utf8 const*)txt);
	size_t iTextPos = 0; // pointer on text position

	while (iTextPos < (size_t)len  &&  decoder.has_next()) {
		// decode char
		utf32 c = decoder.next();
		iTextPos = decoder.get_position() - (utf8 const*)txt;

		if(c == '\n') {
			// stop at linebreak
			break;
		}

		if(c == '\t') {
			int tabsize = BASE_TAB_WIDTH  * LINESPACE / 11;
			// advance to next tab stop
			int p = (x - x0) % tabsize;
			x = x - p + tabsize;
			continue; // nothing to see
		}

		if(c == '\e') {
			if(decoder.has_next()) {
				utf32 c2 = decoder.next();
				color = handle_color_sequences(c2, default_color);
                display_set_color(color);
			}
			continue; // nothing to see
		}

		const int gw = display_glyph(x, y, c, font);
		x += gw + spacing;
	}

	if(  dirty  ) {
		// here, because only now we know the length also for ALIGN_LEFT text
		mark_rect_dirty_wc(x0, y, x - 1, y + LINESPACE - 1);
	}

	// warning: actual len might be longer, due to clipping!
	return x - x0;
}


/// Displays a string which is abbreviated by the (language specific) ellipsis character if too wide
/// If enough space is given then it just displays the full string
void display_proportional_ellipsis_rgb(scr_rect r, const char *text, int align, const rgba_t color, const bool dirty,
	                                   bool shadowed, rgba_t shadow_color, font_size_t size)
{
	const scr_coord_val ellipsis_width = translator::get_lang()->ellipsis_width;
	const scr_coord_val max_screen_width = r.w;
	size_t max_idx = 0;

	uint8 byte_length = 0;
	uint8 pixel_width = 0;
	scr_coord_val current_offset = 0;

	if(  align & ALIGN_CENTER_V  ) {
		r.y += (r.h - LINESPACE)/2;
		align &= ~ALIGN_CENTER_V;
	}

	const char *tmp_text = text;
	while(  get_next_char_with_metrics(tmp_text, byte_length, pixel_width)  &&  max_screen_width >= (current_offset+ellipsis_width+pixel_width)  ) {
		current_offset += pixel_width;
		max_idx += byte_length;
	}
	size_t max_idx_before_ellipsis = max_idx;
	scr_coord_val max_offset_before_ellipsis = current_offset;

	// now check if the text would fit completely
	if(  ellipsis_width  &&  pixel_width > 0  ) {
		// only when while above failed because of exceeding length
		current_offset += pixel_width;
		max_idx += byte_length;
		// check the rest ...
		while(  get_next_char_with_metrics(tmp_text, byte_length, pixel_width)  &&  max_screen_width >= (current_offset+pixel_width)  ) {
			current_offset += pixel_width;
			max_idx += byte_length;
		}
		// if it does not fit
		if(  max_screen_width < (current_offset+pixel_width)  ) {
			scr_coord_val w = 0;
			// since we know the length already, we try to center the text with the remaining pixels of the last character
			if(  align & ALIGN_CENTER_H  ) {
				w = (max_screen_width-max_offset_before_ellipsis-ellipsis_width)/2;
			}
			if (shadowed) {
				display_text_proportional_len_clip_rgb(r.x+w+1, r.y+1, text, ALIGN_LEFT | DT_CLIP, shadow_color, dirty, max_idx_before_ellipsis, 0, size);
			}
			w += display_text_proportional_len_clip_rgb(r.x+w, r.y, text, ALIGN_LEFT | DT_CLIP, color, dirty, max_idx_before_ellipsis, 0, size);

			if (shadowed) {
				display_text_proportional_len_clip_rgb(r.x+w+1, r.y+1, translator::translate("..."), ALIGN_LEFT | DT_CLIP, shadow_color, dirty, -1, 0, size);
			}

			display_text_proportional_len_clip_rgb(r.x+w, r.y, translator::translate("..."), ALIGN_LEFT | DT_CLIP, color, dirty, -1, 0, size);
			return;
		}
		else {
			// if this fits, end of string
			max_idx += byte_length;
			current_offset += pixel_width;
		}
	}
	switch (align & ALIGN_RIGHT) {
		case ALIGN_CENTER_H:
			r.x += (max_screen_width - current_offset)/2;
			break;
		case ALIGN_RIGHT:
			r.x += max_screen_width - current_offset;
		default: ;
	}
	if (shadowed) {
		display_text_proportional_len_clip_rgb( r.x+1, r.y+1, text, ALIGN_LEFT | DT_CLIP, shadow_color, dirty, -1, 0, size);
	}
	display_text_proportional_len_clip_rgb( r.x, r.y, text, ALIGN_LEFT | DT_CLIP, color, dirty, -1, 0, size);
}


/**
 * display text in 3d box with clipping
 */
void display_ddd_proportional_clip(scr_coord_val xpos, scr_coord_val ypos, rgba_t ddd_color, rgba_t text_color, const char *text, int dirty)
{
	const int vpadding = LINESPACE / 7;
	const int hpadding = LINESPACE / 4;

	scr_coord_val width = proportional_string_width(text);

	rgba_t lighter = display_blend_colors(ddd_color, RGBA_WHITE, 0.25);
	rgba_t darker  = display_blend_colors(ddd_color, RGBA_BLACK, 0.25);

	display_fillbox_wh_clip_rgb( xpos+1, ypos - vpadding + 1, width+2*hpadding-2, LINESPACE+2*vpadding-1, ddd_color, dirty );

	display_fillbox_wh_clip_rgb( xpos, ypos - vpadding, width + 2*hpadding - 2, 1, lighter, dirty );
	display_fillbox_wh_clip_rgb( xpos, ypos + LINESPACE + vpadding, width + 2*hpadding - 2, 1, darker,  dirty );

	display_vline_wh_clip_rgb( xpos, ypos - vpadding, LINESPACE + vpadding * 2, lighter, dirty );
	display_vline_wh_clip_rgb( xpos + width + 2*hpadding - 2, ypos - vpadding, LINESPACE + vpadding * 2, darker,  dirty );

	display_text_proportional_len_clip_rgb( xpos+hpadding, ypos+1, text, ALIGN_LEFT | DT_CLIP, text_color, dirty, -1, 0, FS_NORMAL);
}


/**
 * Draw multiline text
 */
int display_multiline_text_rgb(scr_coord_val x, scr_coord_val y, const char *buf, rgba_t color)
{
	int max_px_len = 0;
	if (buf != NULL && *buf != '\0') {
		const char *next;

		do {
			next = strchr(buf, '\n');
			const int px_len = display_text_proportional_len_clip_rgb(
				x, y, buf,
				ALIGN_LEFT | DT_CLIP, color, true,
				next != NULL ? (int)(size_t)(next - buf) : -1,
				0, FS_NORMAL);

			if(  px_len>max_px_len  ) {
				max_px_len = px_len;
			}
			y += LINESPACE;
		} while ((void)(buf = (next ? next+1 : NULL)), buf != NULL);
	}
	return max_px_len;
}


void display_outline_proportional_rgb(scr_coord_val xpos, scr_coord_val ypos, rgba_t text_color, rgba_t shadow_color, const char *text, int dirty, sint32 len)
{
	const int flags = ALIGN_LEFT | DT_CLIP;
	display_text_proportional_len_clip_rgb(xpos - 1, ypos    , text, flags, shadow_color, dirty, len, 0, FS_NORMAL);
	display_text_proportional_len_clip_rgb(xpos + 1, ypos + 2, text, flags, shadow_color, dirty, len, 0, FS_NORMAL);
	display_text_proportional_len_clip_rgb(xpos, ypos + 1, text, flags, text_color, dirty, len, 0, FS_NORMAL);
}


void display_shadow_proportional_rgb(scr_coord_val xpos, scr_coord_val ypos, rgba_t text_color, rgba_t shadow_color, const char *text, int dirty, sint32 len)
{
	const int flags = ALIGN_LEFT | DT_CLIP;
	display_text_proportional_len_clip_rgb(xpos + 1, ypos + 1 + (12 - LINESPACE) / 2, text, flags, shadow_color, dirty, len, 0, FS_NORMAL);
	display_text_proportional_len_clip_rgb(xpos, ypos + (12 - LINESPACE) / 2, text, flags, text_color, dirty, len, 0, FS_NORMAL);
}


int display_text_bold(scr_coord_val xpos, scr_coord_val ypos, rgba_t color,
	                  const char *text, int dirty,
	                  sint32 len, font_size_t size)
{
	const int flags = ALIGN_LEFT | DT_CLIP;
	display_text_proportional_len_clip_rgb(xpos, ypos, text, flags, color, dirty, len, 1, size);
	int width = display_text_proportional_len_clip_rgb(xpos+1, ypos, text, flags, color, dirty, len, 1, size);
	return width + 1;
}


bool display_load_font(const char *fname, bool reload)
{
	font_t loaded_fnt;

	if(fname == NULL) {
		dbg->error("display_load_font", "Filename is NULL");
		return false;
	}

	// skip reloading if already in memory, if bdf font
	if(!reload && default_font.is_loaded() && strcmp( default_font.get_fname(), fname ) == 0) {
		return true;
	}

	const int size = env_t::fontsize;

	if(loaded_fnt.load_from_file(fname, size)) {
		default_font = loaded_fnt;
		default_font_linespace = default_font.get_linespace();

		env_t::fontname = fname;
	}

	if(headline_font.load_from_file(fname, size * 120 / 100)) {
		return default_font.is_loaded() && headline_font.is_loaded();
	}

	return false;
}


/*
 * @return true, if this is a valid character
 */
bool has_character(utf32 char_code)
{
	if(  char_code >= default_font.glyphs.size()  ) {
		// or we crash when accessing the non-existing char ...
		return false;
	}
	bool b1 = default_font.is_loaded();
	font_t::glyph_t& gl = default_font.glyphs[char_code];
	uint8  ad = gl.advance;
	return b1 && ad != 0xFF;

	// this return false for some reason on CJK for valid characters ?!?
	// return default_font.is_valid_glyph(char_code);
}


scr_coord_val display_get_char_width(utf32 c)
{
	return default_font.get_glyph_advance(c);
}


/* returns the width of this character or the default (Nr 0) character size */
scr_coord_val display_get_char_max_width(const char* text, size_t len) {

	scr_coord_val max_len=0;

	for(unsigned n=0; (len && n<len) || (len==0 && *text != '\0'); n++) {
		max_len = max(max_len,display_get_char_width(*text++));
	}

	return max_len;
}


/**
 * For the next logical character in the text, returns the character code
 * as well as retrieves the char byte count and the screen pixel width
 * CAUTION : The text pointer advances to point to the next logical character
 */
utf32 get_next_char_with_metrics(const char* &text, unsigned char &byte_length, unsigned char &pixel_width)
{
	size_t len = 0;
	utf32 const char_code = utf8_decoder_t::decode((utf8 const *)text, len);

	if(  char_code==0  ||  char_code == '\n') {
		// case : end of text reached -> do not advance text pointer
		// also stop at linebreaks
		byte_length = 0;
		pixel_width = 0;
		return 0;
	}
	else {
		text += len;
		byte_length = (uint8)len;
		pixel_width = default_font.get_glyph_advance(char_code);
	}
	return char_code;
}


/*
 * returns the index of the last character that would fit within the width
 * If an ellipsis len is given, it will only return the last character up to this len if the full length cannot be fitted
 * @returns index of next character. if text[index]==0 the whole string fits
 */
size_t display_fit_proportional( const char *text, scr_coord_val max_width, scr_coord_val ellipsis_width )
{
	size_t max_idx = 0;

	uint8 byte_length = 0;
	uint8 pixel_width = 0;
	scr_coord_val current_offset = 0;

	const char *tmp_text = text;
	while(  get_next_char_with_metrics(tmp_text, byte_length, pixel_width)  &&  max_width > (current_offset+ellipsis_width+pixel_width)  ) {
		current_offset += pixel_width;
		max_idx += byte_length;
	}
	size_t ellipsis_idx = max_idx;

	// now check if the text would fit completely
	if(  ellipsis_width  &&  pixel_width > 0  ) {
		// only when while above failed because of exceeding length
		current_offset += pixel_width;
		max_idx += byte_length;
		// check the rest ...
		while(  get_next_char_with_metrics(tmp_text, byte_length, pixel_width)  &&  max_width > (current_offset+pixel_width)  ) {
			current_offset += pixel_width;
			max_idx += byte_length;
		}
		// if this fits, return end of string
		if(  max_width > (current_offset+pixel_width)  ) {
			return max_idx+byte_length;
		}
	}
	return ellipsis_idx;
}


/**
 * For the previous logical character in the text, returns the character code
 * as well as retrieves the char byte count and the screen pixel width
 * CAUTION : The text pointer recedes to point to the previous logical character
 */
utf32 get_prev_char_with_metrics(const char* &text, const char *const text_start, unsigned char &byte_length, unsigned char &pixel_width)
{
	if(  text<=text_start  ) {
		// case : start of text reached or passed -> do not move the pointer backwards
		byte_length = 0;
		pixel_width = 0;
		return 0;
	}

	utf32 char_code;
	// determine the start of the previous logical character
	do {
		--text;
	} while (  text>text_start  &&  (*text & 0xC0)==0x80  );

	size_t len = 0;
	char_code = utf8_decoder_t::decode((utf8 const *)text, len);
	byte_length = (uint8)len;
	pixel_width = default_font.get_glyph_advance(char_code);

	return char_code;
}


/**
 * Proportional string width in pixels with a text of a given length
 * extended for universal font routines with unicode support
 */
int display_calc_proportional_string_len_width(const char *text,
	                                           size_t len,
	                                           int spacing,
	                                           font_size_t size)
{
	const font_t* const fnt = (size == FS_NORMAL) ? &default_font : &headline_font;
	unsigned int width = 0;

	// decode char
	const char *const end = text + len;
	while(  text < end  ) {
		const utf8 *p = reinterpret_cast<const utf8 *>(text);
		const utf32 iUnicode = utf8_decoder_t::decode(p);
		text = reinterpret_cast<const char *>(p);

		if(iUnicode == '\t') {
			int tabsize = BASE_TAB_WIDTH * LINESPACE / 11;
			// advance to next tab stop
			int p = width % tabsize;
			width = (width - p) + tabsize;
			continue;
		}


		if(  iUnicode == UNICODE_NUL ||  iUnicode == '\n') {
			return width;
		}
		width += fnt->get_glyph_advance(iUnicode) + spacing;
	}

	return width;
}



/**
 * display_calc_proportional_multiline_string_len_width
 * calculates the width and height of a box containing the text inside
 */
void display_calc_proportional_multiline_string_len_width(int &xw, int &yh, const char *text)
{
	const font_t* const fnt = &default_font;
	int width = 0;

	xw = yh = 0;

	const utf8 *p = reinterpret_cast<const utf8 *>(text);
	while (const utf32 iUnicode = utf8_decoder_t::decode(p)) {

		if(  iUnicode == '\n'  ) {
			// new line: record max width
			xw = max( xw, width );
			yh += LINESPACE;
			width = 0;
			continue;
		}
		width += fnt->get_glyph_advance(iUnicode);
	}
	xw = max( xw, width );
	yh += LINESPACE;
}


/**
 * Get the height of the specified font in pixels,
 *
 * @param size FS_NORMAL or FS_HEADLINE
 * @return the line height for this font
 */
sint16 get_font_height(font_size_t size)
{
	return (size == FS_NORMAL) ? default_font.get_linespace() : headline_font.get_linespace();
}
