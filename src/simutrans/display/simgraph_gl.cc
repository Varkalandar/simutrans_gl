/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "../simconst.h"
#include "../sys/simsys.h"
#include "../simdebug.h"
#include "../descriptor/image.h"

#include "simgraph.h"

#include <GLFW/glfw3.h>

static GLFWwindow* window; 
	
scr_coord_val tile_raster_width = 16; // zoomed
scr_coord_val base_tile_raster_width = 16; // original


// things to get rid off
/*
 * special colors during daytime
 */
rgb888_t display_day_lights[LIGHT_COUNT] = {
	{ 0x57, 0x65, 0x6F }, // Dark windows, lit yellowish at night
	{ 0x7F, 0x9B, 0xF1 }, // Lighter windows, lit blueish at night
	{ 0xFF, 0xFF, 0x53 }, // Yellow light
	{ 0xFF, 0x21, 0x1D }, // Red light
	{ 0x01, 0xDD, 0x01 }, // Green light
	{ 0x6B, 0x6B, 0x6B }, // Non-darkening grey 1 (menus)
	{ 0x9B, 0x9B, 0x9B }, // Non-darkening grey 2 (menus)
	{ 0xB3, 0xB3, 0xB3 }, // non-darkening grey 3 (menus)
	{ 0xC9, 0xC9, 0xC9 }, // Non-darkening grey 4 (menus)
	{ 0xDF, 0xDF, 0xDF }, // Non-darkening grey 5 (menus)
	{ 0xE3, 0xE3, 0xFF }, // Nearly white light at day, yellowish light at night
	{ 0xC1, 0xB1, 0xD1 }, // Windows, lit yellow
	{ 0x4D, 0x4D, 0x4D }, // Windows, lit yellow
	{ 0xE1, 0x00, 0xE1 }, // purple light for signals
	{ 0x01, 0x01, 0xFF }  // blue light
};


/*
 * special colors during nighttime
 */
rgb888_t display_night_lights[LIGHT_COUNT] = {
	{ 0xD3, 0xC3, 0x80 }, // Dark windows, lit yellowish at night
	{ 0x80, 0xC3, 0xD3 }, // Lighter windows, lit blueish at night
	{ 0xFF, 0xFF, 0x53 }, // Yellow light
	{ 0xFF, 0x21, 0x1D }, // Red light
	{ 0x01, 0xDD, 0x01 }, // Green light
	{ 0x6B, 0x6B, 0x6B }, // Non-darkening grey 1 (menus)
	{ 0x9B, 0x9B, 0x9B }, // Non-darkening grey 2 (menus)
	{ 0xB3, 0xB3, 0xB3 }, // non-darkening grey 3 (menus)
	{ 0xC9, 0xC9, 0xC9 }, // Non-darkening grey 4 (menus)
	{ 0xDF, 0xDF, 0xDF }, // Non-darkening grey 5 (menus)
	{ 0xFF, 0xFF, 0xE3 }, // Nearly white light at day, yellowish light at night
	{ 0xD3, 0xC3, 0x80 }, // Windows, lit yellow
	{ 0xD3, 0xC3, 0x80 }, // Windows, lit yellow
	{ 0xE1, 0x00, 0xE1 }, // purple light for signals
	{ 0x01, 0x01, 0xFF }  // blue light
};

// ---




PIXVAL color_idx_to_rgb(PIXVAL idx)
{
	return idx;
}

PIXVAL color_rgb_to_idx(PIXVAL color)
{
	return color;
}


rgb888_t get_color_rgb(uint8)
{
	return {0,0,0};
}

void env_t_rgb_to_system_colors()
{
}

scr_coord_val display_set_base_raster_width(scr_coord_val)
{
	return 0;
}

void set_zoom_factor(int)
{
}

int zoom_factor_up()
{
	return false;
}

int zoom_factor_down()
{
	return false;
}

void mark_rect_dirty_wc(scr_coord_val, scr_coord_val, scr_coord_val, scr_coord_val)
{
}

void mark_rect_dirty_clip(scr_coord_val, scr_coord_val, scr_coord_val, scr_coord_val  CLIP_NUM_DEF_NOUSE)
{
}

void mark_screen_dirty()
{
}

void display_mark_img_dirty(image_id, scr_coord_val, scr_coord_val)
{
}

scr_coord_val display_get_width()
{
	int width, height;
	glfwGetWindowSize(window, &width, &height);	
	return width;
}

scr_coord_val display_get_height()
{
	int width, height;
	glfwGetWindowSize(window, &width, &height);	
	return height;
}

