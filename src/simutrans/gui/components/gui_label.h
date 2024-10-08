/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef GUI_COMPONENTS_GUI_LABEL_H
#define GUI_COMPONENTS_GUI_LABEL_H


#include "gui_component.h"
#include "../../display/display.h"
#include "../gui_theme.h"
#include "../../utils/cbuffer.h"


/**
 * The label component
 * just displays a text, will be auto-translated
 */
class gui_label_t : virtual public gui_component_t
{
public:
	enum align_t {
		left,
		centered,
		right,
		money_right
	};

private:
	align_t align;

	/**
	 * Color of the label text
	 */
	rgba_t color;

	bool shadowed;
	rgba_t color_shadow;

	bool draw_background;
	rgba_t color_background;


	const char * text; // only for direct access of non-translatable things. Do not use!
	const char * tooltip;
    font_size_t font_size;

protected:
	using gui_component_t::init;

public:
	gui_label_t(const char* text=0, rgba_t color=gui_theme_t::gui_color_text, align_t align=left, font_size_t size=FS_NORMAL);

	void init(const char* text_par, scr_coord pos_par, rgba_t color_par=gui_theme_t::gui_color_text, align_t align_par=left) {
		set_pos  ( pos_par   );
		set_text ( text_par  );
		set_color( color_par );
		set_align( align_par );
	}

        scr_coord_val fixed_min_width = 0;
        scr_coord_val fixed_min_height = 0;

	/**
	 * Sets the text to display, after translating it.
	 */
	void set_text(const char *text, bool autosize=true);

	/**
	 * Sets the text without translation.
	 */
	void set_text_pointer(const char *text, bool autosize=true);

	/**
	 * returns the pointer (i.e. for freeing untranslated contents)
	 */
	const char * get_text_pointer() const { return text; }

	/**
	 * returns the tooltip pointer (i.e. for freeing untranslated contents)
	 */
	const char * get_tooltip_pointer() { return tooltip; }

	/**
	 * Draws the component.
	 */
	void draw(scr_coord offset) OVERRIDE;

	/**
	 * Sets the colour of the label
	 */
	void set_color(rgba_t color) { this->color = color; }
	virtual rgba_t get_color() const { return color; }

	/**
	 * Toggles shadow and sets shadow color.
	 */
	void set_shadow(rgba_t color_shadow, bool shadowed)
	{
		this->color_shadow = color_shadow;
		this->shadowed = shadowed;
	}

	/**
	 * Toggles shadow and sets shadow color.
	 */
	void set_draw_background(rgba_t color_background, bool yesno)
	{
		this->color_background = color_background;
		this->draw_background = yesno;
	}

	/**
	 * Sets the alignment of the label
	 */
	void set_align(align_t align) { this->align = align; }

	/**
	 * Sets the tooltip of this component.
	 */
	void set_tooltip(const char * t);

	scr_size get_min_size() const OVERRIDE;

	scr_size get_max_size() const OVERRIDE;
};

/**
 * Label with own buffer.
 */
class gui_label_buf_t : public gui_label_t
{
	bool buf_changed;
	cbuffer_t buffer_write, buffer_read;
	scr_coord_val min_width = 0;

public:
	gui_label_buf_t(rgba_t color=gui_theme_t::gui_color_text, align_t align=left) : gui_label_t(NULL, color, align), buf_changed(true) { }

	void init(rgba_t color_par, align_t align_par=left);

	/**
	 * Has to be called after access to buf() is finished.
	 * Otherwise size calculations will be off.
	 * Called by @ref draw.
	 */
	void update();

	cbuffer_t& buf()
	{
		if (!buf_changed) {
			buffer_write.clear();
		}
		buf_changed = true;
		return buffer_write;
	}

	/**
	 * appends money string to write buf, sets color
	 */
	void append_money(double money);

	void draw(scr_coord offset) OVERRIDE;

	void set_min_width(scr_coord_val w);

	scr_size get_min_size() const OVERRIDE;

protected:
	using gui_label_t::get_text_pointer;
	using gui_label_t::set_text;
	using gui_label_t::set_text_pointer;
};

#endif
