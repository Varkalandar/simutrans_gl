/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */


// #include "../simconst.h"
#include "../simmem.h"
#include "../sys/simsys.h"
#include "../simdebug.h"
#include "../descriptor/image.h"
#include "../pathes.h"

#include "simgraph.h"
#include "rgba.h"
#include "font.h"
#include "gl_textures.h"
#include "../dataobj/environment.h"

#include <GL/glew.h>
#include <GLFW/glfw3.h>

static GLFWwindow* window;
static GLint gl_max_texture_size;


// we try to collect and combine images into large
// tile sheets to minimize texture switching.
static gl_texture_t * gl_texture_sheets[256];
static int gl_current_sheet;
static int gl_current_sheet_x;
static int gl_current_sheet_y;


// needed for display_array_wh()
static gl_texture_t * display_array_tex_buffer;


// our frame buffer
static GLuint gl_fbo;
static GLuint gl_texture_colorbuffer;
static int gl_framebuffer_size;
static bool framebuffer_active;

scr_coord_val tile_raster_width = 16; // zoomed
scr_coord_val base_tile_raster_width = 16; // original
scr_coord_val current_tile_raster_width = 16;


static scr_coord_val display_width;
static scr_coord_val display_height;

static clip_dimension clip_rect;
static clip_dimension clip_rect_swap;
static bool swap_active = 0;

#define TRANSPARENT_RUN (0x8000u)

struct imd_t {
	sint16 x; // current (zoomed) min x offset
	sint16 y; // current (zoomed) min y offset
	sint16 w; // current (zoomed) width
	sint16 h; // current (zoomed) height

	// uint8 recode_flags;
	// uint16 player_flags; // bit # is player number, ==1 cache image needs recoding

	// PIXVAL* data[MAX_PLAYER_COUNT]; // current data - zoomed and recolored (player + daynight)

	// PIXVAL* zoom_data; // zoomed original data
	uint32 len;    // current zoom image data size (or base if not zoomed) (used for allocation purposes only)

	sint16 base_x; // min x offset
	sint16 base_y; // min y offset
	sint16 base_w; // width
	sint16 base_h; // height

	gl_texture_t * texture;
	sint16 sheet_x;
	sint16 sheet_y;

	uint8_t * base_data; // original image data
};

/*
 * Image table
 */
static struct imd_t* images = NULL;

/*
 * Number of loaded images
 */
static image_id anz_images = 0;

/*
 * Number of allocated entries for images
 * (>= anz_images)
 */
static image_id alloc_images = 0;


extern bool display_load_font(const char *fname, bool reload = false);


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


// the players colors and colors for simple drawing operations
// each three values correspond to a color, each 8 colors are a player color
static const rgb888_t special_pal[SPECIAL_COLOR_COUNT] =
{
	{  36,  75, 103 }, {  57,  94, 124 }, {  76, 113, 145 }, {  96, 132, 167 }, { 116, 151, 189 }, { 136, 171, 211 }, { 156, 190, 233 }, { 176, 210, 255 },
	{  88,  88,  88 }, { 107, 107, 107 }, { 125, 125, 125 }, { 144, 144, 144 }, { 162, 162, 162 }, { 181, 181, 181 }, { 200, 200, 200 }, { 219, 219, 219 },
	{  17,  55, 133 }, {  27,  71, 150 }, {  37,  86, 167 }, {  48, 102, 185 }, {  58, 117, 202 }, {  69, 133, 220 }, {  79, 149, 237 }, {  90, 165, 255 },
	{ 123,  88,   3 }, { 142, 111,   4 }, { 161, 134,   5 }, { 180, 157,   7 }, { 198, 180,   8 }, { 217, 203,  10 }, { 236, 226,  11 }, { 255, 249,  13 },
	{  86,  32,  14 }, { 110,  40,  16 }, { 134,  48,  18 }, { 158,  57,  20 }, { 182,  65,  22 }, { 206,  74,  24 }, { 230,  82,  26 }, { 255,  91,  28 },
	{  34,  59,  10 }, {  44,  80,  14 }, {  53, 101,  18 }, {  63, 122,  22 }, {  77, 143,  29 }, {  92, 164,  37 }, { 106, 185,  44 }, { 121, 207,  52 },
	{   0,  86,  78 }, {   0, 108,  98 }, {   0, 130, 118 }, {   0, 152, 138 }, {   0, 174, 158 }, {   0, 196, 178 }, {   0, 218, 198 }, {   0, 241, 219 },
	{  74,   7, 122 }, {  95,  21, 139 }, { 116,  37, 156 }, { 138,  53, 173 }, { 160,  69, 191 }, { 181,  85, 208 }, { 203, 101, 225 }, { 225, 117, 243 },
	{  59,  41,   0 }, {  83,  55,   0 }, { 107,  69,   0 }, { 131,  84,   0 }, { 155,  98,   0 }, { 179, 113,   0 }, { 203, 128,   0 }, { 227, 143,   0 },
	{  87,   0,  43 }, { 111,  11,  69 }, { 135,  28,  92 }, { 159,  45, 115 }, { 183,  62, 138 }, { 230,  74, 174 }, { 245, 121, 194 }, { 255, 156, 209 },
	{  20,  48,  10 }, {  44,  74,  28 }, {  68,  99,  45 }, {  93, 124,  62 }, { 118, 149,  79 }, { 143, 174,  96 }, { 168, 199, 113 }, { 193, 225, 130 },
	{  54,  19,  29 }, {  82,  44,  44 }, { 110,  69,  58 }, { 139,  95,  72 }, { 168, 121,  86 }, { 197, 147, 101 }, { 226, 173, 115 }, { 255, 199, 130 },
	{   8,  11, 100 }, {  14,  22, 116 }, {  20,  33, 139 }, {  26,  44, 162 }, {  41,  74, 185 }, {  57, 104, 208 }, {  76, 132, 231 }, {  96, 160, 255 },
	{  43,  30,  46 }, {  68,  50,  85 }, {  93,  70, 110 }, { 118,  91, 130 }, { 143, 111, 170 }, { 168, 132, 190 }, { 193, 153, 210 }, { 219, 174, 230 },
	{  63,  18,  12 }, {  90,  38,  30 }, { 117,  58,  42 }, { 145,  78,  55 }, { 172,  98,  67 }, { 200, 118,  80 }, { 227, 138,  92 }, { 255, 159, 105 },
	{  11,  68,  30 }, {  33,  94,  56 }, {  54, 120,  81 }, {  76, 147, 106 }, {  98, 174, 131 }, { 120, 201, 156 }, { 142, 228, 181 }, { 164, 255, 207 },
	{  64,   0,   0 }, {  96,   0,   0 }, { 128,   0,   0 }, { 192,   0,   0 }, { 255,   0,   0 }, { 255,  64,  64 }, { 255,  96,  96 }, { 255, 128, 128 },
	{   0, 128,   0 }, {   0, 196,   0 }, {   0, 225,   0 }, {   0, 240,   0 }, {   0, 255,   0 }, {  64, 255,  64 }, {  94, 255,  94 }, { 128, 255, 128 },
	{   0,   0, 128 }, {   0,   0, 192 }, {   0,   0, 224 }, {   0,   0, 255 }, {   0,  64, 255 }, {   0,  94, 255 }, {   0, 106, 255 }, {   0, 128, 255 },
	{ 128,  64,   0 }, { 193,  97,   0 }, { 215, 107,   0 }, { 235, 118,   0 }, { 255, 128,   0 }, { 255, 149,  43 }, { 255, 170,  85 }, { 255, 193, 132 },
	{   8,  52,   0 }, {  16,  64,   0 }, {  32,  80,   4 }, {  48,  96,   4 }, {  64, 112,  12 }, {  84, 132,  20 }, { 104, 148,  28 }, { 128, 168,  44 },
	{ 164, 164,   0 }, { 180, 180,   0 }, { 193, 193,   0 }, { 215, 215,   0 }, { 235, 235,   0 }, { 255, 255,   0 }, { 255, 255,  64 }, { 255, 255, 128 },
	{  32,   4,   0 }, {  64,  20,   8 }, {  84,  28,  16 }, { 108,  44,  28 }, { 128,  56,  40 }, { 148,  72,  56 }, { 168,  92,  76 }, { 184, 108,  88 },
	{  64,   0,   0 }, {  96,   8,   0 }, { 112,  16,   0 }, { 120,  32,   8 }, { 138,  64,  16 }, { 156,  72,  32 }, { 174,  96,  48 }, { 192, 128,  64 },
	{  32,  32,   0 }, {  64,  64,   0 }, {  96,  96,   0 }, { 128, 128,   0 }, { 144, 144,   0 }, { 172, 172,   0 }, { 192, 192,   0 }, { 224, 224,   0 },
	{  64,  96,   8 }, {  80, 108,  32 }, {  96, 120,  48 }, { 112, 144,  56 }, { 128, 172,  64 }, { 150, 210,  68 }, { 172, 238,  80 }, { 192, 255,  96 },
	{  32,  32,  32 }, {  48,  48,  48 }, {  64,  64,  64 }, {  80,  80,  80 }, {  96,  96,  96 }, { 172, 172, 172 }, { 236, 236, 236 }, { 255, 255, 255 },
	{  41,  41,  54 }, {  60,  45,  70 }, {  75,  62, 108 }, {  95,  77, 136 }, { 113, 105, 150 }, { 135, 120, 176 }, { 165, 145, 218 }, { 198, 191, 232 }
};


