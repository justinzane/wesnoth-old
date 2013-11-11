/**
 * @file src/gui/auxiliary/window_builder/multi_page.hpp
 * @project The Battle for Wesnoth NG - https://github.com/justinzane/wesnoth-ng
 * @brief 
 * @authors 
 * @copyright Copyright (C) 2008 - 2013 by Mark de Wever <koraq@xs4all.nl>
   Part of the Battle for Wesnoth Project http://www.wesnoth.org/

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/

#ifndef GUI_AUXILIARY_WINDOW_BUILDER_MULTI_PAGE_HPP_INCLUDED
#define GUI_AUXILIARY_WINDOW_BUILDER_MULTI_PAGE_HPP_INCLUDED

#include "gui/widgets/widget.hpp"
#include "gui/auxiliary/window_builder/control.hpp"

#include <map>
#include <vector>

typedef std::map<std::string, t_string> string_map;

namespace gui2 {

namespace implementation {

struct tbuilder_multi_page
	: public tbuilder_control
{
	explicit tbuilder_multi_page(const config& cfg);

	twidget* build () const;

	tbuilder_grid_ptr builder;

	/**
	 * Multi page data.
	 *
	 * Contains a vector with the data to set in every cell, it's used to
	 * serialize the data in the config, so the config is no longer required.
	 */
	std::vector<string_map> data;
};

} // namespace implementation

} // namespace gui2

#endif

