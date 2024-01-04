/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include "image_writer_32.h"
#include "root_writer.h"
#include "obj_node.h"
#include "obj_pak_exception.h"
#include "../image.h"
#include "../../macros.h"
#include "../../utils/simstring.h"
#include "../../simdebug.h"
#include "../../io/raw_image.h"

#ifndef _WIN32
#include <dirent.h>
#include <sys/types.h>
#endif


struct dimension
{
	int xmin;
	int xmax;
	int ymin;
	int ymax;
};


std::string image_writer_t::last_img_file;

raw_image_t image_writer_t::input_img;
int image_writer_t::img_size = 64;


uint32 image_writer_t::block_getpix(int x, int y)
{
	const uint8 * pixel_data = input_img.access_pixel(x, y);
    uint32 pix;
    
	switch (input_img.get_format()) {
		case raw_image_t::FMT_GRAY8: {
			const uint8 gray_level = pixel_data[0];
			pix =
                0xFF000000 |
				gray_level <<  0 |
				gray_level <<  8 |
				gray_level << 16;
            break;
		}
		case raw_image_t::FMT_RGBA8888: {
			pix = 
				(pixel_data[2] <<  0) + // B
				(pixel_data[1] <<  8) + // G
				(pixel_data[0] << 16) + // R
				(pixel_data[3] << 24);  // A
            break;
		}
		case raw_image_t::FMT_RGB888: {
			pix =
                0xFF000000 |
				(pixel_data[2] <<  0) | // B
				(pixel_data[1] <<  8) | // G
				(pixel_data[0] << 16);  // R
            break;
		}
		default:
			dbg->fatal("image_writer_t::block_getpix", "Unsupported input image format");
            pix = 0;
	}

    // Convert old special transparent color to transparent black 
    if((pix & 0xFFFFFF) == SPECIAL_TRANSPARENT) {
        pix = 0;
    }
    
    return pix;
}


// true if transparent
inline bool is_transparent(const uint32 pix)
{
	return (pix >> 24) < 0x02;
}


static void init_dim(uint32 * image, dimension * dim, int img_size)
{
	int x,y;
	bool found = false;

	dim->ymin = dim->xmin = img_size;
	dim->ymax = dim->xmax = 0;

	for (y = 0; y < img_size; y++) {
		for (x = 0; x < img_size; x++) {
			if(  !is_transparent(image[x + y * img_size])  ) {
				if (x < dim->xmin) dim->xmin = x;
				if (y < dim->ymin) dim->ymin = y;
				if (x > dim->xmax) dim->xmax = x;
				if (y > dim->ymax) dim->ymax = y;
				found = true;
			}
		}
	}
	if (!found) {
		// Negative values not usable
		dim->xmin = 1;
		dim->ymin = 1;
	}
}


/**
 * Encodes an image into a sprite data structure, considers
 * special colors.
 */
uint32 *image_writer_t::encode_image(int x, int y, dimension * dim, int * len)
{
	int line;
	uint32 *dest;
	uint32 *dest_base = new uint32[img_size * img_size];

	dest = dest_base;

	x += dim->xmin;
	y += dim->ymin;

	const int img_width  = dim->xmax - dim->xmin + 1;
	const int img_height = dim->ymax - dim->ymin + 1;

	for(line = 0; line < img_height; line++) {
        for(int col = 0; col < img_width; col ++) {
            uint32 pix = block_getpix(x + col, y + line);
			// *dest++ = endian(pix);
            
            *dest++ = pix;
            
            // dbg->message("xx", "%x", pix);
		}
	}

	*len = dest - dest_base;

	return dest_base;
}


bool image_writer_t::block_load(const char *fname)
{
	// The last image file is cached
	// Note that this method accepts any file name if the content has a supported format,
	// even though makeobj only supports image file names with a ".png" suffix.
	// See image_writer_t::write_obj for details.
	if(  last_img_file == fname  ) {
		return true;
	}
	else if (load_image_from_file(fname)) {
		if ((input_img.get_width()%img_size != 0) || (input_img.get_height()%img_size != 0)) {
			dbg->error("image_writer_t::block_load", "Cannot load image file '%s': "
				"Size not divisible by %d.", fname, img_size);
			last_img_file = "";
			return false;
		}

		last_img_file = fname;
		return true;
	}

	// error message is handled by image_writer_t::write_obj
	last_img_file = "";
	return false;
}


bool image_writer_t::load_image_from_file(const char* fname)
{
	if (input_img.read_from_file(fname)) {
		return true;
	}

	// Not an exact match, try to case-insensitive search.
#ifndef _WIN32
	std::string actual_path;
	size_t len = strlen(fname);
	actual_path.reserve(len);
	const char * sep_beg = fname;
	const char * sep_end = sep_beg + strspn(sep_beg, "/");
	if (sep_end == sep_beg) {
		// relative
		actual_path = "./";
	}

	char const * end = fname + len;

	std::string name;
	while (true) {
		actual_path.insert(actual_path.end(), sep_beg, sep_end);
		sep_beg = sep_end + strcspn(sep_end, "/");
		if (sep_beg == sep_end) {
			break;
		}
		name.assign(sep_end, sep_beg);
		DIR * dir = opendir(actual_path.c_str());
		if (!dir) {
			break;
		}
		struct dirent * ent = NULL;
		while ((ent = readdir(dir)) != NULL) {
			if (!STRICMP(ent->d_name, name.c_str())) {
				actual_path += ent->d_name;
				break;
			}
		}
		closedir(dir);
		if (!ent) {
			break;
		}

		if (sep_beg == end) {
			return input_img.read_from_file(actual_path.c_str());
		}
		sep_end = sep_beg + strspn(sep_beg, "/");
	}
#endif

	return false;
}


