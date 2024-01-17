#include "../../tpl/slist_tpl.h"
#include "../../simsound.h"
#include "../../descriptor/sound_desc.h"
#include "../gui_theme.h"
#include "gui_action_creator.h"


gui_action_creator_t::gui_action_creator_t()
{
    listeners = new slist_tpl <action_listener_t *> ();
}


gui_action_creator_t::~gui_action_creator_t()
{
    delete listeners;
    listeners = 0;
}


/**
 * Add a new listener to this text input field.
 */
void gui_action_creator_t::add_listener(action_listener_t * l) 
{
    listeners->insert(l); 
}


/**
 * Inform all listeners that an action was triggered.
 * @param v The value to pass to the listeners
 * @param sound Pass -1 for no sound, sound index otherwise
 */
void gui_action_creator_t::call_listeners(value_t v, int sound)
{
    if(sound >= 0) {
        sound_play(sound, 255, UI_SOUND);
    }
    
    for(action_listener_t * const l : *listeners) {
        if (l->action_triggered(this, v)) break;
    }
}