/*
 * Zoom factor
 */
#define MAX_ZOOM_FACTOR (9)
static uint32 zoom_level = ZOOM_NEUTRAL;
static sint32 zoom_num[MAX_ZOOM_FACTOR+1] = { 2, 3, 4, 1, 3, 5, 1, 3, 1, 1 };
static sint32 zoom_den[MAX_ZOOM_FACTOR+1] = { 1, 2, 3, 1, 4, 8, 2, 8, 4, 8 };


rgba_t color_idx_to_rgb(int idx)
{
    if(idx >= 0 && idx < SPECIAL_COLOR_COUNT) {
        return special_pal[idx];
    }
    else if(idx >= SPECIAL_COLOR_COUNT && idx < SPECIAL_COLOR_COUNT + LIGHT_COUNT) {
        return display_day_lights[idx - SPECIAL_COLOR_COUNT];
    }
    else {
        return RGBA_BLACK;
    }
}


rgba_t color_idx_to_rgba(int idx, int transparent_percent)
{
    if(idx >= 0 && idx < SPECIAL_COLOR_COUNT) {
        return rgba_t(special_pal[idx], 1.0f - (transparent_percent / 100.0f));
    }
    else if(idx >= SPECIAL_COLOR_COUNT && idx < SPECIAL_COLOR_COUNT + LIGHT_COUNT) {
        return rgba_t(display_day_lights[idx - SPECIAL_COLOR_COUNT], 1.0f - (transparent_percent / 100.0f));
    }
    else {
        return RGBA_BLACK;
    }
}


rgb888_t get_color_rgb(uint8 idx)
{
    if(idx < SPECIAL_COLOR_COUNT) {
        return special_pal[idx];
    }
    else if(idx < SPECIAL_COLOR_COUNT + LIGHT_COUNT) {
        return display_day_lights[idx - SPECIAL_COLOR_COUNT];
    }

	// Return black for anything else
	return rgb888_t {0,0,0};
}


void env_t_rgb_to_system_colors()
{
}


/**
 * changes the raster width after loading
 */
scr_coord_val display_set_base_raster_width(scr_coord_val new_raster)
{
	const scr_coord_val old = base_tile_raster_width;

	base_tile_raster_width = new_raster;
   	tile_raster_width = (new_raster *  zoom_num[zoom_level]) / zoom_den[zoom_level];
    current_tile_raster_width = new_raster;
    
	return old;
}


int set_zoom_level(int level)
{
    int old_zoom = zoom_level;
    
	// do not zoom smaller than 4 pixels
	if((base_tile_raster_width * zoom_num[level]) / zoom_den[level] > 4) {
		zoom_level = level;
		tile_raster_width = (base_tile_raster_width * zoom_num[zoom_level]) / zoom_den[zoom_level];
        current_tile_raster_width = tile_raster_width;
		// dbg->message("set_zoom_level()", "Zoom level now %d (%i/%i) -> raster %d", 
        //             zoom_level, zoom_num[zoom_level], zoom_den[zoom_level], current_tile_raster_width);
    }

    return old_zoom;
}


int zoom_level_up()
{
	// zoom out, if size permits
	if(zoom_level > 0) {
		set_zoom_level(zoom_level - 1);
		return true;
	}
	return false;
}


int zoom_level_down()
{
	if(zoom_level < MAX_ZOOM_FACTOR) {
		set_zoom_level(zoom_level + 1);
		return true;
	}
	return false;
}


void get_zoom_fraction(int &n, int &d)
{
    n = zoom_num[zoom_level];
    d = zoom_den[zoom_level];    
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
    return display_width;
}


