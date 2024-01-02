/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "gui_map_preview.h"
#include "../../display/simgraph.h"

gui_map_preview_t::gui_map_preview_t() :
	gui_component_t()
{
	map_data = NULL;
	set_size(scr_size(MAP_PREVIEW_SIZE_X, MAP_PREVIEW_SIZE_Y));
}

/**
 * Draws the component.
 */
void gui_map_preview_t::draw(scr_coord offset)
{
    display_ddd_box_clip_rgb(pos.x + offset.x, pos.y + offset.y, size.w, size.h, color_idx_to_rgb(MN_GREY0), color_idx_to_rgb(MN_GREY4));

    if(map_data) {
        display_set_color(RGBA_WHITE);
        display_array_wh(pos.x + offset.x + 1, pos.y + offset.y + 1, map_data->get_width(), map_data->get_height(), map_data->to_array());
    }
}
