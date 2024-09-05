/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "halt_list_stats.h"
#include "components/gui_spacer.h"
#include "../simhalt.h"
#include "../simskin.h"
#include "../simcolor.h"
#include "../tool/simtool.h"
#include "../display/simgraph.h"
#include "../display/viewport.h"
#include "../player/simplay.h"
#include "../world/simworld.h"
#include "../display/simimg.h"

#include "../dataobj/translator.h"

#include "../descriptor/skin_desc.h"

#include "../utils/cbuffer.h"
#include "../utils/simstring.h"

#include "gui_frame.h"
#include "halt_info.h" // gui_halt_type_images_t


static karte_ptr_t welt;

/**
 * Events are notified to GUI components via this method
 */
bool halt_list_stats_t::infowin_event(const event_t *ev)
{
	bool swallowed = gui_aligned_container_t::infowin_event(ev);
	if(!swallowed && halt.is_bound()) {

		if(IS_LEFTRELEASE(ev)) {
			if ((event_get_last_control_shift() ^ tool_t::control_invert) == 2) {
				welt->get_viewport()->change_world_position(halt->get_basis_pos3d());
			}
			else {
				halt->open_info_window();
			}

			return true;
		}
		if(IS_RIGHTRELEASE(ev)) {
			welt->get_viewport()->change_world_position(halt->get_basis_pos3d());
			return true;
		}
	}
	return swallowed;
}


halt_list_stats_t::halt_list_stats_t(halthandle_t h)
{
	halt = h;
	set_table_layout(1, 4);
	set_spacing(scr_size(D_H_SPACE, 2));

	new_component<gui_spacer_t>(scr_coord(0, 0), scr_size(10, LINESPACE/2));
	

	add_table(4, 1);
	{
		indicator.fixed_min_height = gui_theme_t::gui_label_size.h-4;
		indicator.set_max_size(scr_size(D_INDICATOR_WIDTH, D_INDICATOR_HEIGHT));
		add_component(&indicator);

		add_component(&label_name);
		label_name.buf().append(halt->get_name());
		label_name.update();

		new_component<gui_spacer_t>(scr_coord(0, 0), scr_size(0, 2));

		img_types = new_component<gui_halt_type_images_t>(halt);
	}
	end_table();

	// second row
	
	add_table(5, 1);
	{
		gotopos.set_typ(button_t::posbutton_automatic);
		gotopos.set_targetpos3d(halt->get_basis_pos3d());
		add_component(&gotopos);
		
		add_component(&img_enabled[0]);
		img_enabled[0].set_image(skinverwaltung_t::passengers->get_image_id(0), true);
		add_component(&img_enabled[1]);
		img_enabled[1].set_image(skinverwaltung_t::mail->get_image_id(0), true);
		add_component(&img_enabled[2]);
		img_enabled[2].set_image(skinverwaltung_t::goods->get_image_id(0), true);

		add_component(&label_cargo);
		halt->get_short_freight_info( label_cargo.buf() );
		label_cargo.update();
	}
	end_table();

	new_component<gui_spacer_t>(scr_coord(0, 0), scr_size(10, LINESPACE/4));	
}


const char* halt_list_stats_t::get_text() const
{
	return halt->get_name();
}


/**
 * Draw the component
 */
void halt_list_stats_t::draw(scr_coord offset)
{
	gotopos.set_targetpos3d(halt->get_basis_pos3d()); // since roation may have changed the target pos

	img_enabled[0].set_visible(halt->get_pax_enabled());
	img_enabled[1].set_visible(halt->get_mail_enabled());
	img_enabled[2].set_visible(halt->get_ware_enabled());

	label_name.buf().append(halt->get_name());
	label_name.update();
	indicator.set_color(halt->get_status_farbe());

	halt->get_short_freight_info( label_cargo.buf() );
	label_cargo.update();

	set_size(get_size());
	
	// draw_background(offset);

	gui_aligned_container_t::draw(offset);
}