scr_coord_val display_get_height()
{
    return display_height;
}


void display_set_height(scr_coord_val)
{
}


void display_set_actual_width(scr_coord_val)
{
}


static rgba_t day_night_color (1, 1, 1, 1);


void display_day_night_shift(float night)
{
    static rgba_t color_day = RGBA_WHITE;
    static rgba_t color_night (0.3, 0.4, 0.6, 1);

    day_night_color = display_blend_colors(color_day, color_night, night);
}


rgba_t display_get_day_night_color()
{
    return day_night_color;
}


void display_set_player_color_scheme(const int, const uint8, const uint8)
{
}


static uint8_t * rgb343to888(const uint16_t c, const uint8_t alpha, uint8_t *tp)
{

    *tp++ = (c >> 2) & 0xE0; // red
    *tp++ = (c << 1) & 0xF0; // green
    *tp++ = (c << 5) & 0xE0; // blue
    *tp++ = alpha;           // alpha


/*
    *tp++ = 255; // red
    *tp++ = 255; // green
    *tp++ = 0; // blue
    *tp++ = 255;           // alpha

	dbg->message("rgb343to888", "343 input %x, alpha=%d -> %d %d %d", c, alpha, *(tp-4), *(tp-3), *(tp-2));
*/
    return tp;
}


static uint8_t * rgb555to888(const uint16_t c, const uint8_t alpha, uint8_t *tp)
{
    *tp++ = (c >> 7) & 0xF8; // red
    *tp++ = (c >> 2) & 0xF8; // green
    *tp++ = (c << 3) & 0xF8; // blue
    *tp++ = alpha;           // alpha

    return tp;
}


static uint8_t * special_rgb_to_rgba(const uint16_t c, const uint8_t alpha, uint8_t *tp)
{
    uint32 rgb = image_t::rgbtab[c & 255];
    *tp ++ = (rgb >> 16) & 0xFF;
    *tp ++ = (rgb >> 8) & 0xFF;
    *tp ++ = (rgb >> 0) & 0xFF;
    *tp ++ = alpha;

    return tp;
}


static void convert_transparent_pixel_run(uint8_t * dest, const uint16_t * src, const uint16_t * const end)
{
    if(*src < 0x8000) dbg->message("convert_transparent_pixel_run()", "Found suspicious pixel %x", *src);


	// player color or transparent rgb?
	if (*src < 0x8020) {
		// player or special color
		while (src < end) {
			dest = special_rgb_to_rgba(*src++, 255, dest);;
		}
	}
	else {
		while (src < end) {
			// a semi-transparent pixel

			// v = 0x8020 + 31*31 + pix*31 + alpha;
			uint16 aux   = *src++ - 0x8020 - 31*31;
			uint16 alpha = ((aux % 31) + 1);
			dest = rgb343to888((aux / 31) & 0x3FF, alpha << 3, dest);
		}
	}
}


static uint8_t * convert_pixel_run(const uint16_t * sp, const uint16_t runlen, uint8_t * tp)
{
    // dbg->message("convert()", "Converting a run of %d pixels", runlen);

    const uint16_t * end = sp + runlen;

    while(sp < end)
    {
        uint16_t c = *sp ++;
		if(c >= 0x8000)
		{
			// dbg->message("convert()", "Found suspicious pixel %x", c);
			uint32 rgb = image_t::rgbtab[c & 255];
			*tp ++ = (rgb >> 16) & 0xFF;
			*tp ++ = (rgb >> 8) & 0xFF;
			*tp ++ = (rgb >> 0) & 0xFF;
			*tp ++ = 255;
		}
		else
		{
			tp = rgb555to888(c, 255, tp);
		}
    }

    return tp;
}


static void copy_into_texture_sheet(imd_t * image, uint8_t * tex, int scanline)
{
    for(int y=0; y<image->base_h; y++)
    {
        for(int x=0; x<image->base_w; x++)
        {
            uint8_t * src = image->base_data + y*image->base_w * 4 + x * 4;
            uint8_t * dst = tex +
                            (gl_current_sheet_y+y) * scanline * 4+
                            (gl_current_sheet_x+x) * 4;
            // rgba
            *dst++ = *src++;
            *dst++ = *src++;
            *dst++ = *src++;
            *dst = *src;
        }
    }
}


static void convert_image(imd_t * image)
{
	// is this an oversized image?
	if(image->base_h >= base_tile_raster_width)
	{
		// yes, give it a texture of its own
		image->texture = gl_texture_t::create_texture(image->base_w, image->base_h, image->base_data);
		image->sheet_x = 0;
		image->sheet_y = 0;

		dbg->message("convert_image()", "Oversized image got tex id=%d", image->texture->tex_id);
	}
	else
	{
		// dbg->message("register_image()", "Advancing in sheet line to x=%d", gl_current_sheet_x);

		// do we need to start a new row?
		if(gl_current_sheet_x + image->base_w > gl_max_texture_size)
		{
			gl_current_sheet_x = 0;
			gl_current_sheet_y += base_tile_raster_width;

			dbg->message("register_image()", "Starting texture sheet line %d of %d pixels", gl_current_sheet_y, gl_max_texture_size);

			// time to start a new sheet?
			if(gl_current_sheet_y >= gl_max_texture_size)
			{
				gl_current_sheet_y = 0;
				gl_current_sheet ++;
			}
		}

		// do we need to allocate a new  sheet?
		if(gl_texture_sheets[gl_current_sheet] == 0)
		{
			dbg->message("register_image()", "Starting texture sheet #%d", gl_current_sheet);
			gl_current_sheet_x = 0;
			gl_current_sheet_y = 0;

			gl_texture_sheets[gl_current_sheet] =
				gl_texture_t::create_texture(gl_max_texture_size, gl_max_texture_size,
											 (uint8_t *)calloc(gl_max_texture_size * gl_max_texture_size * 4, 1));

            free(gl_texture_sheets[gl_current_sheet]->data);
            gl_texture_sheets[gl_current_sheet]->data = 0;
		}

		image->sheet_x = gl_current_sheet_x;
		image->sheet_y = gl_current_sheet_y;
		image->texture = gl_texture_sheets[gl_current_sheet];

		// copy_into_texture_sheet(image, image->texture->data, gl_max_texture_size);
		image->texture->update_region(image->sheet_x, image->sheet_y, image->base_w, image->base_h, image->base_data);

        // advance in row
		gl_current_sheet_x += image->base_w;
	}
}


