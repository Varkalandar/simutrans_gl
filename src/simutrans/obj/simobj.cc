/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "simobj.h"

#include "baum.h"

#include "../ground/grund.h"
#include "../dataobj/loadsave.h"
#include "../dataobj/translator.h"
#include "../display/simgraph.h"
#include "../display/simimg.h"
#include "../display/viewport.h"
#include "../display/illumination_data.h"
#include "../player/simplay.h"
#include "../gui/obj_info.h"
#include "../gui/simwin.h"
#include "../vehicle/vehicle_base.h"
#include "../simcolor.h"
#include "../simskin.h"
#include "../simdebug.h"
#include "../world/simworld.h"
#include "../utils/cbuffer.h"
#include "../utils/simstring.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>


/**
 * Pointer to the world of this thing. Static to conserve space.
 * Change to instance variable once more than one world is available.
 */
karte_ptr_t obj_t::welt;

bool obj_t::show_owner = false;


void obj_t::init()
{
	pos = koord3d::invalid;

	xoff = 0;
	yoff = 0;

	owner_n = PLAYER_UNOWNED;

	flags = no_flags;
	set_flag(dirty);
}


obj_t::obj_t()
{
	init();
}

obj_t::obj_t(koord3d pos)
{
	init();
	this->pos = pos;
}


// removes an object and tries to delete it also from the corresponding objlist
obj_t::~obj_t()
{
	destroy_win((ptrdiff_t)this);

	if(flags&not_on_map  ||  !welt->is_within_limits(pos.get_2d())) {
		return;
	}

	// find object on the map and remove it
	grund_t *gr = welt->lookup(pos);
	if(!gr  ||  !gr->obj_remove(this)) {
		// not found? => try harder at all map locations
		dbg->warning("obj_t::~obj_t()", "Could not remove %p from (%s)", (void *)this, pos.get_str());

		// first: try different height ...
		gr = welt->access(pos.get_2d())->get_boden_von_obj(this);
		if(gr  &&  gr->obj_remove(this)) {
			dbg->warning("obj_t::~obj_t()",
				"Removed %p from (%hi,%hi,%hhi), but it should have been on (%hi,%hi,%hhi)",
				(void *)this,
				gr->get_pos().x, gr->get_pos().y, gr->get_pos().z,
				pos.x, pos.y, pos.z);
			return;
		}

		// then search entire map
		koord k;
		for(k.y=0; k.y<welt->get_size().y; k.y++) {
			for(k.x=0; k.x<welt->get_size().x; k.x++) {
				grund_t *gr = welt->access(k)->get_boden_von_obj(this);
				if (gr && gr->obj_remove(this)) {
					dbg->warning("obj_t::~obj_t()",
						"Removed %p from (%hi,%hi,%hhi), but it should have been on (%hi,%hi,%hhi)",
						(void *)this,
						gr->get_pos().x, gr->get_pos().y, gr->get_pos().z,
						pos.x, pos.y, pos.z);
					return;
				}
			}
		}
	}
}


/**
 * sets owner of object
 */
void obj_t::set_owner(player_t *player)
{
	int i = welt->sp2num(player);
	assert(i>=0);
	owner_n = (uint8)i;
}


player_t *obj_t::get_owner() const
{
	return welt->get_player(owner_n);
}


/* the only general info we can give is the name
 * we want to format it nicely,
 * with two linebreaks at the end => thus the little extra effort
 */
void obj_t::info(cbuffer_t & buf) const
{
	char              translation[256];
	char const* const owner =
		owner_n == 1              ? translator::translate("Eigenbesitz\n")   :
		owner_n == PLAYER_UNOWNED ? "" : // was translator::translate("Kein Besitzer\n") :
		get_owner()->get_name();
	tstrncpy(translation, owner, lengthof(translation));
	// remove trailing linebreaks etc.
	rtrim(translation);
	buf.append( translation );
	// only append linebreaks if not empty
	if(owner[0]>0) {
		buf.append( "\n\n" );
	}
}


void obj_t::show_info()
{
	create_win( new obj_infowin_t(this), w_info, (ptrdiff_t)this);
}

bool obj_t::has_managed_lifecycle() const {
	return false;
}

// returns NULL, if removal is allowed
const char *obj_t::get_removal_error(const player_t *player)
{
	if(owner_n==PLAYER_UNOWNED  ||  welt->get_player(owner_n) == player  ||  welt->get_public_player() == player) {
		return NULL;
	}
	else {
		return "Der Besitzer erlaubt das Entfernen nicht";
	}
}


void obj_t::rdwr(loadsave_t *file)
{
	xml_tag_t d( file, "obj_t" );
	if(  file->is_version_less(101, 0)  ) {
		pos.rdwr( file );
	}

	sint8 byte = (sint8)(((sint16)16*(sint16)xoff)/OBJECT_OFFSET_STEPS);
	file->rdwr_byte(byte);
	xoff = (sint8)(((sint16)byte*OBJECT_OFFSET_STEPS)/16);
	byte = (sint8)(((sint16)16*(sint16)yoff)/OBJECT_OFFSET_STEPS);
	file->rdwr_byte(byte);
	yoff = (sint8)(((sint16)byte*OBJECT_OFFSET_STEPS)/16);
	byte = owner_n;
	file->rdwr_byte(byte);
	owner_n = byte;
}


