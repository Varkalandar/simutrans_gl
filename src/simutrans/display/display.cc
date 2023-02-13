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
#include "display.h"
#include "../sys/simsys.h"
#include "../dataobj/translator.h"


void display_bevel_box(scr_rect area, 
                       PIXVAL top, PIXVAL left, PIXVAL right, PIXVAL bottom,
	                   bool dirty)
{
	display_vline_wh_clip_rgb(area.x, area.y, area.h, left, dirty);
	display_vline_wh_clip_rgb(area.x+area.w-1, area.y+1, area.h-1, right, dirty);
	
	display_fillbox_wh_clip_rgb(area.x, area.y, area.w, 1, top, dirty);
	display_fillbox_wh_clip_rgb(area.x+1, area.y+area.h-1, area.w-1, 1, bottom, dirty);
}


static PIXVAL handle_color_sequences(utf32 code, PIXVAL default_color)
{
	PIXVAL color;
	
	if(code == 'd') {
		color = default_color;
	} else {
		color = get_system_color(255, 255, 255);
	}
	
	return color;
}


/**
 * len parameter added - use -1 for previous behaviour.
 * completely renovated for unicode and 10 bit width and variable height
 */
int display_text_proportional_len_clip_rgb(scr_coord_val x, scr_coord_val y, 
	                                       const char* txt, control_alignment_t flags, 
	                                       const PIXVAL default_color, bool dirty, 
	                                       sint32 len, sint32 spacing  CLIP_NUM_DEF)
{
	PIXVAL color = default_color;

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
			x -= display_calc_proportional_string_len_width(txt, len, spacing) / 2;
			break;

		case ALIGN_RIGHT:
			x -= display_calc_proportional_string_len_width(txt, len, spacing);
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
			}
			continue; // nothing to see 
		}
		
		const int gw = display_glyph(x, y, c, flags, color);
		x += gw + spacing;
	}

	if(  dirty  ) {
		// here, because only now we know the length also for ALIGN_LEFT text
		mark_rect_dirty_clip( x0, y, x - 1, y + LINESPACE - 1  CLIP_NUM_PAR);
	}

	// warning: actual len might be longer, due to clipping!
	return x - x0;
}


/// Displays a string which is abbreviated by the (language specific) ellipsis character if too wide
/// If enough space is given then it just displays the full string
void display_proportional_ellipsis_rgb( scr_rect r, const char *text, int align, const PIXVAL color, const bool dirty, bool shadowed, PIXVAL shadow_color)
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
				display_text_proportional_len_clip_rgb( r.x+w+1, r.y+1, text, ALIGN_LEFT | DT_CLIP, shadow_color, dirty, max_idx_before_ellipsis  CLIP_NUM_DEFAULT);
			}
			w += display_text_proportional_len_clip_rgb( r.x+w, r.y, text, ALIGN_LEFT | DT_CLIP, color, dirty, max_idx_before_ellipsis  CLIP_NUM_DEFAULT);

			if (shadowed) {
				display_text_proportional_len_clip_rgb( r.x+w+1, r.y+1, translator::translate("..."), ALIGN_LEFT | DT_CLIP, shadow_color, dirty, -1  CLIP_NUM_DEFAULT);
			}

			display_text_proportional_len_clip_rgb( r.x+w, r.y, translator::translate("..."), ALIGN_LEFT | DT_CLIP, color, dirty, -1  CLIP_NUM_DEFAULT);
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
		display_text_proportional_len_clip_rgb( r.x+1, r.y+1, text, ALIGN_LEFT | DT_CLIP, shadow_color, dirty, -1  CLIP_NUM_DEFAULT);
	}
	display_text_proportional_len_clip_rgb( r.x, r.y, text, ALIGN_LEFT | DT_CLIP, color, dirty, -1  CLIP_NUM_DEFAULT);
}


/**
 * display text in 3d box with clipping
 */