void register_image(image_t * image_in, void (*postprocessor)(int w, int h, uint8 * data))
{
	struct imd_t *image;

	/* valid image? */
	if(image_in->len == 0 || image_in->h == 0) {
		dbg->warning("register_image()", "Ignoring image %d because of missing data", anz_images);
		image_in->imageid = IMG_EMPTY;
		return;
	}

	if(anz_images == alloc_images) {
		if(images==NULL) {
			alloc_images = 510;
		}
		else {
			alloc_images += 512;
		}
		if(anz_images > alloc_images) {
			// overflow
			dbg->fatal( "register_image", "*** Out of images (more than %li!) ***", anz_images );
		}
		images = REALLOC(images, imd_t, alloc_images);
	}

	image_in->imageid = anz_images;
	image = &images[anz_images];
	anz_images++;

	// dbg->message("register_image()", "%d images, offset %d , %d, converting %dx%d pixels", anz_images, image_in->x, image_in->y, image_in->w, image_in->h);

	uint8_t * rgba_data = (uint8_t *)calloc(image_in->w * image_in->h * 4, 1);
    
    if(image_in->bpp == 32)
    {
        // a 32bit image
        dbg->message("register_image()", "%d images, offset %d , %d, got %dx%d 32bpp pixels", anz_images, image_in->x, image_in->y, image_in->w, image_in->h);

        uint32 * in = (uint32 *)image_in->data;
        
        // convert argb to rgba
        for(int i = 0; i < image_in->w * image_in->h; i++)
        {
            const uint32 argb = in[i];
            rgba_data[i*4+0] = (argb >> 16) & 0xFF;
            rgba_data[i*4+1] = (argb >> 8) & 0xFF;
            rgba_data[i*4+2] = argb & 0xFF;
            rgba_data[i*4+3] = argb >> 24;
        }
    }
    else
    {
        // Old style 16 bit
        uint8_t * tp = rgba_data;
        const uint16_t * sp = image_in->data;
        scr_coord_val h = image_in->h;

        do { // line decoder
            uint16_t runlen = *sp++;
            uint8_t *p = tp;

            // one line decoder
            do {
                // we start with a clear run
                p += (runlen & ~TRANSPARENT_RUN) * 4;

    //            dbg->message("register_image()", "Converting %d transparent pixels", runlen);

                // now get colored pixels
                runlen = *sp++;
                if(runlen & TRANSPARENT_RUN) {
                    runlen &= ~TRANSPARENT_RUN;

    //                dbg->message("register_image()", "Converting %d special color pixels", runlen);
                    convert_transparent_pixel_run(p, sp, sp+runlen);
                    p += runlen * 4;
                    sp += runlen;
                }
                else {
    //                dbg->message("register_image()", "Converting %d color pixels", runlen);

                    p = convert_pixel_run(sp, runlen, p);
                    sp += runlen;
                }
                runlen = *sp++;
            } while(runlen);

    //        dbg->message("register_image()", "-- Line converted, %d left --", h);

            tp += image_in->w * 4;
        } while(--h > 0);
    }

    // debug
/*
    for(int y = 0; y < image_in->h; y++)
    {
        for(int x = 0; x < image_in->w; x++)
        {
            int i = y * image_in->w * 4 + x * 4;

            rgba_data[i] = 64;
            rgba_data[i+1] = (x == 0 || y == 0) ? 255 : 64;
            rgba_data[i+2] = (x == image_in->w-1 || y == image_in->h-1) ? 255 : 64;
            rgba_data[i+3] = 255;
        }
    }
*/
    // rgba_data is now ready to be used. Some tiles need postprocessing though
    // e.g. shore and climate transitions
    
    if(postprocessor) postprocessor(image_in->w, image_in->h, rgba_data);

    // Now create a new image descriptor
	image->len = image_in->len;
	image->x = image_in->x;
	image->y = image_in->y;
	image->w = image_in->w;
	image->h = image_in->h;
	image->base_x = image_in->x;
	image->base_y = image_in->y;
	image->base_w = image_in->w;
	image->base_h = image_in->h;
	image->base_data = rgba_data;

	convert_image(image);
    
    image_in->base_data = rgba_data;
}


bool display_snapshot(const scr_rect &)
{
	return false;
}


/** query offsets */
void display_get_image_offset(image_id image, scr_coord_val *xoff, scr_coord_val *yoff, scr_coord_val *xw, scr_coord_val *yw)
{
	if(  image < anz_images  ) {
		*xoff = images[image].x;
		*yoff = images[image].y;
		*xw   = images[image].w;
		*yw   = images[image].h;
	}
}


/** query un-zoomed offsets */
void display_get_base_image_offset(image_id image, scr_coord_val *xoff, scr_coord_val *yoff, scr_coord_val *xw, scr_coord_val *yw)
{
	if(  image < anz_images  ) {
		*xoff = images[image].base_x;
		*yoff = images[image].base_y;
		*xw   = images[image].base_w;
		*yw   = images[image].base_h;
	}
}


/**
 * Clips interval [x,x+w] such that left <= x and x+w <= right.
 * If @p w is negative, it stays negative.
 * @returns difference between old and new value of @p x.
 */
inline int clip_intv(scr_coord_val &x, scr_coord_val &w, const scr_coord_val left, const scr_coord_val right)
{
	scr_coord_val xx = min(x+w, right);
	scr_coord_val xoff = left - x;
	if (xoff > 0) { // equivalent to x < left
		x = left;
	}
	else {
		xoff = 0;
	}
	w = xx - x;
	return xoff;
}


/// wrapper for clip_intv
static int clip_wh(scr_coord_val *x, scr_coord_val *w, const scr_coord_val left, const scr_coord_val right)
{
	return clip_intv(*x, *w, left, right);
}


clip_dimension display_get_clip_wh(CLIP_NUM_DEF_NOUSE0)
{
	return clip_rect;
}


void display_set_clip_wh(scr_coord_val x, scr_coord_val y, scr_coord_val w, scr_coord_val h, bool fit)
{
	if (!fit) {
		// clip_wh( &x, &w, 0, display_get_width());
		// clip_wh( &y, &h, 0, display_get_height());
	}
	else {
		clip_wh( &x, &w, clip_rect.x, clip_rect.xx);
		clip_wh( &y, &h, clip_rect.y, clip_rect.yy);
	}

	clip_rect.x = x;
	clip_rect.y = y;
	clip_rect.w = w;
	clip_rect.h = h;
	clip_rect.xx = x + w; // watch out, clips to scr_coord_val max
	clip_rect.yy = y + h; // watch out, clips to scr_coord_val max

    // dbg->message("display_set_clip_wh()", "x=%d y=%d width=%d height=%d", x, y, w, h);
    
    
    if(framebuffer_active)
    {
        int n = zoom_num[zoom_level];
        int d = zoom_den[zoom_level];
        
        glScissor(clip_rect.x*d/n, clip_rect.y*d/n, clip_rect.w*d/n, clip_rect.h*d/n);
        // glScissor(0, 0, gl_framebuffer_size, gl_framebuffer_size);                
    }
    else
    {
        glScissor(clip_rect.x, display_get_height()-clip_rect.y-clip_rect.h, clip_rect.w, clip_rect.h);
    }
}


