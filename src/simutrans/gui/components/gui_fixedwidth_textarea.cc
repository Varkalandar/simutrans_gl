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
		set_visible(true);

		scr_size newsize = calc_display_text(scr_coord::invalid, false);
		gui_component_t::set_size( newsize );
	}
	else {
		set_visible(false);
		size = scr_size(0, 0);
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


static scr_coord display_word(const char * word_start, const char * word_end, 
                              scr_coord pos)
{
    // char word [1024];
    // strncpy(word, word_start, word_end-word_start+1);
    // word[word_end-word_start+1] = 0;
    // dbg->message("xx", "'%s'", word);
    
    display_text_proportional_len_clip_rgb(pos.x, pos.y, word_start, 0, 
                                           (gui_theme_t::gui_color_text), false,  
                                           word_end - word_start, 0, FS_NORMAL); 
    return pos;
}


/* calculates the height of the text that flows around the world_view
 * if draw is true, it will also draw the text
 * borrowed from ding_infowin_t::calc_draw_info() with adaptation
 */
scr_size gui_fixedwidth_textarea_t::calc_display_text(const scr_coord offset, const bool draw) const
{
	scr_coord_val new_width = get_size().w;
    scr_coord pos(offset.x, offset.y);
    
	const char * text = buf->get_str();

	if (!text || !*text) {
		if (!draw) {
			// never had text => too early to guess minimum size
			return scr_size(max(new_width, reserved_area.w), reserved_area.h);
		}
		else {
			// still nothing => then empty ...
			return scr_size(max(1, reserved_area.w), reserved_area.h);
		}
	}

	// pass 1 (and not drawing): find out if we can shrink width
	if(!draw) {
		scr_coord_val new_height = 0;
		scr_coord_val x_size = 0;

		// find the width needed for word next to the reserved text area
		const char* next = text-1;
		do {
			const char* buf = next+1;
			next = strchr(buf, '\n');

			const size_t len = next ? next - buf : 999;
			// we are in the image area
			scr_coord_val px_len = display_calc_proportional_string_len_width(buf, len, 0, FS_NORMAL);
			if (new_height <= reserved_area.h) {
				px_len += reserved_area.w;
			}
			if (px_len > x_size) {
				x_size = px_len;
			}

			new_height += LINESPACE;

		} while (next  &&  next[1]); // ignore trailing \n

		if (x_size < new_width) {
			// shrink if smaller and return new width
			new_width = x_size;
			return scr_size(new_width, max(new_height, reserved_area.h));
		}
		// else we need pass 2
	}

    // 
	// pass 2: height calculation and drawing (if requested)
    //
    // Here text will flow, automatically broken if the width of a line
    // exceeds new_width
    //
	while(*text) {

        int cur_width = (pos.y <= offset.y + reserved_area.h) ? new_width - reserved_area.w : new_width;
        
        const char * word_start = text; 
        const char * next_break = text;
        int line_width = 0;
        
        do {
            const char * last_break = next_break;
            
            while(*next_break && *next_break != '\n' && *next_break != ' ') 
            {
                next_break ++;
            }
            
            line_width = display_calc_proportional_string_len_width(word_start, next_break - word_start, 0, FS_NORMAL);

            if(line_width > cur_width) {
                // we went one word too far
                next_break = last_break-1;
                break;
            }
            
            if(*next_break == '\n' || *next_break == 0) {
                break;
            }
            
            next_break ++;
        } while(true);  

        if(draw) display_word(word_start, next_break, pos);

        pos.x = offset.x;
        pos.y += LINESPACE;            

        text = next_break + 1;

        if(*next_break == 0) text = next_break;
	}

	// reset component height where necessary
	return scr_size(new_width, pos.y - offset.y);
}


void gui_fixedwidth_textarea_t::draw(scr_coord offset)
{
	size = calc_display_text(offset + get_pos(), true);
}

