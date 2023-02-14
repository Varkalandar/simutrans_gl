#ifndef DISPLAY_ALIGNMENT_H
#define DISPLAY_ALIGNMENT_H

/**
* Alignment enum to align controls against each other.
* Vertical and horizontal alignment can be masked together
* Unused bits are reserved for future use, set to 0.
*/
enum control_alignments_t 
{
    ALIGN_NONE       = 0x00,

    ALIGN_TOP        = 0x01,
    ALIGN_CENTER_V   = 0x02,
    ALIGN_BOTTOM     = 0x03,

    ALIGN_LEFT       = 0x04,
    ALIGN_CENTER_H   = 0x08,
    ALIGN_RIGHT      = 0x0C,

    // These flags does not belong in here but
    // are defined here until we sorted this out.
    // They are only used in display_text_proportional_len_clip_rgb()
    DT_CLIP          = 0x4000
};
typedef uint16 control_alignment_t;


#endif /* DISPLAY_ALIGNMENT_H */