/* the syntax for image the string is
 *   "-" empty image
 * [> ]imagefilename_without_extension[[[[.row].col],xoffset],yoffset]
 *  leading "> " set the flag for a non-zoomable image
 *  after the dots also spaces and comments are allowed
 */
void image_writer_t::write_obj(FILE* outfp, obj_node_t& parent, std::string an_imagekey, uint32 index)
{
	image_t image;
	dimension dim;
	uint32 * pixdata = NULL;

	MEMZERO(image);

	// if first char is a '>' then this image is not zoomable
	if(  an_imagekey[0] == '>'  ) {
		an_imagekey = an_imagekey.substr(1);
		image.zoomable = false;
	}
	else {
		image.zoomable = true;
	}
	std::string imagekey = trim(an_imagekey);

	if(  imagekey != "-"  &&  imagekey != ""  ) {

		// divide key in filename and image number
		int row = -1, col = -1;
		std::string numkey;

		int j = imagekey.rfind('/');
		if(  j == -1  ) {
			numkey = imagekey;
		}
		else {
			numkey = imagekey.substr(j + 1, std::string::npos);
		}

		int i = numkey.find('.');
		if(  i == -1  ) {
			char reason[1024];
			sprintf(reason, "no image number in %s", imagekey.c_str() );
			throw obj_pak_exception_t("image_writer_t", reason);
		}
		numkey = numkey.substr( i+1, std::string::npos );

		imagekey = root_writer_t::get_inpath() + imagekey.substr( 0, imagekey.size()-numkey.size() - 1 ) + ".png";

		row = atoi(numkey.c_str());

		i = numkey.find('.');
		if(i != -1) {
			col = atoi( numkey.c_str()+i+1 );

			// add image offsets
			int comma_pos = numkey.find(',');
			if(comma_pos != -1) {
				numkey = numkey.substr( comma_pos+1, std::string::npos);
				image.x = atoi( numkey.c_str() );
				comma_pos = numkey.find(',');
				if(comma_pos != -1) {
					image.y = atoi(numkey.substr( comma_pos + 1, std::string::npos).c_str());
				}
			}
		}

		// Load complete file
		if (!block_load(imagekey.c_str())) {
			char reason[1024];
			sprintf(reason, "cannot open %s", imagekey .c_str());
			throw obj_pak_exception_t("image_writer_t", reason);
		}

		if (col == -1) {
			col = row % (input_img.get_width()  / img_size);
			row = row / (input_img.get_height() / img_size);
		}
		if (col >= (int)(input_img.get_width() / img_size) || row >= (int)(input_img.get_height() / img_size)) {
			char reason[1024];
			sprintf(reason, "invalid image number in %s.%s", imagekey.c_str(), numkey.c_str());
			throw obj_pak_exception_t("image_writer_t", reason);
		}
		row *= img_size;
		col *= img_size;

		// Temp. read image and determine drawing area.
		uint32 *image_data = new uint32[img_size * img_size];
		for (int x = 0; x < img_size; x++) {
			for (int y = 0; y < img_size; y++) {
				image_data[x + y * img_size] = block_getpix(x + col, y + row);
			}
		}
		init_dim(image_data, &dim, img_size);
		delete [] image_data;

		image.x += dim.xmin;
		image.y += dim.ymin;
		image.w = dim.xmax - dim.xmin + 1;
		image.h = dim.ymax - dim.ymin + 1;
		image.len = 0;

		if (image.h > 0) {
			int len;
			pixdata = encode_image(col, row, &dim, &len);
			image.len = len;
		}

		dbg->debug( "", "image[%3u] =%-30s %-20s %5u %5u %5u %5u %5u %6u %4s", index, an_imagekey.c_str(), imagekey.c_str(), col, row, image.x, image.y, image.w, image.h, (image.zoomable) ? "yes" : "no" );
	}
	else {
		dbg->debug( "", "image[%3u] =%-30s %-20s %5u %5u %5u %5u %5u %6u %4s", index, an_imagekey.c_str(), imagekey.c_str(), 0, 0, image.x, image.y, image.w, image.h, (image.zoomable) ? "yes" : "no" );
	}

	obj_node_t node(this, 10 + (image.len * sizeof(uint32)), &parent);

	// to avoid any problems due to structure changes, we write manually the data
	node.write_uint16(outfp, image.x,        0);
	node.write_uint16(outfp, image.y,        2);
	node.write_uint16(outfp, image.w,        4);
	node.write_uint8 (outfp, 4,              6); // version, always at position 6!
	node.write_uint16(outfp, image.h,        7);
	node.write_uint8 (outfp, image.zoomable, 9);

	if (image.len) {
		// only called, if there is something to store
		node.write_data_at(outfp, pixdata, 10, image.len * sizeof(uint32));
		delete [] pixdata;
	}

	node.write(outfp);
}
