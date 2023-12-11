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
#include "../simdebug.h"
#include "../simevent.h"
#include "../display/simgraph.h"
#include "../display/rgba.h"

#include <GLFW/glfw3.h>

static bool sigterm_received = false;


void error_callback(int error, const char* description)
{
    dbg->message("GLFW Error", "Error %d: %s\n", error, description);
}


// bitfield tracking the mouse button states. a 1 bit means pressed
uint16 mouse_buttons = 0;
scr_coord_val mx = 0;
scr_coord_val my = 0;


void sysgl_cursor_pos_callback(GLFWwindow *window, double x, double y)
{
    // dbg->message("cursor_pos_callback()", "x=%f y=%f", x, y);
    mx = (scr_coord_val)x;
    my = (scr_coord_val)y;

    sys_event.type = SIM_MOUSE_MOVE;
    sys_event.code = SIM_MOUSE_MOVED;
    sys_event.mx = mx;
    sys_event.my = my;
    sys_event.mb = 0;
    sys_event.key_mod = 0;
}


void sysgl_mouse_button_callback(GLFWwindow* , int button, int action, int mods)
{
    sys_event.type = SIM_MOUSE_BUTTONS;

    sys_event.mx = mx;
    sys_event.my = my;

    if(action == GLFW_PRESS)
    {
        switch(button)
        {
            case GLFW_MOUSE_BUTTON_1:
                sys_event.code = SIM_MOUSE_LEFTBUTTON + button;
                mouse_buttons |= MOUSE_LEFTBUTTON;
                break;
            case GLFW_MOUSE_BUTTON_2:
                sys_event.code = SIM_MOUSE_MIDBUTTON + button;
                mouse_buttons |= MOUSE_MIDBUTTON;
                break;
            case GLFW_MOUSE_BUTTON_3:
                sys_event.code = SIM_MOUSE_RIGHTBUTTON + button;
                mouse_buttons |= MOUSE_RIGHTBUTTON;
                break;
        }
    }
    else
    {
        switch(button)
        {
            case GLFW_MOUSE_BUTTON_1:
                sys_event.code = SIM_MOUSE_LEFTUP + button;
                mouse_buttons &= ~MOUSE_LEFTBUTTON;
                break;
            case GLFW_MOUSE_BUTTON_2:
                sys_event.code = SIM_MOUSE_MIDUP + button;
                mouse_buttons &= ~MOUSE_MIDBUTTON;
                break;
            case GLFW_MOUSE_BUTTON_3:
                sys_event.code = SIM_MOUSE_RIGHTUP + button;
                mouse_buttons &= ~MOUSE_RIGHTBUTTON;
                break;
        }
    }

    sys_event.mb = mouse_buttons;
    sys_event.key_mod = 0;


    dbg->message("cursor_pos_callback()", "code=%d buttons=%d", sys_event.code, sys_event.mb);
}


void sysgl_scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	sys_event.type    = SIM_MOUSE_BUTTONS;
	sys_event.code    = yoffset > 0 ? SIM_MOUSE_WHEELUP : SIM_MOUSE_WHEELDOWN;
	sys_event.key_mod = 0;
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

	printf("GLFW init: %d", ok);

	if(ok)
	{
		glfwSetErrorCallback(error_callback);
	}

	return ok;
}


resolution dr_query_screen_resolution()
{
	resolution const res = { 640, 480 };
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

	if(  sigterm_received  ) {
		sys_event.type = SIM_SYSTEM;
		sys_event.code = SYSTEM_QUIT;
	}

    glfwPollEvents();
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
	DBG_MESSAGE("Received SIGTERM", "exiting...");
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
