/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifdef _WIN32
#include <windows.h>
#endif

#ifndef _MSC_VER
#include <unistd.h>
#include <sys/time.h>
#else
// need timeGetTime
#include <mmsystem.h>
#endif

#include <signal.h>

#include "simsys.h"
#include "../simversion.h"
#include "../simintr.h"
#include "../simdebug.h"
#include "../simevent.h"
#include "../display/simgraph.h"
#include "../display/rgba.h"
#include "../dataobj/environment.h"
#include "../world/simworld.h"
#include "../music/music.h"

#include <GLFW/glfw3.h>

static bool sigterm_received = false;
static slist_tpl<sys_event_t> events;

// bitfield tracking the mouse button states. a 1 bit means pressed
static uint16 mouse_buttons = 0;
static scr_coord_val mx = 0;
static scr_coord_val my = 0;


static uint32_t convert_special_key(GLFWwindow* window, uint32_t key)
{
    bool numlock = glfwGetKey(window, GLFW_KEY_NUM_LOCK) == GLFW_PRESS; 		
	uint32_t code;

	switch(key) {
		case GLFW_KEY_BACKSPACE:  code = SIM_KEY_BACKSPACE;             break;
		case GLFW_KEY_TAB:        code = SIM_KEY_TAB;                   break;
		case GLFW_KEY_ENTER:      code = SIM_KEY_ENTER;                 break;
		case GLFW_KEY_ESCAPE:     code = SIM_KEY_ESCAPE;                break;
		case GLFW_KEY_DELETE:     code = SIM_KEY_DELETE;                break;
		case GLFW_KEY_DOWN:       code = SIM_KEY_DOWN;                  break;
		case GLFW_KEY_END:        code = SIM_KEY_END;                   break;
		case GLFW_KEY_HOME:       code = SIM_KEY_HOME;                  break;
		case GLFW_KEY_F1:         code = SIM_KEY_F1;                    break;
		case GLFW_KEY_F2:         code = SIM_KEY_F2;                    break;
		case GLFW_KEY_F3:         code = SIM_KEY_F3;                    break;
		case GLFW_KEY_F4:         code = SIM_KEY_F4;                    break;
		case GLFW_KEY_F5:         code = SIM_KEY_F5;                    break;
		case GLFW_KEY_F6:         code = SIM_KEY_F6;                    break;
		case GLFW_KEY_F7:         code = SIM_KEY_F7;                    break;
		case GLFW_KEY_F8:         code = SIM_KEY_F8;                    break;
		case GLFW_KEY_F9:         code = SIM_KEY_F9;                    break;
		case GLFW_KEY_F10:        code = SIM_KEY_F10;                   break;
		case GLFW_KEY_F11:        code = SIM_KEY_F11;                   break;
		case GLFW_KEY_F12:        code = SIM_KEY_F12;                   break;
		case GLFW_KEY_F13:        code = SIM_KEY_F13;                   break;
		case GLFW_KEY_F14:        code = SIM_KEY_F14;                   break;
		case GLFW_KEY_F15:        code = SIM_KEY_F15;                   break;
		
		case GLFW_KEY_KP_0:       code = (numlock ? '0' : (unsigned long)SIM_KEY_NUMPAD_BASE); break;
		case GLFW_KEY_KP_1:       code = (numlock ? '1' : (unsigned long)SIM_KEY_DOWNLEFT); break;
		case GLFW_KEY_KP_2:       code = (numlock ? '2' : (unsigned long)SIM_KEY_DOWN); break;
		case GLFW_KEY_KP_3:       code = (numlock ? '3' : (unsigned long)SIM_KEY_DOWNRIGHT); break;
		case GLFW_KEY_KP_4:       code = (numlock ? '4' : (unsigned long)SIM_KEY_LEFT); break;
		case GLFW_KEY_KP_5:       code = (numlock ? '5' : (unsigned long)SIM_KEY_CENTER); break;
		case GLFW_KEY_KP_6:       code = (numlock ? '6' : (unsigned long)SIM_KEY_RIGHT); break;
		case GLFW_KEY_KP_7:       code = (numlock ? '7' : (unsigned long)SIM_KEY_UPLEFT); break;
		case GLFW_KEY_KP_8:       code = (numlock ? '8' : (unsigned long)SIM_KEY_UP); break;
		case GLFW_KEY_KP_9:       code = (numlock ? '9' : (unsigned long)SIM_KEY_UPRIGHT); break;
		
		case GLFW_KEY_KP_ENTER:   code = SIM_KEY_ENTER;                 break;
		case GLFW_KEY_LEFT:       code = SIM_KEY_LEFT;                  break;
		case GLFW_KEY_PAGE_DOWN:   code = '<';                           break;
		case GLFW_KEY_PAGE_UP:     code = '>';                           break;
		case GLFW_KEY_RIGHT:      code = SIM_KEY_RIGHT;                 break;
		case GLFW_KEY_UP:         code = SIM_KEY_UP;                    break;
		case GLFW_KEY_PAUSE:      code = SIM_KEY_PAUSE;                 break;
		case GLFW_KEY_SCROLL_LOCK: code = SIM_KEY_SCROLLLOCK;            break;
		default: {
			code = 0;
			break;
		}
	}

	return code;
}


