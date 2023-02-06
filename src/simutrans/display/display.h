/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */


/*
 * Compound drawing routines, which by themselves call more basic routines from
 * simgraph??.cc
 */

#ifndef SIM_DISPLAY_H
#define SIM_DISPLAY_H

#include "scr_coord.h"


/**
 * Draw a bevel box.
 * 
 * @param area The box rectangle
 * @param top Top line color
 * @param left Left line color
 * @param right Right line color
 * @param bottom Bottom line color
 * @param dirty If true, refresh screen area
 */
void display_bevel_box(scr_rect area, 
                       PIXVAL top, PIXVAL left, PIXVAL right, PIXVAL bottom,
	               bool dirty);


#endif /* SIM_DISPLAY_H */