void display_set_height(scr_coord_val)
{
}

void display_set_actual_width(scr_coord_val)
{
}

void display_day_night_shift(int)
{
}

void display_set_player_color_scheme(const int, const uint8, const uint8)
{
}

void register_image(image_t* image)
{
	image->imageid = 1;
}

bool display_snapshot(const scr_rect &)
{
	return false;
}

void display_get_image_offset(image_id image, scr_coord_val *xoff, scr_coord_val *yoff, scr_coord_val *xw, scr_coord_val *yw)
{
	if(  image < 2  ) {
		// initialize offsets with dummy values
		*xoff = 0;
		*yoff = 0;
		*xw   = 0;
		*yw   = 0;
	}
}

void display_get_base_image_offset(image_id image, scr_coord_val *xoff, scr_coord_val *yoff, scr_coord_val *xw, scr_coord_val *yw)
{
	if(  image < 2  ) {
		// initialize offsets with dummy values
		*xoff = 0;
		*yoff = 0;
		*xw   = 0;
		*yw   = 0;
	}
}

clip_dimension display_get_clip_wh(CLIP_NUM_DEF_NOUSE0)
{
	clip_dimension clip_rect;
	clip_rect.x = 0;
	clip_rect.xx = 0;
	clip_rect.w = 0;
	clip_rect.y = 0;
	clip_rect.yy = 0;
	clip_rect.h = 0;
	return clip_rect;
}

void display_set_clip_wh(scr_coord_val, scr_coord_val, scr_coord_val, scr_coord_val  CLIP_NUM_DEF_NOUSE, bool)
{
}

void display_push_clip_wh(scr_coord_val, scr_coord_val, scr_coord_val, scr_coord_val  CLIP_NUM_DEF_NOUSE)
{
}

void display_swap_clip_wh(CLIP_NUM_DEF_NOUSE0)
{
}

void display_pop_clip_wh(CLIP_NUM_DEF_NOUSE0)
{
}

void display_scroll_band(const scr_coord_val, const scr_coord_val, const scr_coord_val)
{
}

void display_img_aux(const image_id, scr_coord_val, scr_coord_val, const sint8, const bool, const bool  CLIP_NUM_DEF_NOUSE)
{
}

void display_color_img(const image_id, scr_coord_val, scr_coord_val, const sint8, const bool, const bool  CLIP_NUM_DEF_NOUSE)
{
}

void display_base_img(const image_id, scr_coord_val, scr_coord_val, const sint8, const bool, const bool  CLIP_NUM_DEF_NOUSE)
{
}

void display_fit_img_to_width( const image_id, sint16)
{
}

void display_img_stretch(const stretch_map_t &, scr_rect, FLAGGED_PIXVAL)
{
}

void display_img_stretch_blend(const stretch_map_t &, scr_rect, FLAGGED_PIXVAL)
{
}

void display_rezoomed_img_blend(const image_id, scr_coord_val, scr_coord_val, const sint8, const FLAGGED_PIXVAL, const bool, const bool  CLIP_NUM_DEF_NOUSE)
{
}

void display_rezoomed_img_alpha(const image_id, const image_id, const unsigned, scr_coord_val, scr_coord_val, const sint8, const FLAGGED_PIXVAL, const bool, const bool  CLIP_NUM_DEF_NOUSE)
{
}

void display_base_img_blend(const image_id, scr_coord_val, scr_coord_val, const sint8, const FLAGGED_PIXVAL, const bool, const bool  CLIP_NUM_DEF_NOUSE)
{
}

void display_base_img_alpha(const image_id, const image_id, const unsigned, scr_coord_val, scr_coord_val, const sint8, const FLAGGED_PIXVAL, const bool, bool  CLIP_NUM_DEF_NOUSE)
{
}

// variables for storing currently used image procedure set and tile raster width
display_image_proc display_normal = display_base_img;
display_image_proc display_color = display_base_img;
display_blend_proc display_blend = display_base_img_blend;
display_alpha_proc display_alpha = display_base_img_alpha;

signed short current_tile_raster_width = 0;

PIXVAL display_blend_colors(PIXVAL, PIXVAL, int)
{
	return 0;
}


void display_blend_wh_rgb(scr_coord_val, scr_coord_val, scr_coord_val, scr_coord_val, PIXVAL, int )
{
}