static void draw_illumination(const illumination_data_t * light, scr_coord_val xpos, scr_coord_val ypos)
{
	const skin_desc_t * sd = skinverwaltung_t::get_light(light->light_id);
	image_id id = sd->get_image_id(light->light_index);

	if(sd) {
		display_set_color(light->light_color);
		display_light_img(id, xpos + light->area.x, ypos + light->area.y, light->area.w, light->area.h);
		display_set_color(display_get_day_night_color());
	}
	else {
		dbg->warning("obj_t::display_after", "there is no light for key %s", light->light_id);
	}
}


/**
 * draw the object
 * the dirty-flag is reset from objlist_t::display_obj_fg, or objlist_t::display_overlay when multithreaded
 */
void obj_t::display(int xpos, int ypos  CLIP_NUM_DEF) const
{
	image_id image = get_image();
	image_id const outline_image = get_outline_image();
	if(  image!=IMG_EMPTY  ||  outline_image!=IMG_EMPTY  ) {
		const int raster_width = get_tile_raster_width();

		if (vehicle_base_t const* const v = obj_cast<vehicle_base_t>(this)) {
			// vehicles need finer steps to appear smoother
			v->get_screen_offset( xpos, ypos, raster_width );
		}
		xpos += tile_raster_scale_x(get_xoff(), raster_width);
		ypos += tile_raster_scale_y(get_yoff(), raster_width);
        
		const int start_ypos = ypos;
		for(  int j=0;  image!=IMG_EMPTY;  ) {

			if(  owner_n != PLAYER_UNOWNED  ) {
				if(  obj_t::show_owner  ) {
                    display_set_color(color_idx_to_rgb(welt->get_player(owner_n)->get_player_color1()+2));
					display_color_img(image, xpos, ypos, owner_n);
                    display_set_color(display_get_day_night_color());
				}
				else {
					display_color(image, xpos, ypos, owner_n);
				}
			}
			else {
				display_normal(image, xpos, ypos, 0);
			}

			// this obj has another image on top (e.g. skyscraper)
			ypos -= raster_width;
			image = get_image(++j);
		}

		if(  outline_image != IMG_EMPTY  ) {
			// transparency?
			const rgba_t transparent = get_outline_colour();
			if(transparent.alpha < 1.0) {
				// only transparent outline
                display_set_color(transparent);
				display_color_img(get_outline_image(), xpos, start_ypos, owner_n);
			}
			else if(  obj_t::get_flag( highlight )  ) {
				// highlight this tile
                display_set_color(SYSCOL_OBJECT_HIGHLIGHT);
				display_color_img(get_image(), xpos, start_ypos, owner_n);
			}
		}
		else if(  obj_t::get_flag( highlight )  ) {
			// highlight this tile
            display_set_color(SYSCOL_OBJECT_HIGHLIGHT);
			display_color_img(get_image(), xpos, start_ypos, owner_n);
		}
        
        display_set_color(display_get_day_night_color());
	}
}


// called during map rotation
void obj_t::rotate90()
{
	// most basic: rotate coordinate
	pos.rotate90( welt->get_size().y-1 );
	sint8 new_dx = -2*yoff;
	yoff = xoff/2;
	xoff = new_dx;
}


#ifdef MULTI_THREAD
void obj_t::display_after(int xpos, int ypos, const sint8 clip_num) const
#else
void obj_t::display_after(int xpos, int ypos, bool) const
#endif
{
	image_id image = get_front_image();
	if(  image != IMG_EMPTY  ) {
		const int raster_width = get_tile_raster_width();

		xpos += tile_raster_scale_x( get_xoff(), raster_width );
		ypos += tile_raster_scale_y( get_yoff(), raster_width );

		// lights inside a building
		const illumination_data_t * light_inside = get_light_inside(); 
		if(light_inside) {
			draw_illumination(light_inside, xpos, ypos);
		}

		if(  owner_n != PLAYER_UNOWNED  ) {
			if(  obj_t::show_owner  ) {
                display_set_color(color_idx_to_rgb(welt->get_player(owner_n)->get_player_color1()+2));
				display_color_img(image, xpos, ypos, owner_n);
	            display_set_color(display_get_day_night_color());
        	}
			else if(  obj_t::get_flag( highlight )  ) {
				// highlight this tile
				display_set_color(SYSCOL_OBJECT_HIGHLIGHT);
                display_color_img(image, xpos, ypos, owner_n);
                display_set_color(display_get_day_night_color());
			}
			else {
				display_color_img(image, xpos, ypos, owner_n);
			}
		}
		else if(  obj_t::get_flag( highlight )  ) {
			// highlight this tile
            display_set_color(SYSCOL_OBJECT_HIGHLIGHT);
			display_color_img(image, xpos, ypos, owner_n);
            display_set_color(display_get_day_night_color());
		}
		else {
			display_normal(image, xpos, ypos, 0);
		}
	}

	// printf("%s\n", get_name());
/*
	if(strcmp("new_city_road", get_name()) == 0) {
		const skin_desc_t * sd = skinverwaltung_t::get_light("light_test");
		image_id id = sd->get_image_id(1);

		display_set_color(rgba_t(0.5, 0.5, 0.5, 1.0));
		display_light_img(id, xpos, ypos+16, 64, 48);
		display_set_color(display_get_day_night_color());
	}
*/

	const illumination_data_t * light_above = get_light_above(); 
	if(light_above) {
		draw_illumination(light_above, xpos, ypos);
	}
}
