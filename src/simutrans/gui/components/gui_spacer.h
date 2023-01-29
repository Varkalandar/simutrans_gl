/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef GUI_COMPONENTS_GUI_SPACER_H
#define GUI_COMPONENTS_GUI_SPACER_H


#include "gui_component.h"

/**
 * A fixed size clear space
 */
class gui_spacer_t : public gui_component_t
{
        scr_size fixed_size;
public:
	
	gui_spacer_t(scr_coord xy, scr_size size);

	scr_size get_min_size() const OVERRIDE;

	scr_size get_max_size() const OVERRIDE;

	void draw(scr_coord offset) OVERRIDE;
};

#endif
