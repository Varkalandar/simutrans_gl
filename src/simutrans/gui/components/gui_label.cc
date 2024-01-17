/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "gui_label.h"
#include "../gui_frame.h"
#include "../../dataobj/translator.h"
#include "../../utils/simstring.h"
#include "../simwin.h"
#include "../../display/display.h"

/*
 * just displays a text, will be auto-translated
 */

static scr_coord_val separator_width = 0;
static scr_coord_val large_money_width = 0;

gui_label_t::gui_label_t(const char* text, rgba_t color_, align_t align_, font_size_t size) :
	align(align_), tooltip(NULL), font_size(size)
{
	separator_width = proportional_string_width( ",00$" );

	if (get_large_money_string()) {
		cbuffer_t buf;
		buf.printf("%s$", get_large_money_string());
		large_money_width = proportional_string_width((const char*) buf);
	}
	else {
		large_money_width = 0;
	}

	set_size( scr_size( D_BUTTON_WIDTH, D_LABEL_HEIGHT ) );
	init( text, scr_coord (0,0), color_, align_);
	shadowed = false;
}


scr_size gui_label_t::get_min_size() const
{
	const scr_coord_val dynamic_width = text ? display_calc_proportional_string_len_width(text, strlen(text), 0, font_size) : D_BUTTON_WIDTH;
	return scr_size(max(dynamic_width, fixed_min_width), max(D_LABEL_HEIGHT, fixed_min_height) );
}

scr_size gui_label_t::get_max_size() const
{
	return align == left  ? get_min_size() : scr_size(scr_size::inf.w, get_min_size().h);
}


void gui_label_t::set_text(const char *text, bool autosize)
{
	if (text != NULL) {
		set_text_pointer(translator::translate(text), autosize);
	}
	else {
		set_text_pointer(NULL, false);
	}
}


void gui_label_t::set_text_pointer(const char *text_par, bool autosize)
{
	text = text_par;

	if (autosize && text && *text != '\0') {
		set_size(scr_size(display_calc_proportional_string_len_width(text, strlen(text), 0, font_size), size.h));
	}
}


void gui_label_t::draw(scr_coord offset)
{
    // dbg->message("gui_label_t::draw", "Called");

	if(  align == money_right) {
		if(text) {
			const char *separator = NULL;
			const bool not_a_number = atol(text)==0  &&  !isdigit(*text)  &&  *text != '-';

			scr_coord right = pos + offset;

			if(  !not_a_number  ) {
				// find first letter of large_money_width in text
				if (get_large_money_string()!=NULL) {
					separator = strrchr(text, *(get_large_money_string()) );
					if (separator) {
						right.x += get_size().w - large_money_width;
					}
				}
				// look for fraction_sep (e.g., comma)
				if (separator==NULL) {
					// everything else align at decimal separator
					right.x += get_size().w - separator_width;
					separator = strrchr(text, get_fraction_sep());
				}
			}

			if(separator) {
				display_proportional_clip_rgb(right.x, right.y, separator, ALIGN_LEFT, color, true);
				if(  separator!=text  ) {
					if (shadowed) {
						display_text_proportional_len_clip_rgb(right.x+1, right.y+1, text, ALIGN_RIGHT | DT_CLIP, color_shadow, true, separator - text, 0, font_size);
					}
					display_text_proportional_len_clip_rgb(right.x, right.y, text, ALIGN_RIGHT | DT_CLIP, color, true, separator-text, 0, font_size);
				}
			}
			else {
				// integer or normal text
				if (shadowed) {
					display_proportional_clip_rgb(right.x + 1, right.y + 1, text, ALIGN_RIGHT | DT_CLIP, color_shadow, true);
				}
				display_proportional_clip_rgb(right.x, right.y, text, ALIGN_RIGHT, color, true);
			}
		}
	}

	else if(text) {

		scr_coord_val top = 0;

		// Hajo: if there is a special height set for his label
		// calculate proper text top margin
		if(fixed_min_height)
		{
			// text is LINESPACE, and y pos will be glyphs top
			top = (size.h - LINESPACE) / 2;
		}

		// bounding box for debugging
		// display_fillbox_wh_clip_rgb(offset.x + pos.x, offset.y + pos.y, size.w, size.h, RGBA_WHITE, false);

		const scr_rect area(offset+pos+scr_coord(0, top), size);

		int a = align == left ? ALIGN_LEFT : ( align == right ? ALIGN_RIGHT : ALIGN_CENTER_H);
		display_proportional_ellipsis_rgb(area, text,  a | DT_CLIP, color, true, shadowed, color_shadow, font_size);
	}

	if ( tooltip  &&  getroffen(get_mouse_pos() - offset) ) {
		const scr_coord tooltip_base_pos{ get_mouse_pos().x,  offset.y + pos.y + size.h };
		win_set_tooltip(tooltip_base_pos + TOOLTIP_MOUSE_OFFSET, tooltip, this);
	}
}

void gui_label_t::set_tooltip(const char * t)
{
	tooltip = t;
}


void gui_label_buf_t::init(rgba_t color_par, align_t align_par)
{
	gui_label_t::init(NULL, get_pos(), color_par, align_par);
	buf_changed = false;
}


void gui_label_buf_t::update()
{
	buffer_read = buffer_write;
	buffer_write.clear();
	gui_label_t::set_text_pointer( (const char*)buffer_read, false /*no autoresize*/ );
	buf_changed = false;
}


void gui_label_buf_t::draw(scr_coord offset)
{
	if (buf_changed) {
		update();
	}
	gui_label_t::draw(offset);
}


void gui_label_buf_t::set_min_width(scr_coord_val w)
{
	min_width = w;
}


scr_size gui_label_buf_t::get_min_size() const
{
	scr_size min_size = gui_label_t::get_min_size();
	min_size.w = max(min_size.w, min_width);
	return min_size;
}


void gui_label_buf_t::append_money(double money)
{
	buffer_write.append_money(money);
	set_color(money >= 0 ? (MONEY_PLUS) : (MONEY_MINUS));
}
