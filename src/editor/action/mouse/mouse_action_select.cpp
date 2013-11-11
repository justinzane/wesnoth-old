/**
 * @file src/editor/action/mouse/mouse_action_select.cpp
 * @project The Battle for Wesnoth NG - https://github.com/justinzane/wesnoth-ng
 * @brief 
 * @authors 
 * @copyright Copyright (C) 2008 - 2011 by Fabian Mueller <fabianmueller5@gmx.de>
   Part of the Battle for Wesnoth Project http://www.wesnoth.org/

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/
//TODO uncomment or remove
//#define GETTEXT_DOMAIN "wesnoth-editor"

#include "mouse_action_select.hpp"
#include "../action_select.hpp"

#include "../../editor_display.hpp"

namespace editor {

std::set<map_location> mouse_action_select::affected_hexes(
	editor_display& disp, const map_location& hex)
{
	if (has_shift_modifier()) {
		return disp.map().get_contiguous_terrain_tiles(hex);
	} else {
		return brush_drag_mouse_action::affected_hexes(disp, hex);
	}
}

editor_action* mouse_action_select::key_event(
		editor_display& disp, const SDL_Event& event)
{
	editor_action* ret = mouse_action::key_event(disp, event);
	update_brush_highlights(disp, previous_move_hex_);
	return ret;
}

editor_action* mouse_action_select::click_perform_left(
		editor_display& /*disp*/, const std::set<map_location>& hexes)
{
	return new editor_action_chain(new editor_action_select(hexes));
}

editor_action* mouse_action_select::click_perform_right(
		editor_display& /*disp*/, const std::set<map_location>& /*hexes*/)
{
	return NULL;
}

editor_action* mouse_action_select::click_right(editor_display& /*disp*/, int /*x*/, int /*y*/)
{
	return NULL;
}

void mouse_action_select::set_mouse_overlay(editor_display& disp)
{
	surface image;
	if (has_shift_modifier()) {
		image = image::get_image("editor/tool-overlay-select-wand.png");
	} else {
		image = image::get_image("editor/tool-overlay-select-brush.png");
	}
	Uint8 alpha = 196;
	int size = image->w;
	int zoom = static_cast<int>(size * disp.get_zoom_factor());

	// Add the alpha factor and scale the image
	image = scale_surface(adjust_surface_alpha(image, alpha), zoom, zoom);
	disp.set_mouseover_hex_overlay(image);
}


} //end namespace editor
