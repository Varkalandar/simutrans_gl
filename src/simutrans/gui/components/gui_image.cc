/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "gui_image.h"

#include "../gui_frame.h"


gui_image_t::gui_image_t( const image_id i, const uint8 p, control_alignment_t alignment_par, bool remove_offset_enabled ) :
	alignment(alignment_par),
	player_nr(p),
	remove_offset(0,0),
	remove_enabled(remove_offset_enabled),
	alpha(1.0f),
	display_offset(0,0)
{
	set_image(i,remove_offset_enabled);
}


scr_size gui_image_t::get_min_size() const
{
	if( id  !=  IMG_EMPTY ) {
		scr_coord_val x=0, y=0, w=0, h=0;
		display_get_base_image_offset( id, &x, &y, &w, &h );

		if (remove_enabled) {
			return scr_size(w, h);
		}
		else {
			// FIXME
			assert(0);
			return scr_size(x+w, y+h);
		}
	}
	return gui_component_t::get_min_size();
}


void gui_image_t::set_size( scr_size size_par )
{
	if( id  !=  IMG_EMPTY ) {
		scr_coord_val x=0, y=0, w=0, h=0;
		display_get_base_image_offset( id, &x, &y, &w, &h );

		if( remove_enabled ) {
			remove_offset = scr_coord(-x,-y);
		}

		size_par = scr_size( x+w+remove_offset.x, y+h+remove_offset.y );
	}

	gui_component_t::set_size(size_par);
}


void gui_image_t::set_image( const image_id i, bool remove_offsets ) {

	id = i;
	remove_enabled = remove_offsets;

	if(  id ==IMG_EMPTY  ) {
		remove_offset = scr_coord(0,0);
		remove_enabled = false;
	}

	set_size( size );
}


void gui_image_t::set_display_offset(scr_coord offset)
{
	display_offset = offset;
}


/**
 * Draw the component
 */
void gui_image_t::draw( scr_coord offset ) {

	if(  id!=IMG_EMPTY  ) {
		// Hajo: add general offset fist
		offset = offset + display_offset;

		// Then add alignment offsets
		scr_coord_val x=0, y=0, w=0, h=0;
		display_get_base_image_offset( id, &x, &y, &w, &h );

		switch (alignment) {

			case ALIGN_RIGHT:
				offset.x += size.w - w + remove_offset.x;
				break;

			case ALIGN_BOTTOM:
				offset.y += size.h - h + remove_offset.y;
				break;

			case ALIGN_CENTER_H:
				offset.x += D_GET_CENTER_ALIGN_OFFSET(w,size.w);
				break;

			case ALIGN_CENTER_V:
				offset.y += D_GET_CENTER_ALIGN_OFFSET(h,size.h);
				break;

		}

        display_set_color(rgba_t(1, 1, 1, alpha));
        display_base_img(id, pos.x+offset.x+remove_offset.x, pos.y+offset.y+remove_offset.y, (sint8)player_nr);
	}
}