void display_fillbox_wh_rgb(scr_coord_val x, scr_coord_val y, scr_coord_val w, scr_coord_val h, PIXVAL color, bool )
{
	glBindTexture(GL_TEXTURE_2D, 0);

	glBegin(GL_QUADS);

	glColor4f(1.0f, 0.5f, 0.0f, 1.0f);

	glVertex2i(x, y);
	glVertex2i(x+w, y);
	glVertex2i(x+w, y+h);
	glVertex2i(x, y+h);

	glEnd();	
}


void display_fillbox_wh_clip_rgb(scr_coord_val, scr_coord_val, scr_coord_val, scr_coord_val, PIXVAL, bool  CLIP_NUM_DEF_NOUSE)
{
}

void display_vline_wh_clip_rgb(scr_coord_val, scr_coord_val, scr_coord_val, PIXVAL, bool  CLIP_NUM_DEF_NOUSE)
{
}

void display_array_wh(scr_coord_val, scr_coord_val, scr_coord_val, scr_coord_val, const PIXVAL *)
{
}

int display_glyph(scr_coord_val x, scr_coord_val y, utf32 c, control_alignment_t flags, PIXVAL default_color, const font_t * font  CLIP_NUM_DEF_NOUSE)
{
	return 0;
}

void display_ddd_box_rgb(scr_coord_val, scr_coord_val, scr_coord_val, scr_coord_val, PIXVAL, PIXVAL, bool)
{
}

void display_ddd_box_clip_rgb(scr_coord_val, scr_coord_val, scr_coord_val, scr_coord_val, PIXVAL, PIXVAL)
{
}

void display_ddd_proportional_clip(scr_coord_val, scr_coord_val, FLAGGED_PIXVAL, FLAGGED_PIXVAL, const char *, int  CLIP_NUM_DEF_NOUSE)
{
}

void display_flush_buffer()
{
	glfwSwapBuffers(window);
	printf("Ping!\n");
}

void display_show_pointer(int)
{
}

void display_set_pointer(int)
{
}

void display_show_load_pointer(int)
{
}

bool simgraph_init(scr_size size, sint16)
{
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
	
	window = glfwCreateWindow(size.w, size.h, "Simutrans GL", NULL, NULL);

	dbg->message("simgraph_init()", "GLFW %d,%d -> window: %p", size.w, size.h, window);
	
	if(window)
	{
		glfwMakeContextCurrent(window);
		
		// enable vsync (1 == next frame)
		glfwSwapInterval(1);
	}
	
	return window;
}


bool is_display_init()
{
	return false;
}


void display_free_all_images_above(image_id)
{
}

void simgraph_exit()
{
	glfwDestroyWindow(window);
	
	dr_os_close();
}

void simgraph_resize(scr_size)
{
}

void display_snapshot()
{
}

void display_direct_line_rgb(const scr_coord_val, const scr_coord_val, const scr_coord_val, const scr_coord_val, const PIXVAL)
{
}

void display_direct_line_dotted_rgb(const scr_coord_val, const scr_coord_val, const scr_coord_val, const scr_coord_val, const scr_coord_val, const scr_coord_val, const PIXVAL)
{
}

void display_circle_rgb( scr_coord_val, scr_coord_val, int, const PIXVAL )
{
}

void display_filled_circle_rgb( scr_coord_val, scr_coord_val, int, const PIXVAL )
{
}

void draw_bezier_rgb(scr_coord_val, scr_coord_val, scr_coord_val, scr_coord_val, scr_coord_val, scr_coord_val, scr_coord_val, scr_coord_val, const PIXVAL, scr_coord_val, scr_coord_val )
{
}

void display_right_triangle_rgb(scr_coord_val, scr_coord_val, scr_coord_val, const PIXVAL, const bool)
{
}

void display_signal_direction_rgb( scr_coord_val, scr_coord_val, uint8, uint8, PIXVAL, PIXVAL, bool, uint8 )
{
}

void display_set_progress_text(const char *)
{
}

void display_progress(int, int)
{
}

void display_img_aligned(const image_id, scr_rect, int, sint8, bool)
{
}

void display_proportional_ellipsis_rgb(scr_rect, const char *, int, PIXVAL, bool, bool, PIXVAL)
{
}

image_id get_image_count()
{
	return 0;
}

#ifdef MULTI_THREAD
void add_poly_clip(int, int, int, int, int  CLIP_NUM_DEF_NOUSE)
{
}

void clear_all_poly_clip(const sint8)
{
}

void activate_ribi_clip(int  CLIP_NUM_DEF_NOUSE)
{
}
#else
void add_poly_clip(int, int, int, int, int)
{
}

void clear_all_poly_clip()
{
}

void activate_ribi_clip(int)
{
}
#endif
