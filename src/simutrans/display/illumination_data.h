#ifndef illumination_data_h
#define illumination_data_h

#include "rgba.h"

class illumination_data_t
{
public:

	/**
	 * The color of the light. Use alpha = 1.0 to avopid problems with some
	 * opengl drivers
	 */
	rgba_t light_color;
	
	/**
	 * Identifier to look up the light data table
	 */	
	const char * light_id;

	/**
	 * Index of this light in the light table
	 */
	int light_index;

	/**
	 * The area of the light texture on screen. x and y are relative to the
	 * anchor object of this light.
	 */
	scr_rect area;

	/**
	 * The darkness when this light is turned on. night = 0 is always on.
	 * 0.6 is a good choice for evening to morning.
	 */
	float night;

	illumination_data_t(rgba_t light_color, const char * light_id, int light_index, 
	                    scr_rect area, float night) 
	{
		this->light_color = light_color;
		this->light_id = light_id;
		this->light_index = light_index;
		this->area = area;
		this->night = night;
	}
};

#endif