void error_callback(int error, const char* description)
{
    dbg->message("GLFW Error", "Error %d: %s\n", error, description);
}


void sysgl_cursor_pos_callback(GLFWwindow *, double x, double y)
{
    // dbg->message("cursor_pos_callback()", "x=%f y=%f", x, y);

    scr_coord_val nx = (scr_coord_val)x;
    scr_coord_val ny = (scr_coord_val)y;

    // only trigger an even if mouse pos actually changed
    if(nx != mx || ny != my)
    {
        mx = nx;
        my = ny;

        sys_event_t event;
        event.type = SIM_MOUSE_MOVE;
        event.code = SIM_MOUSE_MOVED;
        event.mx = mx;
        event.my = my;
        event.mb = mouse_buttons;
        event.key_mod = 0;

        events.append(event);
    }
}


void sysgl_mouse_button_callback(GLFWwindow *, int button, int action, int mods)
{
    sys_event_t event;
    event.type = SIM_MOUSE_BUTTONS;
    event.mx = mx;
    event.my = my;

    if(action == GLFW_PRESS)
    {
        switch(button)
        {
            case GLFW_MOUSE_BUTTON_1:
                event.code = SIM_MOUSE_LEFTBUTTON;
                mouse_buttons |= MOUSE_LEFTBUTTON;
                break;
            case GLFW_MOUSE_BUTTON_3:
                event.code = SIM_MOUSE_MIDBUTTON;
                mouse_buttons |= MOUSE_MIDBUTTON;
                break;
            case GLFW_MOUSE_BUTTON_2:
                event.code = SIM_MOUSE_RIGHTBUTTON;
                mouse_buttons |= MOUSE_RIGHTBUTTON;
                break;
        }
    }
    else
    {
        switch(button)
        {
            case GLFW_MOUSE_BUTTON_1:
                event.code = SIM_MOUSE_LEFTUP;
                mouse_buttons &= ~MOUSE_LEFTBUTTON;
                break;
            case GLFW_MOUSE_BUTTON_3:
                event.code = SIM_MOUSE_MIDUP;
                mouse_buttons &= ~MOUSE_MIDBUTTON;
                break;
            case GLFW_MOUSE_BUTTON_2:
                event.code = SIM_MOUSE_RIGHTUP;
                mouse_buttons &= ~MOUSE_RIGHTBUTTON;
                break;
        }
    }

    event.mb = mouse_buttons;
    event.key_mod = 0;

    events.append(event);

    dbg->message("sysgl_mouse_button_callback()", "code=%d buttons=%d", sys_event.code, sys_event.mb);
}


void sysgl_scroll_callback(GLFWwindow *, double xoffset, double yoffset)
{
    sys_event_t event;
	event.type    = SIM_MOUSE_BUTTONS;
	event.code    = yoffset > 0 ? SIM_MOUSE_WHEELUP : SIM_MOUSE_WHEELDOWN;
	event.key_mod = 0;

    events.append(event);
}


void sysgl_character_callback(GLFWwindow *, unsigned int codepoint)
{
    dbg->message("sysgl_character_callback()", "code=%d", codepoint);
	
    sys_event_t event;
	event.type    = SIM_KEYBOARD;
	event.code    = codepoint;	
	event.key_mod = 0;

    events.append(event);
}


void sysgl_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    dbg->message("sysgl_key_callback()", "key=%d action=%d mods=%d", key, action, mods);

    if(action == GLFW_PRESS)
	{
		uint32_t code = convert_special_key(window, key);
		if(code) {
			sys_event_t event;
			event.type    = SIM_KEYBOARD;
			event.code    = code;	
			event.key_mod = 0;
			events.append(event);		
		}
	}
	
    if(action == GLFW_RELEASE)
	{
		sys_event_t event;
		event.type    = SIM_KEYBOARD;
		event.code    = 0;	
		event.key_mod = 0;
	    events.append(event);
	}
}


/**
 * Quit immediately, save settings and game without visual feedback
 */
