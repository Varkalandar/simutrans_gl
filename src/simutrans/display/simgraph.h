/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef DISPLAY_SIMGRAPH_H
#define DISPLAY_SIMGRAPH_H


#include "../simcolor.h"
#include "../simtypes.h"
#include "clip_num.h"
#include "simimg.h"
#include "rgba.h"
#include "scr_coord.h"
#include "alignment.h"

class font_t;
class rgba_t;
class gl_texture_t;

#if COLOUR_DEPTH != 0

extern int default_font_linespace;

#  define LINESPACE  (default_font_linespace)
#else
// a font height of zero could cause division by zero errors, even though it should not be used in a server
#  define LINESPACE  1
#endif

#define BASE_TAB_WIDTH (40)


struct clip_dimension {
	scr_coord_val x, xx, w, y, yy, h;
};

// helper macros

// save the current clipping and set a new one
#define PUSH_CLIP(x,y,w,h) \
{\
clip_dimension const p_cr = display_get_clip_wh(); \
display_set_clip_wh(x, y, w, h);

// save the current clipping and set a new one
// fit it to old clipping region
#define PUSH_CLIP_FIT(x,y,w,h) \
{\
	clip_dimension const p_cr = display_get_clip_wh(); \
	display_set_clip_wh(x, y, w, h, true);

// restore a saved clipping rect
#define POP_CLIP() \
display_set_clip_wh(p_cr.x, p_cr.y, p_cr.w, p_cr.h); \
}

/**
 *
 */
rgba_t color_idx_to_rgb(int idx);

rgba_t color_idx_to_rgba(int idx, int transparent_percent);

/*
 * Get 24bit RGB888 colour from an index of the old 8bit palette
 */
rgb888_t get_color_rgb(uint8 idx);

/*
 * Environment colours from RGB888 to system format
 */
void env_t_rgb_to_system_colors();

/**
 * Helper functions for clipping along tile borders.
 */
void add_poly_clip(int x0_,int y0_, int x1, int y1, int ribi  CLIP_NUM_DEF);
void clear_all_poly_clip(CLIP_NUM_DEF0);
void activate_ribi_clip(int ribi  CLIP_NUM_DEF);

/* Do no access directly, use the get_tile_raster_width()
 * function instead.
 */
extern scr_coord_val tile_raster_width;
inline scr_coord_val get_tile_raster_width(){return tile_raster_width;}


extern scr_coord_val base_tile_raster_width;
inline scr_coord_val get_base_tile_raster_width(){return base_tile_raster_width;}

/* changes the raster width after loading */
scr_coord_val display_set_base_raster_width(scr_coord_val new_raster);

#define ZOOM_NEUTRAL (3)
int set_zoom_level(int level);
int zoom_level_up();
int zoom_level_down();
void get_zoom_fraction(int &numinator, int &denominator);

/**
 * Initialises the graphics module
 */
bool simgraph_init(scr_size window_size, sint16 fullscreen);
bool is_display_init();
void simgraph_exit();
void simgraph_resize(scr_size new_window_size);


image_id get_image_count();
void register_image(class image_t *);

// delete all images above a certain number ...
void display_free_all_images_above( image_id above );

// unzoomed offsets
void display_get_base_image_offset( image_id image, scr_coord_val *xoff, scr_coord_val *yoff, scr_coord_val *xw, scr_coord_val *yw );
// zoomed offsets
void display_get_image_offset( image_id image, scr_coord_val *xoff, scr_coord_val *yoff, scr_coord_val *xw, scr_coord_val *yw );
void display_mark_img_dirty( image_id image, scr_coord_val x, scr_coord_val y );

void mark_rect_dirty_wc(scr_coord_val x1, scr_coord_val y1, scr_coord_val x2, scr_coord_val y2); // clips to screen only
void mark_rect_dirty_clip(scr_coord_val x1, scr_coord_val y1, scr_coord_val x2, scr_coord_val y2  CLIP_NUM_DEF); // clips to clip_rect
void mark_screen_dirty();

scr_coord_val display_get_width();
scr_coord_val display_get_height();
void display_set_height(scr_coord_val);
void display_set_actual_width(scr_coord_val);

void display_day_night_shift(int night);

// set first and second company color for player
void display_set_player_color_scheme(const int player, const uint8 col1, const uint8 col2);

/**
 * Set color for subsequent drawing operations
 */
void display_set_color(const rgba_t & color);

void display_tile_from_sheet(const gl_texture_t * gltex, int x, int y, int w, int h,
                             int tile_x, int tile_y, int tile_w, int tile_h);

// only used for GUI, display image inside a rect
void display_img_aligned(const image_id n, scr_rect area, int align, sint8 player_nr_raw, const bool dirty);

// display image with day and night change
void display_img_aux(const image_id n, scr_coord_val xp, scr_coord_val yp, const signed char player_nr, const bool daynight, const bool dirty  CLIP_NUM_DEF);

/**
 * draws the images with alpha, either blended or as outline
 */
void display_rezoomed_img_blend(const image_id n, scr_coord_val xp, scr_coord_val yp, const signed char player_nr, rgba_t color_index, const bool daynight, const bool dirty  CLIP_NUM_DEF);
#define display_img_blend( n, x, y, c, dn, d ) display_rezoomed_img_blend( (n), (x), (y), 0, (c), (dn), (d)  CLIP_NUM_DEFAULT)

#define ALPHA_RED 0x1
#define ALPHA_GREEN 0x2
#define ALPHA_BLUE 0x4

void display_rezoomed_img_alpha(const image_id n, const image_id alpha_n, const unsigned alpha_flags, scr_coord_val xp, scr_coord_val yp, const signed char player_nr, rgba_t color_index, const bool daynight, const bool dirty  CLIP_NUM_DEF);
#define display_img_alpha( n, a, f, x, y, c, dn, d ) display_rezoomed_img_alpha( (n), (a), (f), (x), (y), 0, (c), (dn), (d)  CLIP_NUM_DEFAULT)

// display zoomed image
void display_color_img(const image_id n, scr_coord_val xp, scr_coord_val yp, const signed char player_nr, scr_coord_val w=0, scr_coord_val h=0);

// display unzoomed image
void display_base_img(const image_id n, scr_coord_val xp, scr_coord_val yp, const signed char player_nr, scr_coord_val w, scr_coord_val h);
void display_base_img(const image_id n, scr_coord_val xp, scr_coord_val yp, const signed char player_nr);

typedef image_id stretch_map_t[3][3];

// this displays a 3x3 array of images to fit the scr_rect
// Hajo: color is a player color index here.
void display_img_stretch(const stretch_map_t &imag, scr_rect area);

// display unzoomed image with alpha, either blended or as outline
void display_base_img_blend(const image_id n, scr_coord_val xp, scr_coord_val yp, const signed char player_nr, rgba_t color_index, const bool daynight, const bool dirty  CLIP_NUM_DEF CLIP_NUM_DEFAULT_ZERO);
void display_base_img_alpha(const image_id n, const image_id alpha_n, const unsigned alpha_flags, scr_coord_val xp, scr_coord_val yp, const sint8 player_nr, rgba_t color_index, const bool daynight, const bool dirty  CLIP_NUM_DEF CLIP_NUM_DEFAULT_ZERO);

// pointer to image display procedures
typedef void (*display_image_proc)(const image_id n, scr_coord_val xp, scr_coord_val yp, const signed char player_nr);
typedef void (*display_blend_proc)(const image_id n, scr_coord_val xp, scr_coord_val yp, const signed char player_nr, rgba_t color_index, const bool daynight, const bool dirty  CLIP_NUM_DEF);
typedef void (*display_alpha_proc)(const image_id n, const image_id alpha_n, const unsigned alpha_flags, scr_coord_val xp, scr_coord_val yp, const signed char player_nr, rgba_t color_index, const bool daynight, const bool dirty  CLIP_NUM_DEF);

// variables for storing currently used image procedure set and tile raster width
extern display_image_proc display_normal;
extern display_image_proc display_color;
extern display_blend_proc display_blend;
extern display_alpha_proc display_alpha;
extern scr_coord_val current_tile_raster_width;

// call this instead of referring to current_tile_raster_width directly
#define get_current_tile_raster_width() (current_tile_raster_width)


// Blends two colors
rgba_t display_blend_colors(rgba_t background, rgba_t foreground, float blend);

// blends a rectangular region
void display_fillbox_wh(scr_coord_val xp, scr_coord_val yp, scr_coord_val w, scr_coord_val h);

void display_fillbox_wh_rgb(scr_coord_val xp, scr_coord_val yp, scr_coord_val w, scr_coord_val h, rgba_t color, bool dirty);

void display_fillbox_wh_clip_rgb(scr_coord_val xp, scr_coord_val yp, scr_coord_val w, scr_coord_val h, rgba_t color, bool dirty  CLIP_NUM_DEF CLIP_NUM_DEFAULT_ZERO);

void display_vline_wh_clip_rgb(scr_coord_val xp, scr_coord_val yp, scr_coord_val h, rgba_t color, bool dirty  CLIP_NUM_DEF CLIP_NUM_DEFAULT_ZERO);

void display_flush_framebuffer_to_buffer();

void display_flush_buffer();

void display_show_pointer(int yesno);
void display_set_pointer(int pointer);
void display_show_load_pointer(int loading);


void display_array_wh(scr_coord_val xp, scr_coord_val yp, scr_coord_val w, scr_coord_val h, const rgb888_t *arr);

// compound painting routines
void display_ddd_box_rgb(scr_coord_val x1, scr_coord_val y1, scr_coord_val w, scr_coord_val h, rgba_t tl_color, rgba_t rd_color, bool dirty);
void display_ddd_box_clip_rgb(scr_coord_val x1, scr_coord_val y1, scr_coord_val w, scr_coord_val h, rgba_t tl_color, rgba_t rd_color);


// #ifdef MULTI_THREAD
int display_glyph(scr_coord_val x, scr_coord_val y, utf32 c, const font_t * font);


// line drawing primitives
void display_direct_line_rgb(const scr_coord_val x, const scr_coord_val y, const scr_coord_val xx, const scr_coord_val yy, rgba_t color);
void display_direct_line_dotted_rgb(const scr_coord_val x, const scr_coord_val y, const scr_coord_val xx, const scr_coord_val yy, const scr_coord_val draw, const scr_coord_val dontDraw, rgba_t color);
void display_circle_rgb( scr_coord_val x0, scr_coord_val  y0, int radius, rgba_t color );
void display_filled_circle_rgb( scr_coord_val x0, scr_coord_val  y0, int radius, rgba_t color );
void draw_bezier_rgb(scr_coord_val Ax, scr_coord_val Ay, scr_coord_val Bx, scr_coord_val By, scr_coord_val ADx, scr_coord_val ADy, scr_coord_val BDx, scr_coord_val BDy, rgba_t colore, scr_coord_val draw, scr_coord_val dontDraw);

void display_right_triangle_rgb(scr_coord_val x, scr_coord_val y, scr_coord_val height, rgba_t colval, const bool dirty);
void display_signal_direction_rgb(scr_coord_val x, scr_coord_val y, uint8 way_dir, uint8 sig_dir, rgba_t col1, rgba_t col1_dark, bool is_diagonal=false, uint8 slope = 0 );

void display_set_clip_wh(scr_coord_val x, scr_coord_val y, scr_coord_val w, scr_coord_val h  CLIP_NUM_DEF CLIP_NUM_DEFAULT_ZERO, bool fit = false);
clip_dimension display_get_clip_wh(CLIP_NUM_DEF0 CLIP_NUM_DEFAULT_ZERO);

void display_push_clip_wh(scr_coord_val x, scr_coord_val y, scr_coord_val w, scr_coord_val h  CLIP_NUM_DEF CLIP_NUM_DEFAULT_ZERO);
void display_swap_clip_wh(CLIP_NUM_DEF0);
void display_pop_clip_wh(CLIP_NUM_DEF0);


bool display_snapshot( const scr_rect &area );

#if COLOUR_DEPTH != 0
extern rgb888_t display_day_lights  [LIGHT_COUNT];
extern rgb888_t display_night_lights[LIGHT_COUNT];
#endif

#endif
