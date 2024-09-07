/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "goods_stats.h"

#include "../simcolor.h"
#include "../world/simworld.h"

#include "../builder/goods_manager.h"
#include "../descriptor/goods_desc.h"

#include "../dataobj/translator.h"

#include "components/gui_button.h"
#include "components/gui_colorbox.h"
#include "components/gui_label.h"

karte_ptr_t goods_stats_t::welt;

void goods_stats_t::update_goodslist(vector_tpl<const goods_desc_t*>goods, int bonus)
{
	scr_size size = get_size();
	remove_all();
	set_table_layout(6, 0);

    // build header
    
    new_component<gui_label_t>("GLColor", gui_theme_t::gui_color_text_highlight, gui_label_t::centered)->fixed_min_height = LINESPACE * 3 / 2;
    new_component<gui_label_t>("GLName", gui_theme_t::gui_color_text_highlight, gui_label_t::centered);
    new_component<gui_label_t>("GLRevenue", gui_theme_t::gui_color_text_highlight, gui_label_t::centered);
    new_component<gui_label_t>("GLBonus", gui_theme_t::gui_color_text_highlight, gui_label_t::centered);
    new_component<gui_label_t>("GLCategory", gui_theme_t::gui_color_text_highlight, gui_label_t::centered);
    new_component<gui_label_t>("GLWeight", gui_theme_t::gui_color_text_highlight, gui_label_t::centered);
    
    // build list
    
	for(const goods_desc_t* wtyp : goods) {

		gui_colorbox_t * indicator = new_component<gui_colorbox_t>(wtyp->get_color());
		indicator->fixed_min_height = gui_theme_t::gui_label_size.h-4;
		indicator->set_max_size(scr_size(D_INDICATOR_WIDTH, D_INDICATOR_HEIGHT));

		new_component<gui_label_t>(wtyp->get_name());

		const sint32 grundwert128 = (sint32)wtyp->get_value() * welt->get_settings().get_bonus_basefactor(); // bonus price will be always at least this
		const sint32 grundwert_bonus = (sint32)wtyp->get_value()*(1000l+(bonus-100l)*wtyp->get_speed_bonus());
		const sint32 price = (grundwert128>grundwert_bonus ? grundwert128 : grundwert_bonus);

		gui_label_buf_t *lb = new_component<gui_label_buf_t>((gui_theme_t::gui_color_text), gui_label_t::right);
		lb->buf().append_money(price/3000.0);
		lb->update();

		lb = new_component<gui_label_buf_t>((gui_theme_t::gui_color_text), gui_label_t::right);
		lb->buf().printf("%d%%", wtyp->get_speed_bonus());
		lb->update();

		new_component<gui_label_t>(wtyp->get_catg_name(), (gui_theme_t::gui_color_text), gui_label_t::right);

		lb = new_component<gui_label_buf_t>((gui_theme_t::gui_color_text), gui_label_t::right);
		lb->buf().printf("%dkg", wtyp->get_weight_per_unit());
		lb->update();
	}

	scr_size min_size = get_min_size();
	set_size(scr_size(max(size.w, min_size.w), min_size.h) );
}


void goods_stats_t::draw(scr_coord offset)
{
	const scr_coord_val spacing = D_LABEL_HEIGHT + D_V_SPACE;
	const scr_coord_val y = pos.y + offset.y + spacing - D_V_SPACE;

    display_set_color(RGBA_WHITE);
    display_img_stretch(gui_theme_t::windowback, scr_rect(pos + offset + scr_coord(0, D_V_SPACE),
	                                                      size.w, display_get_height()));

	for(int line = 1; line < goods_manager_t::get_count(); line ++) {
		rgba_t color = (line & 1) ? (gui_theme_t::gui_color_list_background_odd) : (gui_theme_t::gui_color_list_background_even);
		display_fillbox_wh_clip_rgb(pos.x + offset.x, y + line * spacing, size.w, spacing, color);
	}

	gui_aligned_container_t::draw(offset);
}