void display_push_clip_wh(scr_coord_val x, scr_coord_val y, scr_coord_val w, scr_coord_val h)
{
	assert(!swap_active);
	// save active clipping rectangle
	clip_rect_swap = clip_rect;
	// active rectangle provided by parameters
	display_set_clip_wh(x, y, w, h);
	swap_active = true;
}


void display_swap_clip_wh()
{
	if (swap_active) {
		// swap clipping rectangles
		clip_dimension save = clip_rect;
		clip_rect = clip_rect_swap;
		clip_rect_swap = save;

    	display_set_clip_wh(clip_rect.x, clip_rect.y, clip_rect.w, clip_rect.h);
	}
}


void display_pop_clip_wh(CLIP_NUM_DEF0)
{
	if (swap_active) {
		// swap original clipping rectangle back
		clip_rect   = clip_rect_swap;
    	display_set_clip_wh(clip_rect.x, clip_rect.y, clip_rect.w, clip_rect.h);
		swap_active = false;
	}
}


/**
 * Set color for subsequent drawing operations
 */
void display_set_color(const rgba_t & color)
{
    glColor4f(color.red, color.green, color.blue, color.alpha);
}


void display_tile_from_sheet(const gl_texture_t * gltex, int x, int y, int w, int h,
                             int tile_x, int tile_y, int tile_w, int tile_h)
{
    // texture coordinates in fractions of sheet size
    const float left = tile_x / (float)gltex->width;
    const float top = tile_y / (float)gltex->height;
    const float gw = tile_w / (float)gltex->width;
    const float gh = tile_h / (float)gltex->height;

    glEnable(GL_SCISSOR_TEST);
    
    gl_texture_t::bind(gltex->tex_id);

	glBegin(GL_QUADS);

    glTexCoord2f(left, top);
	glVertex2i(x, y);

    glTexCoord2f(left + gw, top);
	glVertex2i(x + w, y);

    glTexCoord2f(left + gw, top + gh);
	glVertex2i(x + w, y + h);

    glTexCoord2f(left, top + gh);
	glVertex2i(x, y + h);

	glEnd();

    glDisable(GL_SCISSOR_TEST);
}


static void display_box_wh(int x, int y, int w, int h, rgba_t color)
{
	display_fillbox_wh_rgb(x, y, w, 1, color, true);
	display_fillbox_wh_rgb(x, y+h-1, w, 1, color, true);
}


void display_color_img(const image_id id, scr_coord_val x, scr_coord_val y, const sint8 player_nr, scr_coord_val w, scr_coord_val h)
{
	if(id < anz_images)
	{
		// debug clipping
		// display_box_wh(clip_rect.x, clip_rect.y, clip_rect.w, clip_rect.h, rgba_t(1, 0, 0, 0.5f));

		imd_t & imd = images[id];
        int n = zoom_num[zoom_level];
        int d = zoom_den[zoom_level];

		x = x * d / n + imd.base_x;
		y = y * d / n + imd.base_y;

		w = (w == 0) ? imd.base_w : w;
		h = (h == 0) ? imd.base_h : h;

        display_tile_from_sheet(imd.texture, x, y, w, h,
                                imd.sheet_x, imd.sheet_y, imd.base_w, imd.base_h);
	}
}


static void display_img(const image_id id, scr_coord_val x, scr_coord_val y, const sint8 player_nr)
{
	if(id < anz_images)
	{
		imd_t & imd = images[id];
        int n = zoom_num[zoom_level];
        int d = zoom_den[zoom_level];

		x = x * d / n + imd.base_x;
		y = y * d / n + imd.base_y;

		int w = imd.base_w;
		int h = imd.base_h;
        
        display_tile_from_sheet(imd.texture, x, y, w, h,
                                imd.sheet_x, imd.sheet_y, imd.base_w, imd.base_h);
    }
}


void display_light_img(const image_id id, scr_coord_val x, scr_coord_val y, scr_coord_val w, scr_coord_val h)
{
	if(id < anz_images)
	{
		imd_t & imd = images[id];

		x += imd.base_x;
		y += imd.base_y;

        glBlendFunc(GL_SRC_ALPHA, GL_DST_ALPHA);

		display_tile_from_sheet(imd.texture, x, y, w, h,
								imd.sheet_x, imd.sheet_y, imd.base_w, imd.base_h);

        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}
}


void display_base_img(const image_id id, scr_coord_val x, scr_coord_val y, const sint8 player_nr, scr_coord_val w, scr_coord_val h)
{
	if(id < anz_images)
	{
		imd_t & imd = images[id];

		x += imd.base_x;
		y += imd.base_y;

		display_tile_from_sheet(imd.texture, x, y, w, h,
								imd.sheet_x, imd.sheet_y, imd.base_w, imd.base_h);
	}
}


void display_base_img(const image_id id, scr_coord_val x, scr_coord_val y, const sint8 player_nr)
{
	if(id < anz_images)
	{
		imd_t & imd = images[id];

		x += imd.base_x;
		y += imd.base_y;

		int w = imd.base_w;
		int h = imd.base_h;

		display_tile_from_sheet(imd.texture, x, y, w, h,
								imd.sheet_x, imd.sheet_y, imd.base_w, imd.base_h);
	}
}


// local helper function for tiled buttons
static void display_three_image_row( image_id i1, image_id i2, image_id i3, scr_rect row)
{
// 	dbg->message("display_three_image_row", "%d %d %d %d", row.x, row.y, row.w, row.h);
	if(  i1!=IMG_EMPTY  ) {
		scr_coord_val w = images[i1].w;
		display_base_img(i1, row.x, row.y, 0);
		row.x += w;
		row.w -= w;
	}
	// right
	if(  i3!=IMG_EMPTY  ) {
		scr_coord_val w = images[i3].w;
		display_base_img(i3, row.get_right()-w, row.y, 0);
		row.w -= w;
	}
	// middle
	if(  i2!=IMG_EMPTY  ) {
		scr_coord_val w = images[i2].w;
		// tile it wide
		while(  w <= row.w  ) {
			display_base_img(i2, row.x, row.y, 0);
			row.x += w;
			row.w -= w;
		}
		// for the rest we have to clip the rectangle
		if(  row.w > 0  ) {
			clip_dimension const cl = display_get_clip_wh();
			display_set_clip_wh( cl.x, cl.y, max(0,min(row.get_right(),cl.xx)-cl.x), cl.h );
			display_base_img(i2, row.x, row.y, 0);
			display_set_clip_wh(cl.x, cl.y, cl.w, cl.h );
		}
	}
}