void sysgl_window_close_callback(GLFWwindow* window)
{

    // stop game processing
    intr_disable();

    DBG_DEBUG("sysgl_window_close_callback()", "env_t::reload_and_save_on_quit=%d", env_t::reload_and_save_on_quit);

    // save game only if there is a game world
    if(env_t::reload_and_save_on_quit && !env_t::networkmode && world() != NULL) {
        // save current game, if not online
        bool old_restore_UI = env_t::restore_UI;
        env_t::restore_UI = true;

        // construct from pak name an autosave if requested
        std::string pak_name("autosave-");
        pak_name.append(env_t::pak_name);
        pak_name.erase(pak_name.length() - 1);
        pak_name.append(".sve");

        dr_chdir(env_t::user_dir);
        world()->save(pak_name.c_str(), true, SAVEGAME_VER_NR, true);
        env_t::restore_UI = old_restore_UI;
    }

    // save settings
    {
        dr_chdir(env_t::user_dir);
        loadsave_t settings_file;
        if (settings_file.wr_open("settings.xml", loadsave_t::xml, 0, "settings only/", SAVEGAME_VER_NR) == loadsave_t::FILE_STATUS_OK) {
            env_t::rdwr(&settings_file);
            env_t::default_settings.rdwr(&settings_file);
            settings_file.close();
        }
    }

    // stop the music
    dr_stop_midi();

    // leave the ship in an orderly manner
    dr_os_close();

    // and done for now.
    exit(0);
}


bool dr_set_screen_scale(sint16)
{
	// no autoscaling as we have no display ...
	return false;
}


sint16 dr_get_screen_scale()
{
	return 100;
}


bool dr_os_init(const int*)
{
	// prepare for next event
	sys_event.type = SIM_NOEVENT;
	sys_event.code = 0;

	bool ok = glfwInit();

	dbg->message("dr_os_init()", "GLFW init: %d", ok);

	if(ok)
	{
		glfwSetErrorCallback(error_callback);
	}

	return ok;
}


resolution dr_query_screen_resolution()
{
	resolution const res = {1024, 768};
	return res;
}


// open the window
int dr_os_open(int, int, sint16)
{
	return 1;
}


void dr_os_close()
{
	glfwTerminate();
}


// resizes screen
int dr_textur_resize(unsigned short** const textur, int, int)
{
	*textur = NULL;
	return 1;
}


unsigned short *dr_textur_init()
{
	return NULL;
}


rgba_t get_system_color(rgb888_t color)
{
	return rgba_t(color);
}


void dr_prepare_flush()
{
}


void dr_flush()
{
    display_flush_buffer();
}


void dr_textur(int, int, int, int)
{
}


bool move_pointer(int, int)
{
	return false;
}


void set_pointer(int)
{
}


void GetEvents()
{
    // dbg->message("GetEvents()", "Called.");

    glfwPollEvents();

    if(events.empty()) {
        sys_event.type = SIM_NOEVENT;
        sys_event.code = 0;
    }
    else {
        sys_event = events.remove_first();
    }

    // priority event, nothing else matters if this comes
	if(sigterm_received) {
		sys_event.type = SIM_SYSTEM;
		sys_event.code = SYSTEM_QUIT;
	}
}


void show_pointer(int)
{
}


void ex_ord_update_mx_my()
{
}


#ifndef _MSC_VER
static timeval first;
#endif

uint32 dr_time()
{
#ifndef _MSC_VER
	timeval second;
	gettimeofday(&second,NULL);
	if (first.tv_usec > second.tv_usec) {
		// since those are often unsigned
		second.tv_usec += 1000000;
		second.tv_sec--;
	}

	return (second.tv_sec - first.tv_sec)*1000ul + (second.tv_usec - first.tv_usec)/1000ul;
#else
	return timeGetTime();
#endif
}

void dr_sleep(uint32 msec)
{
    // dbg->message("dr_sleep()", "Called, sleeping %dms.", msec);
/*
	// this would be 100% POSIX but is usually not very accurate ...
	if(  msec>0  ) {
		struct timeval tv;
		tv.sec = 0;
		tv.usec = msec*1000;
		select(0, 0, 0, 0, &tv);
	}
*/
#ifdef _WIN32
	Sleep( msec );
#else
	usleep( 1000u * msec );
#endif
}

void dr_start_textinput()
{
}


void dr_stop_textinput()
{
}


void dr_notify_input_pos(scr_coord pos)
{
}


static void posix_sigterm(int)
{
	dbg->message("Received SIGTERM", "exiting...");
	sigterm_received = 1;
}


const char* dr_get_locale()
{
	return "";
}


bool dr_has_fullscreen()
{
	return false;
}


sint16 dr_get_fullscreen()
{
	return 0;
}


sint16 dr_toggle_borderless()
{
	return 0;
}


sint16 dr_suspend_fullscreen()
{
	return 0;
}


void dr_restore_fullscreen(sint16)
{
}



int main(int argc, char **argv)
{
	signal(SIGTERM, posix_sigterm);
#ifndef _MSC_VER
	gettimeofday(&first, NULL);
#endif
	return sysmain(argc, argv);
}
