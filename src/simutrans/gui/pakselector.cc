/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "pakselector.h"
#include "pakinstaller.h"
#include "components/gui_spacer.h"
#include "../dataobj/translator.h"
#include "../dataobj/environment.h"
#include "../dataobj/pakset_manager.h"
#include "../sys/simsys.h"
#include "../pathes.h"
#include "../descriptor/skin_desc.h"
#include "../display/display.h"


class pak_set_panel_t : public gui_component_t
{
	private:
		slist_tpl<savegame_frame_t::dir_entry_t> * entries = 0;
        slist_tpl <const skin_desc_t*> pak_logos;
		pakselector_t * selector = 0;

	public:

		void set_selector(pakselector_t * selector) { this->selector = selector; }

        void load_pak_logos(slist_tpl<savegame_frame_t::dir_entry_t> *entries);

		void set_entries(slist_tpl<savegame_frame_t::dir_entry_t> * entries);

		scr_size get_min_size() const { return size; }
		scr_size get_min_scroll_size() { return size; }

		bool infowin_event(const event_t *ev) OVERRIDE;
		void draw_logo(const scr_coord_val xpos, const scr_coord_val ypos,
	                   const int index, const savegame_frame_t::dir_entry_t &entry);
		void draw(scr_coord offset) OVERRIDE;
};


void pak_set_panel_t::load_pak_logos(slist_tpl<savegame_frame_t::dir_entry_t> *entries)
{
	// Hajo: testing - load logos from several paks

	for(savegame_frame_t::dir_entry_t &i : *entries) {
		if (i.type == savegame_frame_t::LI_ENTRY) {

			dbg->message("load_pak_logos", "Checking pak path %s", i.info);

			cbuffer_t name;
			name.append(i.info);
			name.append("/");
			name.append("symbol.BigLogo.pak");
			pakset_manager_t::load_pak_file(name.get_str());

			// Hajo: Now we should have a logo in this ...
			const skin_desc_t * logo = skinverwaltung_t::biglogosymbol;

			dbg->message("load_pak_logos", "Big logo is %p", logo);

			pak_logos.append(logo);
		}
	}
}


void pak_set_panel_t::set_entries(slist_tpl<savegame_frame_t::dir_entry_t> * entries)
{
	this->entries = entries;
	load_pak_logos(entries);
}


bool pak_set_panel_t::infowin_event(const event_t *ev)
{
	if(ev->ev_class == event_class_t::EVENT_CLICK ||
	   ev->ev_class == event_class_t::EVENT_RELEASE)
	{
		// Hajo: pass wheel event to the scrollpane,
		// only handle "true" clicks here

		if(ev->ev_code != MOUSE_WHEELUP && ev->ev_code != MOUSE_WHEELDOWN) {

			// so, did we hit something?

			const scr_coord_val xspace = 280;
			const scr_coord_val xstart = (size.w % xspace) / 2;

			const scr_coord_val ystart = 20;
			const scr_coord_val yspace = 256 + 42;

			const int mx = ev->mouse_pos.x;
			const int my = ev->mouse_pos.y;

			const int index = ((my - ystart) / yspace) * (size.w / xspace) + (mx - xstart) / xspace;

			int counter = 0;
			for(savegame_frame_t::dir_entry_t &entry : *entries) {
				if (entry.type == savegame_frame_t::LI_ENTRY) {
					if(counter == index) {

						// Hajo: we must restore the logo before loading
						skinverwaltung_t::biglogosymbol = pak_logos.at(index);

						// Now we can load it ...
						selector->item_action(entry.info);
						return true;
					}

					counter ++;
				}
			}
		}
	}
	return false;
}


void pak_set_panel_t::draw_logo(const scr_coord_val xpos, const scr_coord_val ypos,
	                            const int index, const savegame_frame_t::dir_entry_t &entry)
{
	const skin_desc_t * logo = pak_logos.at(index);
	const rgba_t c = gui_theme_t::gui_highlight_color;

    const image_t *image0 = logo->get_image(0);
    const int w = image0->w;
    const int h = image0->h + image0->y;

	display_bevel_box(scr_rect(xpos-1, ypos-1, 258, 258), c, c, c, c, true);

	// Hajo: Logos are designed for black background
	display_fillbox_wh_clip_rgb(xpos, ypos, 256, 256, RGBA_BLACK, true);

	display_base_img(logo->get_image_id(0), xpos, ypos, 0, false, true);
	display_base_img(logo->get_image_id(1), xpos+w, ypos, 0, false, true);
	display_base_img(logo->get_image_id(2), xpos, ypos+h, 0, false, true);
	display_base_img(logo->get_image_id(3), xpos+w, ypos+h, 0, false, true);

	const char * pak_name = strrchr(entry.info, '/');
	if(pak_name) {
		pak_name ++;
	} else {
		pak_name = entry.info;
	}

	scr_coord_val pak_name_width = display_calc_proportional_string_len_width(pak_name, strlen(pak_name), 0, FS_NORMAL);

	display_proportional_clip_rgb(xpos + (256 - pak_name_width)/2, ypos + 256 + 8, pak_name, ALIGN_LEFT,
		                          gui_theme_t::gui_highlight_color, false);
}