static scr_coord_val get_img_width(image_id img)
{
	return img != IMG_EMPTY ? images[ img ].w : 0;
}


static scr_coord_val get_img_height(image_id img)
{
	return img != IMG_EMPTY ? images[ img ].h : 0;
}

/**
 * Base method to display a 3x3 array of images to fit the scr_rect.
 * Special cases:
 * - if images[*][1] are empty, display images[*][0] vertically aligned
 * - if images[1][*] are empty, display images[0][*] horizontally aligned
 */
void display_img_stretch(const stretch_map_t &imag, scr_rect area)
{
	scr_coord_val h_top    = max(max( get_img_height(imag[0][0]), get_img_height(imag[1][0])), get_img_height(imag[2][0]));
	scr_coord_val h_middle = max(max( get_img_height(imag[0][1]), get_img_height(imag[1][1])), get_img_height(imag[2][1]));
	scr_coord_val h_bottom = max(max( get_img_height(imag[0][2]), get_img_height(imag[1][2])), get_img_height(imag[2][2]));

	// center vertically if images[*][1] are empty, display images[*][0]
	if(  imag[0][1] == IMG_EMPTY  &&  imag[1][1] == IMG_EMPTY  &&  imag[2][1] == IMG_EMPTY  ) {
		scr_coord_val h = max(h_top, get_img_height(imag[1][1]));
		// center vertically
		area.y += (area.h-h)/2;
	}

	// center horizontally if images[1][*] are empty, display images[0][*]
	if(  imag[1][0] == IMG_EMPTY  &&  imag[1][1] == IMG_EMPTY  &&  imag[1][2] == IMG_EMPTY  ) {
		scr_coord_val w_left = max(max( get_img_width(imag[0][0]), get_img_width(imag[0][1])), get_img_width(imag[0][2]));
		// center vertically
		area.x += (area.w-w_left)/2;
	}

	// top row
	display_three_image_row( imag[0][0], imag[1][0], imag[2][0], area);

	// bottom row
	if(  h_bottom > 0  ) {
		scr_rect row( area.x, area.y+area.h-h_bottom, area.w, h_bottom );
		display_three_image_row( imag[0][2], imag[1][2], imag[2][2], row);
	}

	// now stretch the middle
	if(  h_middle > 0  ) {
		scr_rect row( area.x, area.y+h_top, area.w, area.h-h_top-h_bottom);
		// tile it wide
		while(  h_middle <= row.h  ) {
			display_three_image_row( imag[0][1], imag[1][1], imag[2][1], row);
			row.y += h_middle;
			row.h -= h_middle;
		}
		// for the rest we have to clip the rectangle
		if(  row.h > 0  ) {
			clip_dimension const cl = display_get_clip_wh();
			display_set_clip_wh( cl.x, cl.y, cl.w, max(0,min(row.get_bottom(),cl.yy)-cl.y) );
			display_three_image_row( imag[0][1], imag[1][1], imag[2][1], row);
			display_set_clip_wh(cl.x, cl.y, cl.w, cl.h );
		}
	}
}


void display_rezoomed_img_blend(const image_id id, scr_coord_val x, scr_coord_val y, const sint8 pn, rgba_t color, const bool, const bool  CLIP_NUM_DEF_NOUSE)
{
    display_set_color(color);
    display_img(id, x, y, pn);
}


void display_img_alpha(const image_id image, const image_id alpha_map, scr_coord_val xp, scr_coord_val yp)
{
    display_set_color(rgba_t(1, 1, 1, 1));

    glBlendFunc(GL_SRC_ALPHA, GL_SRC_ALPHA);
    display_normal(alpha_map, xp, yp, 0);

    glBlendFunc(GL_ONE_MINUS_DST_ALPHA, GL_DST_ALPHA);
    display_normal(image, xp, yp, 0);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    display_set_color(display_get_day_night_color());
}


void display_base_img_blend(const image_id, scr_coord_val, scr_coord_val, const sint8, rgba_t, const bool, const bool  CLIP_NUM_DEF_NOUSE)
{
}


void display_base_img_alpha(const image_id, const image_id, const unsigned, scr_coord_val, scr_coord_val, const sint8, rgba_t, const bool, bool  CLIP_NUM_DEF_NOUSE)
{
}

// variables for storing currently used image procedure set and tile raster width
display_image_proc display_normal = display_img;
display_image_proc display_color = display_img;
display_blend_proc display_blend = display_rezoomed_img_blend;


rgba_t display_blend_colors(rgba_t c1, rgba_t c2, float alpha)
{
    // dbg->message("display_blend_colors()", "alpha=%f", alpha);

	return rgba_t(
        c1.red * (1-alpha) + c2.red * alpha,
        c1.green * (1-alpha) + c2.green * alpha,
        c1.blue * (1-alpha) + c2.blue * alpha,
        1.0f
	);
}


void display_fillbox_wh(scr_coord_val x, scr_coord_val y, scr_coord_val w, scr_coord_val h)
{
	glEnable(GL_SCISSOR_TEST);

	gl_texture_t::bind(0);

	glBegin(GL_QUADS);

	glVertex2i(x, y);
	glVertex2i(x+w, y);
	glVertex2i(x+w, y+h);
	glVertex2i(x, y+h);

	glEnd();

	glDisable(GL_SCISSOR_TEST);
}


void display_fillbox_wh_rgb(scr_coord_val x, scr_coord_val y, scr_coord_val w, scr_coord_val h, rgba_t color, bool )
{
    // dbg->message("display_fillbox_wh_rgb()", "Called %d,%d,%d,%d color %f,%f,%f,%f", x, y, w, h, color.red, color.green, color.blue, color.alpha);

	gl_texture_t::bind(0);

	glBegin(GL_QUADS);

	glColor4f(color.red, color.green, color.blue, color.alpha);

	glVertex2i(x, y);
	glVertex2i(x+w, y);
	glVertex2i(x+w, y+h);
	glVertex2i(x, y+h);

	glEnd();
}


