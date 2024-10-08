/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "citylist_stats.h"
#include "city_info.h"

#include "../world/simcity.h"
#include "../simevent.h"
#include "../world/simworld.h"
#include "../display/viewport.h"
#include "../tool/simmenu.h"

#include "../utils/cbuffer.h"


citylist_stats_t::citylist_stats_t(stadt_t *c) :
    name_label((gui_theme_t::gui_color_text)),
	population_label((gui_theme_t::gui_color_text))
{
	city = c;
	set_table_layout(4, 0);

	button_t *b = new_component<button_t>();
	b->set_typ(button_t::posbutton_automatic);
	b->set_targetpos(city->get_center());

	name_label.fixed_min_height = gui_theme_t::gui_label_size.h + 8;
	name_label.fixed_min_width = 160;
	add_component(&name_label);
	add_component(&population_label);
	update_label();

	new_component<gui_fill_t>();
}


void citylist_stats_t::update_label()
{
	cbuffer_t &nbuf = name_label.buf();
	nbuf.printf("%s", city->get_name());
	name_label.update();

	cbuffer_t &buf = population_label.buf();
	buf.append( city->get_einwohner(), 0 );
	buf.append( " (" );
	buf.append( city->get_wachstum()/10.0, 1 );
	buf.append( ")" );
	population_label.update();
}


void citylist_stats_t::set_size(scr_size size)
{
	gui_aligned_container_t::set_size(size);
	// label.set_size(scr_size(get_size().w - label.get_pos().x, label.get_size().h));
}


void citylist_stats_t::draw(scr_coord offset)
{
	update_label();

	draw_background(offset);

	gui_aligned_container_t::draw(offset);
}


bool citylist_stats_t::is_valid() const
{
	return world()->get_cities().is_contained(city);
}


bool citylist_stats_t::infowin_event(const event_t *ev)
{
	bool swallowed = gui_aligned_container_t::infowin_event(ev);

	if (!swallowed) {
		// either open dialog or goto (with control or right click)
		if (IS_LEFTRELEASE(ev)) {
			if ((event_get_last_control_shift() ^ tool_t::control_invert) == 2) {
				world()->get_viewport()->change_world_position(city->get_pos());
			}
			else {
				city->open_info_window();
			}
			return true;
		}
		if (IS_RIGHTRELEASE(ev)) {
			world()->get_viewport()->change_world_position(city->get_pos());
			return true;
		}
	}
	return swallowed;
}



citylist_stats_t::sort_mode_t citylist_stats_t::sort_mode = citylist_stats_t::SORT_BY_NAME;
uint8 citylist_stats_t::player_nr = -1;

bool citylist_stats_t::compare(const gui_component_t *aa, const gui_component_t *bb)
{
	bool reverse = citylist_stats_t::sort_mode > citylist_stats_t::SORT_MODES;
	int sort_mode = citylist_stats_t::sort_mode & 0x1F;

	const citylist_stats_t* a = dynamic_cast<const citylist_stats_t*>(aa);
	const citylist_stats_t* b = dynamic_cast<const citylist_stats_t*>(bb);
	// good luck with mixed lists
	assert(a != NULL  &&  b != NULL);

	if(  reverse  ) {
		std::swap(a,b);
	}

	if(  sort_mode != SORT_BY_NAME  ) {
		switch(  sort_mode  ) {
			case SORT_BY_NAME: // default
				break;
			case SORT_BY_SIZE:
				return a->city->get_einwohner() < b->city->get_einwohner();
			case SORT_BY_GROWTH:
				return a->city->get_wachstum() < b->city->get_wachstum();
			default: break;
		}
		// default sorting ...
	}

	// first: try to sort by number
	const char *atxt =a->get_text();
	int aint = 0;
	// isdigit produces with UTF8 assertions ...
	if(  atxt[0]>='0'  &&  atxt[0]<='9'  ) {
		aint = atoi( atxt );
	}
	else if(  atxt[0]=='('  &&  atxt[1]>='0'  &&  atxt[1]<='9'  ) {
		aint = atoi( atxt+1 );
	}
	const char *btxt = b->get_text();
	int bint = 0;
	if(  btxt[0]>='0'  &&  btxt[0]<='9'  ) {
		bint = atoi( btxt );
	}
	else if(  btxt[0]=='('  &&  btxt[1]>='0'  &&  btxt[1]<='9'  ) {
		bint = atoi( btxt+1 );
	}
	if(  aint!=bint  ) {
		return (aint-bint)<0;
	}
	// otherwise: sort by name
	return strcmp(atxt, btxt)<0;
}
