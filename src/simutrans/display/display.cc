/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */


/*
 * Compound drawing routines, which by themselves call more basic routines from
 * simgraph??.cc
 */


#include "simgraph.h"
#include "display.h"

void display_bevel_box(scr_rect area, 
                       PIXVAL top, PIXVAL left, PIXVAL right, PIXVAL bottom,
	                   bool dirty)
{
	display_vline_wh_clip_rgb(area.x, area.y, area.h, left, dirty);
	display_vline_wh_clip_rgb(area.x+area.w-1, area.y+1, area.h-1, right, dirty);
	
	display_fillbox_wh_clip_rgb(area.x, area.y, area.w, 1, top, dirty);
	display_fillbox_wh_clip_rgb(area.x+1, area.y+area.h-1, area.w-1, 1, bottom, dirty);
}