void display_fillbox_wh_clip_rgb(scr_coord_val x, scr_coord_val y, scr_coord_val w, scr_coord_val h, rgba_t color, bool  CLIP_NUM_DEF_NOUSE)
{
    // dbg->message("display_fillbox_wh_clip_rgb()", "Called %d,%d,%d,%d color %f,%f,%f,%f", x, y, w, h, color.red, color.green, color.blue, color.alpha);

	glEnable(GL_SCISSOR_TEST);

	gl_texture_t::bind(0);

	glBegin(GL_QUADS);

	glColor4f(color.red, color.green, color.blue, color.alpha);

	glVertex2i(x, y);
	glVertex2i(x+w, y);
	glVertex2i(x+w, y+h);
	glVertex2i(x, y+h);

	glEnd();

	glDisable(GL_SCISSOR_TEST);
}


void display_vline_wh_clip_rgb(scr_coord_val x, scr_coord_val y, scr_coord_val h, rgba_t color, bool dirty CLIP_NUM_DEF_NOUSE)
{
  display_fillbox_wh_clip_rgb(x, y, 1, h, color, dirty);
}


void display_array_wh(scr_coord_val x, scr_coord_val y, scr_coord_val w, scr_coord_val h, const rgb888_t * pixels)
{
    display_array_tex_buffer->update_region(0, 0, w, h, pixels);

    display_tile_from_sheet(display_array_tex_buffer, x, y, w, h,
                            0, 0, w, h);
}


int display_glyph(scr_coord_val x, scr_coord_val y, utf32 c, const font_t * font)
{
    int advance = 1;

    if(font->is_loaded())
    {
        const scr_coord_val w = font->get_glyph_width(c);
        const scr_coord_val h = font->get_glyph_height(c);

        // Pixel coordinates of the glyph in the sheet
        const int gnr = font->glyphs[c].sheet_index;
        const int glyph_x = (gnr & 31) * 32;
        const int glyph_y = (gnr / 32) * 32;

        const int gy = y + font->get_glyph_top(c);

        display_tile_from_sheet(font->glyph_sheet, x, gy, w, h, glyph_x, glyph_y, w, h);
        advance = font->get_glyph_advance(c);
    }

    return advance;
}


void display_ddd_box_rgb(scr_coord_val, scr_coord_val, scr_coord_val, scr_coord_val, rgba_t, rgba_t, bool)
{
}


void display_ddd_box_clip_rgb(scr_coord_val, scr_coord_val, scr_coord_val, scr_coord_val, rgba_t, rgba_t)
{
}


void display_flush_framebuffer_to_buffer()
{
    int n = zoom_num[zoom_level];
    int d = zoom_den[zoom_level];
    
    /*
    float x = 0; 
    float y = (gl_framebuffer_size - (display_height *d / n)) / (float)gl_framebuffer_size;
    float w = (display_width * d / n)  / (float)gl_max_texture_size;
    float h = 1;
    */
    float x = 0; 
    float y = 0;
    float w = (display_width * d / n)  / (float)gl_max_texture_size;
    float h = (display_height * d / n) / (float)gl_framebuffer_size;
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    framebuffer_active = false;

    // framebuffer viewport
    simgraph_resize(scr_size(display_width, display_height));
    
    gl_texture_t::bind(gl_texture_colorbuffer);
    glColor4f(1, 1, 1, 1);

	glBegin(GL_QUADS);

    glTexCoord2f(x, y);
	glVertex2i(0, 0);

    glTexCoord2f(w, y);
	glVertex2i(display_width, 0);

    glTexCoord2f(w, h);
	glVertex2i(display_width, display_height);

    glTexCoord2f(x, h);
	glVertex2i(0, display_height);

	glEnd();    

}


void display_flush_buffer()
{
    // dbg->debug("display_flush_buffer()", "Called");
    
	glfwSwapBuffers(window);

    
    // next will be map drawing again, so set the map buffer
    glBindFramebuffer(GL_FRAMEBUFFER, gl_fbo);          
    framebuffer_active = true;

    // framebuffer viewport
    scr_size size(gl_framebuffer_size, gl_framebuffer_size);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, size.w, 0, size.h, 1, -1);
    glViewport(0, 0, size.w, size.h);
    display_set_clip_wh(0, 0, size.w, size.h);
    
	glClear(GL_COLOR_BUFFER_BIT);
}


void display_show_pointer(int v)
{
	dbg->message("display_show_pointer()", "%d", v);
}


void display_set_pointer(int)
{
}


void display_show_load_pointer(int v)
{
	dbg->message("display_show_load_pointer()", "%d", v);
}


// callback refs
void sysgl_cursor_pos_callback(GLFWwindow *window, double x, double y);
void sysgl_mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
void sysgl_scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void sysgl_character_callback(GLFWwindow* window, unsigned int codepoint);
void sysgl_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
void sysgl_window_close_callback(GLFWwindow* window);
void sysgl_framebuffer_size_callback(GLFWwindow* window, int width, int height);