void pak_set_panel_t::draw(scr_coord offset)
{
	const rgba_t color = gui_theme_t::gui_color_chart_background;
	display_fillbox_wh_clip_rgb(offset.x, offset.y, size.w, size.h, color, true);

	const scr_coord_val xspace = 280;
	const scr_coord_val xstart = (size.w % xspace) / 2;

	const scr_coord_val ystart = 20;
	const scr_coord_val yspace = 256 + 42;

	scr_coord_val x = xstart;
	scr_coord_val y = ystart;

	int index = 0;

	for(savegame_frame_t::dir_entry_t &entry : *entries) {
		if (entry.type == savegame_frame_t::LI_ENTRY) {

			const scr_coord_val xpos = offset.x + x;
			const scr_coord_val ypos = offset.y + y;

			draw_logo(xpos, ypos, index, entry);

			x += xspace;

			if(x > display_get_width() - xspace)
			{
				x = xstart;
				y += yspace;
			}

			index ++;
		}
	}
}


static pak_set_panel_t pak_set_panel;


static void setup_pak_panel(pakselector_t * selector, int logo_count)
{
	pak_set_panel.set_selector(selector);
	scr_size size (display_get_width() - 134, 0);

	const scr_coord_val xspace = 280;
	const scr_coord_val xstart = (size.w % xspace) / 2;

	const scr_coord_val ystart = 20;
	const scr_coord_val yspace = 256 + 42;

	int pitch = (size.w-xstart*2) / xspace;
	int rows = (logo_count + pitch - 1) / pitch;

	size.h = ystart + rows * yspace;

	pak_set_panel.set_size(size);
}


pakselector_t::pakselector_t() :
	savegame_frame_t( NULL, true, NULL, true ),
	notice_label(&notice_buffer)
{
    dbg->message("pakselector_t::pakselector_t()", "Creating %p", this);

	// if true, we would call the installer afterwards
	pakinstaller_t::finish_install = false;

	// remove unnecessary buttons
	top_frame.remove_component( &input );
	savebutton.set_visible(false);
	cancelbutton.set_visible(false);
	fnlabel.set_text("", false);

	top_frame.new_component<gui_spacer_t>(scr_coord(0, 0), scr_size(40, 60));

	setup_pak_panel(this, entries.get_count());
	scrolly.set_show_border(true);
	scrolly.set_maximize(false);
	scrolly.set_component(&pak_set_panel);

	// don't show list item labels
	label_enabled = false;

	installbutton.init(button_t::roundbox, " Install a new graphics set ");
	installbutton.add_listener( &ps );
	add_component(&installbutton);

	new_component<gui_divider_t>();

	notice_buffer.printf("%s",
		"To avoid seeing this dialogue define a path by:\n"
		" - adding 'pak_file_path = pak/' to your simuconf.tab\n"
		" - using '-objects pakxyz/' on the command line"
	);
	notice_label.recalc_size();
	add_component(&notice_label);

	add_path( env_t::base_dir );
	if(  strcmp(env_t::base_dir,env_t::install_dir)  ) {
		add_path( env_t::install_dir );
	}
	dr_chdir( env_t::user_dir );
	if(  !dr_chdir(USER_PAK_PATH)  ) {
		char dummy[PATH_MAX];
		dr_getcwd(dummy, lengthof(dummy) - 2);
		strcat(dummy, PATH_SEPARATOR);
		if(  strcmp(env_t::install_dir, dummy)  ) {
			add_path(dummy);
		}
	}
}


/**
 * what to do after loading
 */
bool pakselector_t::item_action(const char *fullpath)
{
	env_t::pak_dir = fullpath;
	env_t::pak_dir += PATH_SEPARATOR;
	env_t::pak_name = (str_get_filename(fullpath, true)+PATH_SEPARATOR);

	// env_t::default_settings.set_with_private_paks( false );
	env_t::default_settings.set_with_private_paks( true );

	return true;
}


