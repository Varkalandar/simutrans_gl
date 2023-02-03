/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef GUI_PAKSELECTOR_H
#define GUI_PAKSELECTOR_H


#include "savegame_frame.h"
#include "components/gui_textarea.h"
#include "../utils/cbuffer.h"

class pakselector_install_action_t : public action_listener_t
{
	bool action_triggered(gui_action_creator_t*, value_t) OVERRIDE;
};

class pakselector_t : public savegame_frame_t
{
private:
        void load_pak_logos(slist_tpl<dir_entry_t> &entries);
        
        slist_tpl <const skin_desc_t*> pak_logos;
        
protected:
	cbuffer_t      notice_buffer;
	gui_textarea_t notice_label;
	button_t       installbutton;

	pakselector_install_action_t ps;

	const char *get_info    ( const char *filename ) OVERRIDE;
	bool        check_file  ( const char *filename, const char *suffix ) OVERRIDE;
	bool        item_action ( const char *fullpath ) OVERRIDE;
	bool        del_action  ( const char *fullpath ) OVERRIDE;
	void        fill_list   ( void ) OVERRIDE;

public:
	pakselector_t();

	const char *get_help_filename ( void ) const OVERRIDE { return ""; }
	bool        has_title         ( void ) const OVERRIDE { return false; }
	bool        has_pak           ( void ) const          { return !entries.empty(); }

	/**
	 * Events are notified to GUI components via this method.
	 */
	bool        infowin_event(const event_t *ev) OVERRIDE;
        
	void        draw(scr_coord pos, scr_size size);
};

#endif
