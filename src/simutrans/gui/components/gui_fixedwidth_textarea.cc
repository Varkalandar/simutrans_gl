/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include <string.h>

#include "../gui_theme.h"
#include "gui_fixedwidth_textarea.h"
#include "../../dataobj/translator.h"
#include "../../utils/cbuffer.h"
#include "../../utils/unicode.h"
#include "../../display/display.h"


gui_fixedwidth_textarea_t::gui_fixedwidth_textarea_t(cbuffer_t* buf_, const sint16 width) :
	reserved_area(0, 0)
{
	buf = buf_;
	set_width(width);
}


void gui_fixedwidth_textarea_t::recalc_size()
{
	scr_size newsize = calc_display_text(scr_coord::invalid, false);
	if (newsize.h != size.h) {
		gui_component_t::set_size( newsize );
	}
}


void gui_fixedwidth_textarea_t::set_width(const scr_coord_val width)
{
	if(  width>0  ) {
		// height is simply reset to 0 as it requires recalculation anyway
		size = scr_size(width,0);

		scr_size newsize = calc_display_text(scr_coord::invalid, false);
		gui_component_t::set_size( newsize );
	}
}


void gui_fixedwidth_textarea_t::set_reserved_area(const scr_size area)
{
	if(  area.w>=0  &&  area.h>=0  ) {
		reserved_area = area;
	}
}


scr_size gui_fixedwidth_textarea_t::get_min_size() const
{
	scr_size size = calc_display_text(scr_coord(0,0), false);
	size.clip_lefttop(reserved_area);
	return size;
}


scr_size gui_fixedwidth_textarea_t::get_max_size() const
{
	return scr_size::inf;
}


/* calculates the height of the text that flows around the world_view
 * if draw is true, it will also draw the text
 * borrowed from ding_infowin_t::calc_draw_info() with adaptation
 */
scr_size gui_fixedwidth_textarea_t::calc_display_text(const scr_coord offset, const bool draw) const
{
	scr_coord_val new_width = get_size().w;
    scr_coord_val y = offset.y;
    
	const char * text = buf->get_str();

    //
	// pass 1 (and not drawing): find out if we can shrink width
    //
    // In the height of the restricted area, the widest text line will
    // determine the total width of the area. below that, the rest of
    // the text will flow.
    //
	if(*text  &&  !draw  &&   reserved_area.w > 0   ) {
		scr_coord_val new_lines = 0;
		scr_coord_val x_size = 200; 

		if ((text != NULL) && (*text != '\0')) {
			const char* buf = text;
			const char* next;

			do {
				next = strchr(buf, '\n');
				const size_t len = next ? next - buf : 99999;
				// we are in the image area
				const int px_len = display_calc_proportional_string_len_width(buf, len, 0, FS_NORMAL);

				if (px_len > x_size) {
                    dbg->message("", "line was too wide, expanding: %d -> %d, reserved %d, '%s'", x_size, px_len, reserved_area.w, buf);
					x_size = px_len;
				}

				new_lines += LINESPACE;
			} while (new_lines<reserved_area.h  &&  next != NULL && ((void)(buf = next + 1), *buf != 0));
		}
		if (x_size < new_width) {
			new_width = x_size + reserved_area.w;
		}
	}

    // 
	// pass 2: height calculation and drawing (if requested)
    //
    // Here text will flow, automatically broken if the width of a line
    // exceeds new_width
    //
	while(*text) {

        // lets see if the line would fit naturally
        int line_width = display_calc_proportional_string_len_width(text, -1, 0, FS_NORMAL);

        if(line_width < new_width) {
            
            // dbg->message("calc_display_text", "Found a line that fits: '%s'", text);
            
            // yes, it fits. Just draw it.
            display_text_proportional_len_clip_rgb(offset.x, y,
                                                   text, 0, (gui_theme_t::gui_color_text),
                                                   false,
                                                   -1, 0, FS_NORMAL);
            y += LINESPACE;
            const char * end = strchr(text, '\n');
            if(end == 0) {
                // no more newlines, and we had space to draw everything,
                // we must have reached the end of the text
                break;
            }
            else {
                // more text, advance to new line. Skip the newline.
                text = (end + 1);
            }    
        }
		else {
            // line too long, we must wrap
            
            const char * word_start = text; 
            const char * next_break = strchr(text, '\n');
            const char * next_space = strchr(text, ' ');
            int cur_x = offset.x;
            
            if(next_space) {
                // we've got a word
                int ww = display_calc_proportional_string_len_width(text, next_space-word_start, 0, FS_NORMAL);
                if(cur_x + ww > new_width) {
                    // need a new line
                    y += LINESPACE;
                    cur_x = offset.x;
                }
                
                display_text_proportional_len_clip_rgb(cur_x, y, word_start, 0, 
                                                       (gui_theme_t::gui_color_text), false,  
                                                       next_space-word_start, 0, FS_NORMAL); 
                cur_x += ww;
                word_start = next_space;
            }
            else if(next_break) {
                // we've got a word
                int ww = display_calc_proportional_string_len_width(text, next_break-word_start, 0, FS_NORMAL);
                if(cur_x + ww > new_width) {
                    // need a new line
                    y += LINESPACE;
                    cur_x = offset.x;
                }
                
                display_text_proportional_len_clip_rgb(cur_x, y, word_start, 0, 
                                                       (gui_theme_t::gui_color_text), false,  
                                                       next_break-word_start, 0, FS_NORMAL); 
                cur_x += ww;
                word_start = next_break;
            }
            else {
                // last word
                display_text_proportional_len_clip_rgb(cur_x, y, word_start, 0, 
                                                       (gui_theme_t::gui_color_text), false,  
                                                       -1, 0, FS_NORMAL); 
            }
            
            break;
        }
	}

	// reset component height where necessary
	return scr_size(new_width, y);
}


void gui_fixedwidth_textarea_t::draw(scr_coord offset)
{
	size = calc_display_text(offset + get_pos(), true);
}