bool pakselector_t::del_action(const char *fullpath)
{
	// cannot delete set => use this for selection
	env_t::pak_dir = fullpath;
	env_t::pak_dir += PATH_SEPARATOR;
	env_t::pak_name = str_get_filename(fullpath, true)+PATH_SEPARATOR;
	env_t::default_settings.set_with_private_paks( true );

	return true;
}


const char *pakselector_t::get_info(const char *)
{
	return "";
}


/**
 * This method returns true if filename is what we want and false if not.
 * A PAK directory is considered valid if the file ground.outside.pak exists.
*/
bool pakselector_t::check_file(const char *filename, const char *)
{
	cbuffer_t buf;
	buf.printf("%s/ground.Outside.pak", filename);

	// if we can open the file, it is valid.
	if (FILE* const f = dr_fopen(buf, "r")) {
		fclose(f);
		return true;
	}

	// the file was not found or couldn't be opened.
	return false;
}


void pakselector_t::fill_list()
{
	cbuffer_t path;

	// do the search ...
	savegame_frame_t::fill_list();

	// do not sort or the path names are in the wrong positions
//	entries.sort(dir_entry_t::compare);

	for(dir_entry_t &i : entries) {

		if (i.type == LI_HEADER) {
			continue;
		}

		// look for addon directory
		path.clear();
		path.printf("%saddons/%s", env_t::user_dir, i.button->get_text());

		// reuse delete button as load-with-addons button
		delete i.del;
		i.del = new button_t();
		i.del->init(button_t::roundbox, "Load with addons");

		// if we can't change directory to /addon
		// Hide the addon button
		if(  dr_chdir( path ) != 0  ) {
			i.del->disable();

			// if list contains only one header, one pakset entry without addons
			// store path to pakset temporary, reset later if more choices available
			// if env_t::pak_dir is non-empty then simmain.cc will close the window immediately
			env_t::pak_name = (std::string)i.button->get_text() + PATH_SEPARATOR;
			env_t::pak_dir = (std::string)i.info + PATH_SEPARATOR;
		}
	}
	dr_chdir( env_t::base_dir );

	if(entries.get_count() > this->num_sections+1) {
		// empty path as more than one pakset is present, user has to choose
		env_t::pak_dir.clear();
		env_t::pak_name.clear();
	}

	pak_set_panel.set_entries(&entries);
}


/**
 * This is called after the list has been filled. We can do window resizing here
 */
void pakselector_t::list_filled(void)
{
	savegame_frame_t::list_filled();

	setup_pak_panel(this, entries.get_count());

	const scr_coord_val margin = env_t::iconsize.w; // Hajo: this is also the toolbar height?
	scr_size size (display_get_width()-margin*2, display_get_height()-margin*2);

	set_min_windowsize(size);
	resize(scr_coord(0, 0));
	win_set_pos(this, scr_coord(margin, margin));
}


void pakselector_t::draw(scr_coord pos, scr_size size)
{
    // dbg->message("pakselector_t::draw()", "Called, pos=%d, %d", pos.x, pos.y);

	const rgba_t background = get_system_color({64, 64, 64});
	display_fillbox_wh_rgb(0, 0, display_get_width(), display_get_height(), background, true);

	savegame_frame_t::draw(pos, size);

	// Hajo: fake a top-left bevel border
	display_fillbox_wh_rgb(pos.x, pos.y, size.w, 1, gui_theme_t::gui_highlight_color, true);
	display_vline_wh_clip_rgb(pos.x, pos.y, size.h, gui_theme_t::gui_highlight_color, true);

	const char * title = translator::translate("Please Choose a Graphics Set For Playing");
	const scr_coord_val title_width = display_calc_proportional_string_len_width(title, strlen(title), 0, FS_HEADLINE);

	display_fillbox_wh_rgb(pos.x + (size.w - title_width) / 2 - 40, pos.y + 14, title_width + 80, 46, RGBA_WHITE, true);

	const scr_coord_val fh = get_font_height(FS_HEADLINE);
	display_text_proportional_len_clip_rgb(pos.x + (size.w - title_width) / 2, pos.y + 14 + (46 - fh) / 2,
		                                   title, ALIGN_LEFT, RGBA_BLACK, false, -1, 0, FS_HEADLINE);
}


bool pakselector_install_action_t::action_triggered( gui_action_creator_t*, value_t)
{
	pakinstaller_t::finish_install = true;
	return true;
}