bool simgraph_init(scr_size size, sint16)
{
	// glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	// glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
	// glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);

	window = glfwCreateWindow(size.w, size.h, "Simutrans GL v0.04", NULL, NULL);

	dbg->message("simgraph_init()", "GLFW %d,%d -> window: %p", size.w, size.h, window);

	if(window)
	{
		glfwMakeContextCurrent(window);
		display_set_clip_wh(0, 0, display_width, display_height, false);

        GLenum err = glewInit();
        if (GLEW_OK != err)
        {
          dbg->message("simgraph_init()", "GLEW Error: %s\n", glewGetErrorString(err));
        }
        dbg->message("simgraph_init()", "Status: Using GLEW %s\n", glewGetString(GLEW_VERSION));        
        
        
        int width, height;
        glfwGetWindowSize(window, &width, &height);

        display_width = (scr_coord_val)width;
        display_height = (scr_coord_val)height;

		// enable vsync (1 == next frame)
		glfwSwapInterval(1);

		// event callbacks
        glfwSetCursorPosCallback(window, sysgl_cursor_pos_callback);
        glfwSetMouseButtonCallback(window, sysgl_mouse_button_callback);
		glfwSetScrollCallback(window, sysgl_scroll_callback);
		glfwSetCharCallback(window, sysgl_character_callback);
		glfwSetKeyCallback(window, sysgl_key_callback);
        glfwSetWindowCloseCallback(window, sysgl_window_close_callback);
        glfwSetFramebufferSizeCallback(window, sysgl_framebuffer_size_callback);
        
        // 2D Initialization
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glEnable(GL_TEXTURE_2D);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glDisable(GL_LIGHTING);
        glDisable(GL_DEPTH_TEST);

        simgraph_resize(size);

        glGetIntegerv(GL_MAX_TEXTURE_SIZE, &gl_max_texture_size);

        // some drivers seems to lie here?
        // smaller textures work better
        if(gl_max_texture_size > 4096) gl_max_texture_size = 4096;

        dbg->message("simgraph_init()", "GLFW max texture size is %d", gl_max_texture_size);
        
        // set up the frame buffer
        glGenFramebuffers(1, &gl_fbo);        
        glBindFramebuffer(GL_FRAMEBUFFER, gl_fbo);
        framebuffer_active = true;
        gl_framebuffer_size = gl_max_texture_size;
        
        // generate texture
        glGenTextures(1, &gl_texture_colorbuffer);
        glBindTexture(GL_TEXTURE_2D, gl_texture_colorbuffer);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, gl_framebuffer_size, gl_framebuffer_size, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glBindTexture(GL_TEXTURE_2D, 0);

        // attach it to currently bound framebuffer object
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gl_texture_colorbuffer, 0);        

        int status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        
        if(status != GL_FRAMEBUFFER_COMPLETE)
        {
            dbg->message("simgraph_init()", "Frame buffer status is not complete: %x", status);
        }

        // try to load the font given in the environment and if that fails,
        // try to load the standard font which is bundled with Simutrans GL
       	if(!display_load_font(env_t::fontname.c_str()) &&
	       !display_load_font(FONT_PATH_X "LiberationSans-Regular.ttf") ) {
            dr_fatal_notify("No fonts found!");
            return false;
        }
        
        display_array_tex_buffer = gl_texture_t::create_texture(256, 256, NULL);
	}

	return window;
}


bool is_display_init()
{
	return window != NULL && images != NULL;
}


void display_free_all_images_above(image_id limit)
{
	dbg->message("display_free_all_images_above()", "starting past %d", limit);
}

void simgraph_exit()
{
	glfwDestroyWindow(window);

	dr_os_close();
}


void simgraph_resize(scr_size size)
{
    // dbg->message("simgraph_resize()", "width=%d height=%d", size.w, size.h);

    // sanity check
	if(size.h <= 0) {
		size.h = 64;
	}
    
    // setup open gl projection
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, size.w, size.h, 0, 1, -1);
    glViewport(0, 0, size.w, size.h);
    
    
	// only resize, if internal values are different
	if(display_width != size.w || display_height != size.h) {

        display_width = size.w;
        display_height = size.h;

        display_set_clip_wh(0, 0, display_width, display_height);
	}
}


void display_snapshot()
{
}


void display_direct_line_rgb(const scr_coord_val x, const scr_coord_val y, const scr_coord_val xx, const scr_coord_val yy, rgba_t color)
{
	glEnable(GL_SCISSOR_TEST);

	gl_texture_t::bind(0);

	glBegin(GL_LINES);

	glColor4f(color.red, color.green, color.blue, color.alpha);

	glVertex2i(x, y);
	glVertex2i(xx, yy);

	glEnd();

	glDisable(GL_SCISSOR_TEST);
    
}

void display_direct_line_dotted_rgb(const scr_coord_val x, const scr_coord_val y, const scr_coord_val xx, const scr_coord_val yy, const scr_coord_val draw, const scr_coord_val dontDraw, rgba_t color)
{
    display_set_color(color);

	int i, steps;
	sint64 xp, yp;
	sint64 xs, ys;
	int counter=0;
	bool mustDraw=true;

	const int dx = xx - x;
	const int dy = yy - y;

	steps = (abs(dx) > abs(dy) ? abs(dx) : abs(dy));
	if (steps == 0) {
		steps = 1;
	}

	xs = ((sint64)dx << 16) / steps;
	ys = ((sint64)dy << 16) / steps;

	xp = (sint64)x << 16;
	yp = (sint64)y << 16;

	for(  i = 0;  i <= steps;  i++  ) {
		counter ++;
		if(  mustDraw  ) {
			if(  counter == draw  ) {
				mustDraw = !mustDraw;
				counter = 0;
			}
		}
		if(  !mustDraw  ) {
			if(  counter == dontDraw  ) {
				mustDraw=!mustDraw;
				counter=0;
			}
		}

		if(  mustDraw  ) {
			display_fillbox_wh(xp >> 16, yp >> 16, 1, 1);
		}
		xp += xs;
		yp += ys;
	}    
}

void display_circle_rgb( scr_coord_val, scr_coord_val, int, const rgba_t)
{
}

void display_filled_circle_rgb( scr_coord_val, scr_coord_val, int, const rgba_t)
{
}

void draw_bezier_rgb(scr_coord_val, scr_coord_val, scr_coord_val, scr_coord_val, scr_coord_val, scr_coord_val, scr_coord_val, scr_coord_val, const rgba_t, scr_coord_val, scr_coord_val)
{
}

void display_right_triangle_rgb(scr_coord_val, scr_coord_val, scr_coord_val, const rgba_t, const bool)
{
}

void display_signal_direction_rgb( scr_coord_val, scr_coord_val, uint8, uint8, rgba_t, rgba_t, bool, uint8 )
{
}

void display_img_aligned(const image_id n, scr_rect area, int align, sint8 player_nr, bool dirty)
{
	if(  n < anz_images  ) {
		scr_coord_val x,y;

		// align the image horizontally
		x = area.x;
		if(  (align & ALIGN_RIGHT) == ALIGN_CENTER_H  ) {
			x -= images[n].x;
			x += (area.w-images[n].w)/2;
		}
		else if(  (align & ALIGN_RIGHT) == ALIGN_RIGHT  ) {
			x = area.get_right() - images[n].x - images[n].w;
		}

		// align the image vertically
		y = area.y;
		if(  (align & ALIGN_BOTTOM) == ALIGN_CENTER_V  ) {
			y -= images[n].y;
			y += (area.h-images[n].h)/2;
		}
		else if(  (align & ALIGN_BOTTOM) == ALIGN_BOTTOM  ) {
			y = area.get_bottom() - images[n].y - images[n].h;
		}

		display_base_img(n, x, y, player_nr);
	}
}

void display_proportional_ellipsis_rgb(scr_rect, const char *, int, rgba_t, bool, bool, rgba_t)
{
}

image_id get_image_count()
{
	return anz_images;
}

void add_poly_clip(int, int, int, int, int)
{
}

void clear_all_poly_clip()
{
}

void activate_ribi_clip(int)
{
}
