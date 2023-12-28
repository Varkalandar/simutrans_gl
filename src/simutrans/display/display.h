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
#include "alignment.h"
#include "rgba.h"


enum font_size_t
{
    FS_NORMAL,
    FS_HEADLINE,
};


/**
 * Draw a bevel box.
 *
 * @param area The box rectangle
 * @param top Top line color
 * @param left Left line color
 * @param right Right line color
 * @param bottom Bottom line color
 */
void display_bevel_box(scr_rect area,
                       rgba_t top, rgba_t left, rgba_t right, rgba_t bottom);


int display_text_proportional_len_clip_rgb(scr_coord_val x, scr_coord_val y, const char* txt, control_alignment_t flags, rgba_t color, bool dirty, sint32 len, sint32 spacing, font_size_t size);
/* macro are for compatibility */
#define display_proportional_rgb(               x, y, txt, align, color, dirty)       display_text_proportional_len_clip_rgb( x, y, txt, align,           color, dirty, -1, 0, FS_NORMAL )
#define display_proportional_clip_rgb(          x, y, txt, align, color, dirty)       display_text_proportional_len_clip_rgb( x, y, txt, align | DT_CLIP, color, dirty, -1, 0, FS_NORMAL )


/// Display a string that is abbreviated by the (language specific) ellipsis character if too wide
/// If enough space is given, it just display the full string
void display_proportional_ellipsis_rgb(scr_rect r, const char *text, int align, rgba_t color, const bool dirty, bool shadowed = false, rgba_t shadow_color = RGBA_BLACK, font_size_t size = FS_NORMAL);


/**
 * display text in 3d box with clipping
 */
void display_ddd_proportional_clip(scr_coord_val xpos, scr_coord_val ypos, rgba_t ddd_color, rgba_t text_color, const char *text, int dirty);


int display_multiline_text_rgb(scr_coord_val x, scr_coord_val y, const char *inbuf, rgba_t color);

void display_outline_proportional_rgb(scr_coord_val xpos, scr_coord_val ypos, rgba_t text_color, rgba_t shadow_color, const char *text, int dirty, sint32 len=-1);
void display_shadow_proportional_rgb(scr_coord_val xpos, scr_coord_val ypos, rgba_t text_color, rgba_t shadow_color, const char *text, int dirty, sint32 len=-1);
int display_text_bold(scr_coord_val xpos, scr_coord_val ypos, rgba_t color, const char *text, int dirty, sint32 len=-1, font_size_t size=FS_NORMAL);


/**
 * Loads the font, returns the number of characters in it
 * @param reload if true forces reload
 */
bool display_load_font(const char *fname, bool reload = false);


/*
 * @return true, if this is a valid character
 */
bool has_character(utf32 char_code);


scr_coord_val display_get_char_width(utf32 c);


/**
 * Returns the width of the widest character in a string.
 * @param text  pointer to a string of characters to evaluate.
 * @param len   length of text buffer to evaluate. If set to 0,
 *              evaluate until null termination.
 */
scr_coord_val display_get_char_max_width(const char* text, size_t len=0);


/**
 * For the next logical character in the text, returns the character code
 * as well as retrieves the char byte count and the screen pixel width
 * CAUTION : The text pointer advances to point to the next logical character
 */
utf32 get_next_char_with_metrics(const char* &text, unsigned char &byte_length, unsigned char &pixel_width);

/**
 * For the previous logical character in the text, returns the character code
 * as well as retrieves the char byte count and the screen pixel width
 * CAUTION : The text pointer recedes to point to the previous logical character
 */
utf32 get_prev_char_with_metrics(const char* &text, const char *const text_start, unsigned char &byte_length, unsigned char &pixel_width);

/*
 * returns the index of the last character that would fit within the width
 * If an ellipsis len is given, it will only return the last character up to this len if the full length cannot be fitted
 * @returns index of next character. if text[index]==0 the whole string fits
 */
size_t display_fit_proportional( const char *text, scr_coord_val max_width, scr_coord_val ellipsis_width=0 );

/* routines for string len (macros for compatibility with old calls) */
#define proportional_string_width(text)          display_calc_proportional_string_len_width(text, 0x7FFF, 0, FS_NORMAL)
#define proportional_string_len_width(text, len) display_calc_proportional_string_len_width(text, len, 0, FS_NORMAL)

/**
 * Proportional string width in pixels with a text of a given length
 * extended for universal font routines with unicode support
 */
int display_calc_proportional_string_len_width(const char* text, size_t len, int spacing, font_size_t size);

/**
 * display_calc_proportional_multiline_string_len_width
 * calculates the width and height of a box containing the text inside
 */
void display_calc_proportional_multiline_string_len_width(int &xw, int &yh, const char *text);

/**
 * Get the height of the specified font in pixels,
 *
 * @param size FS_NORMAL or FS_HEADLINE
 * @return the line height for this font
 */
sint16 get_font_height(font_size_t size);

#endif /* SIM_DISPLAY_H */

