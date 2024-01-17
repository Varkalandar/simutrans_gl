/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef GUI_COMPONENTS_GUI_ACTION_CREATOR_H
#define GUI_COMPONENTS_GUI_ACTION_CREATOR_H

#include "action_listener.h"

template <class T> class slist_tpl;

/**
 * This interface must be implemented by all classes which want to
 * send actions, i.e. button presses
 */
class gui_action_creator_t
{
protected:
	/**
	 * Our listeners.
	 */
	slist_tpl <action_listener_t *> * listeners;

	/**
	 * Inform all listeners that an action was triggered.
         * @param v The value to pass to the listeners
         * @param sound Pass -1 for no sound, sound index otherwise
	 */
	void call_listeners(value_t v, int sound);

public:
        gui_action_creator_t();
        virtual ~gui_action_creator_t();
    
        /**
	 * Add a new listener to this text input field.
	 */
	void add_listener(action_listener_t * l);
};

#endif
