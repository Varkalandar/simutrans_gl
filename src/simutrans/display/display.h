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



int display_text_proportional_len_clip_rgb(scr_coord_val x, scr_coord_val y, const char* txt, control_alignment_t flags, const PIXVAL color, bool dirty, sint32 len, sint32 spacing  CLIP_NUM_DEF  CLIP_NUM_DEFAULT_ZERO);
/* macro are for compatibility */
#define display_proportional_rgb(               x, y, txt, align, color, dirty)       display_text_proportional_len_clip_rgb( x, y, txt, align,           color, dirty, -1, 0 )
#define display_proportional_clip_rgb(          x, y, txt, align, color, dirty)       display_text_proportional_len_clip_rgb( x, y, txt, align | DT_CLIP, color, dirty, -1, 0 )


/// Display a string that is abbreviated by the (language specific) ellipsis character if too wide
/// If enough space is given, it just display the full string
void display_proportional_ellipsis_rgb( scr_rect r, const char *text, int align, const PIXVAL color, const bool dirty, bool shadowed = false, PIXVAL shadow_color = 0 );


/**
 * display text in 3d box with clipping
 */
void display_ddd_proportional_clip(scr_coord_val xpos, scr_coord_val ypos, FLAGGED_PIXVAL ddd_farbe, FLAGGED_PIXVAL text_farbe, const char *text, int dirty  CLIP_NUM_DEF CLIP_NUM_DEFAULT_ZERO);


int display_multiline_text_rgb(scr_coord_val x, scr_coord_val y, const char *inbuf, PIXVAL color);

void display_outline_proportional_rgb(scr_coord_val xpos, scr_coord_val ypos, PIXVAL text_color, PIXVAL shadow_color, const char *text, int dirty, sint32 len=-1);
void display_shadow_proportional_rgb(scr_coord_val xpos, scr_coord_val ypos, PIXVAL text_color, PIXVAL shadow_color, const char *text, int dirty, sint32 len=-1);
int display_text_bold(scr_coord_val xpos, scr_coord_val ypos, PIXVAL color, const char *text, int dirty, sint32 len=-1);

#endif /* SIM_DISPLAY_H */

