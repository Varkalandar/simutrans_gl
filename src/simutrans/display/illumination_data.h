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

	scr_rect area;

	illumination_data_t(rgba_t light_color, const char * light_id, int light_index, scr_rect area) {
		this->light_color = light_color;
		this->light_id = light_id;
		this->light_index = light_index;
		this->area = area;
	}
};

#endif