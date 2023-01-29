/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "gui_spacer.h"


gui_spacer_t::gui_spacer_t(scr_coord xy, scr_size size) {
            set_pos( xy );
            set_size( size );
            fixed_size = size;
	}


void gui_spacer_t::draw(scr_coord /*offset*/)
{
	// Hajo: invisible space
}


scr_size gui_spacer_t::get_min_size() const
{
	return fixed_size;;
}


scr_size gui_spacer_t::get_max_size() const
{
	return fixed_size;
}