void display_ddd_proportional_clip(scr_coord_val xpos, scr_coord_val ypos, FLAGGED_PIXVAL ddd_color, FLAGGED_PIXVAL text_color, const char *text, int dirty  CLIP_NUM_DEF)
{
	const int vpadding = LINESPACE / 7;
	const int hpadding = LINESPACE / 4;

	scr_coord_val width = proportional_string_width(text);

	PIXVAL lighter = display_blend_colors(ddd_color, color_idx_to_rgb(COL_WHITE), 25);
	PIXVAL darker  = display_blend_colors(ddd_color, color_idx_to_rgb(COL_BLACK), 25);

	display_fillbox_wh_clip_rgb( xpos+1, ypos - vpadding + 1, width+2*hpadding-2, LINESPACE+2*vpadding-1, ddd_color, dirty CLIP_NUM_PAR);

	display_fillbox_wh_clip_rgb( xpos, ypos - vpadding, width + 2*hpadding - 2, 1, lighter, dirty );
	display_fillbox_wh_clip_rgb( xpos, ypos + LINESPACE + vpadding, width + 2*hpadding - 2, 1, darker,  dirty );

	display_vline_wh_clip_rgb( xpos, ypos - vpadding, LINESPACE + vpadding * 2, lighter, dirty );
	display_vline_wh_clip_rgb( xpos + width + 2*hpadding - 2, ypos - vpadding, LINESPACE + vpadding * 2, darker,  dirty );

	display_text_proportional_len_clip_rgb( xpos+hpadding, ypos+1, text, ALIGN_LEFT | DT_CLIP, text_color, dirty, -1, 0);
}


/**
 * Draw multiline text
 */
int display_multiline_text_rgb(scr_coord_val x, scr_coord_val y, const char *buf, PIXVAL color)
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
				0);
			
			if(  px_len>max_px_len  ) {
				max_px_len = px_len;
			}
			y += LINESPACE;
		} while ((void)(buf = (next ? next+1 : NULL)), buf != NULL);
	}
	return max_px_len;
}


void display_outline_proportional_rgb(scr_coord_val xpos, scr_coord_val ypos, PIXVAL text_color, PIXVAL shadow_color, const char *text, int dirty, sint32 len)
{
	const int flags = ALIGN_LEFT | DT_CLIP;
	display_text_proportional_len_clip_rgb(xpos - 1, ypos    , text, flags, shadow_color, dirty, len, 0  CLIP_NUM_DEFAULT);
	display_text_proportional_len_clip_rgb(xpos + 1, ypos + 2, text, flags, shadow_color, dirty, len, 0  CLIP_NUM_DEFAULT);
	display_text_proportional_len_clip_rgb(xpos, ypos + 1, text, flags, text_color, dirty, len, 0  CLIP_NUM_DEFAULT);
}


void display_shadow_proportional_rgb(scr_coord_val xpos, scr_coord_val ypos, PIXVAL text_color, PIXVAL shadow_color, const char *text, int dirty, sint32 len)
{
	const int flags = ALIGN_LEFT | DT_CLIP;
	display_text_proportional_len_clip_rgb(xpos + 1, ypos + 1 + (12 - LINESPACE) / 2, text, flags, shadow_color, dirty, len, 0  CLIP_NUM_DEFAULT);
	display_text_proportional_len_clip_rgb(xpos, ypos + (12 - LINESPACE) / 2, text, flags, text_color, dirty, len, 0  CLIP_NUM_DEFAULT);
}

int display_text_bold(scr_coord_val xpos, scr_coord_val ypos, PIXVAL color, const char *text, int dirty, sint32 len)
{
	const int flags = ALIGN_LEFT | DT_CLIP;
	display_text_proportional_len_clip_rgb(xpos, ypos, text, flags, color, dirty, len, 1  CLIP_NUM_DEFAULT);
	int width = display_text_proportional_len_clip_rgb(xpos+1, ypos, text, flags, color, dirty, len, 1  CLIP_NUM_DEFAULT);	
	return width + 1;
}
